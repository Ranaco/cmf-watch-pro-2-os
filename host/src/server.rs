use crate::protocol::{FrameDecoder, Message, encode_frame};
use crate::state::command_result;
use serde_json::{Value, json};
use std::fs::File;
use std::io::{self, Read, Write};
use std::net::{TcpListener, TcpStream};

pub const DEFAULT_ADDRESS: &str = "127.0.0.1:4660";

pub fn run(address: &str) -> io::Result<()> {
    let listener = TcpListener::bind(address)?;
    eprintln!("CMF watch host listening on {address}");
    for connection in listener.incoming() {
        match connection.and_then(handle_connection) {
            Ok(()) => eprintln!("watch disconnected"),
            Err(error) => eprintln!("connection error: {error}"),
        }
    }
    Ok(())
}

fn handle_connection(mut stream: TcpStream) -> io::Result<()> {
    eprintln!("watch connected from {}", stream.peer_addr()?);
    let mut next_id = 1;
    send(&mut stream, &hello(next_id))?;
    next_id += 1;
    let mut decoder = FrameDecoder::default();
    let mut bytes = [0_u8; 2048];
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
                send(&mut stream, &updates[0])?;
                next_id += 1;
                if let Some(response) = response_for(&message, next_id) {
                    send(&mut stream, &response)?;
                    next_id += 1;
                }
                for mut update in updates.into_iter().skip(1) {
                    update.id = next_id;
                    send(&mut stream, &update)?;
                    next_id += 1;
                }
            } else if let Some(response) = response_for(&message, next_id) {
                send(&mut stream, &response)?;
                next_id += 1;
            }
        }
    }
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
        "notifications": [{
            "id": "notif_welcome",
            "title": "Host connected",
            "body": "Your watch data is synchronized."
        }],
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
}
