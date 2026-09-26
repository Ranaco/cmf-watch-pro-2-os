#!/usr/bin/env python3
"""Offline recovery-artifact inventory and verification tools.

This program never opens a device, invokes a flash tool, or modifies its input
artifacts. Its only write is an explicitly requested JSON output file.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
import xml.etree.ElementTree as ET

FORMAT_VERSION = 1
CHUNK_SIZE = 1024 * 1024


class RecoveryError(Exception):
    pass


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(CHUNK_SIZE):
            digest.update(chunk)
    return digest.hexdigest()


def atomic_json_write(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary_name = tempfile.mkstemp(
        prefix=f".{path.name}.", suffix=".tmp", dir=path.parent
    )
    try:
        with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as stream:
            json.dump(payload, stream, indent=2, sort_keys=True)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary_name, path)
    except BaseException:
        try:
            os.unlink(temporary_name)
        except FileNotFoundError:
            pass
        raise


def regular_files(root: Path, excluded: set[Path] | None = None) -> list[Path]:
    excluded = excluded or set()
    files: list[Path] = []
    for path in root.rglob("*"):
        resolved = path.resolve()
        if resolved in excluded:
            continue
        if path.is_symlink():
            raise RecoveryError(f"symbolic links are not accepted: {path}")
        if path.is_file():
            files.append(path)
    return sorted(files, key=lambda item: item.relative_to(root).as_posix())


def build_inventory(root: Path, output: Path | None = None) -> dict[str, object]:
    root = root.resolve()
    if not root.is_dir():
        raise RecoveryError(f"input directory does not exist: {root}")
    excluded = {output.resolve()} if output is not None else set()
    entries = []
    total_size = 0
    for path in regular_files(root, excluded):
        size = path.stat().st_size
        total_size += size
        entries.append(
            {
                "path": path.relative_to(root).as_posix(),
                "size": size,
                "sha256": sha256_file(path),
            }
        )
    return {
        "format": "cmf-recovery-inventory",
        "format_version": FORMAT_VERSION,
        "created_at": datetime.now(timezone.utc).isoformat(),
        "source_root": str(root),
        "file_count": len(entries),
        "total_size": total_size,
        "files": entries,
    }


def load_inventory(path: Path) -> dict[str, object]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise RecoveryError(f"cannot read inventory {path}: {error}") from error
    if payload.get("format") != "cmf-recovery-inventory":
        raise RecoveryError("not a CMF recovery inventory")
    if payload.get("format_version") != FORMAT_VERSION:
        raise RecoveryError("unsupported recovery inventory version")
    entries = payload.get("files")
    if not isinstance(entries, list):
        raise RecoveryError("inventory files must be an array")
    seen_paths: set[str] = set()
    for entry in entries:
        if not isinstance(entry, dict):
            raise RecoveryError("inventory entries must be objects")
        relative_path = entry.get("path")
        size = entry.get("size")
        digest = entry.get("sha256")
        if not isinstance(relative_path, str) or not relative_path:
            raise RecoveryError("inventory entry has an invalid path")
        parsed_path = PurePosixPath(relative_path)
        if parsed_path.is_absolute() or ".." in parsed_path.parts or relative_path in seen_paths:
            raise RecoveryError(f"unsafe or duplicate inventory path: {relative_path}")
        if not isinstance(size, int) or isinstance(size, bool) or size < 0:
            raise RecoveryError(f"invalid size for inventory path: {relative_path}")
        if not isinstance(digest, str) or re.fullmatch(r"[0-9a-f]{64}", digest) is None:
            raise RecoveryError(f"invalid SHA-256 for inventory path: {relative_path}")
        seen_paths.add(relative_path)
    return payload


def verify_inventory(root: Path, manifest: dict[str, object], manifest_path: Path) -> list[str]:
    root = root.resolve()
    expected = {entry["path"]: entry for entry in manifest["files"]}
    actual_paths = {
        path.relative_to(root).as_posix(): path
        for path in regular_files(root, {manifest_path.resolve()})
    }
    errors: list[str] = []
    for relative_path in sorted(expected.keys() - actual_paths.keys()):
        errors.append(f"missing: {relative_path}")
    for relative_path in sorted(actual_paths.keys() - expected.keys()):
        errors.append(f"unexpected: {relative_path}")
    for relative_path in sorted(expected.keys() & actual_paths.keys()):
        path = actual_paths[relative_path]
        entry = expected[relative_path]
        size = path.stat().st_size
        if size != entry["size"]:
            errors.append(
                f"size mismatch: {relative_path} expected={entry['size']} actual={size}"
            )
            continue
        digest = sha256_file(path)
        if digest != entry["sha256"]:
            errors.append(f"hash mismatch: {relative_path}")
    return errors


def compare_images(first: Path, second: Path) -> dict[str, object]:
    first_size = first.stat().st_size
    second_size = second.stat().st_size
    first_hash = sha256_file(first)
    second_hash = sha256_file(second)
    return {
        "identical": first_size == second_size and first_hash == second_hash,
        "first": {"path": str(first.resolve()), "size": first_size, "sha256": first_hash},
        "second": {"path": str(second.resolve()), "size": second_size, "sha256": second_hash},
    }


def parse_integer(value: str) -> int:
    try:
        parsed = int(value, 0)
    except ValueError as error:
        raise RecoveryError(f"invalid integer in OTA metadata: {value}") from error
    if parsed < 0:
        raise RecoveryError(f"negative integer in OTA metadata: {value}")
    return parsed


def required_text(parent: ET.Element, name: str) -> str:
    child = parent.find(name)
    if child is None or child.text is None or not child.text.strip():
        raise RecoveryError(f"missing OTA metadata field: {name}")
    return child.text.strip()


def parse_ota_metadata(path: Path) -> dict[str, object]:
    raw = path.read_bytes()
    upper = raw.upper()
    if b"<!DOCTYPE" in upper or b"<!ENTITY" in upper:
        raise RecoveryError("DTD and entity declarations are not accepted")
    try:
        root = ET.fromstring(raw)
    except ET.ParseError as error:
        raise RecoveryError(f"invalid OTA XML: {error}") from error
    if root.tag != "ota_firmware":
        raise RecoveryError(f"unexpected OTA root element: {root.tag}")
    firmware = root.find("firmware_version")
    partitions = root.find("partitions")
    if firmware is None or partitions is None:
        raise RecoveryError("OTA metadata lacks firmware_version or partitions")

    parsed_partitions = []
    seen_names: set[str] = set()
    for partition in partitions.findall("partition"):
        name = required_text(partition, "name")
        if name in seen_names:
            raise RecoveryError(f"duplicate partition name: {name}")
        seen_names.add(name)
        size_text = required_text(partition, "file_size")
        original_size_text = required_text(partition, "orig_size")
        parsed_partitions.append(
            {
                "type": required_text(partition, "type"),
                "name": name,
                "file_id": parse_integer(required_text(partition, "file_id")),
                "storage_id": parse_integer(required_text(partition, "storage_id")),
                "file_name": required_text(partition, "file_name"),
                "file_size": parse_integer(size_text),
                "original_size": parse_integer(original_size_text),
                "version": parse_integer(required_text(partition, "version")),
                "checksum": required_text(partition, "checksum"),
            }
        )
    declared_count = parse_integer(required_text(partitions, "partitionsNum"))
    if declared_count != len(parsed_partitions):
        raise RecoveryError(
            f"partition count mismatch: declared={declared_count} parsed={len(parsed_partitions)}"
        )
    return {
        "format": "cmf-ota-metadata-summary",
        "source": str(path.resolve()),
        "source_sha256": hashlib.sha256(raw).hexdigest(),
        "firmware": {
            "version_code": required_text(firmware, "version_code"),
            "version_res": required_text(firmware, "version_res"),
            "version_name": required_text(firmware, "version_name"),
            "board_name": required_text(firmware, "board_name"),
        },
        "partition_count": declared_count,
        "partitions": parsed_partitions,
    }


def output_json(payload: object, output: Path | None) -> None:
    if output is None:
        json.dump(payload, sys.stdout, indent=2, sort_keys=True)
        sys.stdout.write("\n")
    else:
        atomic_json_write(output.resolve(), payload)


def command_inventory(args: argparse.Namespace) -> int:
    output = args.output.resolve()
    output_json(build_inventory(args.input, output), output)
    print(f"inventory written: {output}")
    return 0


def command_verify(args: argparse.Namespace) -> int:
    manifest_path = args.manifest.resolve()
    errors = verify_inventory(args.input, load_inventory(manifest_path), manifest_path)
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1
    print("inventory verification passed")
    return 0


def command_compare(args: argparse.Namespace) -> int:
    result = compare_images(args.first, args.second)
    output_json(result, args.output)
    return 0 if result["identical"] else 1


def command_ota_info(args: argparse.Namespace) -> int:
    output_json(parse_ota_metadata(args.input), args.output)
    return 0


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    inventory = subparsers.add_parser("inventory", help="hash every artifact in a directory")
    inventory.add_argument("--input", required=True, type=Path)
    inventory.add_argument("--output", required=True, type=Path)
    inventory.set_defaults(handler=command_inventory)

    verify = subparsers.add_parser("verify", help="verify a directory against an inventory")
    verify.add_argument("--input", required=True, type=Path)
    verify.add_argument("--manifest", required=True, type=Path)
    verify.set_defaults(handler=command_verify)

    compare = subparsers.add_parser("compare", help="compare two independent binary reads")
    compare.add_argument("--first", required=True, type=Path)
    compare.add_argument("--second", required=True, type=Path)
    compare.add_argument("--output", type=Path)
    compare.set_defaults(handler=command_compare)

    ota_info = subparsers.add_parser("ota-info", help="summarize an OTA info.xml without extraction")
    ota_info.add_argument("--input", required=True, type=Path)
    ota_info.add_argument("--output", type=Path)
    ota_info.set_defaults(handler=command_ota_info)
    return parser


def main() -> int:
    try:
        args = make_parser().parse_args()
        return args.handler(args)
    except (OSError, RecoveryError, KeyError, TypeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
