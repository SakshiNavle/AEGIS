import json
import tempfile
import unittest
from pathlib import Path

from ml.config import EXPECTED_COLUMNS
from ml.load_autohack import parse_row
from ml.phase13a import (
    ALLOWED_FEATURES,
    build_manifest,
    detect_future_window_access,
    discover_canonical_files,
    row_fingerprint,
    validate_feature_contract,
    write_manifest,
)


def write_csv(root: Path, relative: str, rows: list[dict[str, str]]) -> None:
    path = root / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [",".join(EXPECTED_COLUMNS)]
    for row in rows:
        lines.append(",".join(row[column] for column in EXPECTED_COLUMNS))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def row(interface="C-CAN", timestamp="1.0", arbitration_id="123", label="Normal"):
    return {
        "Interface": interface,
        "Timestamp": timestamp,
        "Arbitration_ID": arbitration_id,
        "DLC": "2",
        "Data": "01 02",
        "Label": label,
    }


class Phase13ATests(unittest.TestCase):
    def fixture(self):
        root = Path(self.tempdir.name)
        train = "extracted/Autohack2025_Dataset/Interface/train/autohack_train_both_interface.csv"
        test = "extracted/Autohack2025_Dataset/Interface/test/autohack_test_both_interface.csv"
        write_csv(root, train, [row(timestamp=str(i)) for i in range(1, 6)])
        write_csv(root, test, [row(timestamp="6", label="Fuzzing")])
        return root

    def setUp(self):
        self.tempdir = tempfile.TemporaryDirectory()

    def tearDown(self):
        self.tempdir.cleanup()

    def test_discovery_and_missing_root(self):
        root = self.fixture()
        train, test = discover_canonical_files(root)
        self.assertTrue(train.is_file())
        self.assertTrue(test.is_file())
        with self.assertRaises(FileNotFoundError):
            discover_canonical_files(root / "missing")

    def test_manifest_is_deterministic_and_records_boundaries(self):
        root = self.fixture()
        first = build_manifest(root)
        second = build_manifest(root)
        self.assertEqual(first, second)
        self.assertEqual(first["status"], "PASSED_WITH_LIMITATIONS")
        self.assertEqual(first["recommendation"], "READY_FOR_PHASE13A_4")
        self.assertFalse(first["split_policy"]["row_level_random_split"])
        self.assertEqual(first["row_overlap"]["status"], "EXHAUSTIVE")
        self.assertTrue(first["sequence_boundary_checks"]["passed"])
        self.assertEqual(first["grouping"]["vehicle"], "NOT_VERIFIED")
        self.assertTrue(first["unseen_attack_integrity"]["test_contains_labels_seen_in_training"] is False)

    def test_manifest_write_and_fingerprint_consistency(self):
        record = parse_row(row(), "capture.csv", 2)
        self.assertEqual(row_fingerprint(record), row_fingerprint(record))
        output = Path(self.tempdir.name) / "manifest.json"
        write_manifest({"phase": "13A"}, output)
        self.assertEqual(json.loads(output.read_text())["phase"], "13A")

    def test_temporal_future_and_feature_checks(self):
        self.assertTrue(detect_future_window_access([1.0, 2.0, 2.0])["passed"])
        self.assertFalse(detect_future_window_access([2.0, 1.0])["passed"])
        self.assertFalse(validate_feature_contract(["delta_t", "Label"])["passed"])
        self.assertEqual(tuple(ALLOWED_FEATURES), (
            "delta_t", "rolling_mean_delta_t", "rolling_std_delta_t",
            "id_frequency_ratio", "payload_hamming_distance", "dlc", "arbitration_id",
        ))

    def test_train_only_preprocessing_metadata_is_explicit(self):
        manifest = build_manifest(self.fixture())
        checks = manifest["preprocessing_leakage_checks"]
        self.assertTrue(checks["passed"])
        self.assertEqual(checks["fit_scope"], "training portion only")
        self.assertFalse(checks["validation_test_contribute_to_min_max"])
        self.assertFalse(checks["test_contribute_to_id_vocabulary"])


if __name__ == "__main__":
    unittest.main()
