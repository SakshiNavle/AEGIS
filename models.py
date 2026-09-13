"""
AEGIS Phase 1 — Data Models
============================
Deterministic, dependency-free data structures used across the governance
engine. No PKI/HSM/CAN/ML — those are out of scope for Phase 1.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime, timezone
from enum import Enum
from typing import Optional


# ---------------------------------------------------------------------------
# Enums
# ---------------------------------------------------------------------------

class CommandType(str, Enum):
    REMOTE_UNLOCK = "REMOTE_UNLOCK"
    DIAGNOSTIC = "DIAGNOSTIC"
    OTA = "OTA"
    IMMOBILIZE = "IMMOBILIZE"

    @classmethod
    def _missing_(cls, value: object) -> Optional["CommandType"]:
        # Unknown/garbage command strings fall through to None rather than
        # raising, so the engine can route them to ESCALATE deterministically
        # instead of crashing.
        return None


class GearPosition(str, Enum):
    PARK = "P"
    REVERSE = "R"
    NEUTRAL = "N"
    DRIVE = "D"


class CyberRiskLevel(str, Enum):
    LOW = "LOW"
    MEDIUM = "MEDIUM"
    HIGH = "HIGH"


class Decision(str, Enum):
    EXECUTE = "EXECUTE"
    DELAY = "DELAY"
    REJECT = "REJECT"
    ESCALATE = "ESCALATE"


# ---------------------------------------------------------------------------
# Input context dataclasses
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class AuthContext:
    """Authentication + trusted-time evidence for a command."""
    authenticated: bool
    trusted_time_valid: bool
    replay_detected: bool


@dataclass(frozen=True)
class VehicleContext:
    """Physical vehicle state relevant to safety-of-execution."""
    is_moving: bool
    gear: GearPosition
    speed_kmh: float = 0.0

    def __post_init__(self) -> None:
        if self.speed_kmh < 0:
            raise ValueError("speed_kmh cannot be negative")

    @property
    def is_parked(self) -> bool:
        return (not self.is_moving) and self.gear == GearPosition.PARK

    @property
    def is_stationary(self) -> bool:
        # Weaker than "parked": vehicle isn't moving, but gear may vary
        # (relevant for OTA, which only cares about motion, not gear).
        return not self.is_moving and self.speed_kmh == 0.0

    @property
    def is_safe_state(self) -> bool:
        # Generic "safe to touch diagnostics/immobilize" state:
        # not moving and in Park.
        return self.is_parked


@dataclass(frozen=True)
class CyberContext:
    """Cyber-risk evidence, e.g. sourced from GuardCAN/ML.

    GuardCAN/ML is a *risk provider only* — it supplies risk_score /
    risk_level as an input signal. It never makes the final decision;
    that authority lives solely in the governance engine.
    """
    risk_level: CyberRiskLevel
    risk_score: float = 0.0

    def __post_init__(self) -> None:
        if not (0.0 <= self.risk_score <= 1.0):
            raise ValueError("risk_score must be within [0.0, 1.0]")


@dataclass(frozen=True)
class Command:
    """A runtime command submitted for governance evaluation."""
    command_type: str  # kept as raw str so unknown commands don't explode
    auth: AuthContext
    vehicle: VehicleContext
    cyber: CyberContext
    command_id: str = ""
    issued_at: datetime = field(default_factory=lambda: datetime.now(timezone.utc))

    @property
    def known_command_type(self) -> Optional[CommandType]:
        try:
            return CommandType(self.command_type)
        except ValueError:
            return None


# ---------------------------------------------------------------------------
# Output
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class GovernanceDecision:
    """Structured, explainable output of the governance engine."""
    decision: Decision
    reason: str
    rule_id: str
    risk_score: float
    timestamp: str
    command_id: str = ""
    command_type: str = ""

    def to_dict(self) -> dict:
        return {
            "decision": self.decision.value,
            "reason": self.reason,
            "rule_id": self.rule_id,
            "risk_score": self.risk_score,
            "timestamp": self.timestamp,
            "command_id": self.command_id,
            "command_type": self.command_type,
        }