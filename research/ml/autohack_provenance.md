# AutoHack provenance

Contract version: `AUTOHACK_GUARDCAN_DATA_CONTRACT_V1`

Dataset: AutoHack 2025, Zenodo DOI `10.5281/zenodo.19661007`.

Archive SHA-256:
`7b6112b98f775e2b66ca3aea229754ab1eb30b11020343a674f4c5723d570a51`

The archive and extracted files remain outside Git under the local
`AEGIS_AUTOHACK_ROOT`. Machine-specific absolute paths are recorded in the
ignored JSON audit artifact only.

| File role | Size bytes | Rows | Timestamp range | Interfaces |
|---|---:|---:|---|---|
| train combined interface CSV | 363,619,756 | 6,922,600 | 0.00104--1400.81922 s | C-CAN, P-CAN, B-CAN |
| test combined interface CSV | 141,421,105 | 2,711,497 | 0.00059--583.15111 s | C-CAN, P-CAN, B-CAN |

Both files were streamed completely. Each had 0 rejected rows and 0 exact
duplicate rows under the loader's canonical row-fingerprint audit. The full
machine-readable measurements are in the ignored artifact
`ml/artifacts/autohack_full_audit.json`; it contains no raw records.

The supplied train/test file boundary is reproducible as a file-disjoint
baseline, but vehicle, capture/session, and attack-disjoint provenance is NOT
VERIFIED. No cross-vehicle claim is permitted.
