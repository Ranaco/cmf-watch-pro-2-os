use serde::{Deserialize, Serialize};
use serde_json::{Map, Value};
use std::collections::BTreeMap;
use std::fmt;

pub const PROTOCOL_VERSION: u16 = 1;
pub const MAX_FRAME_BYTES: usize = 16_384;

const MESSAGE_TYPES: &[&str] = &[
    "state_set",
    "state_patch",
    "list_insert",
    "list_remove",
    "list_update",
    "refresh",
    "app_open",
    "show_toast",
    "show_dialog",
    "cache_invalidate",
    "app_opened",
    "action",
    "button",
    "gesture",
    "input",
    "refresh_request",
    "hello",
    "capabilities",
    "ping",
    "pong",
    "sync_request",
    "sync_response",
    "command_result",
];

#[derive(Clone, Debug, Deserialize, PartialEq, Serialize)]
pub struct Message {
    pub version: u16,
    #[serde(rename = "type")]
    pub kind: String,
    pub id: u32,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub reply_to: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub timestamp_ms: Option<u64>,
    pub payload: Value,
    #[serde(flatten)]
    pub extra: BTreeMap<String, Value>,
}

impl Message {
    pub fn new(kind: impl Into<String>, id: u32, payload: Value) -> Self {
        Self {
            version: PROTOCOL_VERSION,
            kind: kind.into(),
            id,
            reply_to: None,
            timestamp_ms: None,
            payload,
            extra: BTreeMap::new(),
        }
    }

    pub fn validate(&self) -> Result<(), ProtocolError> {
        if self.version != PROTOCOL_VERSION {
            return Err(ProtocolError::UnsupportedVersion(self.version));
        }
        if !MESSAGE_TYPES.contains(&self.kind.as_str()) {
            return Err(ProtocolError::UnknownType(self.kind.clone()));
        }
        let payload = self
            .payload
            .as_object()
            .ok_or(ProtocolError::PayloadNotObject)?;
        for field in required_fields(&self.kind) {
            if !payload.contains_key(*field) {
                return Err(ProtocolError::MissingField(field));
            }
        }
        if self.kind == "command_result" && self.reply_to.is_none() {
            return Err(ProtocolError::MissingField("reply_to"));
        }
        Ok(())
    }
}

fn required_fields(kind: &str) -> &'static [&'static str] {
    match kind {
        "hello" => &[
            "device_id",
            "session_id",
            "runtime_version",
            "protocol_versions",
            "simulator",
        ],
        "capabilities" => &["features", "apps", "display"],
        "ping" | "pong" => &["nonce"],
        "sync_request" => &["last_revision"],
        "sync_response" => &["revision", "state"],
        "state_set" | "state_patch" => &["path", "value", "revision"],
        "list_insert" | "list_update" => &["path", "index", "value", "revision"],
        "list_remove" => &["path", "index", "revision"],
        "refresh" => &["scope"],
        "app_open" => &["app_id"],
        "show_toast" => &["message", "duration_ms"],
        "show_dialog" => &["title", "message", "actions"],
        "cache_invalidate" => &["path"],
        "app_opened" => &["app_id"],
        "action" => &["app_id", "action_id", "arguments"],
        "button" => &["button", "gesture"],
        "gesture" => &["gesture"],
        "input" => &["input_id", "value"],
        "refresh_request" => &["scope"],
        "command_result" => &["status", "code"],
        _ => &[],
    }
}

#[derive(Debug, PartialEq)]
pub enum ProtocolError {
    EmptyFrame,
    FrameTooLarge,
    InvalidUtf8,
    InvalidJson(String),
    UnsupportedVersion(u16),
    UnknownType(String),
    PayloadNotObject,
    MissingField(&'static str),
}

impl fmt::Display for ProtocolError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{self:?}")
    }
}

impl std::error::Error for ProtocolError {}

pub fn decode_frame(frame: &[u8]) -> Result<Message, ProtocolError> {
    let frame = frame.strip_suffix(b"\r").unwrap_or(frame);
    if frame.is_empty() {
        return Err(ProtocolError::EmptyFrame);
    }
    if frame.len() > MAX_FRAME_BYTES {
        return Err(ProtocolError::FrameTooLarge);
    }
    let text = std::str::from_utf8(frame).map_err(|_| ProtocolError::InvalidUtf8)?;
    let message: Message = serde_json::from_str(text)
        .map_err(|error| ProtocolError::InvalidJson(error.to_string()))?;
    message.validate()?;
    Ok(message)
}

pub fn encode_frame(message: &Message) -> Result<Vec<u8>, ProtocolError> {
    message.validate()?;
    let mut bytes = serde_json::to_vec(message)
        .map_err(|error| ProtocolError::InvalidJson(error.to_string()))?;
    if bytes.len() > MAX_FRAME_BYTES {
        return Err(ProtocolError::FrameTooLarge);
    }
    bytes.push(b'\n');
    Ok(bytes)
}

#[derive(Default)]
pub struct FrameDecoder {
    buffered: Vec<u8>,
}

impl FrameDecoder {
    pub fn push(&mut self, bytes: &[u8]) -> Result<Vec<Message>, ProtocolError> {
        self.buffered.extend_from_slice(bytes);
        let mut messages = Vec::new();
        while let Some(end) = self.buffered.iter().position(|byte| *byte == b'\n') {
            let mut frame: Vec<u8> = self.buffered.drain(..=end).collect();
            frame.pop();
            if frame == b"\r" || frame.is_empty() {
                continue;
            }
            messages.push(decode_frame(&frame)?);
        }
        if self.buffered.len() > MAX_FRAME_BYTES {
            self.buffered.clear();
            return Err(ProtocolError::FrameTooLarge);
        }
        Ok(messages)
    }
}

pub fn object(value: &Value) -> Result<&Map<String, Value>, ProtocolError> {
    value.as_object().ok_or(ProtocolError::PayloadNotObject)
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn streams_fragmented_crlf_and_ignores_empty_frames() {
        let mut decoder = FrameDecoder::default();
        assert!(
            decoder
                .push(b"\n{\"version\":1,\"type\":\"ping\",")
                .unwrap()
                .is_empty()
        );
        let messages = decoder
            .push(b"\"id\":4,\"payload\":{\"nonce\":9}}\r\n")
            .unwrap();
        assert_eq!(messages.len(), 1);
        assert_eq!(messages[0].kind, "ping");
    }

    #[test]
    fn accepts_additive_fields_but_rejects_unknown_types() {
        let message = decode_frame(
            br#"{"version":1,"type":"ping","id":1,"future":true,"payload":{"nonce":7,"more":1}}"#,
        )
        .unwrap();
        assert_eq!(message.extra["future"], json!(true));
        assert!(matches!(
            decode_frame(br#"{"version":1,"type":"pixels","id":1,"payload":{}}"#),
            Err(ProtocolError::UnknownType(_))
        ));
    }

    #[test]
    fn rejects_invalid_utf8_and_oversized_partial_frames() {
        assert_eq!(decode_frame(&[0xff]), Err(ProtocolError::InvalidUtf8));
        let mut decoder = FrameDecoder::default();
        assert_eq!(
            decoder.push(&vec![b'x'; MAX_FRAME_BYTES + 1]),
            Err(ProtocolError::FrameTooLarge)
        );
    }
}
