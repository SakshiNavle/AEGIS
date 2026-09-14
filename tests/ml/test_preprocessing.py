import unittest

from ml.config import PreprocessConfig
from ml.features import FeatureState, hamming_distance
from ml.leakage_audit import audit_feature_names, audit_split_disjointness
from ml.load_autohack import QualityReport, parse_row
from ml.preprocess import FittedPreprocessor
from ml.sequences import build_sequences
from ml.splits import file_split


class PreprocessingTests(unittest.TestCase):
    def record(self, timestamp="1.0", data="01 02", dlc="2", label="Normal"):
        return parse_row({
            "Interface": "C-CAN", "Timestamp": timestamp, "Arbitration_ID": "123",
            "DLC": dlc, "Data": data, "Label": label,
        }, "capture.csv", 2)

    def test_parsing_and_validation(self):
        record = self.record()
        self.assertEqual(record.arbitration_id, 0x123)
        self.assertEqual(record.payload, b"\x01\x02")
        with self.assertRaisesRegex(ValueError, "payload_dlc_mismatch"):
            self.record(data="01", dlc="2")

    def test_causal_features_and_boundary_reset(self):
        state = FeatureState(rolling_window=2, frequency_window=2)
        first = state.update(self.record("1.0"))
        second = state.update(self.record("1.5", data="01 03"))
        self.assertEqual(first["delta_t"], 0.0)
        self.assertEqual(second["delta_t"], 0.5)
        self.assertEqual(second["payload_hamming_distance"], 1)
        state.reset()
        self.assertEqual(state.update(self.record("10.0"))["delta_t"], 0.0)

    def test_hamming_and_variable_dlc(self):
        self.assertEqual(hamming_distance(b"\x00", b"\xff\x00"), 8)
        self.assertEqual(hamming_distance(b"", b"\xff"), 0)

    def test_sequences_do_not_cross_boundaries(self):
        rows = [
            ({"delta_t": 0.1}, "Normal", "a.csv", "C-CAN"),
            ({"delta_t": 0.2}, "Normal", "a.csv", "C-CAN"),
        ]

        self.assertEqual(len(list(build_sequences(rows, 2))), 1)

        cross_boundary_rows = [
            ({"delta_t": 0.1}, "Normal", "a.csv", "C-CAN"),
            ({"delta_t": 0.2}, "Normal", "b.csv", "C-CAN"),
        ]

        with self.assertRaises(ValueError):
            list(build_sequences(cross_boundary_rows, 2))

    def test_split_integrity_and_leakage(self):
        split = file_split(["a", "b", "c"], ["c"], ["b"])
        self.assertEqual(split.train, ("a",))
        self.assertFalse(audit_feature_names(["Label", "delta_t"]).passed)
        self.assertFalse(audit_split_disjointness({"train": ["a"], "test": ["a"]}).passed)

    def test_quality_report_defaults(self):
        report = QualityReport()
        self.assertEqual(report.input_rows, 0)

    def test_preprocessing_fit_uses_training_records_only(self):
        training = [self.record("1.0"), self.record("1.5", data="01 03")]
        held_out = [self.record("100.0", data="ff ff")]
        fitted = FittedPreprocessor(PreprocessConfig()).fit(training, ["capture.csv"])
        self.assertNotIn(0x999, fitted.id_vocabulary)
        self.assertLess(fitted.feature_max["delta_t"], 100.0)
        self.assertEqual(fitted.training_files, ("capture.csv",))
        fitted.fit(held_out, ["heldout.csv"])
        self.assertEqual(fitted.training_files, ("heldout.csv",))


if __name__ == "__main__":
    unittest.main()
