# Second-pass fact-check: AEGIS CAN datasets

Date: 2026-09-14

This pass independently checked the four candidate datasets against authoritative
dataset records, institutional publication pages, DOI records, and the official
dataset/artifact documentation. GitHub search descriptions and the first-pass
recommendation were not used as evidence. No full dataset was downloaded.

## Evidence sources

### ROAD

- Zenodo record: https://zenodo.org/records/10462796
- Dataset DOI: https://doi.org/10.5281/zenodo.10462796
- Dataset paper: Verma et al., “A comprehensive guide to CAN IDS data and introduction of the ROAD dataset,” PLOS ONE 19(1), DOI https://doi.org/10.1371/journal.pone.0296879
- Paper states that ROAD has one vehicle, 12 ambient captures totaling about 3 hours, 33 attack captures totaling about 30 minutes, real fuzzing/fabrication/advanced attacks, simulated masquerade attacks, and physical verification of the attack effects.
- Zenodo reports `road.zip` size 556,718,276 bytes and CC BY 4.0.

### can-train-and-test

- Institutional publication record: DTU Orbit, https://orbit.dtu.dk/en/publications/can-train-and-test-a-curated-can-dataset-for-automotive-intrusion
- Dataset papers: DOI https://doi.org/10.1016/j.cose.2024.103777 and conference DOI https://doi.org/10.1109/vtc2023-fall60731.2023.10333756
- The DTU abstract states four vehicles from two manufacturers, equivalent attack captures for each vehicle model, replayable `.log` files, labeled and unlabeled `.csv` files, nine unique attacks, and many real on-road spoofing attacks with known physical impacts.
- The abstract does not publish the complete nine-name attack list, archive byte size, capture durations, exact columns, bus count, or dataset license.

### AutoHack

- Official Zenodo record/API: https://zenodo.org/records/19661007 and https://zenodo.org/api/records/19661007
- Dataset DOI: https://doi.org/10.5281/zenodo.19661007
- Official artifact: https://github.com/autohackdataset-cyber/AutoHack-Benchmark
- Zenodo states that traffic was collected from C-CAN, P-CAN, and B-CAN; attack scenarios are Fuzzing, Spoofing, Replay, DoS, and UDS-based attacks; attacks were executed in a real vehicle environment; license is CC BY 4.0; archive size is 309,108,946 bytes.
- The official preprocessing code confirms `Interface`, `Label`, `Timestamp`, `Arbitration_ID`, and `Data` fields, plus official train/test files. It does not establish the number of vehicles, DLC availability, raw payload-byte layout, capture durations, or a vehicle-disjoint split.

### HCRL Car-Hacking

- Official HCRL dataset page: https://ocslab.hksecurity.net/Datasets/CAN-intrusion-dataset
- HCRL states that the archive contains DoS, fuzzy, drive-gear spoofing, and RPM-gauge spoofing; it was logged through OBD-II from a real vehicle during message-injection attacks; each dataset contains 300 intrusions, each lasting 3–5 seconds, and 30–40 minutes of CAN traffic.
- HCRL explicitly documents the fields: Timestamp, CAN ID, DLC, DATA[0] through DATA[7], and Flag, where `T` is injected and `R` is normal.
- The official page does not state an exact vehicle count, bus count, archive byte size, formal license, or physical-effect verification.
- The ROAD paper's independent quality review identifies collection artifacts in this dataset, including a long transmission gap after the attacks and a train/test mismatch risk.

## Comparison table

