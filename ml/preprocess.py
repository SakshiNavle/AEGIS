"""Streaming preprocessing orchestration and train-only statistics."""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass, field
from typing import Iterable, Iterator

from .config import DATA_CONTRACT_VERSION, PreprocessConfig
from .features import FEATURE_NAMES, FeatureState
from .load_autohack import CANRecord
from .sequences import FeatureSequence, build_sequences


@dataclass
class FittedPreprocessor:
    config: PreprocessConfig
    id_vocabulary: tuple[int, ...] = ()
    feature_min: dict[str, float] = field(default_factory=dict)
    feature_max: dict[str, float] = field(default_factory=dict)
    training_files: tuple[str, ...] = ()
    version: str = DATA_CONTRACT_VERSION

    def fit(
        self,
        records: Iterable[CANRecord],
        training_files: Iterable[str],
    ) -> "FittedPreprocessor":
        feature_min: dict[str, float] = {}
        feature_max: dict[str, float] = {}
        ids: set[int] = set()

        state = FeatureState(
            self.config.rolling_window,
            self.config.frequency_window,
        )

        current_stream: tuple[str, str] | None = None

        for record in records:
            stream = (record.source_file, record.interface)

            if stream != current_stream:
                state.reset()
                current_stream = stream

            row = state.update(record)
            ids.add(record.arbitration_id)

            for name in FEATURE_NAMES:
                value = float(row[name])

                feature_min[name] = (
                    value
                    if name not in feature_min
                    else min(feature_min[name], value)
                )

                feature_max[name] = (
                    value
                    if name not in feature_max
                    else max(feature_max[name], value)
                )

        self.id_vocabulary = tuple(sorted(ids))
        self.feature_min = feature_min
        self.feature_max = feature_max
        self.training_files = tuple(training_files)

        return self

    def transform_stream(
        self,
        records: Iterable[CANRecord],
    ) -> Iterator[FeatureSequence]:
        """Transform records and yield sequences incrementally."""

        state = FeatureState(
            self.config.rolling_window,
            self.config.frequency_window,
        )

        current_stream: tuple[str, str] | None = None

        def feature_records() -> Iterator[
            tuple[dict[str, float | int], str, str, str]
        ]:
            nonlocal state, current_stream

            for record in records:
                stream = (record.source_file, record.interface)

                if stream != current_stream:
                    state.reset()
                    current_stream = stream

                yield (
                    state.update(record),
                    record.label,
                    record.source_file,
                    record.interface,
                )

        yield from build_sequences(
            feature_records(),
            self.config.window_length,
        )

    def metadata(self) -> dict:
        return {
            "version": self.version,
            "config": self.config.__dict__,
            "id_vocabulary": list(self.id_vocabulary),
            "feature_min": self.feature_min,
            "feature_max": self.feature_max,
            "training_files": list(self.training_files),
        }


def binary_label(label: str) -> str:
    return "Normal" if label == "Normal" else "Anomaly"


def label_counts(records: Iterable[CANRecord]) -> Counter[str]:
    return Counter(record.label for record in records)