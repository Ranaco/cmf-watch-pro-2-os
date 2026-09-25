use crate::protocol::{FrameDecoder, Message, encode_frame};
use crate::state::command_result;
use serde_json::{Value, json};
use std::fs::File;
use std::io::{self, Read, Write};
use std::net::{TcpListener, TcpStream};
use std::time::Duration;

pub const DEFAULT_ADDRESS: &str = "127.0.0.1:4660";
const MAX_SIMULATED_LATENCY_MS: u64 = 10_000;

pub fn run(address: &str) -> io::Result<()> {
    let latency = configured_latency()?;
    let reject_actions = std::env::var("CMF_HOST_REJECT_ACTIONS").is_ok_and(|value| value == "1");
    run_with_latency(address, latency, reject_actions)
}

fn run_with_latency(address: &str, latency: Duration, reject_actions: bool) -> io::Result<()> {
    let listener = TcpListener::bind(address)?;
    eprintln!("CMF watch host listening on {address}");
    eprintln!("host latency simulation: {} ms", latency.as_millis());
    for connection in listener.incoming() {
        match connection.and_then(|stream| handle_connection(stream, latency, reject_actions)) {
            Ok(()) => eprintln!("watch disconnected"),
            Err(error) => eprintln!("connection error: {error}"),
        }
    }
    Ok(())
}

fn handle_connection(
    mut stream: TcpStream,
    latency: Duration,
    reject_actions: bool,
) -> io::Result<()> {
    stream.set_nodelay(true)?;
    eprintln!("watch connected from {}", stream.peer_addr()?);
    let mut next_id = 1;
    send_delayed(&mut stream, &hello(next_id), latency)?;
    next_id += 1;
    let mut decoder = FrameDecoder::default();
    let mut bytes = [0_u8; 2048];
    let mut current_revision = 0_u64;
    loop {
        let count = stream.read(&mut bytes)?;
        if count == 0 {
            return Ok(());
        }
        let messages = decoder
            .push(&bytes[..count])
            .map_err(|error| io::Error::new(io::ErrorKind::InvalidData, error))?;
        for message in messages {
            eprintln!("received {} id={}", message.kind, message.id);
            if message.kind == "sync_request" {
                let updates = synchronization_updates(next_id);
                send_delayed(&mut stream, &updates[0], latency)?;
                next_id += 1;
                if let Some(response) = response_for(&message, next_id) {
                    send_delayed(&mut stream, &response, latency)?;
                    next_id += 1;
                }
                for mut update in updates.into_iter().skip(1) {
                    update.id = next_id;
                    send_delayed(&mut stream, &update, latency)?;
                    next_id += 1;
                }
                current_revision = 4;
            } else if message.kind == "action" {
                let (response, desired_playing) =
                    music_action_response(&message, next_id, reject_actions);
                send_delayed(&mut stream, &response, latency)?;
                next_id += 1;
                if let Some(playing) = desired_playing {
                    current_revision += 1;
                    let patch = Message::new(
                        "state_patch",
                        next_id,
                        json!({
                            "path": "music.playing",
                            "value": playing,
                            "revision": current_revision
                        }),
                    );
                    send_delayed(&mut stream, &patch, latency)?;
                    next_id += 1;
                }
            } else if let Some(response) = response_for(&message, next_id) {
                send_delayed(&mut stream, &response, latency)?;
                next_id += 1;
            }
        }
    }
}

fn music_action_response(
    message: &Message,
    id: u32,
    reject_actions: bool,
) -> (Message, Option<bool>) {
    if reject_actions {
        return (command_result(message, id, "rejected", "busy"), None);
    }
    let app_id = message.payload.get("app_id").and_then(Value::as_str);
    let action_id = message.payload.get("action_id").and_then(Value::as_str);
    let playing = message
        .payload
        .get("arguments")
        .and_then(|arguments| arguments.get("playing"))
        .and_then(Value::as_bool);
    match (app_id, action_id, playing) {
        (Some("music"), Some("set_playing"), Some(value)) => {
            (command_result(message, id, "ok", "none"), Some(value))
        }
        (Some("music"), _, _) => (
            command_result(message, id, "rejected", "invalid_argument"),
            None,
        ),
        _ => (
            command_result(message, id, "rejected", "app_not_found"),
            None,
        ),
    }
}

fn configured_latency() -> io::Result<Duration> {
    match std::env::var("CMF_HOST_LATENCY_MS") {
        Ok(value) => parse_latency(&value),
        Err(std::env::VarError::NotPresent) => Ok(Duration::ZERO),
        Err(error) => Err(io::Error::new(io::ErrorKind::InvalidInput, error)),
    }
}

fn parse_latency(value: &str) -> io::Result<Duration> {
    let milliseconds = value.parse::<u64>().map_err(|_| {
        io::Error::new(
            io::ErrorKind::InvalidInput,
            "CMF_HOST_LATENCY_MS must be an integer from 0 to 10000",
        )
    })?;
    if milliseconds > MAX_SIMULATED_LATENCY_MS {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            "CMF_HOST_LATENCY_MS must be an integer from 0 to 10000",
        ));
    }
    Ok(Duration::from_millis(milliseconds))
}

fn send_delayed(stream: &mut TcpStream, message: &Message, latency: Duration) -> io::Result<()> {
    std::thread::sleep(latency);
    send(stream, message)
}

