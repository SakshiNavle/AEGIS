"""
AEGIS Phase 1 — Entry Point
=============================
Usage:
    python3 main.py                 # run the scenario simulator
    python3 main.py --demo          # evaluate one hand-built example command

Phase 1 proof-of-concept only. No PKI/HSM/CAN/ESP32/MCP2515/FastAPI/React/
ML/cloud/database is implemented here. Not claimed production-ready or
ISO/UNECE certified.
"""

from __future__ import annotations

import argparse
import json
import sys

from models import (
    AuthContext,
    Command,
    CyberContext,
    CyberRiskLevel,
    GearPosition,
    VehicleContext,
)
from governance import evaluate, InvalidCommandError
from audit import AuditLogger
from simulator import run_simulator


def run_demo() -> int:
    cmd = Command(
        command_id="DEMO-001",
        command_type="REMOTE_UNLOCK",
        auth=AuthContext(authenticated=True, trusted_time_valid=True, replay_detected=False),
        vehicle=VehicleContext(is_moving=False, gear=GearPosition.PARK, speed_kmh=0.0),
        cyber=CyberContext(risk_level=CyberRiskLevel.LOW, risk_score=0.18),
    )

    try:
        decision = evaluate(cmd)
    except InvalidCommandError as exc:
        print(f"Invalid command: {exc}", file=sys.stderr)
        return 1

    logger = AuditLogger("aegis_audit.log")
    logger.record(cmd, decision)

    print(json.dumps(decision.to_dict(), indent=2))
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description="AEGIS Phase 1 governance engine — demo entry point."
    )
    parser.add_argument(
        "--demo", action="store_true",
        help="Evaluate a single example command and print its structured decision.",
    )
    args = parser.parse_args()

    if args.demo:
        return run_demo()

    print("AEGIS Phase 1 — Scenario Simulator")
    print("=" * 60)
    run_simulator(log_path="aegis_audit.log")
    print("=" * 60)
    print("Note: Phase 1 proof-of-concept only. No PKI/HSM/CAN/ESP32/MCP2515/")
    print("FastAPI/React/ML/cloud/database. Not production-ready or certified.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())