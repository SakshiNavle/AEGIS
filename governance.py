"""
AEGIS Phase 1 — Governance Engine
==================================
The single authority that produces a final EXECUTE/DELAY/REJECT/ESCALATE
decision for a command. Cyber-risk providers (e.g. GuardCAN/ML) only ever
supply a risk signal via CyberContext — they never decide directly.

Rule priority (evaluated in this exact order, first match wins):
    1. Authentication failure               -> REJECT
    2. Invalid trusted time                  -> REJECT
    3. Replay detected                       -> REJECT
    4. Critical vehicle-state violation      -> REJECT
    5. High cyber risk                       -> ESCALATE
    6. Command-specific policy               -> (policy decides)
    7. Otherwise                             -> EXECUTE

Unknown command types never reach a policy and never execute; they are
routed to ESCALATE for human/operator review.
"""

from __future__ import annotations

from datetime import datetime, timezone

from models import (
    Command,
    CyberRiskLevel,
    Decision,
    GearPosition,
    GovernanceDecision,
)
from policies import POLICY_REGISTRY


class InvalidCommandError(ValueError):
    """Raised when a Command fails basic structural validation."""


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _decision(
    cmd: Command,
    decision: Decision,
    reason: str,
    rule_id: str,
) -> GovernanceDecision:
    return GovernanceDecision(
        decision=decision,
        reason=reason,
        rule_id=rule_id,
        risk_score=cmd.cyber.risk_score,
        timestamp=_now_iso(),
        command_id=cmd.command_id,
        command_type=cmd.command_type,
    )


# ---------------------------------------------------------------------------
# Input validation
# ---------------------------------------------------------------------------

def validate_command(cmd: Command) -> None:
    """Structural validation. Raises InvalidCommandError on malformed input.

    This is distinct from policy/business-logic rejection: a malformed
    command (wrong types, missing fields, out-of-range values) is a
    programming/integration error and should fail loudly rather than be
    silently routed through the decision pipeline.
    """
    if not isinstance(cmd, Command):
        raise InvalidCommandError(f"Expected Command, got {type(cmd).__name__}")

    if not isinstance(cmd.command_type, str) or not cmd.command_type.strip():
        raise InvalidCommandError("command_type must be a non-empty string")

    if cmd.vehicle.speed_kmh < 0:
        raise InvalidCommandError("vehicle speed_kmh cannot be negative")

    if not isinstance(cmd.vehicle.gear, GearPosition):
        raise InvalidCommandError("vehicle.gear must be a GearPosition")

    if not isinstance(cmd.cyber.risk_level, CyberRiskLevel):
        raise InvalidCommandError("cyber.risk_level must be a CyberRiskLevel")

    if not (0.0 <= cmd.cyber.risk_score <= 1.0):
        raise InvalidCommandError("cyber.risk_score must be within [0.0, 1.0]")


# ---------------------------------------------------------------------------
# Critical vehicle-state violation (rule 4)
# ---------------------------------------------------------------------------
# A "critical violation" is a physically dangerous configuration that must
# block *any* command regardless of which command it is — e.g. a moving
# vehicle reporting an internally inconsistent gear (Drive/Reverse while
# speed is reported as exactly 0 is fine; the flagged case is the reverse:
# is_moving True while gear is Park, which is only physically sane if the
# vehicle is rolling unintentionally — e.g. towed/rolling with no gear
# engaged). We treat that specific combination as a critical violation.
def _has_critical_vehicle_violation(cmd: Command) -> bool:
    v = cmd.vehicle
    # Vehicle reports motion while gear is Park: internally inconsistent /
    # unsafe (e.g. rolling, tow-away, sensor spoof). Block unconditionally.
    return v.is_moving and v.gear == GearPosition.PARK


# ---------------------------------------------------------------------------
# Governance engine
# ---------------------------------------------------------------------------

def evaluate(cmd: Command) -> GovernanceDecision:
    """Evaluate a command and return exactly one GovernanceDecision."""
    validate_command(cmd)

    # Rule 1: Authentication failure
    if not cmd.auth.authenticated:
        return _decision(
            cmd, Decision.REJECT,
            "Authentication failed",
            "GATE-001",
        )

    # Rule 2: Invalid trusted time
    if not cmd.auth.trusted_time_valid:
        return _decision(
            cmd, Decision.REJECT,
            "Trusted time is invalid or unavailable",
            "GATE-002",
        )

    # Rule 3: Replay detected
    if cmd.auth.replay_detected:
        return _decision(
            cmd, Decision.REJECT,
            "Replay attack detected",
            "GATE-003",
        )

    # Rule 4: Critical vehicle-state violation
    if _has_critical_vehicle_violation(cmd):
        return _decision(
            cmd, Decision.REJECT,
            "Critical vehicle-state violation: motion reported while gear is Park",
            "GATE-004",
        )

    # Rule 5: High cyber risk -> ESCALATE (global gate; command-specific
    # policies also check this, but the engine enforces it first so no
    # policy can accidentally bypass it).
    if cmd.cyber.risk_level == CyberRiskLevel.HIGH:
        return _decision(
            cmd, Decision.ESCALATE,
            "High cyber risk detected",
            "GATE-005",
        )

    # Rule 6: Command-specific policy
    policy = POLICY_REGISTRY.get(cmd.command_type)
    if policy is None:
        # Rule: unknown commands must never execute.
        return _decision(
            cmd, Decision.ESCALATE,
            f"Unknown or unsupported command type: '{cmd.command_type}'",
            "GATE-006",
        )

    return policy(cmd)