| Dataset | Evidence source | Vehicles | Buses | Attacks | Normal data | Attack data | Timestamp | CAN ID | DLC | Payload | License | Real/Synthetic | Physical verification | Cross-vehicle evaluation | ML suitability | AEGIS suitability |
|---|---|---:|---:|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| ROAD | Zenodo + PLOS ONE DOI above | **1** | NOT VERIFIED | Fuzzing; targeted fabrication; Correlated Signal; Max Speedometer; Max Engine Coolant Temperature; Reverse Light; Accelerator; other advanced targeted attacks; simulated masquerade variants. The paper gives the attack taxonomy and named examples, with 33 attack captures. | **12 ambient captures, about 3 h**; the paper also describes about 3 h of road-driving training data | **33 captures, about 30 min** | Yes for raw CAN captures | Yes | Yes by raw CAN format, exact column spelling NOT VERIFIED | Yes; raw CAN data and signal-translated data | **CC BY 4.0** | Real vehicle CAN; mixed real and simulated attack traces | **Yes**: paper says intended effects were observed and documented; masquerade traces are simulated by deletion | **No**: one vehicle means no valid cross-vehicle test | High for IDS development; especially good for temporal/stealth research, but requires leakage-safe splits | High for realistic attack robustness; not sufficient alone for cross-vehicle claims |
| can-train-and-test | DTU Orbit record + DOI papers above | **4 vehicles, 2 manufacturers** | NOT VERIFIED | **9 unique attacks**; DoS, gear spoofing, and standstill are explicitly named in the institutional abstract; full nine-name list NOT VERIFIED | Normal captures exist, but exact count and duration NOT VERIFIED | Equivalent labeled attack captures per vehicle model; exact count and duration NOT VERIFIED | NOT VERIFIED from authoritative abstract | NOT VERIFIED from authoritative abstract | NOT VERIFIED | NOT VERIFIED; CSV/log availability is verified, byte columns are not | NOT VERIFIED | Real vehicle data; many spoofing attacks performed live/on-road | **Partially verified**: the abstract says many spoofing attacks had known physical impacts; it does not say every attack did | **Yes in principle**: four vehicles/two manufacturers and equivalent captures are explicitly intended for generalization; exact vehicle-disjoint split still requires archive inspection | **High**: the authors benchmark ML and explicitly support train/test and unseen-attack use | **High for development/generalization**, subject to archive/schema/license verification |
| AutoHack | Zenodo record/API + official artifact code | NOT VERIFIED | **3 named buses: C-CAN, P-CAN, B-CAN** | Fuzzing, Spoofing, Replay, DoS, UDS-based attacks | Official train file exists; exact normal duration/count NOT VERIFIED | Official test file exists; exact attack duration/count NOT VERIFIED | **Yes** (`Timestamp`) | **Yes** (`Arbitration_ID`) | NOT VERIFIED | **Yes as `Data` field; exact byte encoding NOT VERIFIED** | **CC BY 4.0** | Real vehicle traffic | **Yes at dataset level**: Zenodo says attacks were executed in a real vehicle environment and calls traffic physically verified; exact per-attack effect evidence NOT VERIFIED | NOT VERIFIED: vehicle count and vehicle identifiers are not established by the authoritative metadata reviewed | **High for reproducible IDS benchmarking**, with 8-feature and 38-feature official preprocessing; vehicle-disjoint evaluation is not established | High for independent multi-bus validation; weaker for cross-vehicle claims until vehicle metadata is verified |
| HCRL Car-Hacking | Official HCRL page + ROAD paper review | NOT VERIFIED | NOT VERIFIED | DoS, fuzzy, drive-gear spoofing, RPM-gauge spoofing | Each archive is 30–40 minutes total; exact normal duration NOT VERIFIED | 300 intrusions per dataset, each 3–5 seconds | **Yes** | **Yes** | **Yes** | **Yes**, DATA[0]–DATA[7] | NOT VERIFIED | Real vehicle capture with injected messages | NOT VERIFIED by the official page | No evidence of cross-vehicle capability in the official page; treat as NOT VERIFIED | Moderate for baseline/prototyping, but the ROAD review documents collection artifacts and warns about train/test mismatch | Low-to-moderate as primary evidence; useful as a compact historical baseline |

## Fourteen-point verification notes

### ROAD

