"""Train the single Phase13A.4 GuardCAN LSTM baseline."""

from __future__ import annotations

import argparse
import json
import os
import random
import time
from collections import Counter, defaultdict, deque
from pathlib import Path
from typing import Iterator, Mapping

import numpy as np
import tensorflow as tf

from .config import PreprocessConfig
from .features import FEATURE_NAMES, FeatureState
from .load_autohack import CANRecord, iter_records
from .phase13a import discover_canonical_files
from .preprocess import FittedPreprocessor

WINDOW_LENGTH = 50
SEED = 13
OVERLAP_METADATA = {
    "human_approved_dataset_limitation": True,
    "accepted_exact_train_test_overlap_count": 85,
    "overlap_handling": "preserved_and_disclosed",
    "phase13a_manifest_status": "FAILED",
    "phase13a_recommendation": "BLOCKED_PENDING_DATA_VALIDATION",
}


def load_frozen_manifest(path: str | Path = "ml/artifacts/phase13a_split_manifest.json") -> dict:
    return json.loads(Path(path).read_text(encoding="utf-8"))


def binary_label(label: str) -> int:
    if not label:
        raise ValueError("empty label")
    return 0 if label == "Normal" else 1


def _partition_records(
    path: Path, cutoffs: Mapping[str, int], split: str
) -> Iterator[CANRecord]:
    positions: dict[str, int] = defaultdict(int)
    for record in iter_records(path):
        positions[record.interface] += 1
        in_train = positions[record.interface] <= cutoffs[record.interface]
        if (split == "train" and in_train) or (split == "validation" and not in_train):
            yield record


def iter_feature_sequences(
    records: Iterator[CANRecord],
    preprocessor: FittedPreprocessor,
    split: str,
) -> Iterator[tuple[np.ndarray, int, str, str]]:
    """Yield bounded, non-overlapping sequences and the last-row target."""
    state = FeatureState(
        preprocessor.config.rolling_window,
        preprocessor.config.frequency_window,
    )
    window: deque[tuple[dict[str, float | int], str, str]] = deque()
    current_stream: tuple[str, str] | None = None
    last_source_split: str | None = None
    for record in records:
        stream = (record.source_file, record.interface)
        if stream != current_stream:
            state.reset()
            window.clear()
            current_stream = stream
        row = state.update(record)
        normalized = np.asarray(
            [
                _scale(float(row[name]), preprocessor.feature_min[name], preprocessor.feature_max[name])
                for name in FEATURE_NAMES
            ],
            dtype=np.float32,
        )
        if last_source_split != split:
            window.clear()
            last_source_split = split
        window.append((dict(zip(FEATURE_NAMES, normalized)), record.label, record.interface))
        if len(window) < WINDOW_LENGTH:
            continue
        values = np.asarray(
            [[float(item[0][name]) for name in FEATURE_NAMES] for item in window],
            dtype=np.float32,
        )
        label = window[-1][1]
        yield values, binary_label(label), label, window[-1][2]
        window.clear()


def _scale(value: float, minimum: float, maximum: float) -> float:
    if maximum <= minimum:
        return 0.0
    return (value - minimum) / (maximum - minimum)


def fit_train_preprocessor(
    train_path: Path, cutoffs: Mapping[str, int]
) -> FittedPreprocessor:
    return FittedPreprocessor(PreprocessConfig()).fit(
        _partition_records(train_path, cutoffs, "train"), [str(train_path)]
    )


def class_weights(train_path: Path, cutoffs: Mapping[str, int]) -> dict[str, float]:
    counts = Counter(
        binary_label(record.label)
        for record in _partition_records(train_path, cutoffs, "train")
    )
    total = sum(counts.values())
    if not counts.get(0) or not counts.get(1):
        raise ValueError("training portion must contain both binary classes")
    return {
        "0": total / (2.0 * counts[0]),
        "1": total / (2.0 * counts[1]),
        "counts": {"0": counts[0], "1": counts[1]},
    }


