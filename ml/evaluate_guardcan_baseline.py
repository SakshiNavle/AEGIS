"""Evaluate the Phase13A.4 baseline without retraining or threshold tuning.

This evaluator is designed for large CAN datasets:

- Loads the already-trained model only.
- Never retrains.
- Uses fixed threshold = 0.5.
- Streams feature sequences from CSV.
- Runs inference in small batches.
- Does not keep millions of sequences in Python lists.
- Stores prediction/label data using compact NumPy files/memmaps.
- Computes exact ROC-AUC and PR-AUC.
- Preserves original attack labels and interfaces for subgroup metrics.
- Prints progress so long evaluations do not appear frozen.
- Uses Windows-safe cleanup for NumPy mmap/file handles.
"""

from __future__ import annotations

import argparse
import gc
import json
import shutil
import tempfile
import time
from pathlib import Path

import numpy as np
from sklearn.metrics import (
    accuracy_score,
    average_precision_score,
    confusion_matrix,
    f1_score,
    precision_score,
    recall_score,
    roc_auc_score,
)
import tensorflow as tf

from .features import FEATURE_NAMES
from .phase13a import discover_canonical_files
from .train_guardcan_baseline import (
    OVERLAP_METADATA,
    WINDOW_LENGTH,
    iter_feature_sequences,
    load_frozen_manifest,
    _partition_records,
)


# ---------------------------------------------------------------------
# Fixed Phase13A.4 experiment settings
# ---------------------------------------------------------------------

BATCH_SIZE = 256
PROGRESS_INTERVAL = 10_000
THRESHOLD = 0.5

SCORE_DTYPE = np.float32
LABEL_DTYPE = np.int8


# ---------------------------------------------------------------------
# Metrics
# ---------------------------------------------------------------------


def binary_metrics_from_arrays(
    y_true: np.ndarray,
    scores: np.ndarray,
) -> dict:
    """Calculate the required binary classification metrics."""

    y = np.asarray(
        y_true,
        dtype=np.int32,
    )

    scores_array = np.asarray(
        scores,
        dtype=np.float32,
    )

    predictions = (
        scores_array >= THRESHOLD
    ).astype(np.int32)

    result = {
        "count": int(len(y)),
        "threshold": THRESHOLD,
        "accuracy": float(
            accuracy_score(
                y,
                predictions,
            )
        ),
        "precision": float(
            precision_score(
                y,
                predictions,
                zero_division=0,
            )
        ),
        "recall": float(
            recall_score(
                y,
                predictions,
                zero_division=0,
            )
        ),
        "f1": float(
            f1_score(
                y,
                predictions,
                zero_division=0,
            )
        ),
        "macro_f1": float(
            f1_score(
                y,
                predictions,
                average="macro",
                zero_division=0,
            )
        ),
        "confusion_matrix": confusion_matrix(
            y,
            predictions,
            labels=[0, 1],
        ).tolist(),
    }

    # -------------------------------------------------------------
    # ROC-AUC
    # -------------------------------------------------------------

    try:
        result["roc_auc"] = float(
            roc_auc_score(
                y,
                scores_array,
            )
        )
    except ValueError as exc:
        result["roc_auc"] = None
        result["roc_auc_reason"] = str(exc)

    # -------------------------------------------------------------
    # PR-AUC / Average Precision
    # -------------------------------------------------------------

    try:
        result["pr_auc"] = float(
            average_precision_score(
                y,
                scores_array,
            )
        )
    except ValueError as exc:
        result["pr_auc"] = None
        result["pr_auc_reason"] = str(exc)

    return result


def binary_metrics(
    y_true: list[int] | np.ndarray,
    scores: list[float] | np.ndarray,
) -> dict:
    """Backward-compatible wrapper used by Phase13A.4 tests."""

    return binary_metrics_from_arrays(
        np.asarray(y_true),
        np.asarray(scores),
    )


