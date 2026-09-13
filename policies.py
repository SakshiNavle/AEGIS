"""
AEGIS Phase 1 — Command-Specific Policies
==========================================
Each policy function receives a validated Command and returns a
GovernanceDecision. Policies are only invoked AFTER the shared governance
gates (auth, trusted time, replay, critical vehicle-state, high cyber risk)
have already passed — see governance.py for priority ordering.

Each policy still re-checks HIGH cyber risk defensively (per the spec's
explicit per-command rule), even though the engine also checks it globally,
so that policy modules remain independently testable/explainable.
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
# REMOTE_UNLOCK
# ---------------------------------------------------------------------------

def policy_remote_unlock(cmd: Command) -> GovernanceDecision:
    v = cmd.vehicle

    if cmd.cyber.risk_level == CyberRiskLevel.HIGH:
        return _decision(
            cmd, Decision.ESCALATE,
            "High cyber risk detected during remote unlock request",
            "UNLOCK-004",
        )

    if v.is_moving:
        return _decision(
            cmd, Decision.REJECT,
            "Vehicle is moving",
            "UNLOCK-001",
        )

    if v.gear != GearPosition.PARK:
        return _decision(
            cmd, Decision.REJECT,
            f"Gear is not in Park (current: {v.gear.value})",
            "UNLOCK-002",
        )

    if v.is_parked and cmd.cyber.risk_level == CyberRiskLevel.LOW:
        return _decision(
            cmd, Decision.EXECUTE,
            "Vehicle parked and cyber risk is low",
            "UNLOCK-003",
        )

    # Parked, gear P, but risk is MEDIUM (not LOW, not HIGH) — no explicit
    # rule covers this; be conservative and delay for reassessment.
    return _decision(
        cmd, Decision.DELAY,
        "Vehicle parked but cyber risk is not low enough for immediate execution",
        "UNLOCK-005",
    )


# ---------------------------------------------------------------------------
# DIAGNOSTIC
# ---------------------------------------------------------------------------

def policy_diagnostic(cmd: Command) -> GovernanceDecision:
    v = cmd.vehicle

    if cmd.cyber.risk_level == CyberRiskLevel.HIGH:
        return _decision(
            cmd, Decision.ESCALATE,
            "High cyber risk detected during diagnostic request",
            "DIAG-003",
        )

    if not v.is_safe_state:
        return _decision(
            cmd, Decision.DELAY,
            "Vehicle is not in a safe state for diagnostics",
            "DIAG-002",
        )

    if v.is_safe_state and cmd.cyber.risk_level == CyberRiskLevel.LOW:
        return _decision(
            cmd, Decision.EXECUTE,
            "Vehicle state is safe and cyber risk is low",
            "DIAG-001",
        )

    # Safe state but MEDIUM risk — no explicit rule; delay conservatively.
    return _decision(
        cmd, Decision.DELAY,
        "Vehicle state is safe but cyber risk is not low enough for immediate execution",
        "DIAG-004",
    )


# ---------------------------------------------------------------------------
# OTA
# ---------------------------------------------------------------------------

def policy_ota(cmd: Command) -> GovernanceDecision:
    v = cmd.vehicle

    if cmd.cyber.risk_level == CyberRiskLevel.HIGH:
        return _decision(
            cmd, Decision.ESCALATE,
            "High cyber risk detected during OTA update request",
            "OTA-003",
        )

    if v.is_moving:
        return _decision(
            cmd, Decision.DELAY,
            "Vehicle is moving; OTA update delayed until stationary",
            "OTA-002",
        )

    if v.is_stationary and cmd.cyber.risk_level == CyberRiskLevel.LOW:
        return _decision(
            cmd, Decision.EXECUTE,
            "Vehicle is stationary and cyber risk is low",
            "OTA-001",
        )

    # Stationary but MEDIUM risk — no explicit rule; delay conservatively.
    return _decision(
        cmd, Decision.DELAY,
        "Vehicle is stationary but cyber risk is not low enough for immediate execution",
        "OTA-004",
    )


# ---------------------------------------------------------------------------
# IMMOBILIZE
# ---------------------------------------------------------------------------

def policy_immobilize(cmd: Command) -> GovernanceDecision:
    v = cmd.vehicle
    a = cmd.auth

    # Spec lists invalid authentication/time explicitly for this command.
    # The engine's global gates (rules 1-2) already reject these before
    # reaching this policy; this check is a defensive second layer so the
    # policy is independently correct/testable.
    if not (a.authenticated and a.trusted_time_valid):
        return _decision(
            cmd, Decision.REJECT,
            "Invalid authentication or trusted time for immobilize request",
            "IMMO-003",
        )

    if cmd.cyber.risk_level == CyberRiskLevel.HIGH:
        return _decision(
            cmd, Decision.ESCALATE,
            "High cyber risk detected during immobilize request",
            "IMMO-002",
        )

    if v.is_safe_state and cmd.cyber.risk_level == CyberRiskLevel.LOW:
        return _decision(
            cmd, Decision.EXECUTE,
            "Valid authentication/time, safe vehicle state, and low cyber risk",
            "IMMO-001",
        )

    # Safe state but MEDIUM risk, or state ambiguous — escalate rather than
    # silently delay: immobilizing a vehicle is high-consequence, so any
    # case not explicitly cleared for EXECUTE goes to human review.
    return _decision(
        cmd, Decision.ESCALATE,
        "Immobilize request does not meet explicit low-risk execution criteria",
        "IMMO-004",
    )


# ---------------------------------------------------------------------------
# Policy registry
# ---------------------------------------------------------------------------

POLICY_REGISTRY = {
    "REMOTE_UNLOCK": policy_remote_unlock,
    "DIAGNOSTIC": policy_diagnostic,
    "OTA": policy_ota,
    "IMMOBILIZE": policy_immobilize,
}