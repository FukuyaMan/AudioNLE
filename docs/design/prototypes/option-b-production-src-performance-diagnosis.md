# Option B Production SRC Performance Diagnosis

## Result

**Performance diagnosis classification: Backend DSP cost was primary cause.** Fixed-affinity diagnostics did not reduce failure severity, empty callback overhead is negligible, and scaling is approximately linear. Product-like core migration exists but does not explain the supported-tier failures by itself; CPU-frequency correlation is unproven.

**Optimisation decision: Current quality mode cannot meet supported-tier performance target on baseline.** This is limited to the measured `SRC_SINC_BEST_QUALITY` fixture and baseline environment. It does not change quality policy or select an alternative backend.

**Production performance classification: Current backend/configuration cannot meet production thresholds.** Several 8-view 96 kHz rows and several 16-view supported-rate rows fail the unchanged duration thresholds. 32 views remains `Stress tier unsupported for production guarantee` and was not optimized or used as a gate.

## Consequence

The stop rule applies: measured `src_process` dominates, scaling is not superlinear, and no AudioNLE wrapper cost is available to shave. A future, separate architecture decision may compare quality modes, alternative SRC backends, production concurrency policy, and supported-rate policy. It must not be inferred from this diagnostic alone.

## Realtime and correctness

No production SRC behavior changed. The existing mandatory caller-IAT smoke remains `Backend callback path supported with bounded residual blind spots`; CRT, heap, locks, wait/sleep, and file I/O are zero for first-use and steady prepared `src_process`. Existing timing/reconstruction/generation/boundary tests remain the correctness evidence; the performance runner does not itself revalidate sample-by-sample determinism.
