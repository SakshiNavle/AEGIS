# AEGIS CACGF Phase 5

AEGIS (Adaptive Edge Governance for Intelligent Security) is a proof-of-concept
edge-local command-governance layer. It operates after message authentication
and freshness validation.

## CACGF

CACGF is the deterministic policy engine. It answers whether an already
validated command is eligible for execution under the current vehicle context,
cybersecurity risk evidence, command criticality, and queue state.

Authentication answers whether a message is valid and fresh. CACGF is separate:
it governs execution eligibility and does not authenticate messages. GuardCAN is
represented only by `CacgfRiskState`; it supplies evidence and does not decide
whether a command executes.

The evaluator is pure, bounded, non-blocking, has no dynamic allocation, and
does not access hardware, networking, databases, or machine-learning systems.

## Decision states

Exactly four decisions are supported:

- `EXECUTE`: the command is eligible now.
- `DELAY`: the command is eligible later when the context allows it.
- `REJECT`: the command must not execute.
- `ESCALATE`: the command requires higher-level handling and must not execute.

Safety/fail-safe rejection uses `REJECT` with an error reason.

## Decision priority

The policy is evaluated in this exact order:

1. Invalid authentication -> `REJECT / ERR_AUTH_FAIL`
2. Invalid freshness -> `REJECT / ERR_REPLAY`
3. Expired TTL -> `REJECT / ERR_TTL_EXPIRED`
4. Invalid vehicle context -> `REJECT / ERR_STALE_CONTEXT`
5. Low-criticality command above 5 km/h:
   - full queue -> `REJECT / ERR_QUEUE_FULL`
   - otherwise -> `DELAY / INFO_QUEUE_DELAYED`
6. High risk plus high criticality -> `ESCALATE / ERR_HIGH_RISK`
7. Unknown risk with non-low criticality -> `REJECT / ERR_DETECTOR_UNKNOWN`
8. Otherwise -> `EXECUTE / SUCCESS_EXECUTE`

Invalid pointers and enum values are rejected as
`REJECT / ERR_INVALID_STATE` before policy evaluation.

## Phase 1.5 local command policy

The local immutable policy table in `command_policy.c` resolves these command
IDs:

| Command ID | Description | Criticality | Delayable | TTL |
| --- | --- | --- | --- | --- |
| `0x100` | Door Unlock | LOW | Yes | 200 ms |
| `0x150` | Trunk Open | LOW | Yes | 500 ms |
| `0x200` | Apply Braking Torque | HIGH | No | 200 ms |
| `0x250` | Power Cutoff | HIGH | No | 200 ms |

These TTLs are proof-of-concept configuration values, not automotive
standards. The table is `static const`; the resolver copies a matching policy
into caller-owned storage and never exposes mutable table storage.

The command structure may carry sender-provided criticality, delayability, and
TTL fields to model an untrusted input, but CACGF ignores those fields. Only
the local policy resolver supplies policy attributes. Unknown command IDs
return `REJECT / ERR_UNKNOWN_COMMAND`.

## Phase 1 assumptions

- Command criticality is a locally resolved policy attribute. It is not trusted
  from an incoming command.
- `command_id` is resolved through the Phase 1.5 local policy table.
- Vehicle context and risk timestamps are represented for integration planning;
  timestamp age enforcement is intentionally not defined in the supplied
  Phase 1 policy.
- A low-criticality command may execute while risk is unknown if it is not
  delayed by vehicle motion.
- The 5 km/h comparison is strictly greater-than; exactly 5 km/h executes.
- This is not a certified automotive security or safety implementation.

## Tests

The test executable covers the original CACGF regression scenarios, Phase 1.5
policy resolution, and boundary/invalid-input cases: queue-full behavior, null
pointers, invalid enums, threshold speeds, risk/criticality combinations,
stale context, multi-failure priority, unknown commands, and attempted
sender-policy overrides. A separate executable covers Phase 2 freshness,
wraparound, TTL, reset, and invalid-state behavior.

## Phase 2 freshness and TTL

AEGIS separates communication freshness/replay protection from command
lifetime. Sequence state protects against replay/order ambiguity, while the
local command TTL limits how long a validated command may remain eligible
within the AEGIS processing pipeline.

