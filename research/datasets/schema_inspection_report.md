# Phase 7: Minimal data acquisition and schema inspection

Date: 2026-09-14  
Primary dataset: `can-train-and-test`

## Acquisition outcome

**No dataset files were downloaded.**

The smallest safe acquisition could not be completed because the authoritative
release endpoint available from the inspected publication references was not
reachable:

- DTU institutional publication record:
  https://orbit.dtu.dk/en/publications/can-train-and-test-a-curated-can-dataset-for-automotive-intrusion
- Dataset paper DOI:
  https://doi.org/10.1016/j.cose.2024.103777
- Conference paper DOI:
  https://doi.org/10.1109/vtc2023-fall60731.2023.10333756
- Release URL cited in the DOI/paper references and attempted during this
  phase: `https://bitbucket.org/lampe/can-train-and-test`

The Bitbucket landing path returned HTTP 404, and the Bitbucket API lookup for
the cited repository also returned HTTP 404. No authoritative file listing,
per-file URL, sample file, or archive manifest was exposed by the DTU page or
the DOI metadata. Because the only apparent fallback would be an archive
download whose size and contents are not verified, the acquisition was stopped
before downloading any full or large archive.

This is a blocker report, not a schema inference. No claims below imply that
the fields were observed in a data file.

## Downloaded-file manifest

| Filename | Source URL | Size | Vehicle/capture identity | Normal/attack | Attack type |
|---|---|---:|---|---|---|
| **NONE** | **NONE** | **0 bytes** | Not applicable | Not applicable | Not applicable |

## Authoritative high-level observations

The DTU institutional record verifies:

- Dataset variants named by the authors: `can-dataset`, `can-log`, `can-csv`,
  `can-ml`, and `can-train-and-test`.
- Four vehicles from two manufacturers.
- Equivalent attack captures for each vehicle model.
- Replayable `.log` files.
- Labeled and unlabeled `.csv` files.
- Nine unique attacks in `can-train-and-test`.
- DoS, gear spoofing, and standstill are named examples; the complete attack
  list was not exposed on the inspected landing page.
- Many spoofing-related attacks were performed live on the road and had known
  physical impacts.
- The authors benchmarked machine-learning IDSs and describe selecting attacks
  for training while reserving others for unseen-attack testing.

These statements are paper-level evidence only. They do not verify the exact
row schema of a machine-readable file.

## Field-level schema inspection

Because no representative file could be obtained, exact examples and measured
types are unavailable. `NOT VERIFIED` is used deliberately.

| Field | Exact name | Type | Example | Meaning | Verified? |
|---|---|---|---|---|---|
| Timestamp | NOT VERIFIED | NOT VERIFIED | NOT AVAILABLE | Message event time | NOT VERIFIED |
| CAN ID | NOT VERIFIED | NOT VERIFIED | NOT AVAILABLE | CAN arbitration/message identifier | NOT VERIFIED |
| DLC | NOT VERIFIED | NOT VERIFIED | NOT AVAILABLE | Number of payload bytes | NOT VERIFIED |
| Payload | NOT VERIFIED | NOT VERIFIED | NOT AVAILABLE | CAN data bytes | NOT VERIFIED |
| Normal/attack label | NOT VERIFIED | NOT VERIFIED | NOT AVAILABLE | Frame or capture label | NOT VERIFIED |
| Attack type | NOT VERIFIED | NOT VERIFIED | NOT AVAILABLE | Attack class/name | NOT VERIFIED |
| Vehicle identifier | NOT VERIFIED | NOT VERIFIED | NOT AVAILABLE | Vehicle/model identity | NOT VERIFIED |
| Capture/session identifier | NOT VERIFIED | NOT VERIFIED | NOT AVAILABLE | Drive/capture/session grouping | NOT VERIFIED |
| Source filename | NOT VERIFIED | NOT VERIFIED | NOT AVAILABLE | Provenance key | NOT VERIFIED |

The paper verifies `.log` and `.csv` availability, but does not expose exact
column names or encoding on the inspected official page.

## Requested inspection measurements

No records were available for measurement:

1. File format: `.log` and `.csv` are paper-level verified; exact syntax is
   NOT VERIFIED.
