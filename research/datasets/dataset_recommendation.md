# AEGIS dataset recommendation and ranking

> **Second-pass correction (2026-09-14):** The earlier ROAD recommendation was
> overstated. The authoritative ROAD paper and Zenodo record establish **one
> vehicle**, **12 ambient captures totaling about 3 hours**, and **33 attack
> captures totaling about 30 minutes**. They do not establish three vehicles or
> multiple CAN buses. See [second_pass_fact_check.md](./second_pass_fact_check.md)
> for the evidence-based replacement recommendation.

## Primary dataset: can-train-and-test (MEDIUM confidence)

Why can-train-and-test is now the strongest development candidate:

- The authoritative DTU record verifies four vehicles from two manufacturers.
- Equivalent attack captures are available across vehicle models, supporting vehicle-aware generalization experiments.
- It provides nine unique attacks, replayable logs, labeled and unlabeled CSV files, and explicit ML benchmarking.

Evidence and caveat:

- Archive byte size, exact schema, bus count, complete attack names, and license remain NOT VERIFIED.
- Verify those items before treating it as the final competition dataset.

Recommended AEGIS use:

- Train and tune the main model with vehicle-held-out and attack-held-out splits.
- Keep source vehicle and capture IDs in all intermediate data.
- Report exact archive and license evidence before publication.

## External validation dataset: AutoHack

Why AutoHack is the strongest external validation candidate:

- It is physically verified and has three named buses (C-CAN, P-CAN, and B-CAN), which makes it a strong independent benchmark from ROAD.
- It complements ROAD by checking whether learned patterns generalize to another acquisition setup and another bus structure.
- It is better than older HCRL benchmarks because it is explicitly designed around multi-bus CAN intrusion evaluation.

Evidence and caveat:

- The dataset is archived with artifact code and paper metadata, indicating it was designed for reproducible evaluation.
- Vehicle count and vehicle-disjointness are NOT VERIFIED, so it must not be described as cross-vehicle validation until the archive is inspected.

Recommended AEGIS use:

- Evaluate the final model on AutoHack without recalibrating thresholds or hyperparameters on that dataset.
- Report cross-dataset performance gap relative to ROAD.

## Stealth robustness supplement: ROAD

ROAD is retained for its strongest verified property: real-vehicle, physically
verified, increasingly stealthy attacks. It is **not** a cross-vehicle dataset.

## Backup dataset: HCRL Car-Hacking

Why HCRL Car-Hacking is the best backup:

- It is the classic public standard used in automotive IDS research.
- It is small, easy to parse, and reproducible.
- It is useful for sanity checks, baseline comparisons, and TinyML feasibility testing.

Evidence and caveat:

- It is older and more limited than ROAD and AutoHack, but remains a standard benchmark and easy fallback dataset.
- It should not be the primary proof set for AEGIS because it has lower realism and less attack diversity than modern benchmarks.

Recommended AEGIS use:

- Use it for lightweight regression checks and model sanity tests.
- Keep it as a fallback if ROAD or AutoHack are unavailable.

## Supporting datasets

- can-train-and-test: strong benchmark supplement for cross-source evaluation and train/test comparison.
- GEM-CAN: useful modern real-world autonomous-vehicle benchmark and good supplement for deployment realism.
- CAN-MIRGU: useful modern moving-vehicle dataset, strong as a secondary real-world check.
- SynCAN: strong for TinyML prototyping, but not for deployment claims.
- OTIDS: acceptable historical baseline, but weaker than newer public datasets.

## Final recommendation

For AEGIS, the recommended public benchmark strategy is:

1. Primary development benchmark: can-train-and-test
2. External validation: AutoHack
3. Stealth robustness supplement: ROAD
4. Fast prototype / TinyML loop: HCRL Car-Hacking or SynCAN
5. Secondary modern supplements: GEM-CAN, CAN-MIRGU

This ordering balances realism, attack diversity, external validity, and lightweight experimentation without overfitting to synthetic or legacy benchmarks.

## Ranking summary

| Rank | Dataset | Recommendation |
|---|---|---|
| 1 | can-train-and-test | Primary development benchmark |
| 2 | AutoHack | External validation |
| 3 | ROAD | Real-world/stealth robustness supplement |
| 4 | GEM-CAN | Modern real-world supplement |
| 5 | CAN-MIRGU | Real moving-vehicle supplement |
| 6 | HCRL Car-Hacking | Backup benchmark |
| 7 | OTIDS | Legacy baseline |
| 8 | SynCAN | TinyML prototype only |

## Evidence behind the choices

- The top-ranked datasets are the ones that combine realistic traffic, attack diversity, and actual attack execution rather than synthetic pattern generation.
- The chosen primary development dataset is can-train-and-test because four vehicles and two manufacturers are explicitly verified by the institutional source.
- ROAD is retained for real-world and stealth-attack robustness, not cross-vehicle generalization.
- The external validation choice is AutoHack because it is independent, multi-bus, and physically verified.
- The backup choice remains HCRL Car-Hacking because it is simple, reproducible, and historically relevant.
- SynCAN is kept for fast feature-engineering loops because it is synthetic and compact, which makes it ideal for TinyML iteration but not for final deployment-grade validation.