1. Vehicles: **1**, explicitly stated by both paper and Zenodo.
2. CAN buses: **NOT VERIFIED**. The prior claim of multiple buses was unsupported and is withdrawn.
3. Attack types: The paper verifies real fuzzing, fabrication, advanced targeted attacks, and simulated masquerade; named examples include Correlated Signal, Max Speedometer, Max Engine Coolant Temperature, Reverse Light, and Accelerator. A complete per-file list should be taken from Table 5/metadata before modeling.
4. Real/simulated: **Both**. Real fuzzing/fabrication/advanced attacks; simulated masquerade captures.
5. Size: **556,718,276 bytes** for the Zenodo ZIP.
6. Normal captures: **12, about 3 hours**.
7. Attack captures: **33, about 30 minutes**.
8. Physical effects: **Verified and documented** for intended attack effects; simulated masquerade is a post-processing deletion, not a physically executed suspension.
9. Fields: Raw CAN plus signal-translated time series; exact raw column names are NOT VERIFIED in the paper text.
10. Payload: **Yes**, raw data fields are present; signal-translated files are also available for many captures.
11. Timestamps: **Yes** for raw CAN captures.
12. Vehicle-aware split: **No**. Only one vehicle exists. A session/time-aware split is possible, but it is not cross-vehicle.
13. ML training: **Yes**, the paper explicitly presents ROAD as a benchmark for CAN IDS methods; it is suitable for development but not sufficient for generalization claims.
14. License: **CC BY 4.0**, suitable for research and competition use with attribution, subject to competition rules.

### can-train-and-test

1. Vehicles: **4**, from **2 manufacturers**.
2. CAN buses: **NOT VERIFIED**.
3. Attack types: **9 unique attacks** verified; only DoS, gear spoofing, and standstill are named in the institutional abstract, so the complete list is NOT VERIFIED here.
4. Real/simulated: Real CAN data; many spoofing attacks were performed live/on-road. Whether every attack is real rather than simulated is NOT VERIFIED.
5. Size: **NOT VERIFIED**.
6. Normal captures: **Exist**, exact count/duration NOT VERIFIED.
7. Attack captures: Equivalent per-vehicle-model attack captures exist; exact count/duration NOT VERIFIED.
8. Physical effects: **Partially verified** for many spoofing attacks; not established for every attack.
9. Fields: Replayable `.log`, labeled `.csv`, and unlabeled `.csv` are verified; exact columns NOT VERIFIED.
10. Payload: **NOT VERIFIED** from the authoritative abstract.
11. Timestamps: **NOT VERIFIED** from the authoritative abstract.
12. Vehicle-aware split: **Yes in principle**, and explicitly supported by equivalent captures across four vehicles/two manufacturers; archive-level vehicle identifiers and a prescribed split are NOT VERIFIED.
13. ML training: **Yes**, the authors benchmark ML IDSs and describe train/test and unseen-attack use.
14. License: **NOT VERIFIED**. “Open/public” does not establish a competition-use license.

### AutoHack

1. Vehicles: **NOT VERIFIED**.
2. CAN buses: **3 named buses**, C-CAN, P-CAN, and B-CAN.
3. Attack types: Fuzzing, Spoofing, Replay, DoS, and UDS-based attacks.
4. Real/simulated: Real vehicle traffic and real-vehicle attack execution are verified; whether any files are simulated or post-processed is NOT VERIFIED.
5. Size: **309,108,946 bytes** for `Autohack2025_Dataset.zip`; the artifact ZIP is separate and only 29,634 bytes.
6. Normal captures: Train file exists; exact count/duration NOT VERIFIED.
7. Attack captures: Test file exists; exact count/duration NOT VERIFIED.
8. Physical effects: Dataset metadata says physically verified and executed in a real vehicle environment; per-attack effect documentation is NOT VERIFIED.
9. Fields: Official code verifies `Interface`, `Label`, `Timestamp`, `Arbitration_ID`, and `Data`.
10. Payload: `Data` exists; exact raw byte representation and DLC are NOT VERIFIED.
11. Timestamps: **Yes**, `Timestamp`.
12. Vehicle-aware split: **NOT VERIFIED**. The supplied train/test split is not evidence of vehicle-disjoint evaluation.
13. ML training: **Yes**, official preprocessing and RF observations are supplied; it is suitable for benchmark development.
14. License: **CC BY 4.0**, compatible with attribution-based research/competition use.

### HCRL Car-Hacking

