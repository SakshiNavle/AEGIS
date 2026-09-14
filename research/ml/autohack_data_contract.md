# AutoHack GuardCAN ML data contract

Version: `AUTOHACK_GUARDCAN_DATA_CONTRACT_V1`.

This is an industry-style research proof of concept informed by traceability,
quality, and leakage-control practices. It is not ISO/SAE 21434, ISO 26262,
AUTOSAR, UNECE R155 compliant, certified, production-ready, or ASIL-certified.

## 1. Provenance and measured scope

The primary dataset is AutoHack 2025, Zenodo DOI
`10.5281/zenodo.19661007`. The archive SHA-256 is
`7b6112b98f775e2b66ca3aea229754ab1eb30b11020343a674f4c5723d570a51`.
The full audit streamed 6,922,600 train rows and 2,711,497 test rows from the
combined interface CSVs. No raw records or generated sequences were committed.

The actual schema is:
`Interface,Timestamp,Arbitration_ID,DLC,Data,Label`.
Interfaces are C-CAN, P-CAN, and B-CAN. Vehicle-disjoint, capture-disjoint,
and attack-disjoint provenance are NOT VERIFIED.

## 2. Quality gates

| File | Input | Valid | Rejected | Exact duplicates | Timestamp reversals | Negative delta | Zero delta |
|---|---:|---:|---:|---:|---:|---:|---:|
| Train | 6,922,600 | 6,922,600 | 0 | 0 | 0 | 0 | 6,154 |
| Test | 2,711,497 | 2,711,497 | 0 | 0 | 0 | 0 | 2,325 |

Rejection reasons were empty for both files. Invalid timestamp, CAN ID, DLC,
payload, and payload/DLC mismatch counts were therefore zero. Empty payloads
were counted through the DLC=0 path; the full audit found no rejected empty
payload records. All observed payloads agreed with DLC. Exact duplicates were
counted by canonical row fingerprint and were not removed.

Zero deltas are retained and documented, not silently treated as invalid.
Duplicate timestamps may be legitimate same-time CAN records.

## 3. Causal feature contract

The seven feature outputs are:

1. `delta_t` in seconds, from the previous row in the same file/interface.
2. `rolling_mean_delta_t`, causal over the previous 20 observations.
3. `rolling_std_delta_t`, causal population standard deviation over the
   previous 20 observations.
4. `id_frequency_ratio`, current-ID count in the trailing 100-row context
   divided by available context length.
5. `payload_hamming_distance`, raw bit distance against the previous payload
   for the same arbitration ID, comparing only bytes present in both payloads.
6. `dlc`, validated integer in [0, 8].
7. `arbitration_id`, parsed integer representation.

At stream start, `delta_t=0`, rolling statistics are zero until history exists,
frequency ratio is zero with empty history, and same-ID Hamming distance is
zero until a previous same-ID payload exists. These warm-up values are
documented and causal.

No feature uses `Label`, attack names, filename, path, capture ID, vehicle ID,
future timestamps, or future-derived statistics. `Interface` is a grouping
boundary and is not a model feature by default.

## 4. Full feature audit

Quantiles below are deterministic bounded-reservoir estimates with capacity
100,000; finite counts are exact. Units for timing features are seconds.

