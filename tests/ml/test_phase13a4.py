import json
import tempfile
import unittest
from pathlib import Path

from ml.features import FEATURE_NAMES
from ml.train_guardcan_baseline import OVERLAP_METADATA, WINDOW_LENGTH, binary_label
from ml.evaluate_guardcan_baseline import binary_metrics


class Phase13A4Tests(unittest.TestCase):
    def test_feature_contract_and_window(self):
        self.assertEqual(len(FEATURE_NAMES), 7)
        self.assertEqual(
            FEATURE_NAMES,
            ("delta_t", "rolling_mean_delta_t", "rolling_std_delta_t",
             "id_frequency_ratio", "payload_hamming_distance", "dlc", "arbitration_id"),
        )
        self.assertEqual(WINDOW_LENGTH, 50)

    def test_binary_labels(self):
        self.assertEqual(binary_label("Normal"), 0)
        self.assertEqual(binary_label("Fuzzing"), 1)
        with self.assertRaises(ValueError):
            binary_label("")

    def test_metrics_schema_and_confusion_shape(self):
        metrics = binary_metrics([0, 1, 1, 0], [0.1, 0.9, 0.8, 0.2])
        for key in ("accuracy", "precision", "recall", "f1", "macro_f1", "roc_auc", "pr_auc", "confusion_matrix"):
            self.assertIn(key, metrics)
        self.assertEqual(len(metrics["confusion_matrix"]), 2)
        self.assertEqual(len(metrics["confusion_matrix"][0]), 2)

    def test_overlap_metadata_and_artifact_path(self):
        self.assertEqual(OVERLAP_METADATA["accepted_exact_train_test_overlap_count"], 85)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "training_config.json"
            path.write_text(json.dumps(OVERLAP_METADATA), encoding="utf-8")
            self.assertEqual(json.loads(path.read_text())["overlap_handling"], "preserved_and_disclosed")


if __name__ == "__main__":
    unittest.main()
