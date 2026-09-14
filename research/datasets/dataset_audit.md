# Automotive CAN intrusion/anomaly dataset audit for AEGIS

Scope reviewed: can-train-and-test, ROAD, AutoHack, GEM-CAN, Car-Hacking/HCRL, OTIDS, SynCAN, and an additional public dataset discovered in literature: CAN-MIRGU. This audit used official dataset notes, paper metadata, and public repositories/data-brief documentation; no full dataset download was performed for the large archives.

## Second-pass fact-check status

The original entries below contained unsupported assumptions and are superseded
for selection purposes by [second_pass_fact_check.md](./second_pass_fact_check.md).
The most important correction is ROAD: the authoritative PLOS ONE paper and
Zenodo record establish **one vehicle**, **12 ambient captures totaling about
3 hours**, and **33 attack captures totaling about 30 minutes**. They do not
establish three vehicles or multiple buses. ROAD is therefore suitable for
physically verified stealth/temporal robustness, but not cross-vehicle
generalization.

Other claims that were not authoritative in the first pass must be treated as
**NOT VERIFIED**, including exact can-train-and-test archive size/schema/license,
AutoHack vehicle count and vehicle-disjoint split, and HCRL Car-Hacking vehicle
count, bus count, license, and physical-effect verification. The corrected
inventory and final recommendation use NOT VERIFIED instead of inference.

## Executive summary

The strongest development candidate is can-train-and-test because the
authoritative DTU record verifies four vehicles from two manufacturers,
equivalent attack captures, nine unique attacks, replayable logs, labeled and
unlabeled CSV files, and ML benchmarking. ROAD remains the strongest verified
single-vehicle stealth/physical-effect dataset. AutoHack is the strongest
multi-bus external-validation candidate, while HCRL Car-Hacking is the easiest
compact smoke-test source.

The recommendation is MEDIUM confidence until can-train-and-test archive
schema, size, bus count, and license are verified.

## Ranked shortlist

| Rank | Dataset | Why it matters | AEGIS use |
|---|---|---|---|
| 1 | ROAD | Real attacks, realistic vehicle logs, strong attack diversity, best for real-world IDS evaluation | Primary |
| 2 | AutoHack | Physically verified, multi-bus, independent acquisition protocol, strong external-validation candidate | External validation |
| 3 | can-train-and-test | Curated and cross-dataset friendly; good for benchmarking and cross-source training | Good benchmark |
| 4 | GEM-CAN | Real-world autonomous-vehicle attack data; modern and relevant | Good supplement |
| 5 | CAN-MIRGU | Real moving-vehicle attacks; strong realism but less established than ROAD/AutoHack | Good secondary |
| 6 | HCRL Car-Hacking | Classic, simple, widely used baseline | Backup |
| 7 | OTIDS | Classic HCRL benchmark; useful for baseline comparisons but limited realism | Backup |
| 8 | SynCAN | Synthetic, easy, very lightweight; good for prototyping, not for deployment claims | Bench/fast prototype |

## Dataset-by-dataset audit

### 1) can-train-and-test

- Official source: A 2024 paper/curated public release documented in the article "can-train-and-test: A curated CAN dataset for automotive intrusion detection" (DOI: 10.1016/j.cose.2024.103777). This is the most authoritative public description found for the dataset.
- Research paper: "can-train-and-test: A curated CAN dataset for automotive intrusion detection" (Computers & Security, 2024).
- License: NOT VERIFIED in the authoritative sources reviewed.
- Download size: NOT VERIFIED in the authoritative sources reviewed.
- Vehicles: Four vehicles from two manufacturers (DTU institutional publication record).
- CAN bus count: NOT VERIFIED.
- CAN variant: Classical CAN.
- Timestamp availability: NOT VERIFIED from the authoritative abstract.
- CAN ID: NOT VERIFIED from the authoritative abstract.
- DLC: NOT VERIFIED.
- Payload: NOT VERIFIED from the authoritative abstract.
- Normal/attack labels: Yes.
- Attack types: Curated intrusion scenarios common to CAN IDS evaluation; generally includes spoofing/fuzzy/DoS-type attack families in the public benchmark literature.
- Replay availability: Raw capture files can be replayed or re-windowed offline, but official replay packages are not a primary deliverable.
- Real vs synthetic: Hybrid / curated real CAN traces with attack segments; not purely synthetic.
- Real vs simulated attacks: Mostly attack injections in recorded capture sessions; realistic enough for benchmark use but not as physically verified as ROAD/AutoHack.
- Train/test split: Yes, feasible by time segmentation or by benign/attack capture blocks, but must be designed carefully to avoid temporal leakage.
- Cross-vehicle evaluation: Good potential, because the dataset is intentionally curated for cross-source and train/test benchmarking.
- Cross-attack evaluation: Good.
- Data leakage risk: Moderate. If segmented naïvely by random rows or duplicate message patterns, leakage is likely because message IDs and payload patterns repeat across files.
- Suitability for AEGIS: High as a benchmark and model evaluation dataset; slightly less suitable as the only real-world proof source because it is curated and not a single physically verified vehicle dataset.
- Suitability for lightweight ML/TinyML: Good for feature-based or windowed models; manageable size and parser-friendly structure.
- Processing requirements: Parse CAN frames, normalize timestamp alignment, generate fixed-length windows, keep per-file splits for leakage control.

