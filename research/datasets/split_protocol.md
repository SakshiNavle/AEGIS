# Phase 6: scientifically defensible split protocol

## Status and evidence boundary

The can-train-and-test paper verifies four vehicles from two manufacturers,
equivalent attack captures across vehicle models, nine unique attacks, and
replayable `.log` plus labeled/unlabeled `.csv` variants. Exact filenames,
vehicle/session fields, published train/test filenames, and archive schema are
**NOT VERIFIED** because the official release endpoint inspected in this pass
was unavailable. The protocol below is therefore conditional on confirming
those identifiers in a representative sample.

## Required unit of splitting

The indivisible split unit must be a complete capture/session, not an
individual CAN row or an overlapping window. A derived row must retain:

- `source_file`
- `vehicle_id`
- `vehicle_model` and manufacturer, if provided
- `capture_id` or drive/session identifier
- `attack_type`
- original label

If any identifier is absent, create a deterministic `source_file`/capture
identifier and mark vehicle-aware evaluation unavailable rather than guessing
vehicle identity.

Never randomly split rows from the same capture. Never let overlapping or
adjacent windows cross a split boundary.

## A. Normal development/training

Purpose: feature engineering, parser validation, threshold/model development,
and ordinary in-domain development.

Protocol:

1. Reserve a final development holdout by complete capture/session.
2. Within the remaining development pool, use chronological capture-level
   folds, not row-level random folds.
3. Keep at least one benign capture from every training vehicle only if the
   final evaluation is explicitly in-domain.
4. Fit all normalization, frequency baselines, and threshold statistics on the
   training portion only.
5. Keep attack labels out of unsupervised-normal training; use them only for
   evaluation or supervised development.
6. Report the exact vehicle/session composition of every fold.

This protocol is possible only if capture/session identifiers can be recovered.
Their availability is currently **NOT VERIFIED**.

## B. Vehicle-held-out evaluation

Purpose: test generalization to an unseen vehicle and reduce vehicle-specific
ID/timing memorization.

Preferred protocol:

1. Group all captures by vehicle identity.
2. Hold out one complete vehicle for final evaluation.
3. Train/develop on the remaining vehicles.
4. If four vehicles are confirmed individually, use leave-one-vehicle-out
   (four folds) or a pre-registered 2/1/1 design: two vehicles for training,
   one for development, one for final test.
5. Keep manufacturer balance visible. A test vehicle from a manufacturer not
   represented in training is a stronger manufacturer-transfer test.
6. Do not tune thresholds on the held-out vehicle.

Feasibility: **Yes in principle**, because the institutional source verifies four
vehicles from two manufacturers. Exact vehicle identifiers and capture mapping
are **NOT VERIFIED**.

## C. Attack-held-out evaluation

Purpose: test detection of attack types not used in development.

Preferred protocol:

1. Enumerate the exact nine attack labels from the release; the complete list
   is currently **NOT VERIFIED**.
2. Define attack families before looking at test results.
3. Hold out one or more complete attack types, including every capture and
   every vehicle instance of each held-out type.
4. Train on normal data plus the selected known attack classes only if the
   model is supervised; for anomaly detection, train on benign captures only.
5. Keep at least one attack type represented in development for threshold
   calibration and reserve the held-out types for final evaluation.
6. Do not split individual attack intervals from the same capture across
   train and test.

Feasibility: **Yes in principle**, because the paper explicitly describes
selecting attacks for training and saving the remainder for unseen-attack
testing. Exact attack filenames, labels, and interval boundaries are **NOT
VERIFIED**.

## D. Cross-dataset external validation

Purpose: evaluate transfer to a different collection protocol and vehicle
domain without contaminating development.

Protocol:

1. Freeze feature definitions, parsing rules, normalization, threshold policy,
   and model parameters using only can-train-and-test development data.
2. Do not fit a new scaler, frequency baseline, payload dictionary, or
   threshold on the external dataset.
3. Map only semantically equivalent fields. If a field is unavailable, report
   the feature as unavailable rather than imputing it silently.
4. Evaluate by complete external captures and report results separately by
   dataset, bus/interface, attack type, and normal traffic.
5. Keep the external dataset completely untouched until the final evaluation.
6. For AutoHack or ROAD, report cross-dataset transfer as a domain-shift
   experiment, not as evidence of cross-vehicle generalization unless vehicle
   disjointness is independently verified.

## Leakage and duplication controls

Before splitting, quantify:

- exact duplicate rows;
- duplicate `(CAN ID, DLC, payload)` records with repeated timestamps;
- repeated windows generated from the same raw rows;
- near-duplicate adjacent payloads for the same ID;
- identical attack traces repeated across vehicle files;
- same capture/session appearing under both labeled and unlabeled exports.

Exact duplicate rate and near-duplicate rate for can-train-and-test are
**NOT VERIFIED**. The audit must hash raw records and capture-level windows
before any model-ready dataset is produced.

## Missing and malformed data policy

Until the sample is inspected, the correct policy is conservative:

- preserve original rows and write parser errors to a separate report;
- do not coerce malformed timestamps, IDs, DLCs, or payloads to zero;
- reject rows with invalid required fields from feature computation, while
  retaining counts and source locations;
- require `DLC == number of valid payload bytes` when payload bytes are present;
- treat truncated payloads, extra bytes, duplicate timestamps, and non-monotonic
  timestamps as explicit quality findings;
- never silently fill missing labels;
- do not cross file/session boundaries when computing temporal features.

The actual malformed-record patterns are **NOT VERIFIED**.

## Feature-specific split requirements

- `delta_t`, rolling mean, and rolling standard deviation must reset at every
  capture/session boundary.
- `ID_frequency_ratio` must be computed with training-only frequency state when
  evaluating held-out vehicles, attacks, or datasets.
- `payload_hamming_distance` must compare canonical payload bytes for the same
  CAN ID and respect DLC; it must not use a global payload dictionary fitted on
  test data.
- Any feature normalization must be fitted on the training partition only.

## Pre-registration checklist

Before training:

1. Obtain one benign, one attack, and one different-vehicle representative
   file without downloading the full archive.
2. Verify exact headers, timestamp units, ID/DLC/payload encoding, labels,
   vehicle IDs, and capture IDs.
3. Publish the capture-level split manifest.
4. Record duplicate/malformed-row statistics.
5. Freeze the split manifest and feature parser.

No accuracy or model-performance claim is justified until this checklist is
complete.
