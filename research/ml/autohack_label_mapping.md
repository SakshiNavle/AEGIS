# AutoHack label mapping

Raw labels remain separate from all model features. The full streaming audit
observed the following exact labels.

| RAW LABEL | BINARY TARGET | MULTICLASS TARGET |
|---|---|---|
| `Normal` | `Normal` | `Normal` |
| `DoS` | `Anomaly` | `DoS` |
| `Fuzzing` | `Anomaly` | `Fuzzing` |
| `Spoofing` | `Anomaly` | `Spoofing` |
| `Replay` | `Anomaly` | `Replay` |
| `Spoofing_UDS_B_700_B34` | `Anomaly` | exact raw class |
| `Spoofing_UDS_B_702_B4` | `Anomaly` | exact raw class |
| `Spoofing_UDS_B_706_B17` | `Anomaly` | exact raw class |
| `Spoofing_UDS_FrontCam_7C4` | `Anomaly` | exact raw class |
| `Spoofing_UDS_LeftRadar_7B7` | `Anomaly` | exact raw class |
| `Spoofing_UDS_P_7E0_B4` | `Anomaly` | exact raw class |
| `Spoofing_UDS_P_7E0_B6` | `Anomaly` | exact raw class |
| `Spoofing_UDS_RightRadar_755` | `Anomaly` | exact raw class |
| `Spoofing_UDS_Steering_7D4` | `Anomaly` | exact raw class |

## Full counts

| RAW LABEL | Train count | Train % | Test count | Test % |
|---|---:|---:|---:|---:|
| `Normal` | 6,593,770 | 95.249906 | 2,557,180 | 94.308790 |
| `DoS` | 77,104 | 1.113801 | 35,629 | 1.313997 |
| `Fuzzing` | 135,602 | 1.958830 | 53,164 | 1.960688 |
| `Replay` | 55,509 | 0.801852 | 42,030 | 1.550066 |
| `Spoofing` | 58,568 | 0.846041 | 22,270 | 0.821318 |
| `Spoofing_UDS_B_700_B34` | 240 | 0.003467 | 180 | 0.006638 |
| `Spoofing_UDS_B_702_B4` | 168 | 0.002427 | 148 | 0.005458 |
| `Spoofing_UDS_B_706_B17` | 172 | 0.002485 | 132 | 0.004868 |
| `Spoofing_UDS_FrontCam_7C4` | 86 | 0.001242 | 42 | 0.001549 |
| `Spoofing_UDS_LeftRadar_7B7` | 204 | 0.002947 | 192 | 0.007081 |
| `Spoofing_UDS_P_7E0_B4` | 339 | 0.004897 | 168 | 0.006196 |
| `Spoofing_UDS_P_7E0_B6` | 548 | 0.007916 | 176 | 0.006491 |
| `Spoofing_UDS_RightRadar_755` | 216 | 0.003120 | 138 | 0.005089 |
| `Spoofing_UDS_Steering_7D4` | 74 | 0.001069 | 48 | 0.001770 |

The individual UDS counts are retained exactly in
`ml/artifacts/autohack_full_audit.json`. UDS variants must not be collapsed
without recording the original classes and affected counts. No resampling or
class weighting was performed.

## Imbalance summary

| Split | Normal | Anomaly | Attack/normal ratio | Minority percentage |
|---|---:|---:|---:|---:|
| Train | 6,593,770 | 328,830 | 0.04985 | 4.750094% |
| Test | 2,557,180 | 154,317 | 0.06035 | 5.691210% |

The audit artifact also records exact `Interface|RAW LABEL` counts. The
interfaces are not interchangeable vehicle identities; these counts support
stratified characterization only and do not establish vehicle-disjoint
generalization.