def subgroup_metrics_from_arrays(
    y_true: np.ndarray,
    scores: np.ndarray,
    groups: np.ndarray,
) -> dict:
    """Calculate metrics for each subgroup."""

    output = {}

    unique_groups = np.unique(
        groups
    )

    for group in unique_groups:

        mask = groups == group

        if not np.any(mask):
            continue

        group_name = str(group)

        output[group_name] = (
            binary_metrics_from_arrays(
                y_true[mask],
                scores[mask],
            )
        )

    return output


# ---------------------------------------------------------------------
# Progress reporting
# ---------------------------------------------------------------------


def print_progress(
    split_name: str,
    evaluated: int,
    total_hint: int | None,
    start_time: float,
) -> None:
    """Print evaluation progress."""

    elapsed = max(
        time.perf_counter()
        - start_time,
        1e-9,
    )

    rate = evaluated / elapsed

    if total_hint:
        percentage = (
            100.0
            * evaluated
            / total_hint
        )

        print(
            f"[{split_name}] "
            f"{evaluated:,} sequences "
            f"/ ~{total_hint:,} "
            f"({percentage:.1f}%) | "
            f"{rate:,.0f} seq/s | "
            f"elapsed {elapsed / 60:.1f} min",
            flush=True,
        )
    else:
        print(
            f"[{split_name}] "
            f"{evaluated:,} sequences | "
            f"{rate:,.0f} seq/s | "
            f"elapsed {elapsed / 60:.1f} min",
            flush=True,
        )


# ---------------------------------------------------------------------
# Streaming evaluation
# ---------------------------------------------------------------------