fn send(stream: &mut TcpStream, message: &Message) -> io::Result<()> {
    let bytes =
        encode_frame(message).map_err(|error| io::Error::new(io::ErrorKind::InvalidData, error))?;
    stream.write_all(&bytes)
}

fn response_for(message: &Message, id: u32) -> Option<Message> {
    match message.kind.as_str() {
        "hello" => Some(Message::new(
            "capabilities",
            id,
            json!({
                "features": ["state_sync", "commands"],
                "apps": ["home", "notifications", "music", "assistant", "settings"],
                "display": { "shape": "round", "width": 466, "height": 466 }
            }),
        )),
        "ping" => {
            let mut pong = Message::new("pong", id, json!({"nonce": message.payload["nonce"]}));
            pong.reply_to = Some(message.id);
            Some(pong)
        }
        "sync_request" => Some(Message::new(
            "sync_response",
            id,
            json!({
                "revision": 0,
                "state": default_state()
            }),
        )),
        "refresh" | "app_open" | "show_toast" | "show_dialog" | "cache_invalidate" => {
            Some(command_result(message, id, "rejected", "unsupported"))
        }
        _ => None,
    }
}

fn hello(id: u32) -> Message {
    let mut message = Message::new(
        "hello",
        id,
        json!({
            "device_id": "cmf-watch-host",
            "session_id": session_id(),
            "runtime_version": env!("CARGO_PKG_VERSION"),
            "protocol_versions": [1],
            "simulator": true,
            "future_compatible_payload_field": true
        }),
    );
    message
        .extra
        .insert("trace_id".into(), json!("integration-probe"));
    message
}

fn default_state() -> Value {
    json!({
        "weather": { "temperature": 26, "condition": "Cloudy" },
        "notifications": [
            {
                "id": "notif_welcome",
                "title": "Host connected",
                "body": "Your watch data is synchronized."
            },
            {
                "id": "notif_build",
                "title": "Build complete",
                "body": "The native simulator passed its checks."
            },
            {
                "id": "notif_cache",
                "title": "Offline ready",
                "body": "Recent data is cached on the watch."
            }
        ],
        "music": { "title": "Ready to play", "playing": false },
        "calendar": [],
        "assistant": { "available": false },
        "steps": 9000
    })
}

fn synchronization_updates(first_id: u32) -> Vec<Message> {
    vec![
        Message::new(
            "state_patch",
            first_id,
            json!({"path":"weather.temperature","value":24,"revision":1}),
        ),
        Message::new(
            "state_patch",
            first_id + 1,
            json!({"path":"steps","value":9632,"revision":2}),
        ),
        Message::new(
            "state_patch",
            first_id + 2,
            json!({"path":"music.title","value":"Localhost Radio","revision":3}),
        ),
        Message::new(
            "state_patch",
            first_id + 3,
            json!({"path":"music.playing","value":true,"revision":4}),
        ),
    ]
}

fn session_id() -> String {
    let mut bytes = [0_u8; 16];
    if File::open("/dev/urandom")
        .and_then(|mut file| file.read_exact(&mut bytes))
        .is_err()
    {
        let fallback = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap_or_default()
            .as_nanos()
            .to_le_bytes();
        bytes.copy_from_slice(&fallback);
    }
    bytes[6] = (bytes[6] & 0x0f) | 0x40;
    bytes[8] = (bytes[8] & 0x3f) | 0x80;
    format!(
        "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
        bytes[0],
        bytes[1],
        bytes[2],
        bytes[3],
        bytes[4],
        bytes[5],
        bytes[6],
        bytes[7],
        bytes[8],
        bytes[9],
        bytes[10],
        bytes[11],
        bytes[12],
        bytes[13],
        bytes[14],
        bytes[15]
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::protocol::decode_frame;

    #[test]
    fn handshake_and_sync_responses_validate() {
        hello(1).validate().unwrap();
        let incoming = decode_frame(br#"{"version":1,"type":"hello","id":7,"payload":{"device_id":"watch","session_id":"s","runtime_version":"0.1","protocol_versions":[1],"simulator":true}}"#).unwrap();
        response_for(&incoming, 2).unwrap().validate().unwrap();
        let sync = decode_frame(
            br#"{"version":1,"type":"sync_request","id":8,"payload":{"last_revision":0}}"#,
        )
        .unwrap();
        response_for(&sync, 3).unwrap().validate().unwrap();
    }

    #[test]
    fn latency_configuration_is_bounded() {
        assert_eq!(parse_latency("0").unwrap(), Duration::ZERO);
        assert_eq!(parse_latency("100").unwrap(), Duration::from_millis(100));
        assert_eq!(parse_latency("300").unwrap(), Duration::from_millis(300));
        assert_eq!(parse_latency("1000").unwrap(), Duration::from_millis(1000));
        assert!(parse_latency("slow").is_err());
        assert!(parse_latency("10001").is_err());
    }

    #[test]
    fn music_actions_return_correlated_results() {
        let action = Message::new(
            "action",
            91,
            json!({
                "app_id": "music",
                "action_id": "set_playing",
                "arguments": { "playing": false }
            }),
        );
        let (accepted, desired) = music_action_response(&action, 92, false);
        assert_eq!(accepted.reply_to, Some(91));
        assert_eq!(accepted.payload["status"], "ok");
        assert_eq!(desired, Some(false));
        let (rejected, desired) = music_action_response(&action, 93, true);
        assert_eq!(rejected.payload["status"], "rejected");
        assert_eq!(desired, None);
    }
}