1. Vehicles: **NOT VERIFIED** by the official HCRL page.
2. CAN buses: **NOT VERIFIED**.
3. Attack types: DoS, fuzzy, drive-gear spoofing, RPM-gauge spoofing.
4. Real/simulated: Real vehicle CAN logging with message injection is verified; exact physical attack execution/effects are NOT VERIFIED.
5. Size: **NOT VERIFIED** as bytes; each attack dataset has 30–40 minutes of traffic.
6. Normal captures: Included normal frames exist, but exact normal-only duration is NOT VERIFIED.
7. Attack captures: 300 intrusions per dataset, each 3–5 seconds.
8. Physical effects: **NOT VERIFIED** by the official page.
9. Fields: Timestamp, CAN ID, DLC, DATA[0..7], Flag.
10. Payload: **Yes**, DATA[0..7].
11. Timestamps: **Yes**, recorded time in seconds.
12. Vehicle-aware split: **No evidence** in the official documentation; treat as NOT VERIFIED.
13. ML training: Possible, but the independent ROAD review identifies collection artifacts and a problematic train/test mismatch; only baseline use is recommended.
14. License: **NOT VERIFIED**.

## Answers to the required questions

### Q1. Strongest PRIMARY training/development dataset

**can-train-and-test, confidence MEDIUM.** It is the only candidate here with authoritative evidence of four vehicles from two manufacturers, equivalent attack captures, nine attacks, replayable logs, labeled/unlabeled CSVs, and explicit ML benchmarking. That makes it stronger for development and generalization than ROAD's single vehicle. The confidence is not HIGH because archive size, exact schema, bus count, complete attack list, and license are not yet verified.

ROAD remains a strong **single-vehicle stealth/temporal development dataset**, but the previous claim that it was the best general primary dataset was too broad.

### Q2. Strongest EXTERNAL validation dataset

**AutoHack, confidence MEDIUM.** It has a different source and explicitly verified three-bus real-vehicle traffic, five named attack families, a public 309 MB archive, official preprocessing, and CC BY 4.0. It is a strong external check of multi-bus robustness. Vehicle count and vehicle-disjointness are not verified, so it should not be described as cross-vehicle validation.

### Q3. Strongest evidence for cross-vehicle/generalization

**can-train-and-test, confidence HIGH for the evidence claim.** The institutional abstract explicitly states four vehicles from two manufacturers and equivalent attack captures intended to assess generalization. This is substantially stronger than ROAD, AutoHack, or HCRL where an authoritative vehicle-disjoint design was not verified.

### Q4. Strongest evidence for real-world/stealth attack robustness

**ROAD, confidence HIGH.** The PLOS paper documents real targeted fabrication, fuzzing, and advanced attacks, increasing stealth, physical-effect verification, and simulated masquerade variants. This corrects the previous conflation of “realistic” with “cross-vehicle”: ROAD is strongest for stealth/temporal attack realism, not vehicle generalization.

### Q5. Easiest to process within limited project time

**HCRL Car-Hacking, confidence HIGH.** The official page gives a simple, explicit schema with Timestamp, CAN ID, DLC, eight data bytes, and a normal/injected flag. It is compact in duration and has no multi-bus parsing requirement. Its known artifacts make it unsuitable as the main evidence set, but it is the fastest parser/model smoke-test source.

### Q6. Should datasets be merged?

**No, not for the primary scientific result.** Merging raw datasets would mix different vehicle semantics, ID spaces, timestamp conventions, bus loads, labels, attack definitions, and collection artifacts. It would also make leakage and domain shift hard to diagnose. Use dataset-specific training and evaluation, then report cross-dataset transfer. A pooled auxiliary-training experiment may be run later only with explicit source/vehicle/session IDs, canonicalized schemas, source-held-out tests, and no claim that the pooled score is a single homogeneous benchmark.

### Q7. Final recommendation and confidence

**Final recommendation: MEDIUM confidence.**

1. **Primary development dataset: can-train-and-test** — strongest verified cross-vehicle development evidence, but verify the archive schema, license, bus count, byte size, and capture durations before committing.
2. **External validation: AutoHack** — strongest independently archived multi-bus candidate with CC BY 4.0 and official preprocessing; do not call it cross-vehicle until vehicle metadata is verified.
3. **Stealth/real-world robustness supplement: ROAD** — use for physically verified and increasingly stealthy attacks; do not claim cross-vehicle validity.
4. **Fast smoke-test backup: HCRL Car-Hacking** — easiest schema and processing, but known artifacts and limited attack diversity require careful caveats.

The former `PRIMARY = ROAD` recommendation is explicitly corrected. It was based on unsupported claims that ROAD had three vehicles and multiple buses. The authoritative ROAD sources establish one vehicle; bus count, exact raw column names, and vehicle-aware generalization are not verified.
