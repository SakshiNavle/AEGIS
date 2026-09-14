# Phase 13A split manifest

This document defines the deterministic, auditable split used for the Phase 13A.4 experiments. It does not train a model and does not replace the Phase12C audit.

## Verified data boundary

The canonical files are the interface-aware `both` representation:

- `Interface/train/autohack_train_both_interface.csv`
- `Interface/test/autohack_test_both_interface.csv`

The schema is `Interface, Timestamp, Arbitration_ID, DLC, Data, Label`. The supplied Train/Test separation is verified, as is the presence of interface labels (C-CAN, P-CAN, and B-CAN where present in the files). The canonical files are hashed and described in `ml/artifacts/phase13a_split_manifest.json`.

The official test file remains untouched. The internal validation set is a deterministic trailing 20% of each Interface stream in the canonical training file. No row-level random split is used. Training is the earlier portion of each stream, and validation is later in time.

## Checks

The implementation streams CSV rows through the existing loader. It checks timestamp reversals and negative deltas per Interface, records the configured zero-delta policy, and keeps feature state and sequences separate at source-file/Interface boundaries. The split manifest also records causal future-window checks and the exact train-only preprocessing metadata.

Train, validation, and test row fingerprints are compared exhaustively in a temporary disk-backed SQLite database. The fingerprint includes Interface, Timestamp, Arbitration_ID, DLC, Data, and Label. No complete dataset is loaded into RAM.

The feature contract is the existing seven causal features: `delta_t`, `rolling_mean_delta_t`, `rolling_std_delta_t`, `id_frequency_ratio`, `payload_hamming_distance`, `dlc`, and `arbitration_id`. Labels, attack types, file names, identifiers, split names, and future timestamps are not features.

Preprocessing statistics and the ID vocabulary are fitted only on the training portion. Validation and test data are transformation-only inputs. Sequence state is reset at source-file/Interface and split boundaries.

## Not verified

The available README and canonical rows do not establish:

- vehicle-disjointness;
- capture-disjointness;
- session-disjointness; or
- attack-session/provenance disjointness.

Consequently, this evaluation must be described as **file/temporal separation under the verified dataset structure**, not vehicle-disjoint generalization. If attack labels occur in both files, that is recorded in the manifest; it is not evidence of unseen-attack generalization. Unseen-attack evaluation is `NOT_VERIFIED`.

## Recommendation

When the required files exist and the critical checks pass, the generated manifest reports `PASSED_WITH_LIMITATIONS` and recommends `READY_FOR_PHASE13A_4`. That recommendation is limited to the documented file/temporal protocol. Missing files or critical leakage cause a blocked/failed result. No model, metric, or claim of unseen-attack or vehicle-disjoint performance is produced here.
