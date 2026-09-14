"""Boundary-safe causal sequence construction with bounded memory."""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from typing import Iterable, Iterator


@dataclass(frozen=True)
class FeatureSequence:
    values: tuple[tuple[float | int, ...], ...]
    labels: tuple[str, ...]
    source_file: str
    interface: str


RecordTuple = tuple[
    dict[str, float | int],
    str,
    str,
    str,
]


def build_sequences(
    records: Iterable[RecordTuple],
    window_length: int,
) -> Iterator[FeatureSequence]:
    """Yield non-overlapping sequences using bounded memory."""

    if window_length <= 0:
        raise ValueError("window_length must be positive")

    window: deque[RecordTuple] = deque()

    for record in records:
        window.append(record)

        if len(window) < window_length:
            continue

        features, labels, files, interfaces = zip(*window)

        if len(set(files)) != 1:
            raise ValueError("sequence crosses source-file boundary")

        if len(set(interfaces)) != 1:
            raise ValueError("sequence crosses interface boundary")

        names = tuple(features[0].keys())

        yield FeatureSequence(
            values=tuple(
                tuple(item[name] for name in names)
                for item in features
            ),
            labels=tuple(labels),
            source_file=files[0],
            interface=interfaces[0],
        )

        # Non-overlapping sequence construction.
        window.clear()