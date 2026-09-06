# Tracktion Feasibility — Phase E Record

Execution date: 2026-09-06 (Asia/Tokyo)

## T-005 PDC

Test ID: T-005 PDC

Hypothesis: H6 Plugin Delay Compensation; H9 offline-render equivalence.

Environment: Windows 11 Home 10.0.26200 x64; AMD Ryzen 9 7900X; 31.12 GiB RAM; Visual Studio 2026 Build Tools 18.9.2; MSVC 19.51.36256.0 (toolset 14.51.36231); CMake 4.3.1-msvc1; Ninja 1.13.2; Windows SDK 10.0.26100.0.

Dependency: Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`. The Phase 0 pins were retained without revision change.

Method: A prototype-only, framework-free `Processor` description defines deterministic `latency` and `multiply` processors. The latency processor is designed to report `1024` (or `2048`) samples via `getLatencySeconds()` and to implement an equal-sized circular-buffer delay. Its parameters are mapped only by the Phase E adapter's custom runtime factory; no Tracktion/JUCE latency type, calculated compensation, runtime graph, or runtime persistence enters Domain State.

The intended E1 fixtures use a generated, silent-padded mono impulse; Timeline event `1024`; requested offline range `[0, 4096)`; Clip PDC, Track PDC, Master latency, mixed `256 + 768` versus `1024`, bypass, stopped-state `1024 → 2048` rebuild, reorder, and reconstruction. Timing comparisons are integer sample indices only. The original 48,000-sample event / long-range attempt and a reduced range attempt both failed to complete their baseline offline render before any latency processor was reached.

| Measurement | Result |
| --- | --- |
| Render requested start / end | `0` / `4096` Timeline samples in the final bounded retry. |
| Timeline event | `1024` samples (reduced from the initially attempted `48000` only to isolate the renderer stall). |
| Baseline path | no latency processor; expected impulse index `1024`. |
| Baseline observed impulse index | Unmeasured — `Renderer::RenderTask` did not complete within 10 seconds. |
| Reported / actual latency | Unmeasured at runtime — the deterministic processor was not reached because the no-latency baseline stalled. |
| Clip / Track / Master / mixed-layer alignment | Unmeasured. |
| Bypass / latency-change / reorder / reconstruction | Unmeasured. |
| Preroll / leading silence / lost samples | Unmeasured; no render buffer was delivered. |
| Maximum alignment error | Unmeasured; **not** treated as 0. |
| Physical audio device | Not opened. |

Limited retries: (1) the initial event-48000 range was allowed to run beyond the normal bounded test window and did not complete; (2) a silent-padded impulse source replaced a one-sample source; (3) the event and render range were reduced; and (4) message-thread registration and an optional dispatch-loop experiment were tried. None made the baseline render complete. The final harness has a 10-second guard so the automated test records the condition without leaving a background render process. Direct invocation printed `PHASE_E_INCONCLUSIVE Renderer::RenderTask did not complete a 4096-sample baseline range within 10 seconds` and exited 0 to represent a completed **measurement**, not a successful PDC test.

Status: Inconclusive.

E2 actual VST3: Not attempted. E1 did not establish deterministic offline PDC, so adding a VST3 test plugin would not identify the current blocker. No plugin scanner, GUI, VST3 SDK dependency, binary artifact, or production plugin system was added.

Workaround: None accepted. Extending the timeout, treating an unfinished render as aligned, persisting calculated latency in Domain State, or adding a Tracktion fork/major patch would not be a valid PDC result.

Requirement impact: H6 and H9 remain Inconclusive. This does not meet `alignment error = 0 timeline samples`, so Phase E does not pass and Phase F must not begin. The result is not classified as H6 Fail because no completed deterministic latency path produced a non-zero or non-deterministic alignment measurement; the immediate blocker is baseline headless offline rendering over the range required to observe the delay.

## Retry after renderer diagnostic

Test ID: T-005 PDC retry, baseline gate only. The dependency pins, generated 48 kHz mono source, Timeline event, and requested render range remained unchanged. The Phase E adapter now installs an adapter-owned `Edit::filePathResolver` before Clip insertion. It returns the adapter's finite generated source and is runtime input mapping only; it does not enter Domain State or Project persistence.

| Field | Observation |
| --- | --- |
| Scenario | no-latency baseline; no latency processor; headless/offline; no physical device |
| Resolver configured | Yes |
| Timeline event / render range | `1024` / `[0, 4096)` samples at 48 kHz |
| Render completion / receiver sample count | Complete under the existing 10-second guard / `4096` samples |
| Expected / observed impulse | `1024` / `1026` Timeline samples |
| Alignment error against required baseline position | `2` Timeline samples |
| Reported / actual processor latency | Not applicable; no latency processor was run |
| Preroll / output boundary | Tracktion renderer warm-up is internal; receiver delivered the requested 4096 samples. No adapter preroll policy was added. |
| Later PDC cases | Not run: Clip, Track, Master, mixed, bypass, latency change, reorder, reconstruction, and VST3 remain unmeasured. |

Direct invocation output:

```text
PHASE_E_OBSERVATION name=baseline expected=1024 observed=1026 samples=0,0,0.25
PHASE_E_FAIL baseline expected sample 1024, observed 1026
```

Source inspection at the pinned revision identifies Tracktion's default `ResamplingQuality::lagrange` path as a relevant cause: `LagrangeResamplerReader` uses `juce::LagrangeInterpolator::getBaseLatency()` and advances `timeSourceIsAheadDueToLatency` when reset. The observed offset is deterministic but no zero-latency report or adapter-level compensation policy was established for this no-latency baseline. Shifting Domain Timeline state, silently subtracting two samples in the adapter, or treating the offset as PDC would violate this fixture's direct integer-Timeline baseline.

Retry status: Inconclusive. The renderer source-resolution defect is fixed, but the required no-latency baseline observation is `2`, not `0`, Timeline samples from its Domain event. Therefore this retry cannot validly enter E1 PDC measurements. H6 PDC remains Inconclusive rather than Fail because no correct-reported-latency PDC path was measured; H9 offline-render timing is Inconclusive. Phase F and E2 actual VST3 do not start. No workaround is accepted in this task.
