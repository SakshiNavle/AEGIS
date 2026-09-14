# Phase 13A.4 experiment design

This is a future experiment specification only. Phase 13A does not train a model or produce metrics.

## Data and boundaries

Use only the canonical AutoHack interface-aware `both` training file. Use the earlier portion of each Interface stream for training and its deterministic trailing 20% for validation. Keep the supplied canonical test file untouched until final evaluation. No random row split is permitted, and no sequence may cross a source-file, Interface, or split boundary.

The current data does not verify vehicle, capture, session, or attack-session provenance. Results must therefore be reported as file/temporal separation under the verified dataset structure. An unseen-attack experiment is `NOT_SUPPORTED_BY_CURRENT_VERIFIED_PROVENANCE` unless future metadata proves attack-session disjointness.

## Feature and preprocessing contract

Use the existing seven causal features: `delta_t`, `rolling_mean_delta_t`, `rolling_std_delta_t`, `id_frequency_ratio`, `payload_hamming_distance`, `dlc`, and `arbitration_id`. Features must not include labels, attack type, file names, identifiers, split membership, or future timestamps.

Fit min/max statistics and the CAN-ID vocabulary on training rows only. Transform validation and test with those training-fitted values. Reset causal feature state at every source-file/Interface and split boundary. Construct the configured fixed-length sequences independently within each boundary.

## Evaluation

Report binary classification metrics selected before inspecting the test results, including precision, recall, F1, false-positive rate, and confusion matrices. Select any operating threshold using training/validation data only; never optimize a threshold on the untouched test file. Report metrics overall and by available attack label/category, while clearly stating that category overlap does not establish unseen-attack generalization.

The final test evaluation is a single untouched evaluation after preprocessing and threshold policy are frozen. Do not fabricate missing categories or provenance groups.

## Future robustness experiment

External ROAD evaluation may be conducted later as a separate robustness experiment. ROAD must not be mixed into the AutoHack train/validation/test split, and its schema, preprocessing compatibility, and label mapping must be audited before use. Any cross-dataset result must be labelled external robustness, not vehicle-disjoint AutoHack generalization.