2. Column names: NOT VERIFIED.
3. Timestamp field and units: NOT VERIFIED.
4. Timestamp precision: NOT VERIFIED.
5. CAN ID representation: NOT VERIFIED.
6. DLC representation: NOT VERIFIED.
7. Payload representation: NOT VERIFIED.
8. Label column: NOT VERIFIED.
9. Attack labels/types: nine unique attacks are paper-level verified; exact
   machine-readable values are NOT VERIFIED.
10. Vehicle identifier: NOT VERIFIED.
11. Capture/session identifier: NOT VERIFIED.
12. Number of records per file: NOT MEASURABLE; no files downloaded.
13. Missing values: NOT MEASURABLE.
14. Malformed rows: NOT MEASURABLE.
15. Duplicate rows: NOT MEASURABLE.
16. Payload length versus DLC: NOT MEASURABLE.
17. Timestamp monotonicity: NOT MEASURABLE.

## ML Feature Feasibility

The five features cannot yet be implemented against the actual dataset schema.
Their status is therefore `NEEDS TRANSFORMATION` pending a representative
file, not `FEASIBLE` based only on the paper description.

| Feature | Status | Reason |
|---|---|---|
| `delta_t` | NEEDS TRANSFORMATION | A timestamp is strongly implied by CAN logs, but exact field name, units, precision, ordering, and reset boundaries are NOT VERIFIED. |
| `rolling_mean_delta_t` | NEEDS TRANSFORMATION | Requires verified timestamps and capture/session boundaries; neither row schema nor boundary field is available. |
| `rolling_std_delta_t` | NEEDS TRANSFORMATION | Same timestamp and session-boundary dependency; missing-row behavior is also unknown. |
| `ID_frequency_ratio` | NEEDS TRANSFORMATION | Requires verified CAN ID field/encoding and a declared frequency window; vehicle/session scope is not verified. |
| `payload_hamming_distance` | NEEDS TRANSFORMATION | Requires verified payload bytes, DLC, and same-ID comparison semantics; all are unverified. |

No feature is marked `FEASIBLE` until the raw sample confirms its required
fields and encodings.

## Data Quality Findings

No raw records were available, so exact data-quality counts are:

| Finding | Count |
|---|---:|
| Downloaded data files | 0 |
| Records inspected | 0 |
| Missing values | NOT MEASURABLE |
| Malformed rows | NOT MEASURABLE |
| Exact duplicate rows | NOT MEASURABLE |
| Near-duplicate rows | NOT MEASURABLE |
| DLC/payload mismatches | NOT MEASURABLE |
| Non-monotonic timestamps | NOT MEASURABLE |

## Leakage-relevant fields and risks

The paper-level description makes vehicle-aware and attack-aware evaluation
plausible, but the actual leakage keys are not exposed:

- Vehicle/model identity may be encoded in filenames or directories rather than
  a row column.
- Capture/session identity may likewise be filename-only.
- Equivalent attack captures across vehicle models can make repeated attack
  traces or aligned intervals near-duplicates.
- Labeled and unlabeled exports may contain the same raw frames.
- Replayable `.log` and `.csv` variants may represent the same capture in
  different formats.
- Random row or overlapping-window splitting would risk leakage even if the
  vehicle field is absent.

These risks require file-level hashing and provenance preservation after access
is obtained. They were not measurable in this phase.

## Smallest next safe acquisition

Do not download the full archive until the release owner provides a reachable
file listing or direct per-file URLs. The minimum requested sample remains:

1. One normal capture.
2. One attack capture.
3. One capture from a different vehicle.

For each file, record its URL, byte size, filename, vehicle/model, capture or
session identity, label, and attack type before parsing. If the release only
offers a monolithic archive, obtain its byte size and manifest first; if that
requires downloading the archive itself, stop and request an official sample
or file-level endpoint instead.

## Final status

Phase 7 is **blocked at minimal acquisition**. The blocker is the unavailable
official release path and absence of a safe, authoritative per-file/sample
endpoint. No full dataset was downloaded and no schema, data-quality, or
feature-computation claim was fabricated.