def sequence_dataset(
    path: Path,
    cutoffs: Mapping[str, int],
    split: str,
    preprocessor: FittedPreprocessor,
) -> tf.data.Dataset:
    def generator():
        for values, target, _, _ in iter_feature_sequences(
            _partition_records(path, cutoffs, split), preprocessor, split
        ):
            yield values, np.float32(target)

    return tf.data.Dataset.from_generator(
        generator,
        output_signature=(
            tf.TensorSpec((WINDOW_LENGTH, len(FEATURE_NAMES)), tf.float32),
            tf.TensorSpec((), tf.float32),
        ),
    ).batch(256).prefetch(1)


def count_sequences(
    path: Path, cutoffs: Mapping[str, int], split: str, preprocessor: FittedPreprocessor
) -> int:
    return sum(
        1
        for _ in iter_feature_sequences(
            _partition_records(path, cutoffs, split), preprocessor, split
        )
    )


def build_model() -> tf.keras.Model:
    model = tf.keras.Sequential(
        [
            tf.keras.layers.Input((WINDOW_LENGTH, len(FEATURE_NAMES))),
            tf.keras.layers.LSTM(32),
            tf.keras.layers.Dense(1, activation="sigmoid"),
        ]
    )
    model.compile(
        optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
        loss="binary_crossentropy",
        metrics=[tf.keras.metrics.BinaryAccuracy(name="accuracy")],
    )
    return model


def train(output_dir: str | Path = "ml/artifacts/phase13a4") -> dict:
    random.seed(SEED)
    np.random.seed(SEED)
    tf.random.set_seed(SEED)
    tf.config.experimental.enable_op_determinism()
    manifest = load_frozen_manifest()
    train_path, _ = discover_canonical_files()
    cutoffs = manifest["files"]["training_validation_cutoffs"]
    preprocessor = fit_train_preprocessor(train_path, cutoffs)
    weights = class_weights(train_path, cutoffs)
    train_count = count_sequences(train_path, cutoffs, "train", preprocessor)
    validation_count = count_sequences(train_path, cutoffs, "validation", preprocessor)
    if not train_count or not validation_count:
        raise RuntimeError("frozen split produced no train or validation sequences")
    output = Path(output_dir)
    output.mkdir(parents=True, exist_ok=True)
    model = build_model()
    started = time.perf_counter()
    history = model.fit(
        sequence_dataset(train_path, cutoffs, "train", preprocessor),
        validation_data=sequence_dataset(train_path, cutoffs, "validation", preprocessor),
        epochs=10,
        class_weight={0: weights["0"], 1: weights["1"]},
        callbacks=[
            tf.keras.callbacks.EarlyStopping(
                monitor="val_loss", patience=2, restore_best_weights=True
            )
        ],
        verbose=2,
    )
    training_seconds = time.perf_counter() - started
    model.save(output / "model.keras")
    config = {
        "architecture": "Input(50,7) -> LSTM(32) -> Dense(1,sigmoid)",
        "window_length": WINDOW_LENGTH,
        "feature_names": list(FEATURE_NAMES),
        "optimizer": "Adam",
        "learning_rate": 0.001,
        "batch_size": 256,
        "maximum_epochs": 10,
        "epochs_completed": len(history.history["loss"]),
        "seed": SEED,
        "threshold": 0.5,
        "train_only_preprocessing": True,
        "train_sequence_count": train_count,
        "validation_sequence_count": validation_count,
        "training_seconds": training_seconds,
        "parameter_count": model.count_params(),
        "tensorflow_version": tf.__version__,
        **OVERLAP_METADATA,
    }
    (output / "training_config.json").write_text(json.dumps(config, indent=2), encoding="utf-8")
    (output / "feature_contract.json").write_text(
        json.dumps({"feature_names": list(FEATURE_NAMES), "count": len(FEATURE_NAMES)}, indent=2),
        encoding="utf-8",
    )
    (output / "class_weights.json").write_text(json.dumps(weights, indent=2), encoding="utf-8")
    (output / "preprocessing.json").write_text(
        json.dumps(preprocessor.metadata(), indent=2), encoding="utf-8"
    )
    (output / "history.json").write_text(json.dumps(history.history, indent=2), encoding="utf-8")
    print(json.dumps({"model": str(output / "model.keras"), **config}, sort_keys=True))
    return config


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", default="ml/artifacts/phase13a4")
    args = parser.parse_args()
    train(args.output_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
