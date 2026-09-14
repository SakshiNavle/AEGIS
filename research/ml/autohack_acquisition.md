# AutoHack acquisition report

`AUTOHACK_ACQUISITION_V1`

Source: `https://zenodo.org/api/records/19661007/files/Autohack2025_Dataset.zip/content`

Archive: `Autohack2025_Dataset.zip`

Local path: `C:\Users\ASUS\OneDrive\Desktop\AEGIS_DATA\AutoHack\raw\Autohack2025_Dataset.zip`

Byte size: `309108946`

SHA-256: `7b6112b98f775e2b66ca3aea229754ab1eb30b11020343a674f4c5723d570a51`

Official checksum: `NOT_PUBLISHED` (the SHA-256 above is a local provenance
identifier, not an official checksum)

Download date: `2026-09-14`

Verification status: `VERIFIED`

The archive was downloaded once from the official Zenodo endpoint. The
published content length and local byte count both equal 309,108,946 bytes.
The archive contained 21 members; all member paths were checked for absolute
paths and traversal before extraction. No archive member was executed.

Extraction root:
`C:\Users\ASUS\OneDrive\Desktop\AEGIS_DATA\AutoHack\extracted\`

Candidate combined files:

| File | Bytes | Role |
|---|---:|---|
| `Autohack2025_Dataset\Interface\train\autohack_train_both_interface.csv` | 363,619,756 | Train |
| `Autohack2025_Dataset\Interface\test\autohack_test_both_interface.csv` | 141,421,105 | Test |

Both files have the actual header:
`Interface,Timestamp,Arbitration_ID,DLC,Data,Label`.

## Bounded smoke test

The environment variable `AEGIS_AUTOHACK_ROOT` was set locally to the
extraction root. No machine-specific path was added to tracked source code.
The test consumed only the first 250 rows from each combined file:

* train: 250 input, 250 valid, 0 rejected;
* test: 250 input, 250 valid, 0 rejected;
* observed labels: `Normal`;
* observed interfaces: C-CAN and B-CAN;
* timestamps parsed as numeric seconds;
* hexadecimal CAN IDs parsed;
* DLC and payload lengths agreed;
* causal feature rows had seven numeric features;
* 50-frame sequences were constructed separately per file/interface stream;
* labels were absent from feature matrices;
* leakage and split-integrity audits passed with zero violations.

The smoke test did not scan the full dataset and did not train a model.
