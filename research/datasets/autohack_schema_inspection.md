# AutoHack Dataset Acquisition and Schema Inspection

## A. Acquisition

The official Zenodo record is [AutoHack 2025](https://zenodo.org/records/19661007),
DOI [10.5281/zenodo.19661007](https://doi.org/10.5281/zenodo.19661007). The
Zenodo API exposed two files:

| File | URL | Size | Use |
|---|---|---:|---|
| `Autohack2025_Dataset.zip` | `https://zenodo.org/records/19661007/files/Autohack2025_Dataset.zip` | 309,108,946 bytes | Dataset archive |
| `AutoHack-Benchmark.zip` | Official artifact archive listed by Zenodo | 29,634 bytes | Code and documentation only |

No individual raw CSV files or downloadable sample subset were exposed by the
Zenodo file listing. The full archive was therefore downloaded only after its
exact size was known and after confirming that a representative-file download
was not available. The observed download size was exactly 309,108,946 bytes.
The archive was extracted locally for inspection; no model was trained and no
dataset was merged.

## B. Dataset identity and provenance

The archive contains the official repository layout described by the
[AutoHack-Benchmark repository](https://github.com/autohackdataset-cyber/AutoHack-Benchmark):

* `Autohack2025_Dataset/Interface/train/`
* `Autohack2025_Dataset/Interface/test/`
* `Autohack2025_Dataset/Train/`
* `Autohack2025_Dataset/Test/`

The official README identifies three CAN interfaces: C-CAN, P-CAN, and B-CAN.
The Zenodo description identifies physically verified CAN traffic, attacks
executed in a real vehicle environment, and Fuzzing, Spoofing, Replay, DoS, and
UDS-based attack families. The archive itself does not expose a vehicle-name or
vehicle-ID column. Consequently, real-vehicle provenance is verified at the
dataset level, but the number of vehicles and vehicle-disjoint membership of
the supplied train/test files are **NOT VERIFIED**.

## C. File manifest

The following files were inspected or identified in the official archive:

| Relative path | Bytes | Contents |
|---|---:|---|
| `Interface/train/autohack_train_both_interface.csv` | 363,619,756 | Interface, data, and labels |
| `Interface/test/autohack_test_both_interface.csv` | 141,421,105 | Interface, data, and labels |
| `Interface/train/autohack_train_data_interface.csv` | 315,106,051 | Interface and data only |
| `Interface/train/autohack_train_label_interface.csv` | 55,436,306 | Labels only |
| `Interface/test/autohack_test_data_interface.csv` | 122,428,983 | Interface and data only |
| `Interface/test/autohack_test_label_interface.csv` | 21,703,620 | Labels only |
| `Train/autohack_train_both.csv` | 322,054,167 | Data and labels without interface |
| `Test/autohack_test_both.csv` | 125,133,741 | Data and labels without interface |

The combined interface files were used for the measured results below because
they contain all six fields in one row and preserve bus identity.

## D. Exact schema

The exact header in both inspected combined interface files is:
`Interface,Timestamp,Arbitration_ID,DLC,Data,Label`.

| Field | Exact name | Type | Example | Meaning | Verified? |
|---|---|---|---|---|---|
| CAN bus | `Interface` | string | `C-CAN` | Recorded CAN interface/bus | YES |
| Time | `Timestamp` | decimal seconds | `0.00104` | Message time from capture start | YES; units are supported by the official preprocessing code |
| CAN identifier | `Arbitration_ID` | hexadecimal-looking string | `5E0` | CAN arbitration identifier | YES |
| Length | `DLC` | integer | `3` | Number of payload bytes; observed range 0--8 | YES |
| Payload | `Data` | space-separated hexadecimal byte string | `03 30 20` | CAN payload bytes | YES |
| Class label | `Label` | string | `Normal` | Normal or attack family/variant | YES |

`Interface` is a bus identifier, not a verified vehicle identifier. No
capture/session column, source filename column, or train/test membership field
appears in the combined rows. Train/test membership is nevertheless implicit
in the directory and filename and must be treated as metadata, not as a model
feature.

## E. Data Quality Findings

Measurements below were made by streaming every row in the two combined
interface files. Counts exclude the header.

| File | Records | Missing `Data` | Malformed rows | Exact duplicate rows | Timestamp reversals | DLC/payload mismatches |
|---|---:|---:|---:|---:|---:|---:|
| Train interface | 6,922,600 | 15,044 | 0 | 0 | 0 | 0 |
| Test interface | 2,711,497 | 5,896 | 0 | 0 | 0 | 0 |

The missing `Data` values are the rows with `DLC=0`; they are valid empty
payloads rather than malformed records. Every non-empty payload consisted of
two-digit hexadecimal byte tokens, and its byte count agreed with `DLC`.

Observed timestamp ranges were `0.00104`--`1400.81922` seconds for train and
`0.00059`--`583.15111` seconds for test. Both files were monotonically
non-decreasing. Timestamp precision was five decimal places in the inspected
rows (10 microseconds), although the dataset documentation does not establish
that every source capture has identical precision.

Observed label values were `Normal`, `Fuzzing`, `Spoofing`, `Replay`, `DoS`, and
the following UDS spoofing variants:

* `Spoofing_UDS_B_702_B4`
* `Spoofing_UDS_B_706_B17`
* `Spoofing_UDS_B_700_B34`
* `Spoofing_UDS_Steering_7D4`
* `Spoofing_UDS_LeftRadar_7B7`
* `Spoofing_UDS_FrontCam_7C4`
* `Spoofing_UDS_RightRadar_755`
* `Spoofing_UDS_P_7E0_B6`
* `Spoofing_UDS_P_7E0_B4`

Train records by interface were C-CAN 4,026,488, P-CAN 2,655,149, and B-CAN
240,963. Test records were C-CAN 1,678,489, P-CAN 883,323, and B-CAN 149,685.
The archive validator reports the same intended six-column schema and checks
null critical fields, chronological order, and DLC range, but it does not
prove vehicle-disjointness or cross-file deduplication.

Near-duplicate records, duplicate capture segments, and overlap between train
and test were **NOT VERIFIED**. These require capture provenance or a more
specialized sequence-level comparison and must not be assumed absent.

## F. Leakage Risks

The following fields or metadata must be excluded from unsupervised input or
handled only for grouping and evaluation:

1. `Label` directly identifies normal versus attack and must be the target only.
2. `Interface` can identify bus-specific traffic. It may be retained for a
   deliberately bus-specific experiment, but must be excluded when measuring
   bus-independent generalization.
3. Directory and filename (`Train`/`Test`, `both`, `data`, `label`) reveal
   split membership and file role.
4. Timestamp can encode capture boundaries or session position. It should be
   transformed to within-capture timing and never used as an absolute global
   identifier.
5. Any future preprocessing fields such as rolling frequencies, previous
   intervals, or labels copied into derived files must be recomputed inside
   each training fold.
6. Vehicle and capture identity are not explicit in the inspected schema.
   If later metadata supplies them, they must be grouping variables and never
   ordinary features.

The supplied train/test partition is usable as a baseline split, but whether
it is vehicle-disjoint, capture-disjoint, or attack-session-disjoint is
**NOT VERIFIED**. It must not be described as a generalization split without
additional provenance evidence.

## G. ML Feature Feasibility

| Feature | Status | Evidence and required handling |
|---|---|---|
| `delta_t` | FEASIBLE | Subtract adjacent numeric `Timestamp` values after sorting within a capture/bus; do not cross capture boundaries. |
| `rolling_mean_delta_t` | FEASIBLE | Compute from `delta_t` in a fixed frame-count or time window, fit/calibrate only on training data, and reset at capture boundaries. |
| `rolling_std_delta_t` | FEASIBLE | Same boundary and train-only calibration controls as the rolling mean. |
| `ID_frequency_ratio` | FEASIBLE | Count parsed `Arbitration_ID` values in a defined window and normalize using training-derived statistics; the ID string must be normalized consistently. |
| `payload_hamming_distance` | NEEDS TRANSFORMATION | Parse `Data` into byte vectors, pad or compare only over a defined DLC-compatible width, and define behavior for `DLC=0`; it is not computable correctly from the raw string without this transformation. |

## H. Primary-dataset suitability

| Criterion | Rating | Assessment |
|---|---|---|
| Data accessibility | HIGH | Official Zenodo download is stable and its exact archive size is known, although no small-file endpoint is exposed. |
| Schema suitability | HIGH | Combined CSVs have explicit time, ID, DLC, payload, label, and bus fields. |
| Feature feasibility | HIGH | All five proposed features are feasible after the stated ordering, windowing, and payload transformations. |
| Attack diversity | HIGH | Fuzzing, Spoofing, Replay, DoS, and multiple UDS spoofing variants are present. |
| Vehicle/context diversity | LOW | Three buses are verified, but vehicle count and vehicle identity are not verified in the data release. |
| Leakage risk | MEDIUM | Labels, bus identity, split directories, timestamps, and possible unexposed capture structure require strict controls. |
| Reproducibility | HIGH | DOI, checksum metadata, archive layout, README, and validator are available from the official release. |

AutoHack is practical for a reproducible GuardCAN development pipeline and
supports the requested low-cost timing, identifier-frequency, and payload
features. It is not sufficient by itself to demonstrate cross-vehicle
generalization because vehicle identity and vehicle-disjoint split provenance
remain unverified. No accuracy or model-performance claim is made here.

## I. Final decision

B) AUTOHACK REJECT — EVALUATE ROAD NEXT
