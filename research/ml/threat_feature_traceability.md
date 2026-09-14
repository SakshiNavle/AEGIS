# Threat-to-feature traceability hypothesis

This table maps possible evidence to observable signals. It is a hypothesis,
not proof that any feature uniquely identifies an attack.

| Threat or anomaly | Candidate evidence |
|---|---|
| CAN flooding / DoS | `delta_t`, rolling timing statistics, ID frequency |
| Fuzzing | arbitration-ID distribution, DLC, payload Hamming distance |
| Replay | repeated payload/timing/sequence behavior |
| Spoofing | ID behavior, payload change, timing |
| UDS spoofing | ID, payload, and timing behavior |
| Generic timing anomaly | `delta_t`, rolling mean, rolling standard deviation |

Labels, filenames, capture IDs, interface metadata by default, and future
statistics are not evidence features.