def evaluate_split(
    model,
    record_iterator,
    split_name: str,
    work_dir: Path,
    total_hint: int | None = None,
):
    """Evaluate one split with bounded batch inference.

    Feature sequences are never accumulated for the entire dataset.

    Prediction scores and labels are written to disk-backed arrays.
    Attack/interface metadata is kept separately.

    Returns:
        labels memmap
        scores memmap
        attacks ndarray
        interfaces ndarray
    """

    start_time = time.perf_counter()

    print(
        f"\n[{split_name}] "
        "Starting streaming evaluation...",
        flush=True,
    )

    chunk_dir = (
        work_dir
        / f"{split_name}_chunks"
    )

    chunk_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    score_chunks: list[Path] = []
    label_chunks: list[Path] = []
    attack_chunks: list[Path] = []
    interface_chunks: list[Path] = []

    batch_values = []
    batch_targets = []
    batch_labels = []
    batch_interfaces = []

    evaluated = 0
    chunk_index = 0

    def flush_batch() -> None:
        nonlocal evaluated
        nonlocal chunk_index

        if not batch_values:
            return

        # ---------------------------------------------------------
        # Convert only the current batch.
        # ---------------------------------------------------------

        x_batch = np.asarray(
            batch_values,
            dtype=np.float32,
        )

        # ---------------------------------------------------------
        # Batched model inference.
        # ---------------------------------------------------------

        predictions = (
            model(
                x_batch,
                training=False,
            )
            .numpy()
            .reshape(-1)
            .astype(
                SCORE_DTYPE,
                copy=False,
            )
        )

        targets = np.asarray(
            batch_targets,
            dtype=LABEL_DTYPE,
        )

        attack_labels = np.asarray(
            batch_labels,
            dtype="U64",
        )

        interfaces = np.asarray(
            batch_interfaces,
            dtype="U16",
        )

        # ---------------------------------------------------------
        # Write current batch to disk.
        # ---------------------------------------------------------

        score_path = (
            chunk_dir
            / f"scores_{chunk_index:08d}.npy"
        )

        label_path = (
            chunk_dir
            / f"labels_{chunk_index:08d}.npy"
        )

        attack_path = (
            chunk_dir
            / f"attacks_{chunk_index:08d}.npy"
        )

        interface_path = (
            chunk_dir
            / f"interfaces_{chunk_index:08d}.npy"
        )

        np.save(
            score_path,
            predictions,
        )

        np.save(
            label_path,
            targets,
        )

        np.save(
            attack_path,
            attack_labels,
        )

        np.save(
            interface_path,
            interfaces,
        )

        score_chunks.append(
            score_path
        )

        label_chunks.append(
            label_path
        )

        attack_chunks.append(
            attack_path
        )

        interface_chunks.append(
            interface_path
        )

        evaluated += len(
            batch_values
        )

        chunk_index += 1

        # ---------------------------------------------------------
        # Progress.
        # ---------------------------------------------------------

        if (
            evaluated
            % PROGRESS_INTERVAL
            < BATCH_SIZE
            or evaluated == BATCH_SIZE
        ):
            print_progress(
                split_name,
                evaluated,
                total_hint,
                start_time,
            )

        # ---------------------------------------------------------
        # Release current batch.
        # ---------------------------------------------------------

        batch_values.clear()
        batch_targets.clear()
        batch_labels.clear()
        batch_interfaces.clear()

        del x_batch
        del predictions
        del targets
        del attack_labels
        del interfaces

    # -----------------------------------------------------------------
    # Stream feature sequences.
    # -----------------------------------------------------------------

    for (
        values,
        target,
        original,
        interface,
    ) in record_iterator:

        batch_values.append(
            values
        )

        batch_targets.append(
            target
        )

        batch_labels.append(
            original
        )

        batch_interfaces.append(
            interface
        )

        if len(batch_values) >= BATCH_SIZE:
            flush_batch()

    # Final partial batch.
    flush_batch()

    if evaluated == 0:
        raise RuntimeError(
            f"[{split_name}] "
            "No sequences were produced."
        )

    elapsed = (
        time.perf_counter()
        - start_time
    )

    print(
        f"\n[{split_name}] "
        f"Feature extraction + inference "
        f"complete: {evaluated:,} sequences "
        f"in {elapsed / 60:.2f} minutes "
        f"({evaluated / max(elapsed, 1e-9):,.0f} seq/s)",
        flush=True,
    )

    # -----------------------------------------------------------------
    # Combine chunks into disk-backed arrays.
    # -----------------------------------------------------------------

    final_scores_path = (
        work_dir
        / f"{split_name}_scores.dat"
    )

    final_labels_path = (
        work_dir
        / f"{split_name}_labels.dat"
    )

    final_attacks_path = (
        work_dir
        / f"{split_name}_attacks.npy"
    )

    final_interfaces_path = (
        work_dir
        / f"{split_name}_interfaces.npy"
    )

    scores = np.memmap(
        final_scores_path,
        dtype=SCORE_DTYPE,
        mode="w+",
        shape=(evaluated,),
    )

    labels = np.memmap(
        final_labels_path,
        dtype=LABEL_DTYPE,
        mode="w+",
        shape=(evaluated,),
    )

    attacks = np.empty(
        evaluated,
        dtype="U64",
    )

    interfaces = np.empty(
        evaluated,
        dtype="U16",
    )

    position = 0

    # -----------------------------------------------------------------
    # Read chunks.
    # -----------------------------------------------------------------

    for (
        score_path,
        label_path,
        attack_path,
        interface_path,
    ) in zip(
        score_chunks,
        label_chunks,
        attack_chunks,
        interface_chunks,
    ):

        chunk_scores = np.load(
            score_path,
            mmap_mode="r",
        )

        chunk_labels = np.load(
            label_path,
            mmap_mode="r",
        )

        chunk_attacks = np.load(
            attack_path,
            mmap_mode="r",
        )

        chunk_interfaces = np.load(
            interface_path,
            mmap_mode="r",
        )

        size = len(
            chunk_scores
        )

        scores[
            position : position + size
        ] = chunk_scores

        labels[
            position : position + size
        ] = chunk_labels

        attacks[
            position : position + size
        ] = chunk_attacks

        interfaces[
            position : position + size
        ] = chunk_interfaces

        position += size

        # ---------------------------------------------------------
        # IMPORTANT:
        # Explicitly release mmap-backed objects before deleting
        # the source files on Windows.
        # ---------------------------------------------------------

        del chunk_scores
        del chunk_labels
        del chunk_attacks
        del chunk_interfaces

    scores.flush()
    labels.flush()

    # -----------------------------------------------------------------
    # Save subgroup metadata.
    # -----------------------------------------------------------------

    np.save(
        final_attacks_path,
        attacks,
    )

    np.save(
        final_interfaces_path,
        interfaces,
    )

    # -----------------------------------------------------------------
    # Release objects before cleanup.
    # -----------------------------------------------------------------

    del attacks
    del interfaces

    gc.collect()

    # -----------------------------------------------------------------
    # Delete temporary chunk files.
    #
    # WinError 32 is handled deliberately. If Windows still has a
    # transient file handle open, the final workspace cleanup will
    # remove the directory when possible.
    # -----------------------------------------------------------------

    for path in (
        score_chunks
        + label_chunks
        + attack_chunks
        + interface_chunks
    ):

        try:
            path.unlink()

        except PermissionError:
            print(
                f"[{split_name}] "
                f"Warning: Windows still holds "
                f"{path.name}; leaving it for "
                f"workspace cleanup.",
                flush=True,
            )

        except FileNotFoundError:
            pass

    try:
        chunk_dir.rmdir()

    except OSError:
        pass

    gc.collect()

    print(
        f"[{split_name}] "
        "Temporary chunks cleaned.",
        flush=True,
    )

    # -----------------------------------------------------------------
    # Load subgroup metadata normally.
    #
    # These are small relative to the feature/prediction data and
    # normal arrays avoid keeping mmap handles open.
    # -----------------------------------------------------------------

    attacks_result = np.load(
        final_attacks_path,
        allow_pickle=False,
    )

    interfaces_result = np.load(
        final_interfaces_path,
        allow_pickle=False,
    )

    gc.collect()

    return (
        labels,
        scores,
        attacks_result,
        interfaces_result,
    )


