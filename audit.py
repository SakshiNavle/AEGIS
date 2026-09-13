"""
AEGIS Phase 1 — Audit Logging
===============================
Minimal, dependency-free audit trail. Every governance decision (whatever
it is) gets appended as one JSON line, so the log is append-only and
trivially diff/greppable. No database in Phase 1 — a flat JSONL file is
sufficient and keeps the module honest about scope.
"""

from __future__ import annotations

import json
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterator, Optional

from models import Command, GovernanceDecision


def now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


DEFAULT_LOG_PATH = Path("aegis_audit.log")


class AuditLogger:
    """Append-only JSONL audit logger.

    Each record combines the originating command's identifying fields with
    the resulting decision, so the log is self-contained for review without
    needing to cross-reference a separate command store.
    """

    def __init__(self, log_path: Path | str = DEFAULT_LOG_PATH) -> None:
        self.log_path = Path(log_path)

    def record(self, cmd: Command, decision: GovernanceDecision) -> dict:
        entry = {
            "logged_at": now_iso(),
            "command_id": cmd.command_id,
            "command_type": cmd.command_type,
            "auth": {
                "authenticated": cmd.auth.authenticated,
                "trusted_time_valid": cmd.auth.trusted_time_valid,
                "replay_detected": cmd.auth.replay_detected,
            },
            "vehicle": {
                "is_moving": cmd.vehicle.is_moving,
                "gear": cmd.vehicle.gear.value,
                "speed_kmh": cmd.vehicle.speed_kmh,
            },
            "cyber": {
                "risk_level": cmd.cyber.risk_level.value,
                "risk_score": cmd.cyber.risk_score,
            },
            "decision": decision.to_dict(),
        }
        with self.log_path.open("a", encoding="utf-8") as f:
            f.write(json.dumps(entry, sort_keys=True) + "\n")
        return entry

    def read_all(self) -> Iterator[dict]:
        if not self.log_path.exists():
            return
        with self.log_path.open("r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if line:
                    yield json.loads(line)

    def clear(self) -> None:
        if self.log_path.exists():
            self.log_path.unlink()