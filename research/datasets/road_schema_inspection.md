# ROAD Dataset Acquisition and Schema Inspection

## A. Acquisition and provenance

The official release is the Zenodo record [Real ORNL Automotive Dynamometer
(ROAD) CAN Intrusion Dataset](https://zenodo.org/records/10462796), DOI
[10.5281/zenodo.10462796](https://doi.org/10.5281/zenodo.10462796). Its
metadata identifies Oak Ridge National Laboratory authors, a CC BY 4.0
license, and the ROAD paper ([arXiv:2012.14600](https://arxiv.org/abs/2012.14600)).

Zenodo exposed exactly one downloadable file:

* `road.zip`
* exact size: 556,718,276 bytes
* MD5: `cab184cfc2fe12c0834bc46188c0f330`
* official content URL:
  `https://zenodo.org/api/records/10462796/files/road.zip/content`

No individual capture or representative-file endpoint was exposed. The known
size was verified before downloading the bounded archive. The downloaded file
matched both the advertised size and MD5. No model was trained, no datasets
were merged, and no phase1 or firmware files were changed.

The archive README states that ROAD contains one vehicle, approximately 12
ambient captures totaling about three hours, and 33 attack captures totaling
about 30 minutes. It provides raw CAN logs, signal-extraction CSVs, metadata
JSON, `data_table.csv`, and an anonymized DBC.

## B. File manifest

The archive layout is:

| Path | Contents |
|---|---|
| `road/ambient/*.log` | 12 ambient raw SocketCAN captures |
| `road/attacks/*.log` | 33 attack raw SocketCAN captures, including masquerade variants |
| `road/ambient/capture_metadata.json` | Ambient descriptions, elapsed time, dyno/road flag |
| `road/attacks/capture_metadata.json` | Attack descriptions, injection metadata, elapsed time, dyno/road flag |
| `road/data_table.csv` | Capture-category inventory |
| `road/signal_extractions/ambient/*.csv` | Signal time-series representations |
| `road/signal_extractions/attacks/*.csv` | Signal time-series representations |
| `road/signal_extractions/*/metadata.json` | Signal-file metadata |
| `road/signal_extractions/DBC/anonymized.dbc` | Anonymized signal definitions |
| `road/readme.md` | Official schema and usage documentation |

Representative files inspected:

| File | Bytes | Role |
|---|---:|---|
| `ambient/ambient_dyno_drive_basic_short.log` | 48,926,704 | Normal/ambient capture |
| `attacks/fuzzing_attack_1.log` | 2,269,732 | Fuzzing attack capture |
| `attacks/max_speedometer_attack_1_masquerade.log` | 9,703,838 | Masquerade attack capture |
| `signal_extractions/ambient/ambient_dyno_drive_basic_short.csv` | 67,058,935 | Signal representation |
| `signal_extractions/ambient/max_speedometer_attack_1_masquerade.csv` | 13,171,552 | Signal attack representation |

The complete archive was used for count and integrity measurements rather than
only the representative files.

## C. Exact schema

### Raw `.log` files

Raw rows have no header. The observed exact form is:

```text
(1030000000.000000) can0 162#00080003EA11F4CE
```

| Field | Exact name | Type | Example | Meaning | Verified? |
|---|---|---|---|---|---|
| Timestamp | unnamed parenthesized value | decimal seconds | `1030000000.000000` | Capture timestamp | YES |
| Interface | unnamed token | string | `can0` | CAN interface | YES |
| CAN ID | unnamed token before `#` | hexadecimal string | `162` | CAN arbitration ID | YES |
| Payload | unnamed hex value after `#` | hexadecimal string | `00080003EA11F4CE` | CAN data bytes | YES |
| DLC | not present explicitly | derived integer | `8` | Payload hex length divided by two | YES as derivation; NOT VERIFIED as an original field |
| Label | not present in row | capture-level metadata | `ambient` or attack filename | Normal/attack assignment | YES at capture level |

All audited raw rows used interface `can0`. All had six decimal places in the
stored timestamp text. The raw logs contain eight-byte payloads throughout the
audited release, so they are consistent with Classical CAN payload sizing;
there is no explicit CAN-FD flag or explicit DLC field in this representation.
The release documentation does not establish a separate CAN-FD capture.

### Signal-extraction CSV files

The exact header is:

```text
Label,Time,ID,Signal_1_of_ID,...,Signal_22_of_ID
```

`Label` is numeric: ambient rows are `0`, attack rows are generally `1`, but
the four accelerator captures are intentionally labelled `0` because they
record a physically anomalous vehicle state without injected messages. `Time`
is decimal seconds, `ID` is decimal, and the remaining columns are decoded
signal values with missing values represented as empty CSV cells/NaN. The
signal CSVs are therefore not interchangeable with the raw CAN schema.

## D. Data quality

The complete raw release was streamed and checked.

| Set | Captures | Records | Malformed rows | Timestamp reversals | Exact duplicate raw lines* | Payload parse errors |
|---|---:|---:|---:|---:|---:|---:|
| Ambient | 12 | 24,149,519 | 0 | 0 | 1,353 | 0 |
| Attacks | 33 | 4,095,020 | 0 | 0 | 180 | 0 |

`Exact duplicate raw lines` counts repeated complete text rows across the
respective set. They are not automatically data errors: repeated CAN frames
can be legitimate, and this check does not identify near-duplicate windows or
duplicate capture segments.

Every timestamp was numeric and non-decreasing within each file. Every payload
had an even number of hexadecimal characters and exactly eight bytes. Since
the raw format has no DLC column, payload/DLC agreement is verified only as the
derived rule `DLC = len(payload_hex)/2 = 8`; original DLC encoding is
**NOT VERIFIED**. No missing raw fields were observed because the raw format
is positional and all rows matched the parser.

The selected signal CSV `ambient_dyno_drive_basic_short.csv` had 996,482
records, zero malformed rows, zero exact duplicate rows, and monotonically
increasing `Time`. Its expected signal cells contain many blanks by design
because an ID does not carry every possible decoded signal on every frame.

## E. Leakage risks

1. **Attack label:** raw rows do not contain a label; assigning `ambient` or
   attack from the parent directory and filename is required. That assignment
   must be used only as the target/group metadata, never as an input feature.
2. **Capture identity:** filenames are unique capture/session identifiers.
   They can leak operating condition, attack type, and temporal context.
3. **Attack filename:** names such as `fuzzing_attack_1` and
   `max_speedometer_attack_1_masquerade` directly reveal the attack family and
   must not be exposed to the model.
4. **Vehicle identity:** ROAD documents one vehicle. There is no per-row
   vehicle field; the dataset-wide vehicle identity is constant and cannot
   support cross-vehicle evaluation.
5. **Train/test membership:** the release supplies no train/test files.
   Membership must be created by the experimenter at capture level and kept
   outside the feature table.
6. **Preprocessing metadata:** `capture_metadata.json`, injection intervals,
   modified/masquerade status, decoded signal names, and DBC-derived signal
   columns can leak attack timing or attack identity if used improperly.
7. **Near-duplicate content:** masquerade logs are paired with corresponding
   modified logs. Related captures must be kept in the same split, or the
   paired content can leak across train and test.

## F. Feature feasibility

| Feature | Status | Required handling |
|---|---|---|
| `delta_t` | FEASIBLE | Parse parenthesized seconds and subtract adjacent timestamps within one capture; reset at capture boundaries. |
| `rolling_mean_delta_t` | FEASIBLE | Compute after `delta_t` using a fixed window that resets per capture; fit any normalization on training captures only. |
| `rolling_std_delta_t` | FEASIBLE | Same capture-boundary and training-only normalization controls as the rolling mean. |
| `ID_frequency_ratio` | FEASIBLE | Parse hexadecimal IDs, count them in a defined window, and normalize using training captures only. |
| `payload_hamming_distance` | NEEDS TRANSFORMATION | Convert the hex payload to an eight-byte vector and define the comparison baseline/window; do not compare raw strings. |

The features are feasible for the raw logs without CAN-D decoding. Signal-level
features are a separate representation and should not be mixed with raw-byte
features in the same untracked preprocessing path.

## G. Recommended train/validation/test split

ROAD has one vehicle, so no split can measure cross-vehicle generalization.
Use capture-level and attack-family-level grouping:

### Normal development

Use only ambient captures for unsupervised development. Split the 12 ambient
captures into disjoint train, validation, and test groups, for example 8/2/2
captures, while keeping entire captures intact. Compute feature scaling,
frequency baselines, and rolling-state initialization from training captures
only.

### Capture-held-out evaluation

Hold out complete ambient captures and complete attack captures. Do not split
rows from one capture across partitions. Keep each modified/masquerade pair in
the same partition. This measures temporal and operating-condition
generalization within the single vehicle.

### Attack-held-out evaluation

Hold out attack families, not random rows. Candidate held-out families include
fuzzing, correlated-signal attacks, max-engine-coolant-temperature,
max-speedometer, reverse-light, and accelerator-state captures. Accelerator
captures require special interpretation because their published label is
normal despite a physically anomalous state.

### External validation

Use ROAD as an external robustness dataset only if development uses a
different dataset. If ROAD is used for development, external validation must
come from an independently sourced dataset and must preserve the raw-vs-signal
representation distinction.

## H. Suitability for AEGIS

| Criterion | Assessment |
|---|---|
| Data accessibility | HIGH: official DOI/Zenodo archive is downloadable and checksum-verified, but no small-file endpoint exists. |
| Schema suitability | HIGH for raw timing/ID/payload features; raw labels are capture-level rather than per-row. |
| Feature feasibility | HIGH: the five requested features are computable, with payload parsing required. |
| Capture-held-out evaluation | HIGH: 12 ambient and 33 attack captures are explicitly separated and named. |
| Attack-held-out evaluation | MEDIUM/HIGH: multiple attack families and variants exist, but attack-family semantics are filename/metadata based. |
| Real-world/stealth robustness | HIGH: the release documents physically verified real attacks and simulated masquerade variants. |
| Temporal generalization | HIGH within this vehicle when complete captures are held out. |
| Anomaly-detection suitability | HIGH for capture-held-out, one-vehicle development; labels and attack timing must be kept out of inputs. |
| Cross-vehicle generalization | NOT SUITABLE: ROAD contains one vehicle and cannot provide cross-vehicle evidence. |
| Primary-dataset suitability | MEDIUM: strong schema and attack realism, but one vehicle, capture-level labels, paired captures, and no supplied split limit its role as a sole primary dataset. |

ROAD is stronger than AutoHack for verified physical-effect and stealth-attack
robustness, and it supports defensible capture-held-out and attack-held-out
experiments. It does not solve the project’s missing cross-vehicle training
evidence. No ML accuracy claim is made.

B) ROAD ROBUSTNESS ONLY — PRIMARY DATASET STILL REQUIRED
