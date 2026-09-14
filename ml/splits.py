"""File-level and strict temporal split helpers."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable


@dataclass(frozen=True)
class SplitAssignment:
    train: tuple[str, ...]
    validation: tuple[str, ...]
    test: tuple[str, ...]


def file_split(files: Iterable[str], test_files: Iterable[str], validation_files: Iterable[str]) -> SplitAssignment:
    all_files = tuple(dict.fromkeys(files))
    test = set(test_files)
    validation = set(validation_files)
    if test & validation:
        raise ValueError("validation/test file overlap")
    if not test | validation <= set(all_files):
        raise ValueError("split references unknown file")
    train = tuple(item for item in all_files if item not in test and item not in validation)
    return SplitAssignment(train, tuple(item for item in all_files if item in validation), tuple(item for item in all_files if item in test))
