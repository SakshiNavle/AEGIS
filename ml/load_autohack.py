"""Streaming AutoHack CSV loading and structural quality validation."""

from __future__ import annotations

import csv
import hashlib
import json
from collections import Counter
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterator, Mapping

from .config import EXPECTED_COLUMNS


@dataclass(frozen=True)
class CANRecord:
    interface: str
    timestamp: float
    arbitration_id: int
    dlc: int
    payload: bytes
    label: str
    source_file: str
    row_number: int


@dataclass
class QualityReport:
    input_rows: int = 0
    valid_rows: int = 0
    rejected_rows: int = 0
    rejection_reasons: Counter[str] = None
    label_distribution: Counter[str] = None
    interface_distribution: Counter[str] = None
    dlc_distribution: Counter[str] = None
    interface_label_distribution: Counter[str] = None
    empty_payload: int = 0
    timestamp_min: float | None = None
    timestamp_max: float | None = None
    duplicate_rows: int = 0
    duplicate_timestamps: int = 0
    zero_delta: int = 0
    negative_delta: int = 0

    def __post_init__(self) -> None:
        self.rejection_reasons = self.rejection_reasons or Counter()
        self.label_distribution = self.label_distribution or Counter()
        self.interface_distribution = self.interface_distribution or Counter()
        self.dlc_distribution = self.dlc_distribution or Counter()
        self.interface_label_distribution = self.interface_label_distribution or Counter()

    def as_dict(self) -> dict:
        return {
            "input_rows": self.input_rows,
            "valid_rows": self.valid_rows,
            "rejected_rows": self.rejected_rows,
            "rejection_reasons": dict(self.rejection_reasons),
            "label_distribution": dict(self.label_distribution),
            "interface_distribution": dict(self.interface_distribution),
            "dlc_distribution": dict(self.dlc_distribution),
            "interface_label_distribution": dict(self.interface_label_distribution),
            "empty_payload": self.empty_payload,
            "timestamp_min": self.timestamp_min,
            "timestamp_max": self.timestamp_max,
            "duplicate_rows": self.duplicate_rows,
            "duplicate_timestamps": self.duplicate_timestamps,
            "zero_delta": self.zero_delta,
            "negative_delta": self.negative_delta,
        }


def sha256_file(path: Path, chunk_size: int = 1024 * 1024) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(chunk_size), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _parse_hex_id(value: str) -> int:
    normalized = value.strip().lower()
    if not normalized:
        raise ValueError("invalid_can_id")
    return int(normalized, 16)


def _parse_payload(value: str) -> bytes:
    tokens = value.strip().split()
    if not tokens:
        return b""
    if any(len(token) != 2 for token in tokens):
        raise ValueError("invalid_payload")
    try:
        return bytes(int(token, 16) for token in tokens)
    except ValueError as exc:
        raise ValueError("invalid_payload") from exc


def parse_row(row: Mapping[str, str], source_file: str, row_number: int) -> CANRecord:
    if any(column not in row for column in EXPECTED_COLUMNS):
        raise ValueError("missing_required_column")
    interface = row["Interface"].strip()
    label = row["Label"].strip()
    if not interface:
        raise ValueError("missing_interface")
    if not label:
        raise ValueError("missing_label")
    try:
        timestamp = float(row["Timestamp"])
    except ValueError as exc:
        raise ValueError("invalid_timestamp") from exc
    try:
        dlc = int(row["DLC"])
    except ValueError as exc:
        raise ValueError("invalid_dlc") from exc
    if not 0 <= dlc <= 8:
        raise ValueError("invalid_dlc_range")
    arbitration_id = _parse_hex_id(row["Arbitration_ID"])
    payload = _parse_payload(row["Data"])
    if len(payload) != dlc:
        raise ValueError("payload_dlc_mismatch")
    return CANRecord(interface, timestamp, arbitration_id, dlc, payload, label, source_file, row_number)


def iter_records(path: str | Path, report: QualityReport | None = None) -> Iterator[CANRecord]:
    """Yield valid rows and record every rejected row reason when a report is supplied."""
    file_path = Path(path)
    local_report = report or QualityReport()
    previous_by_interface: dict[str, float] = {}
    seen_hashes: set[bytes] = set()
    with file_path.open("r", encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(stream)
        if tuple(reader.fieldnames or ()) != EXPECTED_COLUMNS:
            raise ValueError(f"unexpected_schema:{reader.fieldnames}")
        for row_number, row in enumerate(reader, start=2):
            local_report.input_rows += 1
            raw_tuple = tuple(row.get(column, "") for column in EXPECTED_COLUMNS)
            digest = hashlib.blake2b("\x1f".join(raw_tuple).encode(), digest_size=16).digest()
            if digest in seen_hashes:
                local_report.duplicate_rows += 1
            else:
                seen_hashes.add(digest)
            try:
                record = parse_row(row, str(file_path), row_number)
            except ValueError as exc:
                local_report.rejected_rows += 1
                local_report.rejection_reasons[str(exc)] += 1
                continue
            previous = previous_by_interface.get(record.interface)
            if previous is not None:
                delta = record.timestamp - previous
                if delta == 0:
                    local_report.duplicate_timestamps += 1
                    local_report.zero_delta += 1
                elif delta < 0:
                    local_report.negative_delta += 1
            previous_by_interface[record.interface] = record.timestamp
            local_report.valid_rows += 1
            local_report.label_distribution[record.label] += 1
            local_report.interface_distribution[record.interface] += 1
            local_report.dlc_distribution[str(record.dlc)] += 1
            local_report.interface_label_distribution[f"{record.interface}|{record.label}"] += 1
            if not record.payload:
                local_report.empty_payload += 1
            local_report.timestamp_min = record.timestamp if local_report.timestamp_min is None else min(local_report.timestamp_min, record.timestamp)
            local_report.timestamp_max = record.timestamp if local_report.timestamp_max is None else max(local_report.timestamp_max, record.timestamp)
            yield record


def provenance(path: str | Path, report: QualityReport, version: str, config: Mapping[str, object]) -> dict:
    file_path = Path(path)
    return {
        "original_filename": file_path.name,
        "file_size": file_path.stat().st_size,
        "sha256": sha256_file(file_path),
        "schema": list(EXPECTED_COLUMNS),
        "quality": report.as_dict(),
        "preprocessing_version": version,
        "configuration": dict(config),
    }


def write_json(path: str | Path, value: object) -> None:
    Path(path).write_text(json.dumps(value, indent=2, sort_keys=True), encoding="utf-8")