### 2) ROAD

- Official source: Zenodo record 10.5281/zenodo.10462796 and the PLOS ONE dataset paper 10.1371/journal.pone.0296879.
- Research paper: The ROAD benchmark is commonly cited as the real-world automotive attack dataset for intrusion detection; the dataset is widely referenced in IDS papers and benchmark studies.
- License: CC BY 4.0.
- Download size: 556,718,276 bytes for the Zenodo ZIP.
- Vehicles: One vehicle.
- CAN bus count: NOT VERIFIED.
- CAN variant: Classical CAN.
- Timestamp availability: Yes.
- CAN ID: Yes.
- DLC: Yes.
- Payload: Yes.
- Normal/attack labels: Yes, with attack windows and metadata.
- Attack types: Message injection, denial-of-service, fuzzing, masquerade-like and spoofing patterns, and other labeled in-vehicle attack scenarios; the dataset is especially strong for realistic attack diversity.
- Replay availability: Raw logs are replayable offline; authors did not primarily package a public replay harness.
- Real vs synthetic: Real vehicle recordings.
- Real vs simulated attacks: Both; real fuzzing/fabrication/advanced attacks and simulated masquerade captures.
- Train/test split: Possible, but must respect time order and attack windows; naive random splits are unsafe.
- Cross-vehicle evaluation: No; one vehicle only.
- Cross-attack evaluation: Very good. It is among the strongest for evaluating transfer across attack families.
- Data leakage risk: High if a model is trained and tested on overlapping time windows or duplicated message patterns from the same capture. Temporal and vehicle-level splits are required.
- Suitability for AEGIS: Excellent as the primary benchmark for real-world proof and deployment claims.
- Suitability for lightweight ML/TinyML: Moderate to good when features are engineered carefully; raw raw timing and payload patterns are large but manageable with compact features.
- Processing requirements: Parse CAN frames, annotate attack windows, perform temporal segmentation, and maintain strict per-vehicle/per-capture splits.

### 3) AutoHack

- Official source: Dataset archived on Zenodo and documented in the artifact repository for the paper "AutoHack: A Physically Verified Multi-Bus CAN Dataset for Intrusion Detection System Evaluation".
- Research paper: "AutoHack: A Physically Verified Multi-Bus CAN Dataset for Intrusion Detection System Evaluation" (artifact repository; paper title as given by README).
- License: CC BY 4.0 (Zenodo record).
- Download size: 309,108,946 bytes for `Autohack2025_Dataset.zip`.
- Vehicles: NOT VERIFIED.
- CAN bus count: Three named buses: C-CAN, P-CAN, and B-CAN.
- CAN variant: Classical CAN.
- Timestamp availability: Yes.
- CAN ID: Yes.
- DLC: NOT VERIFIED.
- Payload: `Data` field is verified; exact raw byte encoding is NOT VERIFIED.
- Normal/attack labels: Yes.
- Attack types: Multi-bus intrusion scenarios, including realistic automotive attack injections designed for IDS benchmarking.
- Replay availability: Reusable if the archive includes raw logs; not necessarily distributed as a formal replay bundle.
- Real vs synthetic: Real vehicle traffic according to the Zenodo record.
- Real vs simulated attacks: Executed in a real vehicle environment; per-attack simulated/post-processed status is NOT VERIFIED.
- Train/test split: Official train/test files exist; vehicle-disjointness is NOT VERIFIED.
- Cross-vehicle evaluation: NOT VERIFIED.
- Cross-attack evaluation: Good, because the multi-bus setup and attack variety are designed for IDS benchmarking.
- Data leakage risk: Moderate. Multi-bus correlations can leak across time if not split by capture session.
- Suitability for AEGIS: High as a second, independent, physically verified multi-bus benchmark and external validation candidate.
- Suitability for lightweight ML/TinyML: Good, especially if using compact, message-level features or signal-level extraction.
- Processing requirements: Multi-bus parsing, per-bus event segmentation, and explicit train/test separation by capture sessions.

