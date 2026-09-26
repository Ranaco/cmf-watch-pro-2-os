import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]
MODULE_PATH = PROJECT_ROOT / "tools" / "recovery_tool.py"
SPEC = importlib.util.spec_from_file_location("recovery_tool", MODULE_PATH)
recovery_tool = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(recovery_tool)


class RecoveryToolTests(unittest.TestCase):
    def test_inventory_is_sorted_and_detects_changes(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "b.bin").write_bytes(b"second")
            (root / "a.bin").write_bytes(b"first")
            manifest_path = root / "inventory.json"
            manifest = recovery_tool.build_inventory(root, manifest_path)
            self.assertEqual([entry["path"] for entry in manifest["files"]], ["a.bin", "b.bin"])
            recovery_tool.atomic_json_write(manifest_path, manifest)
            self.assertEqual(
                recovery_tool.verify_inventory(root, manifest, manifest_path), []
            )
            (root / "a.bin").write_bytes(b"changed")
            self.assertTrue(
                any(
                    "mismatch: a.bin" in error
                    for error in recovery_tool.verify_inventory(root, manifest, manifest_path)
                )
            )

    def test_compare_requires_identical_size_and_hash(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            first = root / "read-1.bin"
            second = root / "read-2.bin"
            first.write_bytes(b"verified image")
            second.write_bytes(b"verified image")
            self.assertTrue(recovery_tool.compare_images(first, second)["identical"])
            second.write_bytes(b"modified image")
            self.assertFalse(recovery_tool.compare_images(first, second)["identical"])

    def test_inventory_rejects_unsafe_paths(self):
        with tempfile.TemporaryDirectory() as temporary:
            manifest_path = Path(temporary) / "inventory.json"
            manifest_path.write_text(
                json.dumps(
                    {
                        "format": "cmf-recovery-inventory",
                        "format_version": 1,
                        "files": [
                            {"path": "../outside.bin", "size": 1, "sha256": "0" * 64}
                        ],
                    }
                ),
                encoding="utf-8",
            )
            with self.assertRaises(recovery_tool.RecoveryError):
                recovery_tool.load_inventory(manifest_path)

    def test_ota_metadata_summary(self):
        with tempfile.TemporaryDirectory() as temporary:
            metadata = Path(temporary) / "info.xml"
            metadata.write_text(
                """<?xml version="1.0"?>
<ota_firmware>
  <firmware_version>
    <version_code>0x10000</version_code><version_res>0x10000</version_res>
    <version_name>test</version_name><board_name>jx402_01_3089c</board_name>
  </firmware_version>
  <partitions><partitionsNum>1</partitionsNum><partition>
    <type>DATA</type><name>res_a</name><file_id>10</file_id><storage_id>2</storage_id>
    <file_name>res.bin</file_name><file_size>0x20</file_size><orig_size>0x40</orig_size>
    <version>5</version><checksum>0x12345678</checksum>
  </partition></partitions>
</ota_firmware>
""",
                encoding="utf-8",
            )
            summary = recovery_tool.parse_ota_metadata(metadata)
            self.assertEqual(summary["firmware"]["board_name"], "jx402_01_3089c")
            self.assertEqual(summary["partitions"][0]["file_size"], 32)
            self.assertEqual(summary["partition_count"], 1)
            json.dumps(summary)

    def test_ota_metadata_rejects_entities(self):
        with tempfile.TemporaryDirectory() as temporary:
            metadata = Path(temporary) / "info.xml"
            metadata.write_text(
                '<!DOCTYPE x [<!ENTITY unsafe SYSTEM "file:///etc/passwd">]><ota_firmware/>',
                encoding="utf-8",
            )
            with self.assertRaises(recovery_tool.RecoveryError):
                recovery_tool.parse_ota_metadata(metadata)


if __name__ == "__main__":
    unittest.main()
