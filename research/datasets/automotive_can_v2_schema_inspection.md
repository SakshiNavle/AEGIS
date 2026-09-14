# Automotive CAN Bus Intrusion Dataset v2 Audit

## A. Official source verification

The candidate was described as an “Automotive CAN Bus Intrusion Dataset v2”
associated with TU Eindhoven and allegedly containing Opel Astra, Renault
Clio, and a prototype vehicle data. A bounded search was performed against:

* [4TU.ResearchData](https://data.4tu.nl/)
* DataCite DOI metadata
* Crossref publication metadata
* Semantic Scholar publication metadata
* Public institutional and repository search results

No authoritative record whose title, DOI, file listing, or metadata establishes
this exact dataset identity was found. In particular:

| Requested item | Status | Finding |
|---|---|---|
| Official institution/source | NOT VERIFIED | TU Eindhoven association was not tied to a dataset record. |
| Official landing page | NOT VERIFIED | No authoritative landing page for the exact v2 name was located. |
| Official download endpoint | NOT VERIFIED | No official file URL or API endpoint was located. |
| License | NOT VERIFIED | No dataset license record was located. |
| Publication/date | NOT VERIFIED | No publication could be confidently matched to this exact dataset. |
| Exact dataset size | NOT VERIFIED | No official file manifest or size was available. |
| Representative sample | NOT VERIFIED | No official sample or metadata endpoint was available. |

The reported vehicle names and attack types were therefore not accepted as
verified facts. They may describe a real dataset under another title, but the
candidate cannot be audited reproducibly without an authoritative identifier.
No access control was bypassed, no unofficial mirror was used, and no data
file was downloaded.

## B. Data structure

No row-level file was obtained. Every requested field remains **NOT VERIFIED**:

| Field or metadata | Status | Evidence |
|---|---|---|
| Timestamp and units | NOT VERIFIED | No official schema or row sample |
| Timestamp precision | NOT VERIFIED | No file |
| CAN ID representation | NOT VERIFIED | No file |
| DLC representation | NOT VERIFIED | No file |
| Payload/data bytes | NOT VERIFIED | No file |
| Normal/attack label | NOT VERIFIED | No file or authoritative schema |
| Attack type | NOT VERIFIED | The reported Diagnostic, Fuzzing, Replay, Suspension, and DoS list was not tied to an official record |
| Vehicle identity | NOT VERIFIED | The Opel Astra, Renault Clio, and prototype claims were not institutionally verified |
| Capture/session identity | NOT VERIFIED | No manifest or metadata |
| Bus/interface identity | NOT VERIFIED | No schema |
| Train/test membership | NOT VERIFIED | No official split |
| Additional metadata | NOT VERIFIED | No files |

## C. Data quality

Because no authoritative representative file could be acquired, no data-quality
measurement is possible:

| Measurement | Result |
|---|---|
| Record counts | NOT VERIFIED |
| Missing fields | NOT VERIFIED |
| Malformed rows | NOT VERIFIED |
| Invalid CAN IDs | NOT VERIFIED |
| DLC/payload consistency | NOT VERIFIED |
| Timestamp reversals | NOT VERIFIED |
| Duplicate rows | NOT VERIFIED |
| Near-duplicate windows | NOT VERIFIED |
| Empty payloads | NOT VERIFIED |
| Class counts | NOT VERIFIED |
| Capture/session boundaries | NOT VERIFIED |

No claims about data quality, class balance, or storage requirements should be
made from the unverified descriptions.

## D. AEGIS feature feasibility

The feature statuses below describe conditional computability from a
conventional CAN log, not verified properties of this candidate.

| Feature | Status | Reason |
|---|---|---|
| `delta_t` | NEEDS TRANSFORMATION | Feasible only after an authoritative timestamp field, unit, precision, and capture ordering are verified. |
| `rolling_mean_delta_t` | NEEDS TRANSFORMATION | Requires verified timestamps and capture/session boundaries with state reset. |
| `rolling_std_delta_t` | NEEDS TRANSFORMATION | Requires the same timestamp, window, and boundary controls. |
| `ID_frequency_ratio` | NEEDS TRANSFORMATION | Requires verified CAN-ID parsing and a declared capture-local frequency baseline. |
| `payload_hamming_distance` | NEEDS TRANSFORMATION | Requires verified payload bytes, DLC behavior, padding policy, and frame alignment. |
| Sequence/replay indicators | NOT FEASIBLE | No verified ordered records, capture identity, or replay annotation is available. |
| Vehicle/capture/session grouping | NOT FEASIBLE | No verified vehicle, capture, or session identifiers are available. |

## E. Leakage audit

The dataset cannot currently support a defensible leakage audit. The following
risks are unresolved rather than absent:

1. Attack filenames may encode attack class.
2. Directory names may encode normal/attack labels or vehicle identity.
3. Capture IDs may be embedded in filenames or preprocessing output.
4. Vehicle IDs may be present in metadata or file naming.
5. Labels or attack classes may be copied into row-level features.
6. Train/test membership may be encoded by directories or pre-generated files.
7. Random row splitting could mix adjacent frames from the same capture and
   would be invalid until capture boundaries are known.
8. Replayed sequences or duplicate windows cannot be checked without records.

The candidate therefore cannot presently demonstrate capture-level,
attack-held-out, vehicle-disjoint, or temporal evaluation.

## F. AEGIS experiment design

The desired experiment would require:

* training on complete selected captures;
* validation on separate complete captures;
* a held-out attack condition for test;
* vehicle-disjoint grouping where multiple verified vehicles exist;
* absolute filenames, labels, vehicle IDs, and preprocessing artifacts excluded
  from model inputs;
* AutoHack and ROAD retained as external datasets, with independently
  documented schema adapters.

Automotive CAN v2 cannot be assigned this role because its official source,
files, schema, grouping variables, and license are all unverified. A
vehicle-count claim alone is not sufficient evidence for a primary ML
experiment.

## G. Suitability for AEGIS

| Criterion | Rating | Confidence and assessment |
|---|---|---|
| Accessibility | LOW | No authoritative landing page or verified download endpoint was found. |
| Schema suitability | LOW | No row-level schema or sample was available. |
| Feature feasibility | LOW | All feature computations depend on unverified fields and boundaries. |
| Attack diversity | LOW | The reported attack list is not verified against an official record. |
| Vehicle diversity | LOW | The reported three vehicle contexts are not verified. |
| Leakage risk | HIGH | Filenames, labels, capture identity, and split provenance cannot be audited. |
| Reproducibility | LOW | No stable official identifier, license, manifest, checksum, or split is established. |

The exact blocking reason is evidence provenance: the candidate dataset could
not be identified through an authoritative institutional or publication record,
so neither acquisition nor schema/split validation is reproducible. Given the
time constraint, the next candidate should not be downloaded automatically.
The practical next step is to proceed with preprocessing on the already
acquired and schema-verified AutoHack data under capture/file-level leakage
controls, while retaining ROAD for external robustness evaluation. AutoHack
must not be presented as vehicle-disjoint evidence.

B) AUTOMOTIVE CAN V2 REJECT — EVALUATE ANOTHER PRIMARY DATASET
