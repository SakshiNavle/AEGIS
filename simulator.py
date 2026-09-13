"""
AEGIS Phase 1 — Scenario Simulator
====================================
Runs a fixed set of representative scenarios through the governance engine
and prints/logs the resulting decisions. This is a demonstration harness,
not a test suite (see tests/test_governance.py for that) — it's meant to
give a human a fast, readable walkthrough of the engine's behavior.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Callable

from models import (
    AuthContext,
    Command,
    CyberContext,
    CyberRiskLevel,
    GearPosition,
    VehicleContext,
)
from governance import evaluate
from audit import AuditLogger


@dataclass(frozen=True)
class Scenario:
    name: str
    build: Callable[[], Command]


def _cmd(
    command_id: str,
    command_type: str,
    authenticated: bool = True,
    trusted_time_valid: bool = True,
    replay_detected: bool = False,
    is_moving: bool = False,
    gear: GearPosition = GearPosition.PARK,
    speed_kmh: float = 0.0,
    risk_level: CyberRiskLevel = CyberRiskLevel.LOW,
    risk_score: float = 0.1,
) -> Command:
    return Command(
        command_id=command_id,
        command_type=command_type,
        auth=AuthContext(authenticated, trusted_time_valid, replay_detected),
        vehicle=VehicleContext(is_moving=is_moving, gear=gear, speed_kmh=speed_kmh),
        cyber=CyberContext(risk_level=risk_level, risk_score=risk_score),
    )


SCENARIOS: list[Scenario] = [
    Scenario(
        "Unlock while parked, low risk -> EXECUTE",
        lambda: _cmd("S01", "REMOTE_UNLOCK", gear=GearPosition.PARK, is_moving=False,
                      risk_level=CyberRiskLevel.LOW, risk_score=0.05),
    ),
    Scenario(
        "Unlock while moving -> REJECT",
        lambda: _cmd("S02", "REMOTE_UNLOCK", gear=GearPosition.DRIVE, is_moving=True,
                      speed_kmh=40, risk_level=CyberRiskLevel.LOW, risk_score=0.05),
    ),
    Scenario(
        "Unlock, gear not Park -> REJECT",
        lambda: _cmd("S03", "REMOTE_UNLOCK", gear=GearPosition.REVERSE, is_moving=False,
                      risk_level=CyberRiskLevel.LOW, risk_score=0.05),
    ),
    Scenario(
        "Unlock, HIGH cyber risk -> ESCALATE",
        lambda: _cmd("S04", "REMOTE_UNLOCK", gear=GearPosition.PARK, is_moving=False,
                      risk_level=CyberRiskLevel.HIGH, risk_score=0.92),
    ),
    Scenario(
        "Diagnostic, safe state + low risk -> EXECUTE",
        lambda: _cmd("S05", "DIAGNOSTIC", gear=GearPosition.PARK, is_moving=False,
                      risk_level=CyberRiskLevel.LOW, risk_score=0.1),
    ),
    Scenario(
        "Diagnostic, unsafe vehicle state -> DELAY",
        lambda: _cmd("S06", "DIAGNOSTIC", gear=GearPosition.DRIVE, is_moving=False,
                      risk_level=CyberRiskLevel.LOW, risk_score=0.1),
    ),
    Scenario(
        "OTA, stationary + low risk -> EXECUTE",
        lambda: _cmd("S07", "OTA", gear=GearPosition.PARK, is_moving=False, speed_kmh=0,
                      risk_level=CyberRiskLevel.LOW, risk_score=0.1),
    ),
    Scenario(
        "OTA, moving -> DELAY",
        lambda: _cmd("S08", "OTA", gear=GearPosition.DRIVE, is_moving=True, speed_kmh=60,
                      risk_level=CyberRiskLevel.LOW, risk_score=0.1),
    ),
    Scenario(
        "Immobilize, valid + safe + low risk -> EXECUTE",
        lambda: _cmd("S09", "IMMOBILIZE", gear=GearPosition.PARK, is_moving=False,
                      risk_level=CyberRiskLevel.LOW, risk_score=0.05),
    ),
    Scenario(
        "Immobilize, HIGH cyber risk -> ESCALATE",
        lambda: _cmd("S10", "IMMOBILIZE", gear=GearPosition.PARK, is_moving=False,
                      risk_level=CyberRiskLevel.HIGH, risk_score=0.95),
    ),
    Scenario(
        "Immobilize, invalid trusted time -> REJECT",
        lambda: _cmd("S11", "IMMOBILIZE", trusted_time_valid=False, gear=GearPosition.PARK,
                      is_moving=False, risk_level=CyberRiskLevel.LOW, risk_score=0.05),
    ),
    Scenario(
        "Replay detected -> REJECT (before any command logic)",
        lambda: _cmd("S12", "OTA", replay_detected=True, gear=GearPosition.PARK,
                      is_moving=False, risk_level=CyberRiskLevel.LOW, risk_score=0.05),
    ),
    Scenario(
        "Unknown command type -> ESCALATE",
        lambda: _cmd("S13", "FLASH_FIRMWARE_DIRECT", gear=GearPosition.PARK, is_moving=False,
                      risk_level=CyberRiskLevel.LOW, risk_score=0.05),
    ),
    Scenario(
        "Authentication failure -> REJECT (highest priority)",
        lambda: _cmd("S14", "REMOTE_UNLOCK", authenticated=False, gear=GearPosition.PARK,
                      is_moving=False, risk_level=CyberRiskLevel.LOW, risk_score=0.05),
    ),
    Scenario(
        "Critical vehicle-state violation (moving + gear P) -> REJECT",
        lambda: _cmd("S15", "DIAGNOSTIC", gear=GearPosition.PARK, is_moving=True,
                      speed_kmh=15, risk_level=CyberRiskLevel.LOW, risk_score=0.05),
    ),
]


def run_simulator(log_path: Path | str = "aegis_simulator_audit.log", verbose: bool = True) -> list[dict]:
    logger = AuditLogger(log_path)
    results = []
    for scenario in SCENARIOS:
        cmd = scenario.build()
        decision = evaluate(cmd)
        logger.record(cmd, decision)
        results.append({
            "scenario": scenario.name,
            "command_id": cmd.command_id,
            "command_type": cmd.command_type,
            "decision": decision.decision.value,
            "rule_id": decision.rule_id,
            "reason": decision.reason,
        })
        if verbose:
            print(f"[{cmd.command_id}] {scenario.name}")
            print(f"    -> {decision.decision.value} ({decision.rule_id}): {decision.reason}")
    return results


if __name__ == "__main__":
    print("AEGIS Phase 1 — Scenario Simulator")
    print("=" * 60)
    run_simulator()
    print("=" * 60)
    print("Note: Phase 1 proof-of-concept only. No PKI/HSM/CAN/ML/cloud/DB.")
    print("Not claimed production-ready or ISO/UNECE certified.")