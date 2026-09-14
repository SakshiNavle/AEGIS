"""Causal, incremental GuardCAN feature calculations."""

from __future__ import annotations

from collections import Counter, deque
from dataclasses import dataclass
from math import sqrt

from .load_autohack import CANRecord

FEATURE_NAMES = (
    "delta_t",
    "rolling_mean_delta_t",
    "rolling_std_delta_t",
    "id_frequency_ratio",
    "payload_hamming_distance",
    "dlc",
    "arbitration_id",
)


def hamming_distance(left: bytes, right: bytes) -> int:
    """Count differing bits only over bytes present in both payloads."""
    return sum((a ^ b).bit_count() for a, b in zip(left, right))


@dataclass
class FeatureState:
    rolling_window: int = 20
    frequency_window: int = 100
    previous_timestamp: float | None = None
    previous_by_id: dict[int, bytes] | None = None
    deltas: deque[float] | None = None
    ids: deque[int] | None = None

    def __post_init__(self) -> None:
        self.previous_by_id = self.previous_by_id or {}
        self.deltas = self.deltas or deque(maxlen=self.rolling_window)
        self.ids = self.ids or deque(maxlen=self.frequency_window)

    def reset(self) -> None:
        self.previous_timestamp = None
        self.previous_by_id.clear()
        self.deltas.clear()
        self.ids.clear()

    def update(self, record: CANRecord) -> dict[str, float | int]:
        delta = 0.0 if self.previous_timestamp is None else record.timestamp - self.previous_timestamp
        historical = list(self.deltas)
        mean = sum(historical) / len(historical) if historical else 0.0
        variance = sum((item - mean) ** 2 for item in historical) / len(historical) if historical else 0.0
        id_count = sum(item == record.arbitration_id for item in self.ids)
        ratio = id_count / len(self.ids) if self.ids else 0.0
        previous_payload = self.previous_by_id.get(record.arbitration_id)
        distance = 0 if previous_payload is None else hamming_distance(previous_payload, record.payload)
        self.previous_timestamp = record.timestamp
        self.previous_by_id[record.arbitration_id] = record.payload
        self.deltas.append(delta)
        self.ids.append(record.arbitration_id)
        return {
            "delta_t": delta,
            "rolling_mean_delta_t": mean,
            "rolling_std_delta_t": sqrt(variance),
            "id_frequency_ratio": ratio,
            "payload_hamming_distance": distance,
            "dlc": record.dlc,
            "arbitration_id": record.arbitration_id,
        }
