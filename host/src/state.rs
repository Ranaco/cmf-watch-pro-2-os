use crate::protocol::{Message, ProtocolError, object};
use serde_json::{Map, Value, json};
use std::collections::BTreeMap;

const HOST_ROOTS: &[&str] = &[
    "weather",
    "notifications",
    "music",
    "calendar",
    "assistant",
    "steps",
];
const LEAF_PATHS: &[&str] = &[
    "weather.temperature",
    "weather.condition",
    "music.playing",
    "music.title",
    "steps",
];

#[derive(Debug, PartialEq)]
pub enum Disposition {
    Applied,
    IgnoredStale,
    SyncRequired,
    Buffered,
}

#[derive(Debug)]
pub enum StateError {
    Protocol(ProtocolError),
    Missing(&'static str),
    Invalid(&'static str),
    UnregisteredPath(String),
    WatchOwnedPath(String),
}

impl From<ProtocolError> for StateError {
    fn from(value: ProtocolError) -> Self {
        Self::Protocol(value)
    }
}

#[derive(Debug)]
pub struct StateStore {
    state: Value,
    revision: u64,
    syncing: bool,
    buffered: BTreeMap<u64, Message>,
}

impl StateStore {
    pub fn new(state: Value, revision: u64) -> Self {
        assert!(state.is_object());
        Self {
            state,
            revision,
            syncing: false,
            buffered: BTreeMap::new(),
        }
    }

    pub fn state(&self) -> &Value {
        &self.state
    }
    pub fn revision(&self) -> u64 {
        self.revision
    }
    pub fn is_syncing(&self) -> bool {
        self.syncing
    }

    pub fn apply(&mut self, message: Message) -> Result<Disposition, StateError> {
        message.validate()?;
        if !is_mutation(&message.kind) {
            return Err(StateError::Invalid("not a state mutation"));
        }
        let revision = payload_u64(&message, "revision")?;
        if self.syncing {
            if revision > self.revision {
                self.buffered.entry(revision).or_insert(message);
            }
            return Ok(Disposition::Buffered);
        }
        if revision <= self.revision {
            return Ok(Disposition::IgnoredStale);
        }
        if revision != self.revision + 1 {
            self.syncing = true;
            self.buffered.insert(revision, message);
            return Ok(Disposition::SyncRequired);
        }
        self.apply_current(&message)?;
        self.revision = revision;
        Ok(Disposition::Applied)
    }

    pub fn install_snapshot(&mut self, message: &Message) -> Result<(), StateError> {
        message.validate()?;
        if message.kind != "sync_response" {
            return Err(StateError::Invalid("not a sync response"));
        }
        let payload = object(&message.payload)?;
        let revision = payload
            .get("revision")
            .and_then(Value::as_u64)
            .ok_or(StateError::Missing("revision"))?;
        let snapshot = payload
            .get("state")
            .filter(|value| value.is_object())
            .ok_or(StateError::Invalid("snapshot state"))?
            .clone();

        self.state = snapshot;
        self.revision = revision;
        self.buffered
            .retain(|queued_revision, _| *queued_revision > revision);
        while let Some(message) = self.buffered.remove(&(self.revision + 1)) {
            let next_revision = self.revision + 1;
            self.apply_current(&message)?;
            self.revision = next_revision;
        }
        self.syncing = self
            .buffered
            .keys()
            .next()
            .is_some_and(|next| *next > self.revision + 1);
        Ok(())
    }

    fn apply_current(&mut self, message: &Message) -> Result<(), StateError> {
        let payload = object(&message.payload)?;
        let path = payload
            .get("path")
            .and_then(Value::as_str)
            .ok_or(StateError::Missing("path"))?;
        validate_host_path(path, message.kind == "state_patch")?;
        match message.kind.as_str() {
            "state_set" | "state_patch" => {
                let value = payload
                    .get("value")
                    .ok_or(StateError::Missing("value"))?
                    .clone();
                replace_path(&mut self.state, path, value)
            }
            "list_insert" | "list_update" | "list_remove" => apply_list(&mut self.state, message),
            _ => Err(StateError::Invalid("unsupported mutation")),
        }
    }
}

fn is_mutation(kind: &str) -> bool {
    matches!(
        kind,
        "state_set" | "state_patch" | "list_insert" | "list_update" | "list_remove"
    )
}

fn payload_u64(message: &Message, field: &'static str) -> Result<u64, StateError> {
    object(&message.payload)?
        .get(field)
        .and_then(Value::as_u64)
        .ok_or(StateError::Missing(field))
}

fn validate_host_path(path: &str, leaf_only: bool) -> Result<(), StateError> {
    if path.is_empty() || path.split('.').any(|segment| segment.is_empty()) {
        return Err(StateError::UnregisteredPath(path.into()));
    }
    let root = path.split('.').next().unwrap_or_default();
    if matches!(root, "system" | "navigation" | "input") {
        return Err(StateError::WatchOwnedPath(path.into()));
    }
    if !HOST_ROOTS.contains(&root) || (leaf_only && !LEAF_PATHS.contains(&path)) {
        return Err(StateError::UnregisteredPath(path.into()));
    }
    Ok(())
}

fn replace_path(state: &mut Value, path: &str, value: Value) -> Result<(), StateError> {
    let mut segments = path.split('.').peekable();
    let mut current = state;
    while let Some(segment) = segments.next() {
        let object = current
            .as_object_mut()
            .ok_or(StateError::Invalid("non-object path parent"))?;
        if segments.peek().is_none() {
            object.insert(segment.into(), value);
            return Ok(());
        }
        current = object
            .entry(segment)
            .or_insert_with(|| Value::Object(Map::new()));
    }
    Err(StateError::Invalid("empty path"))
}

fn apply_list(state: &mut Value, message: &Message) -> Result<(), StateError> {
    let payload = object(&message.payload)?;
    let path = payload
        .get("path")
        .and_then(Value::as_str)
        .ok_or(StateError::Missing("path"))?;
    validate_host_path(path, false)?;
    let index = payload
        .get("index")
        .and_then(Value::as_u64)
        .ok_or(StateError::Missing("index"))? as usize;
    let mut current = state;
    for segment in path.split('.') {
        current = current
            .get_mut(segment)
            .ok_or(StateError::Invalid("missing list path"))?;
    }
    let list = current
        .as_array_mut()
        .ok_or(StateError::Invalid("path is not a list"))?;
    match message.kind.as_str() {
        "list_insert" if index <= list.len() => {
            list.insert(
                index,
                payload
                    .get("value")
                    .ok_or(StateError::Missing("value"))?
                    .clone(),
            );
            Ok(())
        }
        "list_update" if index < list.len() => {
            list[index] = payload
                .get("value")
                .ok_or(StateError::Missing("value"))?
                .clone();
            Ok(())
        }
        "list_remove" if index < list.len() => {
            list.remove(index);
            Ok(())
        }
        _ => Err(StateError::Invalid("list index")),
    }
}

pub fn command_result(command: &Message, id: u32, status: &str, code: &str) -> Message {
    let mut result = Message::new(
        "command_result",
        id,
        json!({ "status": status, "code": code }),
    );
    result.reply_to = Some(command.id);
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::protocol::decode_frame;
    use serde::Deserialize;
    use std::fs;
    use std::path::PathBuf;

    #[derive(Deserialize)]
    struct Fixture {
        message: Value,
        initial: Option<Value>,
        expected: Value,
    }

    #[test]
    fn mutation_fixtures_match() {
        let fixture_dir = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../protocol/fixtures");
        for entry in fs::read_dir(fixture_dir).unwrap() {
            let path = entry.unwrap().path();
            let fixture: Fixture = serde_json::from_slice(&fs::read(&path).unwrap()).unwrap();
            if fixture.initial.is_none() {
                continue;
            }
            let initial = fixture.initial.unwrap();
            let revision = initial["revision"].as_u64().unwrap();
            let mut store = StateStore::new(initial["state"].clone(), revision);
            let message =
                decode_frame(serde_json::to_string(&fixture.message).unwrap().as_bytes()).unwrap();
            let disposition = store.apply(message).unwrap();
            let name = path.file_name().unwrap().to_string_lossy();
            let expected_disposition = fixture.expected["disposition"].as_str().unwrap();
            let actual = match disposition {
                Disposition::Applied => "applied",
                Disposition::IgnoredStale => "ignored_stale",
                Disposition::SyncRequired => "sync_required",
                Disposition::Buffered => "buffered",
            };
            assert_eq!(actual, expected_disposition, "{name}");
            assert_eq!(
                store.revision(),
                fixture.expected["revision"].as_u64().unwrap(),
                "{name}"
            );
            assert_eq!(store.state(), &fixture.expected["state"], "{name}");
        }
    }

    #[test]
    fn snapshot_is_atomic_and_replays_contiguous_buffer() {
        let mut store = StateStore::new(json!({"weather":{"temperature":20}}), 10);
        let gap = Message::new(
            "state_patch",
            1,
            json!({"path":"weather.temperature","value":22,"revision":12}),
        );
        assert_eq!(store.apply(gap).unwrap(), Disposition::SyncRequired);
        let snapshot = Message::new(
            "sync_response",
            2,
            json!({"revision":11,"state":{"weather":{"temperature":21}}}),
        );
        store.install_snapshot(&snapshot).unwrap();
        assert_eq!(store.revision(), 12);
        assert_eq!(store.state()["weather"]["temperature"], 22);
        assert!(!store.is_syncing());
    }

    #[test]
    fn watch_owned_paths_are_rejected() {
        let mut store = StateStore::new(json!({}), 0);
        let message = Message::new(
            "state_set",
            1,
            json!({"path":"system.battery","value":100,"revision":1}),
        );
        assert!(matches!(
            store.apply(message),
            Err(StateError::WatchOwnedPath(_))
        ));
        assert_eq!(store.revision(), 0);
    }
}