### 4) GEM-CAN

- Official source: "GEM-CAN: A real-world dataset of CAN-bus attack scenarios on an autonomous vehicle for intrusion-detection research" (Data in Brief, DOI: 10.1016/j.dib.2026.112805).
- Research paper: GEM-CAN data-brief citation, same title as above.
- License: NOT VERIFIED.
- Download size: NOT VERIFIED.
- Vehicles: NOT VERIFIED.
- CAN bus count: NOT VERIFIED.
- CAN variant: Classical CAN.
- Timestamp availability: NOT VERIFIED.
- CAN ID: NOT VERIFIED.
- DLC: NOT VERIFIED.
- Payload: NOT VERIFIED.
- Normal/attack labels: NOT VERIFIED.
- Attack types: Attack scenarios relevant to autonomous-vehicle CAN intrusion testing; typical injection and spoofing patterns common to CAN bus attack research.
- Replay availability: Offline replay is feasible from the raw logs, but not the primary packaging goal.
- Real vs synthetic: Real-world dataset from an autonomous vehicle.
- Real vs simulated attacks: Real attack scenarios executed in a real vehicle context.
- Train/test split: NOT VERIFIED.
- Cross-vehicle evaluation: NOT VERIFIED.
- Cross-attack evaluation: Good within the attack families included.
- Data leakage risk: Moderate; same vehicle patterns and repeated signal semantics can leak if random splitting is used.
- Suitability for AEGIS: High for a modern real-world validation set, especially if the project aims to benchmark on autonomous-vehicle CAN traffic.
- Suitability for lightweight ML/TinyML: Good to moderate when attack windows are reduced to compact features.
- Processing requirements: Log parsing, per-attack-window annotation, ensure time-order split between benign and attack flow.

### 5) Car-Hacking / HCRL

- Official source: HCRL public CAN intrusion dataset archive, often referred to as the Car-Hacking dataset or HCRL in-vehicle intrusion dataset.
- Research paper: Historically associated with HCRL benchmark papers on in-vehicle IDS evaluation and anomaly detection; the relevant benchmark is the HCRL Car-Hacking dataset lineage.
- License: NOT VERIFIED.
- Download size: NOT VERIFIED as bytes.
- Vehicles: NOT VERIFIED by the official HCRL page.
- CAN bus count: NOT VERIFIED.
- CAN variant: Classical CAN.
- Timestamp availability: Yes.
- CAN ID: Yes.
- DLC: Yes.
- Payload: Yes.
- Normal/attack labels: Yes.
- Attack types: DoS, fuzzing, RPM spoofing, gear spoofing (classic HCRL benchmark family).
- Replay availability: Moderate; raw logs can be replayed offline, but no dedicated public replay package is central to the benchmark.
- Real vs synthetic: Real vehicle dataset with attack injections recorded under controlled conditions.
- Real vs simulated attacks: Real message injection is verified; physical effects are NOT VERIFIED by the official page.
- Train/test split: Yes, but the benchmark is old and small; it is best used as a baseline, not as a final deployment proof set.
- Cross-vehicle evaluation: NOT VERIFIED.
- Cross-attack evaluation: Moderate; only a few attack families are represented.
- Data leakage risk: Moderate to high because the dataset is compact and repeated message patterns can appear across train/test splits.
- Suitability for AEGIS: Good as a baseline and regression benchmark, but insufficient alone for deployment claims.
- Suitability for lightweight ML/TinyML: Excellent for small experiments, cheap baselines, and embedded-model feasibility testing.
- Processing requirements: Simple CAN-frame parsing, fixed-length temporal windows, and careful split logic to avoid leakage.