The portable freshness module uses a 16-bit unsigned sequence counter. For an
initialized receiver state, it calculates:

```text
delta = incoming - last
```

using unsigned modular arithmetic. A sequence is newer only when:

```text
delta != 0 && delta < 32768
```

This accepts normal forward progress, permits forward gaps when frames are
lost, and correctly accepts `65535 -> 0`. Equal and older sequences are
rejected as replay. Freshness state can be initialized or reset; an
uninitialized state accepts its first sequence.

Sequence integrity and command lifetime are separate checks. The prototype
uses receiver-local monotonic elapsed time for command lifetime. It does not
infer sender-side absolute message age because synchronized/authenticated
cross-device time is not implemented. A receiver records `accepted_at_ms`
after authentication and freshness validation, then checks
`now_ms - accepted_at_ms <= local_ttl_ms` when a command is re-evaluated.
An expired newer command is sequence-valid but lifetime-invalid and produces
`REJECT / ERR_TTL_EXPIRED`; an equal or older sequence produces
`REJECT / ERR_REPLAY`.

The sequence module does not authenticate senders and does not implement
cryptographically complete replay protection. Its counter-based freshness
concept is a prototype, SecOC-inspired interface boundary, and its
sequence/timeout concepts are E2E-inspired. It is not an AUTOSAR SecOC or E2E
implementation, and no ISO/SAE 21434 compliance is claimed.

The local prototype TTL values remain:

- `0x100` Door Unlock: 200 ms
- `0x150` Trunk Open: 500 ms
- `0x200` Apply Braking Torque: 200 ms
- `0x250` Power Cutoff: 200 ms

These are configuration values for this proof of concept, not automotive
standards. Sender-supplied TTL remains untrusted and is ignored by the local
policy resolver.

Known limitation: sequence state is held in volatile process memory and is
reset on restart in this portable prototype. Persistence and restart
resynchronization are intentionally deferred.

The public `freshness_validate_and_accept` helper is intended only for callers
that have already authenticated the command. For a real receive flow, callers
should first call `freshness_check_sequence`, authenticate the command, and
then call `freshness_accept_sequence`; an unauthenticated frame must not advance
accepted sequence state. This portable module does not itself perform HMAC.

## Phase 3 vehicle context

Vehicle context is supplied through the dedicated `vehicle_context` module.
It uses fixed-width integer fields, including `uint16_t speed_kmh`, and does
not use floating point. Gear values include explicit `VEHICLE_GEAR_UNKNOWN`;
unknown or invalid gear values never become PARK.

Telemetry uses the existing Phase 2 16-bit counter freshness mechanism and a
receiver-local telemetry age threshold of 50 ms. The 50 ms value is prototype
engineering configuration, not a universal automotive requirement and not an
ISO or AUTOSAR timing requirement. Sequence freshness and telemetry age remain
separate: a newer sequence over 50 ms old is stale, while a replayed sequence
is rejected as stale input.

Vehicle context validity is explicit:

- `CONTEXT_UNKNOWN`: no usable context has been initialized.
- `CONTEXT_INVALID`: malformed telemetry or invalid context data.
- `CONTEXT_STALE`: replayed or over-age telemetry.
- `CONTEXT_VALID`: validated telemetry available to CACGF.

Malformed or stale telemetry never overwrites the stored values. Previous
values are retained for diagnostics, but `validity` becomes non-policy-valid,
so CACGF fails safely with `REJECT / ERR_STALE_CONTEXT` for unavailable
context. The context provider is not an authorization engine; CACGF remains
responsible for the execution decision.

Telemetry freshness protects against stale/replayed context but does not
establish semantic truth of the reported vehicle state. For this portable
proof of concept, the context source is assumed to be semantically
trusted/validated. A malicious source could still report a fresh but false
`speed = 0`, `gear = PARK` state.

The architecture is:

```text
Vehicle Context Provider
        |
        | validated context
        v
      CACGF
        |
        +-- command policy
        +-- security state
        +-- vehicle context
        +-- risk state (future)
```

## Phase 4 bounded delay queue

The delay queue is a bounded mechanism for holding only commands for which
CACGF has already returned `DELAY`. It is not an independent decision engine
and cannot bypass CACGF. The queue is a statically allocated FIFO ring buffer
with capacity 8; no dynamic allocation, linked lists, blocking, or priority
scheduling are used.

