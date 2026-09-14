# HCRL Car-Hacking Dataset Evaluation

## A. Acquisition and provenance

The authoritative source is the [HCRL Car-Hacking Dataset page](https://ocslab.hksecurity.net/Datasets/CAN-intrusion-dataset),
maintained by the HCRL laboratory. It identifies the dataset as CAN traffic
logged through the OBD-II port of a real vehicle during message-injection
attacks and cites:

* Song, Woo, and Kim, “In-vehicle network intrusion detection using deep
  convolutional neural network,” *Vehicular Communications* 21 (2020), 100198.
* Seo, Song, and Kim, “GIDS: GAN based Intrusion Detection System for
  In-Vehicle Network,” PST 2018 / arXiv:1907.07377.

The official page says that users should fill out a questionnaire and that the
download URL will be sent by email. The current page also exposes a
HCRL-owned Dropbox folder as its dataset download link. The folder listing is
authoritative because it is linked directly from the HCRL page and identifies
the owner as “Kim HuyKang (Hugo) (KU HCRL).”

The official listing exposes:

| Item | Official size shown | Role |
|---|---:|---|
| `normal_run_data.7z` | 7.16 MB | Normal-data archive |
| `normal_run_data/normal_run_data.txt` | 83.32 MB | Normal data inside the folder |
| `DoS_dataset.csv` | 181.25 MB | DoS attack |
| `Fuzzy_dataset.csv` | 189.32 MB | Fuzzy attack |
| `gear_dataset.csv` | 219.65 MB | Drive-gear spoofing |
| `RPM_dataset.csv` | 228.48 MB | RPM-gauge spoofing |

No smaller official attack sample was exposed. The 7.16 MB normal archive was
the smallest representative item, but its Dropbox content URL could not be
resolved by the available acquisition environment (`*.dl.dropboxusercontent.com`
DNS failure). No access control was bypassed and no unofficial mirror was
used. Therefore no HCRL data rows were downloaded or inspected in this phase.

The exact archive/file provenance is nevertheless tied to HCRL through the
official page link, owner attribution, file names, and sizes. File checksums
and row-level observations remain **NOT VERIFIED**.

## B. Dataset contents

The official documentation describes four attack datasets and one normal
dataset:

1. DoS attack: inject CAN ID `0000` every 0.3 ms.
2. Fuzzy attack: random CAN IDs and data every 0.5 ms.
3. Drive-gear spoofing.
4. RPM-gauge spoofing.
5. Attack-free normal traffic.

Each attack dataset is documented as containing 300 intrusions. Each intrusion
lasts 3--5 seconds, and each attack dataset contains approximately 30--40
minutes of CAN traffic. The page describes a real vehicle and real message
injection, but does not identify a vehicle make/model or provide a vehicle
identifier. The number of vehicles represented is therefore **NOT VERIFIED**
from the official page or available file listing.

## C. Exact schema

The official HCRL page documents the following 12 columns, in this order:

`Timestamp, CAN ID, DLC, DATA[0], DATA[1], DATA[2], DATA[3], DATA[4], DATA[5], DATA[6], DATA[7], Flag`

| Field | Exact name | Type | Example | Meaning | Verified? |
|---|---|---|---|---|---|
| Time | `Timestamp` | numeric | NOT VERIFIED | Recorded time in seconds | Documentation says seconds; row type/precision NOT VERIFIED |
| Identifier | `CAN ID` | hexadecimal text | `043f` | CAN message identifier | Documentation verified; row representation NOT VERIFIED |
| Length | `DLC` | integer 0--8 | NOT VERIFIED | Number of data bytes | Documentation verified; row values NOT VERIFIED |
| Payload byte 0 | `DATA[0]` | byte | NOT VERIFIED | First data byte | Documentation verified; row type NOT VERIFIED |
| Payload bytes 1--7 | `DATA[1]` ... `DATA[7]` | byte | NOT VERIFIED | Remaining data bytes | Documentation verified; row type NOT VERIFIED |
| Injection marker | `Flag` | `T` or `R` | `T` / `R` | `T` injected, `R` normal | Documentation verified; observed values NOT VERIFIED |

Attack/normal labels are represented by the file name and by `Flag`, rather
than by a documented attack-type column. Vehicle identity and capture/session
identity are not documented as columns. File names provide dataset-level
attack identity and therefore act as capture/attack metadata.

## D. Data quality

Because the official small normal archive could not be transferred and the
attack files are 181--228 MB each, the following row-level measurements are
**NOT VERIFIED**:

| Measurement | Result |
|---|---|
| Record counts in inspected files | NOT VERIFIED |
| Missing values | NOT VERIFIED |
| Malformed rows | NOT VERIFIED |
| Exact duplicate rows | NOT VERIFIED |
| Near-duplicate frames/windows | NOT VERIFIED |
| Timestamp monotonicity | NOT VERIFIED |
| Payload length versus DLC | NOT VERIFIED |
| Timestamp precision | NOT VERIFIED |

The official documentation establishes the intended DLC range (0--8) and
payload-byte fields, but this is not a substitute for validating actual files.

## E. Leakage risks

1. **Attack type in filename:** `DoS_dataset.csv`, `Fuzzy_dataset.csv`,
   `gear_dataset.csv`, and `RPM_dataset.csv` directly reveal the attack type.
   File name and path must never be model inputs.
2. **Directory labels:** normal versus attack is represented by the selected
   file/folder, and `Flag` directly identifies injected (`T`) versus normal
   (`R`) frames. `Flag` must be treated as the target or annotation, not a
   feature.
3. **Train/test membership:** no official train/test partition is documented.
   Any split must be created by the experimenter at capture/intrusion-block
   level.
4. **Vehicle identity:** no vehicle identifier is documented. Cross-vehicle
   generalization cannot be claimed.
5. **Capture identity:** the four attack files and normal file are coarse
   capture/group identifiers. Random row splitting would mix neighboring
   traffic and attack sessions.
6. **Duplicate and paired traffic:** the documented 300 repeated intrusions
   per attack dataset may create highly similar temporal windows. Exact and
   near-duplicate rates are NOT VERIFIED until files are inspected.
7. **Normal/attack same capture:** the official page says traffic was logged
   while attacks were performed, but does not document whether normal and
   injected frames are from separate captures. This is NOT VERIFIED.
8. **Temporal leakage:** timestamps and row order can encode intrusion blocks
   or file identity. Normalize time within a capture and reset state at
   boundaries.

## F. Feature feasibility

| Feature | Status | Required handling |
|---|---|---|
| `delta_t` | NEEDS TRANSFORMATION | Parse the documented seconds field, verify precision, sort within a capture, and reset at capture/intrusion boundaries. |
| `rolling_mean_delta_t` | NEEDS TRANSFORMATION | Compute only after verified timestamp parsing and capture-level windowing; fit normalization on training captures only. |
| `rolling_std_delta_t` | NEEDS TRANSFORMATION | Same timestamp, boundary, and training-only normalization requirements. |
| `ID_frequency_ratio` | NEEDS TRANSFORMATION | Normalize hexadecimal CAN IDs and define capture-local windows; do not use filename or attack labels. |
| `payload_hamming_distance` | NEEDS TRANSFORMATION | Convert `DATA[0..7]` to fixed-width byte vectors and define behavior for DLC values below eight. |

No feature is marked fully `FEASIBLE` because actual file parsing and data
quality checks were blocked.

## G. Recommended split protocol

### Capture-held-out testing

Do not randomly split rows. Use complete source files or documented capture
segments as groups. If the normal archive contains multiple runs, hold out
whole runs. If it contains only one run, capture-held-out normal evaluation is
not possible without additional official metadata.

### Attack-held-out testing

Train on a subset of attack families and hold out an entire family, such as
DoS, fuzzy, gear spoofing, or RPM spoofing. Hold out complete files and do not
mix rows from the held-out file into training.

### Temporal testing

Use chronological blocks within a capture only if the block boundaries are
known and all rolling state is reset or carried according to a declared causal
protocol. A random row split is invalid.

### Anomaly detection

Use only `Flag=R` normal frames for training. Use injected `Flag=T` frames and
held-out attack files for evaluation. Exclude `Flag`, file name, attack type,
and absolute timestamp from features.

### Supervised classification

Use file-level or intrusion-block-level groups. The target may be binary
normal/attack or attack family, but attack family is inferred from file
metadata unless a row-level family label is created outside the input table.

### External validation

HCRL can serve as external validation for a detector developed on another
dataset only after schema and provenance checks are completed. It is not
currently suitable as evidence of cross-vehicle generalization because no
vehicle-disjoint identity is documented.

## H. Suitability for AEGIS

| Criterion | Rating | Confidence and assessment |
|---|---|---|
| Accessibility | MEDIUM | Official page and linked HCRL Dropbox listing are public, but the page requests a questionnaire/email workflow and the available transfer path failed in this environment. |
| Schema suitability | MEDIUM | Documentation explicitly provides timestamp, CAN ID, DLC, eight payload bytes, and injection flag; actual row encoding remains uninspected. |
| Feature feasibility | MEDIUM | All five features are conceptually supported, but each requires parsing and validation before use. |
| Attack diversity | MEDIUM | Four documented attack families plus normal traffic; narrower than ROAD or AutoHack. |
| Vehicle diversity | LOW | A real vehicle is documented, but vehicle count and identity are not verified. |
| Leakage risk | MEDIUM/HIGH | File names, flags, repeated intrusions, and unknown capture boundaries make row-level random splitting unsafe. |
| Reproducibility | MEDIUM | Official source and paper are clear, but no official train/test split, checksum, or row-level manifest is provided. |

HCRL is useful as a compact, conventional CAN intrusion benchmark and a
potential smoke-test or external-validation dataset. It is not presently
strong enough to be the reproducible primary dataset for GuardCAN because
actual files could not be inspected, vehicle/capture provenance is limited,
and no official split protocol or vehicle-disjoint evidence is supplied.
No ML accuracy claim is made.

B) HCRL REJECT — EVALUATE ANOTHER PRIMARY DATASET