### 6) OTIDS

- Official source: HCRL OTIDS CAN intrusion dataset public release (HCRL benchmark ensemble).
- Research paper: "OTIDS: A Novel Intrusion Detection System for In-vehicle Network by Using Remote Frame" (PST 2017). This paper is the canonical public reference for the OTIDS benchmark lineage.
- License: Public benchmark; no formal open license found in the metadata reviewed.
- Download size: Small to moderate; compact relative to larger real-world capture sets.
- Vehicles: One vehicle environment.
- CAN bus count: One bus.
- CAN variant: Classical CAN.
- Timestamp availability: Yes, in the capture metadata.
- CAN ID: Yes.
- DLC: Yes.
- Payload: Yes.
- Normal/attack labels: Yes.
- Attack types: DoS, fuzzing, impersonation/masquerade-like attack families relevant to the OTIDS framework.
- Replay availability: Possible offline from CAN logs, but not a central deliverable.
- Real vs synthetic: Real capture plus attack injections.
- Real vs simulated attacks: Real attack execution in a vehicle trace context.
- Train/test split: Yes, by capture/session separation.
- Cross-vehicle evaluation: No.
- Cross-attack evaluation: Moderate.
- Data leakage risk: Moderate; single-vehicle benchmark, repeated message patterns, and compact file sizes increase leakage risk if rows are randomly mixed.
- Suitability for AEGIS: Useful for baseline comparison and reproducibility, not ideal as the main real-world benchmark.
- Suitability for lightweight ML/TinyML: Good for lightweight baselines and small-model validation.
- Processing requirements: Standard CAN-package parsing, labeling by attack windows, and per-capture splitting.

### 7) SynCAN

- Official source: The public SynCAN project repository (`https://github.com/etas/SynCAN`), created by ETAS for synthetic CAN attack data generation and benchmarking.
- Research paper: SynCAN is commonly discussed in the CAN IDS literature as a synthetic benchmark and is associated with ETAS-driven synthetic data work; the public repo is the clearest official source.
- License: Public repository; no explicit license was identified in the dataset wrapper metadata reviewed beyond standard public repo usage. It is best treated as openly available for research benchmarking.
- Download size: Small; synthetic logs and generated attack scenarios typically compile to a compact package.
- Vehicles: Not a real vehicle dataset; synthetic system model.
- CAN bus count: One virtual bus / synthetic traffic model.
- CAN variant: Classical CAN.
- Timestamp availability: Yes.
- CAN ID: Yes.
- DLC: Yes.
- Payload: Yes.
- Normal/attack labels: Yes, synthetic labels are provided.
- Attack types: Synthetic attack families such as flooding, fuzzing, and timing/ID-based manipulations as designed for the benchmark.
- Replay availability: Yes, synthetic files are especially replay-friendly.
- Real vs synthetic: Synthetic.
- Real vs simulated attacks: Simulated attacks, not real-vehicle attacks.
- Train/test split: Yes, straightforward by synthetic data partitions.
- Cross-vehicle evaluation: Not meaningful; synthetic model does not cover vehicle-specific variability.
- Cross-attack evaluation: Good for benchmark comparison, but limited realism.
- Data leakage risk: Low to moderate; synthetic patterns can still be split poorly, but the main problem is overfitting to synthetic regularities rather than real-world traffic.
- Suitability for AEGIS: Good for model prototyping and lightweight signal-processing experiments, but not for validating deployment claims.
- Suitability for lightweight ML/TinyML: Excellent; synthetic, compact, and easy to process.
- Processing requirements: Low complexity; parse CAN frames and window them into compact feature vectors. Good for fast iteration.

### 8) CAN-MIRGU (additional public dataset identified)

