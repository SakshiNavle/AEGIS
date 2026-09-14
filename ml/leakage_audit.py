"""Fail-closed checks for data, temporal, sequence, and preprocessing leakage."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, Mapping

FORBIDDEN_FEATURE_NAMES = {
    "label", "attack", "attack_name", "filename", "file_path", "capture_id",
    "vehicle_id", "dataset_split", "future_timestamp",
}


@dataclass(frozen=True)
class AuditResult:
    passed: bool
    violations: tuple[str, ...]


def audit_feature_names(names: Iterable[str]) -> AuditResult:
    violations = tuple(
        f"forbidden feature: {name}"
        for name in names
        if name.lower() in FORBIDDEN_FEATURE_NAMES or any(token in name.lower() for token in ("filename", "filepath", "capture_id", "vehicle_id"))
    )
    return AuditResult(not violations, violations)


def audit_split_disjointness(split_files: Mapping[str, Iterable[str]]) -> AuditResult:
    seen: dict[str, str] = {}
    violations: list[str] = []
    for split, files in split_files.items():
        for file_name in files:
            if file_name in seen:
                violations.append(f"file assigned to {seen[file_name]} and {split}: {file_name}")
            seen[file_name] = split
    return AuditResult(not violations, tuple(violations))


def assert_clean(*results: AuditResult) -> None:
    violations = tuple(item for result in results for item in result.violations)
    if violations:
        raise RuntimeError("CRITICAL LEAKAGE: " + "; ".join(violations))
