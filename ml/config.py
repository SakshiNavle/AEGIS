"""Central configuration for the AutoHack GuardCAN data contract."""

from dataclasses import dataclass
from pathlib import Path

DATA_CONTRACT_VERSION = "AUTOHACK_GUARDCAN_DATA_CONTRACT_V1"
EXPECTED_COLUMNS = ("Interface", "Timestamp", "Arbitration_ID", "DLC", "Data", "Label")
WINDOW_LENGTH = 50


@dataclass(frozen=True)
class PreprocessConfig:
    window_length: int = WINDOW_LENGTH
    rolling_window: int = 20
    frequency_window: int = 100
    time_unit: str = "seconds"
    zero_delta_policy: str = "record_and_keep"
    invalid_record_policy: str = "reject_and_log"


def default_config() -> PreprocessConfig:
    return PreprocessConfig()


def resolve_input(path: str) -> Path:
    """Resolve a user-supplied raw path without copying or embedding it in artifacts."""
    return Path(path).expanduser().resolve()
