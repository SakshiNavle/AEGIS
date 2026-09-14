# Phase 8A: Final dataset acquisition decision

Date: 2026-09-14  
Dataset investigated: curated `can-train-and-test`

## Decision scope

This was a bounded, read-only acquisition investigation. No dataset file,
archive, repository clone, or large download was performed.

## Sources checked

### 1. DTU institutional repository

Checked:

https://orbit.dtu.dk/en/publications/can-train-and-test-a-curated-can-dataset-for-automotive-intrusion

The page is authoritative for the publication and confirms the high-level
dataset description:

- four vehicles from two manufacturers;
- dataset variants named `can-dataset`, `can-log`, `can-csv`, `can-ml`, and
  `can-train-and-test`;
- replayable `.log` files;
- labeled and unlabeled `.csv` files;
- nine unique attacks;
- equivalent attack captures across vehicle models;
- ML benchmarking and unseen-attack split intent.

The DTU publication page does not expose a downloadable sample, file listing,
archive manifest, direct per-file URL, or file sizes for the curated dataset.
The DTU datasets index was also not usable for this purpose: it returned HTTP
403 during this check.

### 2. DOI and DataCite metadata

Checked:

- https://doi.org/10.1016/j.cose.2024.103777
- https://doi.org/10.1109/vtc2023-fall60731.2023.10333756
- Crossref metadata API for DOI `10.1016/j.cose.2024.103777`
- DataCite search for `can-train-and-test`

The DOI metadata confirms the publication and references both `can-train-and-test`
and `can-dataset` as Bitbucket/DTU Data resources. It does not provide a
currently working downloadable dataset URL, file manifest, archive size, or
sample file. The DataCite search did not return a matching authoritative
curated dataset DOI.

### 3. Publication supplementary/repository links

The Crossref reference records contain only repository labels such as
“can-train-and-test — Bitbucket” and “can-train-and-test — DTU Data”; they do
not contain a resolvable file URL or a file size. The publisher text-mining
endpoint did not provide a usable supplementary dataset manifest during this
check.

### 4. Official author/research-group repository

No authoritative GitHub repository was found for the dataset or the authors
under the bounded searches performed. GitHub repository/code searches returned
unrelated projects or references, not a verified copy of the curated release.

### 5. Documented Bitbucket/Git repository

The documented/attempted path:

`https://bitbucket.org/lampe/can-train-and-test`

returned HTTP 404. The Bitbucket API lookup for the repository also returned
HTTP 404/410 responses. Additional obvious author/workspace paths checked,
including `lampe/can-dataset`, `brookelampe/can-train-and-test`, and
`dtu-sss/can-train-and-test`, were unavailable or returned 404. No valid
repository path could be established from authoritative metadata.

## Downloaded-file manifest

No files were downloaded.

| URL | Filename | Size | Authority | Normal/attack | Vehicle/capture |
|---|---|---:|---|---|---|
| None | None | 0 bytes | Not applicable | Not applicable | Not applicable |

## Safety assessment

There is no safe minimal acquisition path currently available because:

1. No authoritative per-file URL is exposed.
2. No representative CSV/log is directly downloadable from the inspected
   institutional or DOI metadata.
3. The documented Bitbucket path is inaccessible.
4. The available metadata does not establish the size of any fallback archive.
5. Downloading an unverified archive would violate the instruction to avoid a
   large/full download when minimal acquisition cannot be guaranteed.
6. Third-party mirrors were intentionally not used.

The Phase 7 blocker therefore remains unresolved. Exact schema inspection,
file-level provenance, and representative data-quality measurements cannot be
performed safely in this environment.

## Consequence for AEGIS

The curated can-train-and-test dataset remains a strong paper-level candidate
for cross-vehicle development, but it is not operationally available for the
next schema-dependent phase. Proceeding would require one of:

- an author-provided direct sample file;
- a restored official Bitbucket/DTU Data landing page with per-file metadata;
- an authoritative archive manifest with known size and safe partial retrieval.

Until then, do not claim verified columns, timestamp units, payload encoding,
vehicle/session fields, duplicate rates, or feature computability.

## Final recommendation

PRIMARY DATASET ACQUISITION BLOCKED

B) SWITCH PRIMARY DATASET