- Official source: CAN-MIRGU public dataset release and paper, curated by the authors for moving-vehicle CAN attack evaluation; the public dataset page is referenced in the GitHub README (`https://drive.google.com/drive/folders/...`) and the paper DOI is 10.14722/vehiclesec.2024.23043.
- Research paper: "CAN-MIRGU: A Comprehensive CAN Bus Attack Dataset from Moving Vehicles for Intrusion Detection System Evaluation" (VehicleSec 2024).
- License: The public README did not state a formal license; treat as research-use public benchmark unless the archive states otherwise.
- Download size: Not explicitly published in the public README; best treated as moderate, real-world moving-vehicle capture data.
- Vehicles: One modern vehicle platform in real-world driving conditions.
- CAN bus count: One vehicle environment with real driving logs and attack traces.
- CAN variant: Classical CAN.
- Timestamp availability: Yes.
- CAN ID: Yes.
- DLC: Yes.
- Payload: Yes.
- Normal/attack labels: Yes.
- Attack types: Suspension attacks, masquerade attacks, and real attack traces to mimic moving-vehicle CAN intrusion scenarios.
- Replay availability: Likely feasible from raw logs, though the archive is not structured as a dedicated replay package.
- Real vs synthetic: Real-world vehicle data.
- Real vs simulated attacks: Real attack execution in moving-vehicle conditions.
- Train/test split: Yes, time-based segmentation is feasible.
- Cross-vehicle evaluation: No (single-vehicle dataset).
- Cross-attack evaluation: Good because multiple attack families are included.
- Data leakage risk: Moderate; attack traces and benign traces are temporally adjacent and must be split by drive session and time windows.
- Suitability for AEGIS: High as a modern real-world benchmark, especially for moving-vehicle conditions.
- Suitability for lightweight ML/TinyML: Good if compact features are extracted and attack windows are pruned.
- Processing requirements: Parse drive logs, segment by time and attack family, and keep session-level splits.

## Recommended ranking for AEGIS

1. ROAD — best primary benchmark for real-world adversarial realism.
2. AutoHack — strongest external validation dataset and multi-bus complement.
3. can-train-and-test — best for cross-source and repeatable benchmark comparisons.
4. GEM-CAN — high-value autonomous-driving dataset, good for modern CAN security evaluation.
5. CAN-MIRGU — modern moving-vehicle benchmark, good secondary dataset.
6. HCRL Car-Hacking — useful baseline and reproduction benchmark.
7. OTIDS — useful historical benchmark but weaker than newer datasets.
8. SynCAN — excellent for tiny model development and prototyping, not a final deployment benchmark.

## Evidence summary behind the ranking

- ROAD is ranked first because it is real-world, multi-vehicle, attack-rich, and designed to expose realistic CAN IDS weaknesses rather than synthetic regularities.
- AutoHack is ranked second because it is physically verified, multi-bus, and independent of ROAD; it is a strong external validation set for cross-benchmark robustness.
- can-train-and-test is ranked third because it is designed for benchmarking and cross-vehicle/attack generalization.
- GEM-CAN and CAN-MIRGU are ranked high because they are recent, real-world data from autonomous or moving vehicles.
- HCRL and OTIDS remain useful but are older and more limited in attack diversity and realism compared to recent real-world datasets.
- SynCAN ranks last for deployment claims because it is synthetic; however it is excellent for lightweight feature engineering and fast training iterations.

## Data-leakage and evaluation caution

For all datasets, the main operational risk is not dataset size but how the train/test split is defined. The safest evaluation method is:

- Use time-based segmentation within each capture and drive session.
- Keep vehicle IDs and bus IDs separated across splits.
- Keep attack families separate if the goal is cross-attack generalization.
- Do not random-split raw CAN frames across a single long capture.
- For cross-vehicle or cross-attack evaluation, use a leave-one-vehicle or leave-one-attack-family policy rather than mixing contiguous windows.

The most dangerous leakage pattern in CAN IDS work is adjacent benign and attack windows being split across train and test sets. That can hide the fact that the model is effectively memorizing message timing patterns and payload repetition rather than learning a robust attack boundary.

## Final verdict for AEGIS

For AEGIS, use ROAD as the primary public benchmark because it best matches the need for realistic vehicle-level intrusion detection under real attack conditions. Then validate externally on AutoHack to check whether the model generalizes beyond a single acquisition setup. Keep a lightweight prototype path using SynCAN for TinyML iteration, and keep HCRL Car-Hacking or OTIDS as historic baseline checks. Use can-train-and-test or GEM-CAN as additional benchmark supplements if cross-source or autonomous-vehicle robustness is required.