| File | Feature | Min | P50 | P95 | P99 | Max | finite |
|---|---|---:|---:|---:|---:|---:|---:|
| Train | `delta_t` | 0 | 0.000240 | 0.001350 | 0.003360 | 0.019520 | 6,922,600 |
| Train | rolling mean | 0.000102 | 0.000357 | 0.000484 | 0.003294 | 0.005764 | 6,922,600 |
| Train | rolling std | 0.00000497 | 0.000315 | 0.000645 | 0.003058 | 0.006181 | 6,922,600 |
| Train | ID frequency ratio | 0 | 0.03 | 0.03 | 0.37 | 0.94 | 6,922,600 |
| Train | payload Hamming | 0 | 3 | 14 | 21 | 41 | 6,922,600 |
| Train | DLC | 0 | 8 | 8 | 8 | 8 | 6,922,600 |
| Test | `delta_t` | 0 | 0.000240 | 0.001400 | 0.003440 | 0.018970 | 2,711,497 |
| Test | rolling mean | see artifact | see artifact | see artifact | see artifact | see artifact | 2,711,497 |
| Test | rolling std | see artifact | see artifact | see artifact | see artifact | see artifact | 2,711,497 |
| Test | ID frequency ratio | 0 | 0.03 | 0.03 | 0.40 | 0.94 | 2,711,497 |
| Test | payload Hamming | 0 | 3 | 14 | 21 | 42 | 2,711,497 |
| Test | DLC | 0 | 8 | 8 | 8 | 8 | 2,711,497 |

All feature values were finite. The observed ID range was 0--2047; one-hot
encoding is not required and is not selected for embedded deployment.

## 5. Variable DLC and Hamming behavior

The full audit observed same-ID transitions for DLC 0 through 8, including
different-DLC transitions. The implementation uses raw bit Hamming distance
over `zip(previous_payload, current_payload)`: bytes absent because of a
shorter DLC are not compared. DLC=0 therefore contributes distance zero. This
is an explicit prefix-overlap policy, not a silent eight-byte padding policy.
The same-ID DLC transition counts are retained in the audit JSON.

## 6. Split and duplicate audit

The supplied train/test files form a reproducible file-disjoint baseline. All
three interfaces occur in both files, so interface overlap is present by
construction. Timestamp ranges overlap numerically but timestamps are local
to each file and are not capture IDs. Vehicle, capture/session, and attack
disjointness are NOT VERIFIED.

Within-file exact duplicate counts were zero. A bounded cross-file fingerprint
check over the first 250,000 train rows against the test stream found zero
matches. This is evidence only for the audited sample, not proof of no
cross-file overlap over all rows. Repeated CAN traffic remains legitimate and
is not deleted. Sequences must be built after file/group splitting, separately
inside each file/interface stream.

## 7. Train-only preprocessing

`FittedPreprocessor.fit()` now maintains min/max statistics incrementally and
learns the ID vocabulary only from the records passed as training input.
Validation, test, and ROAD records are transform-only. Unit tests explicitly
verify that held-out records do not affect fitted statistics or vocabulary.
No scaler, threshold, frequency dictionary, or vocabulary was fitted on test
data in this audit.

## 8. Deployment contract

| Feature | Online? | State | Incremental? | Future-dependent? | ESP32-S3 feasibility |
|---|---|---|---|---|---|
| `delta_t` | Yes | previous timestamp | Yes | No | plausible; measure later |
| rolling mean/std | Yes | bounded 20-value history | Yes | No | plausible; measure later |
| ID frequency | Yes | bounded 100-ID history | Yes | No | plausible; measure later |
| payload Hamming | Yes | previous payload per ID | Yes | No | plausible; bound ID state |
| DLC | Yes | current frame | Yes | No | trivial |
| arbitration ID | Yes | current frame / optional train vocabulary | Yes | No | compact integer |

No ESP32 timing, RAM, or latency claim is made before measurement. The model
must produce cybersecurity evidence/risk for GuardCAN and must never become
the direct AEGIS command authority.

## 9. Reproduction and status

```powershell
$env:PYTHONPATH = 'C:\path\to\AEGIS'
$env:AEGIS_AUTOHACK_ROOT = 'C:\path\to\AutoHack\extracted\Autohack2025_Dataset'
python -m ml.full_audit $env:AEGIS_AUTOHACK_ROOT --output ml\artifacts\autohack_full_audit.json
python -m unittest discover -s tests\ml -p "test_*.py"
```

The audit used bounded streaming, no model training, no resampling, and no
dataset merging. The complete JSON audit is ignored by Git because it includes
machine-specific paths.

A) AUTOHACK DATA CONTRACT PASSED — READY FOR GUARDCan ML
