# Phase13A overlap forensics

## 1. Scope

This is a read-only forensic investigation of the 85 existing Phase13A cross-split row fingerprints. It does not modify the Phase13A implementation, manifest, status, recommendation, or source data. Both canonical CSVs were processed with streaming CSV reads; no complete file was loaded into RAM.

## 2. Dataset files

Only these files were used:

- `AEGIS_AUTOHACK_ROOT/extracted/Autohack2025_Dataset/Interface/train/autohack_train_both_interface.csv`
- `AEGIS_AUTOHACK_ROOT/extracted/Autohack2025_Dataset/Interface/test/autohack_test_both_interface.csv`

The local root used for this investigation was `C:\Users\ASUS\OneDrive\Desktop\AEGIS_DATA\AutoHack`. The alternative `data`, `label`, `Train`, and `Test` representations were not used.

## 3. Fingerprint definition

The existing Phase13A fingerprint definition was treated as immutable:

`Interface + Timestamp + Arbitration_ID + DLC + Data + Label`

The same existing parser and fingerprint function were used. The investigation retained only matching fingerprints and their source row numbers.

## 4. Results

### Exact equality

- Cross-split matching fingerprints: **85**
- Exact identical Train/Test row pairs across all six fields: **85**
- Pairs with any field difference: **0**

For every match, `Interface`, `Timestamp`, `Arbitration_ID`, `DLC`, `Data`, and `Label` were equal. This confirms that these are exact duplicate records under the Phase13A identity, not merely similar CAN frames.

### Interface distribution

| Interface | Count |
|---|---:|
| C-CAN | 85 |
| P-CAN | 0 |
| B-CAN | 0 |

No interface categories other than C-CAN occurred among the 85 overlaps.

### Label distribution

| Label | Count |
|---|---:|
| Normal | 85 |

No attack label occurred among the overlapping records.

### Arbitration ID distribution

The parser-preserved hexadecimal IDs were used:

| Arbitration ID | Count |
|---|---:|
| `356` | 82 |
| `391` | 2 |
| `389` | 1 |

### Timestamp relationship

Because the timestamp is part of the fingerprint, the timestamp comparison was still performed explicitly:

- Train/Test timestamp differences: **0 for all 85 pairs**
- Minimum timestamp: **230.75259**
- Maximum timestamp: **579.13617**
- Unique timestamps: **85**

Representative exact pairs include:

| Train row | Test row | Interface | Timestamp | Arbitration ID | DLC | Data | Label |
|---:|---:|---|---:|---|---:|---|---|
| 764426 | 930695 | C-CAN | 230.75259 | `391` | 8 | `00 00 00 00 00 00 00 00` | Normal |
| 1785272 | 2163698 | C-CAN | 441.81745 | `389` | 8 | `00 00 00 00 00 00 00 00` | Normal |
| 1948731 | 2318286 | C-CAN | 470.09384 | `356` | 8 | `00 00 00 00 00 00 00 AA` | Normal |
| 2575662 | 2698522 | C-CAN | 579.13617 | `391` | 8 | `00 00 00 00 00 00 00 00` | Normal |

### Duplicate multiplicity

All 85 fingerprints had exactly one occurrence in Train and exactly one occurrence in Test:

| Train multiplicity | Test multiplicity | Fingerprint count |
|---:|---:|---:|
| 1 | 1 | 85 |

No matching fingerprint appeared multiple times in Train or multiple times in Test. Thus the result is not caused by multiplicity of the same complete row within either file.

### Representative neighboring contexts

The following bounded samples inspect at most three rows before and after each selected match. They are representative examples for the observed overlap population. Since every overlap is C-CAN/Normal, an attack-labelled overlap and a non-C-CAN overlap were not available to select.

#### Sample A: timestamp 230.75259, Arbitration ID `391`

- Train matching row: 764426.
- Test matching row: 930695.
- The exact matching row is C-CAN/Normal in both files.
- Train neighbors include C-CAN/Normal frames with IDs `368`, `340`, `4f1`, `164`, and `386`, followed immediately by a B-CAN/Fuzzing row at 764427.
- Test neighbors include C-CAN/Normal IDs `485`, `495`, `4f1`, followed by C-CAN/Spoofing and P-CAN/Normal rows.
- The neighboring IDs, interfaces, labels, and payloads are not the same sequence. This supports an identical individual frame appearing in different surrounding traffic, but does not establish how the files were produced.

#### Sample B: timestamp 441.81745, Arbitration ID `389`

- Train matching row: 1785272.
- Test matching row: 2163698.
- Train neighbors include mixed P-CAN/C-CAN Normal frames and a C-CAN/DoS row at 1785274.
- Test neighbors include a different mixed C-CAN/P-CAN set; all seven displayed rows are Normal.
- The local contexts are not identical and do not demonstrate a copied multi-row sequence.

#### Sample C: timestamp 470.09384, Arbitration ID `356`

- Train matching row: 1948731.
- Test matching row: 2318286.
- The surrounding seven-row windows contain different preceding and following IDs and payloads, although both windows contain ordinary mixed C-CAN/P-CAN traffic.
- No identical ±3-row context was established.

#### Sample D: timestamp 579.13617, Arbitration ID `391`

- Train matching row: 2575662.
- Test matching row: 2698522.
- Both contexts contain nearby C-CAN DoS rows, but the surrounding B-CAN/P-CAN/C-CAN records and payloads differ.
- This is evidence of a repeated complete frame in distinct local contexts, not proof of a duplicated capture segment.

## 5. Evidence classification

- **85 × `EXACT_DUPLICATE_RECORD`**: proven by equality of all six fingerprint fields.
- **`REPEATED_CAN_OBSERVATION`**: supported as the conservative description of the records: the same complete CAN observation occurs once in each file, predominantly ID `356`.
- **`POSSIBLE_SEQUENCE_OVERLAP`**: not established by the inspected ±3-row contexts. The selected windows are not identical sequences.
- **`UNRESOLVED`**: capture/session/vehicle provenance and the mechanism that placed the records in both files remain unresolved.

The evidence does not prove that the records are harmless naturally recurring frames, and it does not prove that they came from duplicated capture segments. The dataset contains no verified capture/session provenance in this investigation.

## 6. Interpretation

The 85 matches are exact duplicated records at the six-field row-identity level. They are all Normal C-CAN observations, with a highly concentrated Arbitration ID distribution. Their local neighborhoods differ in the representative samples, so the available evidence indicates repeated individual CAN observations rather than demonstrated sequence-level duplication.

However, sequence-level/capture-level disjointness is **NOT_VERIFIED**. The timestamps being equal is expected because timestamp is part of the fingerprint; it does not identify whether Train and Test came from the same capture or from independent captures containing the same observation.

## 7. Impact on Phase13A

The existing Phase13A decision is intentionally unchanged:

> Existing Phase13A status remains unchanged pending human decision.

The manifest remains `FAILED` with recommendation `BLOCKED_PENDING_DATA_VALIDATION`. This report does not regenerate or alter that manifest.

## 8. Recommendation for the human reviewer

**Retain blocking status and require dataset/provider clarification** regarding whether the canonical Train/Test files can share exact timestamped CAN observations and whether capture/session identifiers or a documented split-construction procedure exist.

If a human reviewer later accepts exact row overlap as a documented dataset limitation, continuation should still carry an explicit caveat that row-level disjointness is not present and that vehicle/capture/session/attack-provenance disjointness remains `NOT_VERIFIED`. No conclusion of sequence-level leakage or legitimate independence should be made from these rows alone.