Only commands marked `delayable` by the immutable local command-policy table
may enter. High-criticality and non-delayable commands, including braking
torque and power cutoff, are never queued. Queue insertion rejects a full
queue with `REJECT / ERR_QUEUE_FULL` and never overwrites an existing entry.

Each entry stores the command, bounded payload, accepted sequence metadata,
receiver-local enqueue time, authentication metadata, and the TTL resolved
from local policy. Sender-provided criticality, delayability, and TTL are not
used. Vehicle context and risk snapshots are deliberately not stored as
authoritative queue data.

Before execution, the queue checks the receiver-local lifetime using unsigned
elapsed-time arithmetic, then re-runs CACGF with the current validated vehicle
context and current risk state. A command that remains unsafe stays queued
when its lifetime is valid; an expired command is removed and rejected with
`ERR_TTL_EXPIRED`; rejected or escalated commands are removed and never
executed. Processing handles only the FIFO front entry per call, providing a
bounded operation with no automatic retry loop.

## Phase 5 GuardCAN prototype monitor

GuardCAN is a deterministic evidence provider, not an authorization engine.
It observes already parsed Classical CAN frame representations and produces a
risk snapshot for CACGF:

```text
CAN traffic
    |
    v
GuardCAN monitoring
    |
    +-- LOW / MEDIUM / HIGH / UNKNOWN
    +-- detector health
    v
CACGF policy evaluation
    |
    +-- EXECUTE / DELAY / REJECT / ESCALATE
```

The implemented prototype heuristics are:

- allowlist checking for IDs `0x060`, `0x100`, `0x150`, `0x200`, `0x250`, and
  `0x700`
- DLC validation for those prototype message types
- bounded one-second message counting
- elevated traffic above 20 messages per window
- high traffic above 50 messages per window
- short inter-arrival intervals below 10 ms
- 16-bit sequence replay and forward-gap evidence using the existing
  counter-based freshness helper

The thresholds are named prototype engineering configurations, not vehicle CAN
limits or automotive standards. Payload semantics are intentionally minimal;
GuardCAN does not invent physical vehicle limits.

Risk fusion is integer-based and deterministic: no evidence is LOW, a score of
1-2 is MEDIUM, and a score of 3 or more is HIGH. Invalid observations degrade
detector health and produce UNKNOWN rather than silently producing LOW.
Health is separate from risk. A clean receiver-local window can recover
evidence to LOW, and a failed/degraded detector remains UNKNOWN until explicit
recovery.

GuardCAN provides anomaly evidence, risk estimation, and detector health. It
does not authenticate senders, prove authorization or physical truth, provide
complete intrusion-detection coverage, or claim production automotive
cybersecurity compliance. ML is not in the Phase 5 decision path. Future
TinyML may provide additional evidence, but CACGF remains the sole policy
authority.

CACGF receives a mapped current risk snapshot, and the delay queue receives
that current risk during re-evaluation. A command delayed under LOW risk
cannot bypass a later HIGH or UNKNOWN risk state.

The queue uses the same receiver-local monotonic timer model as Phase 2.
Unsigned subtraction supports timer wrap within the normal modular elapsed
time assumption. GuardCAN is not implemented yet; the process API accepts a
current `CacgfRiskState` so a later risk provider can be connected without
making the queue an authorization engine.

## Build and run

From the repository root:

```text
cmake -S phase1 -B phase1/build
cmake --build phase1/build
ctest --test-dir phase1/build --output-on-failure
```

The executable can also be run directly:

```text
phase1/build/Debug/test_cacgf.exe
```

On single-configuration generators where the executable is not under `Debug`,
run the corresponding `test_cacgf` binary from the build directory.

## Intentionally not implemented

Phase 4 intentionally excludes ESP32/ESP-IDF, FreeRTOS, CAN/TWAI/MCP2515,
SN65HVD230, HMAC or cryptographic libraries, networking, FastAPI, React,
databases, dashboards, machine learning, and hardware actuators. No claim is
made of ISO 21434, ISO 26262, UNECE R155, AUTOSAR SecOC, production security,
formal verification, or deterministic latency guarantees.
