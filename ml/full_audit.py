"""Streaming full-dataset audit for the acquired AutoHack interface files."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import time
from collections import Counter, defaultdict
from pathlib import Path
from typing import Iterable

from .config import DATA_CONTRACT_VERSION, PreprocessConfig
from .features import FEATURE_NAMES, FeatureState
from .load_autohack import QualityReport, iter_records


class Reservoir:
    def __init__(self, capacity: int = 100_000) -> None:
        self.capacity = capacity
        self.values: list[float] = []
        self.seen = 0

    def add(self, value: float) -> None:
        if not math.isfinite(value):
            return
        self.seen += 1
        if len(self.values) < self.capacity:
            self.values.append(value)
            return
        index = ((self.seen * 1103515245 + 12345) & 0x7FFFFFFF) % self.seen
        if index < self.capacity:
            self.values[index] = value

    def summary(self) -> dict[str, float | int]:
        if not self.values:
            return {"finite": 0}
        ordered = sorted(self.values)
        def quantile(q: float) -> float:
            return ordered[min(len(ordered) - 1, int(q * (len(ordered) - 1)))]
        return {
            "finite": self.seen,
            "min": ordered[0],
            "p50": quantile(0.50),
            "p95": quantile(0.95),
            "p99": quantile(0.99),
            "max": ordered[-1],
            "reservoir_capacity": self.capacity,
        }


def row_fingerprint(record) -> bytes:
    value = "\x1f".join((
        record.interface,
        repr(record.timestamp),
        format(record.arbitration_id, "x"),
        str(record.dlc),
        record.payload.hex(),
        record.label,
    ))
    return hashlib.blake2b(value.encode(), digest_size=16).digest()


def json_safe(value):
    if isinstance(value, dict):
        return {str(key): json_safe(item) for key, item in value.items()}
    if isinstance(value, (list, tuple)):
        return [json_safe(item) for item in value]
    return value


def audit_file(path: Path, config: PreprocessConfig, cross_sample: set[bytes] | None = None) -> dict:
    report = QualityReport()
    feature_values = {name: Reservoir() for name in FEATURE_NAMES}
    delta_values = Reservoir()
    feature_counts = Counter()
    cross_duplicate_rows = 0
    previous: dict[str, float] = {}
    state_by_interface: dict[str, FeatureState] = {}
    duplicate_timestamps = 0
    negative_deltas = 0
    zero_deltas = 0
    previous_by_stream: dict[str, tuple[int, bytes]] = {}
    same_id_dlc_pairs = Counter()
    payload_distance_by_dlc = Reservoir()
    first_seen_streams: set[str] = set()
    for record in iter_records(path, report):
        digest = row_fingerprint(record)
        if cross_sample is not None and digest in cross_sample:
            cross_duplicate_rows += 1
        stream = record.interface
        state = state_by_interface.setdefault(stream, FeatureState(config.rolling_window, config.frequency_window))
        row = state.update(record)
        for name, value in row.items():
            feature_values[name].add(float(value))
            feature_counts[name] += 1
        current = (record.arbitration_id, record.dlc)
        prior = previous_by_stream.get(stream)
        if prior is not None and prior[0] == record.arbitration_id:
            same_id_dlc_pairs[(prior[1], record.dlc)] += 1
            payload_distance_by_dlc.add(float(row["payload_hamming_distance"]))
        previous_by_stream[stream] = current
        prior_timestamp = previous.get(stream)
        if prior_timestamp is not None:
            delta = record.timestamp - prior_timestamp
            delta_values.add(delta)
            if delta == 0:
                zero_deltas += 1
            elif delta < 0:
                negative_deltas += 1
        previous[stream] = record.timestamp
    local_duplicates = report.duplicate_rows
    return {
        "path": str(path),
        "size": path.stat().st_size,
        "quality": report.as_dict(),
        "feature_stats": {name: values.summary() for name, values in feature_values.items()},
        "delta_t": delta_values.summary(),
        "zero_delta": zero_deltas,
        "negative_delta": negative_deltas,
        "same_id_dlc_pairs": {f"{a}->{b}": count for (a, b), count in sorted(same_id_dlc_pairs.items())},
        "payload_distance_summary": payload_distance_by_dlc.summary(),
        "local_exact_duplicate_rows": local_duplicates,
        "cross_file_duplicate_rows": cross_duplicate_rows,
        "streams": sorted(first_seen_streams | set(report.interface_distribution)),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    parser.add_argument("--output", type=Path, default=Path("ml/artifacts/autohack_full_audit.json"))
    args = parser.parse_args()
    root = args.root
    paths = (
        root / "Interface" / "train" / "autohack_train_both_interface.csv",
        root / "Interface" / "test" / "autohack_test_both_interface.csv",
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    config = PreprocessConfig()
    started = time.time()
    results = []
    train_sample: set[bytes] = set()
    for path in paths:
        results.append(audit_file(path, config, train_sample if path == paths[1] else None))
        if path == paths[0]:
            # Bounded cross-file overlap check: the first 250,000 train rows.
            import itertools
            for record in itertools.islice(iter_records(path), 250_000):
                train_sample.add(row_fingerprint(record))
    output = {
        "contract_version": DATA_CONTRACT_VERSION,
        "audit_timestamp_utc": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "configuration": config.__dict__,
        "quantiles": "bounded deterministic reservoir; finite feature counts exact",
        "files": results,
        "elapsed_seconds": time.time() - started,
    }
    safe_output = json_safe(output)
    args.output.write_text(json.dumps(safe_output, indent=2, sort_keys=True), encoding="utf-8")
    print(json.dumps(safe_output, indent=2))


if __name__ == "__main__":
    main()