# ---------------------------------------------------------------------
# Main evaluation
# ---------------------------------------------------------------------


def evaluate(
    output_dir: str | Path = "ml/artifacts/phase13a4",
) -> dict:
    """Run Phase13A.4 evaluation using the existing trained model."""

    output = Path(
        output_dir
    )

    if not output.exists():
        raise FileNotFoundError(
            f"Output directory does not exist: "
            f"{output}"
        )

    # -----------------------------------------------------------------
    # Frozen manifest
    # -----------------------------------------------------------------

    print(
        "Loading frozen Phase13A manifest...",
        flush=True,
    )

    manifest = (
        load_frozen_manifest()
    )

    print(
        "Phase13A manifest status: "
        f"{manifest.get('status', 'UNKNOWN')}",
        flush=True,
    )

    # -----------------------------------------------------------------
    # Canonical dataset
    # -----------------------------------------------------------------

    train_path, test_path = (
        discover_canonical_files()
    )

    print(
        f"Training file: {train_path}",
        flush=True,
    )

    print(
        f"Test file:     {test_path}",
        flush=True,
    )

    # -----------------------------------------------------------------
    # Frozen preprocessing
    # -----------------------------------------------------------------

    preprocessing_path = (
        output
        / "preprocessing.json"
    )

    if not preprocessing_path.exists():
        raise FileNotFoundError(
            "Missing preprocessing artifact: "
            f"{preprocessing_path}"
        )

    preprocessing = json.loads(
        preprocessing_path.read_text(
            encoding="utf-8"
        )
    )

    from .preprocess import (
        FittedPreprocessor,
    )

    from .config import (
        PreprocessConfig,
    )

    fitted = FittedPreprocessor(
        PreprocessConfig(
            **preprocessing["config"]
        ),
        tuple(
            preprocessing[
                "id_vocabulary"
            ]
        ),
        preprocessing[
            "feature_min"
        ],
        preprocessing[
            "feature_max"
        ],
        tuple(
            preprocessing[
                "training_files"
            ]
        ),
        preprocessing[
            "version"
        ],
    )

    # -----------------------------------------------------------------
    # Existing model
    # -----------------------------------------------------------------

    model_path = (
        output
        / "model.keras"
    )

    if not model_path.exists():
        raise FileNotFoundError(
            f"Missing trained model: "
            f"{model_path}"
        )

    print(
        "\nLoading existing trained model...",
        flush=True,
    )

    model = tf.keras.models.load_model(
        model_path
    )

    print(
        f"Model loaded successfully: "
        f"{model.count_params():,} parameters",
        flush=True,
    )

    print(
        f"Input shape: "
        f"{model.input_shape}",
        flush=True,
    )

    print(
        f"Fixed threshold: "
        f"{THRESHOLD}",
        flush=True,
    )

    # -----------------------------------------------------------------
    # Temporary workspace
    # -----------------------------------------------------------------

    work_dir = Path(
        tempfile.mkdtemp(
            prefix=(
                "aegis_phase13a4_eval_"
            )
        )
    )

    print(
        f"Evaluation workspace: "
        f"{work_dir}",
        flush=True,
    )

    try:

        cutoffs = (
            manifest[
                "files"
            ][
                "training_validation_cutoffs"
            ]
        )

        results = {}
        attack_results = {}
        interface_results = {}

        # =============================================================
        # VALIDATION
        # =============================================================

        print(
            "\n"
            + "=" * 70,
            flush=True,
        )

        print(
            "VALIDATION EVALUATION",
            flush=True,
        )

        print(
            "=" * 70,
            flush=True,
        )

        validation_iterator = (
            iter_feature_sequences(
                _partition_records(
                    train_path,
                    cutoffs,
                    "validation",
                ),
                fitted,
                "validation",
            )
        )

        (
            val_y,
            val_scores,
            val_attacks,
            val_interfaces,
        ) = evaluate_split(
            model,
            validation_iterator,
            "validation",
            work_dir,
        )

        print(
            "\n[validation] "
            "Calculating metrics...",
            flush=True,
        )

        results["validation"] = (
            binary_metrics_from_arrays(
                val_y,
                val_scores,
            )
        )

        attack_results[
            "validation"
        ] = subgroup_metrics_from_arrays(
            val_y,
            val_scores,
            val_attacks,
        )

        interface_results[
            "validation"
        ] = subgroup_metrics_from_arrays(
            val_y,
            val_scores,
            val_interfaces,
        )

        print(
            "[validation] "
            "Metrics calculated.",
            flush=True,
        )

        # -------------------------------------------------------------
        # Release validation metadata before test.
        # -------------------------------------------------------------

        del val_attacks
        del val_interfaces

        gc.collect()

        # =============================================================
        # TEST
        # =============================================================

        print(
            "\n"
            + "=" * 70,
            flush=True,
        )

        print(
            "CANONICAL TEST EVALUATION",
            flush=True,
        )

        print(
            "=" * 70,
            flush=True,
        )

        from .load_autohack import (
            iter_records,
        )

        test_iterator = (
            iter_feature_sequences(
                iter_records(
                    test_path
                ),
                fitted,
                "test",
            )
        )

        (
            test_y,
            test_scores,
            test_attacks,
            test_interfaces,
        ) = evaluate_split(
            model,
            test_iterator,
            "test",
            work_dir,
        )

        print(
            "\n[test] "
            "Calculating metrics...",
            flush=True,
        )

        results["test"] = (
            binary_metrics_from_arrays(
                test_y,
                test_scores,
            )
        )

        attack_results[
            "test"
        ] = subgroup_metrics_from_arrays(
            test_y,
            test_scores,
            test_attacks,
        )

        interface_results[
            "test"
        ] = subgroup_metrics_from_arrays(
            test_y,
            test_scores,
            test_interfaces,
        )

        print(
            "[test] "
            "Metrics calculated.",
            flush=True,
        )

        # =============================================================
        # METADATA
        # =============================================================

        metadata = {
            **OVERLAP_METADATA,
            "feature_names": list(
                FEATURE_NAMES
            ),
            "window_length": (
                WINDOW_LENGTH
            ),
            "threshold_policy": (
                "fixed_default_0.5; "
                "no test threshold tuning"
            ),
            "evaluation_batch_size": (
                BATCH_SIZE
            ),
            "evaluation_storage": (
                "disk_backed_numpy_memmap"
            ),
            "evaluation_streaming": True,
            "retrained": False,
            "validation": (
                results["validation"]
            ),
            "test": results["test"],
        }

        # -------------------------------------------------------------
        # Human-approved dataset limitation.
        #
        # These values are deliberately preserved and disclosed.
        # -------------------------------------------------------------

        metadata[
            "human_approved_dataset_limitation"
        ] = True

        metadata[
            "accepted_exact_train_test_overlap_count"
        ] = 85

        metadata[
            "overlap_handling"
        ] = (
            "preserved_and_disclosed"
        )

        metadata[
            "phase13a_manifest_status"
        ] = manifest.get(
            "status",
            "UNKNOWN",
        )

        metadata[
            "phase13a_recommendation"
        ] = manifest.get(
            "recommendation",
            "BLOCKED_PENDING_DATA_VALIDATION",
        )

        # =============================================================
        # ARTIFACTS
        # =============================================================

        print(
            "\nWriting evaluation artifacts...",
            flush=True,
        )

        metrics_path = (
            output
            / "metrics.json"
        )

        attack_path = (
            output
            / "per_attack_metrics.json"
        )

        interface_path = (
            output
            / "per_interface_metrics.json"
        )

        metrics_path.write_text(
            json.dumps(
                metadata,
                indent=2,
            ),
            encoding="utf-8",
        )

        attack_path.write_text(
            json.dumps(
                {
                    **OVERLAP_METADATA,
                    "human_approved_dataset_limitation": True,
                    "accepted_exact_train_test_overlap_count": 85,
                    "overlap_handling": (
                        "preserved_and_disclosed"
                    ),
                    "validation": (
                        attack_results[
                            "validation"
                        ]
                    ),
                    "test": (
                        attack_results[
                            "test"
                        ]
                    ),
                },
                indent=2,
            ),
            encoding="utf-8",
        )

        interface_path.write_text(
            json.dumps(
                {
                    **OVERLAP_METADATA,
                    "human_approved_dataset_limitation": True,
                    "accepted_exact_train_test_overlap_count": 85,
                    "overlap_handling": (
                        "preserved_and_disclosed"
                    ),
                    "validation": (
                        interface_results[
                            "validation"
                        ]
                    ),
                    "test": (
                        interface_results[
                            "test"
                        ]
                    ),
                },
                indent=2,
            ),
            encoding="utf-8",
        )

        print(
            "\n"
            + "=" * 70,
            flush=True,
        )

        print(
            "EVALUATION COMPLETED SUCCESSFULLY",
            flush=True,
        )

        print(
            "=" * 70,
            flush=True,
        )

        print(
            json.dumps(
                metadata,
                indent=2,
            ),
            flush=True,
        )

        return metadata

    finally:

        # -------------------------------------------------------------
        # Release TensorFlow model resources.
        # -------------------------------------------------------------

        gc.collect()

        # -------------------------------------------------------------
        # Windows-safe workspace cleanup.
        # -------------------------------------------------------------

        print(
            "\nCleaning temporary "
            "evaluation workspace...",
            flush=True,
        )

        try:
            shutil.rmtree(
                work_dir,
                ignore_errors=True,
            )

        except Exception as exc:

            print(
                f"Warning: cleanup failed: "
                f"{exc}",
                flush=True,
            )


# ---------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------


def main() -> int:

    parser = argparse.ArgumentParser(
        description=(
            "Evaluate the Phase13A.4 "
            "GuardCAN baseline without "
            "retraining."
        )
    )

    parser.add_argument(
        "--output-dir",
        default=(
            "ml/artifacts/phase13a4"
        ),
        help=(
            "Directory containing "
            "model.keras and preprocessing "
            "artifacts."
        ),
    )

    args = parser.parse_args()

    evaluate(
        args.output_dir
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(
        main()
    )