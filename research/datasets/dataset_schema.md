# Phase 6: can-train-and-test schema feasibility audit

Date: 2026-09-14

## Scope and inspection method

The full dataset was **not downloaded**. The smallest authoritative inspection
available in this pass was:

1. DTU's institutional publication record for the dataset paper:
   https://orbit.dtu.dk/en/publications/can-train-and-test-a-curated-can-dataset-for-automotive-intrusion
2. DOI records for the dataset papers:
   https://doi.org/10.1016/j.cose.2024.103777 and
   https://doi.org/10.1109/vtc2023-fall60731.2023.10333756
3. The release endpoint cited by the paper, which currently returns 404 at the
   attempted Bitbucket path (`bitbucket.org/lampe/can-train-and-test`).

The DTU record is authoritative for the high-level dataset structure, but it
does not expose a sample CSV or directory listing. Therefore, exact row-level
schema facts are marked **NOT VERIFIED** rather than inferred from other CAN
datasets.

## Verified facts

| Item | Finding | Evidence/status |
|---|---|---|
| Dataset variants | `can-dataset`, `can-log`, `can-csv`, `can-ml`, and `can-train-and-test` are named by the authors | DTU institutional abstract |
| Vehicles | Four vehicles from two manufacturers | Verified by DTU abstract |
| Attack coverage | Nine unique attacks for `can-train-and-test` | Verified by DTU abstract |
| Attack examples | DoS, gear spoofing, and standstill are explicitly named | Verified; complete nine-name list NOT VERIFIED |
| File formats | Replayable `.log` files, labeled `.csv` files, and unlabeled `.csv` files | Verified by DTU abstract |
| ML purpose | Authors benchmark machine-learning IDSs and describe train/test and unseen-attack use | Verified by DTU abstract |
| Physical setting | Many spoofing-related attacks were performed live on the road and had known physical impacts | Verified as a qualified statement; not every attack |
| Official train/test files | The paper says the dataset supports train/test use, but exact filenames/folders were not exposed by the inspected authoritative page | NOT VERIFIED |

## Unverified schema facts

The following require access to the release archive or a paper appendix/sample
file. They must not be assumed from ROAD, HCRL, or other CAN datasets:

- Exact folder and filename structure.
- Whether `can-train-and-test` contains one file per vehicle, model, attack,
  drive, and/or session.
- Exact CSV column names and ordering.
- Timestamp units, epoch/relative origin, precision, and monotonicity.
- CAN ID encoding (hex string, decimal integer, prefixed hex, or mixed).
- DLC encoding and whether it is present explicitly in CSV.
- Payload encoding (one byte per column, one hex string, delimiter, padding,
  missing bytes, or decoded integers).
- Exact normal/attack label column and label values.
- Vehicle identifier field.
- Capture/session/drive identifier field.
- Whether train/test are published as separate files or only described in the
  paper.
- Exact archive size.
- Duplicate and near-duplicate record rates.
- Malformed-row frequency and the authors' intended handling.
- Formal dataset license and whether commercial competition use is permitted.

## Feature feasibility

The five proposed features are feasible **if** the sample contains the
standard raw CAN fields described by the paper's `.log`/`.csv` claim. This is
an implementation feasibility assessment, not a claim that the fields were
already observed.

| Feature | Required fields | Feasibility now | Required handling |
|---|---|---|---|
| `delta_t` | Timestamp | **Conditional; timestamp units NOT VERIFIED** | Normalize to seconds, sort within capture, reject negative/non-finite deltas |
| `rolling_mean_delta_t` | Timestamp, ordered capture | Conditional | Compute only within a capture/session; never roll across file or vehicle boundaries |
| `rolling_std_delta_t` | Timestamp, ordered capture | Conditional | Define minimum warm-up count and behavior for short windows |
| `ID_frequency_ratio` | Timestamp, CAN ID | Conditional; CAN ID representation NOT VERIFIED | Canonicalize ID to integer, compute frequency over a fixed time window, keep vehicle/session scope |
| `payload_hamming_distance` | Payload bytes and same-ID comparison | Conditional; payload/DLC NOT VERIFIED | Compare equal-length canonical byte vectors; mask invalid/padded bytes; do not compare unrelated IDs |

## Minimal metadata/sample acquisition plan

Do not download the full archive. Before any model work, obtain only one of:

1. A release landing-page file listing/checksum and a small representative
   sample supplied by the authors; or
2. A single labeled CSV and one replayable `.log` from one vehicle, one benign
   capture, and one attack capture, if the release supports per-file retrieval;
   or
3. If the release is archive-only, download the archive metadata and use HTTP
   range access only if the server supports it. Do not assume ZIP central
   directory/range access is available until verified.

The minimum useful sample is three files/streams:

- one normal capture,
- one attack capture,
- one different-vehicle capture.

The sample must be large enough to include at least one complete attack
interval, but its byte size is **NOT VERIFIED** until the release is reachable.

## Proposed intake checks once a small sample is available

1. Enumerate files and identify vehicle/model, attack, drive, and session tokens
   from names and metadata.
2. Read only headers and the first/last 20 rows before full parsing.
3. Parse timestamps without coercion first; report units, range, duplicates,
   monotonicity, and within-file gaps.
4. Canonicalize CAN IDs and report parse failures.
5. Validate DLC against payload length and CAN limits.
6. Parse labels and produce an exact label inventory.
7. Hash complete rows and key tuples `(timestamp, id, dlc, payload, label)` to
   quantify exact duplicates.
8. Detect near-duplicates using same-ID adjacent payload Hamming distance and
   repeated timestamp/ID patterns.
9. Preserve source file, vehicle, capture, and session identifiers in every
   derived row.

## Storage estimate

Exact storage for the smallest useful subset is **NOT VERIFIED** because the
official release size and per-file sizes were not exposed by the inspected
source. The smallest practical subset should be designed as three raw files
plus a derived feature table, not as the entire multi-vehicle archive. Record
the actual byte count before proceeding.

## Bottom line

can-train-and-test is schema-feasible for AEGIS, but only at the paper-level
until a representative release file is obtained. The authoritative source
supports the existence of replayable logs, labeled/unlabeled CSVs, four
vehicles, and nine attacks; it does **not** yet verify the exact raw columns
needed to implement the five features. No model training should begin until
the minimal sample acquisition and intake checks above are completed.
