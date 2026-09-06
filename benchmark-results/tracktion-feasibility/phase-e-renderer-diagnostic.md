# Tracktion Feasibility - Phase E Renderer Diagnostic

Execution date: 2026-09-06 (Asia/Tokyo)

Scope: limited root-cause diagnostic for the Phase E no-latency offline-render stall. This is not a PDC retry; no latency processor, VST3 plugin, effect-tail fixture, or later Phase was run.

## Source-level findings

The pinned Tracktion Engine source is unchanged: `b88a6ee51913668cb53e911e030ab736b13342cf` (nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`). `Renderer::RenderTask::runJob()` returns `jobNeedsRunningAgain` while `renderAudio()` is incomplete. The built-in synchronous `Renderer::renderToFile()` uses the same repeated `runJob()` loop, so no worker-pool ownership is required for that loop. `NodeRenderContext::renderNextBlock()` returns incomplete when a leaf node remains not ready after `prepareForNextBlock()`.

Phase C/D assign `Edit::filePathResolver` through their adapter-owned source registry. The original Phase E adapter inserted an absolute temporary WAV but did not assign `filePathResolver`. The diagnostic isolated this difference after reproducing the Phase C renderer configuration: temporary source lifetime, float receiver, `RenderTask` ownership, direct synchronous polling, 48 kHz rate, 128-sample blocks, and device-free setup are all retained.

## Minimal migration measurements

The executable has a 10-second guard. `run_calls` counts `RenderTask::runJob()` calls after initial task construction; `reset_calls` and `add_block_calls` are receiver observations.

| Scenario | Change from prior scenario | Result |
| --- | --- | --- |
| `c-baseline` | Phase C equivalent: 256-sample constant source, Clip `[100,164)`, render `[0,256)`, resolver set | Complete: 468 ms; 191 calls; reset 1; blocks 2; 256 samples. |
| `impulse-content` | constant source -> one impulse | Complete: 471 ms; 191 calls; reset 1; blocks 2; 256 samples. |
| `long-source` | source 256 -> 4096 samples | Complete: 465 ms; 191 calls; reset 1; blocks 2; 256 samples. |
| `long-range` | render 256 -> 4096 samples | Complete: 465 ms; 221 calls; reset 1; blocks 32; 4096 samples. |
| `event-position` | Clip start 100 -> 1024 | Complete: 467 ms; 221 calls; reset 1; blocks 32; 4096 samples. |
| `long-clip` | Clip duration 64 -> 4096 samples | Complete: 468 ms; 221 calls; reset 1; blocks 32; 4096 samples. |
| `without-resolver` | remove only `Edit::filePathResolver` | Timed out: 10000 ms; 708110 calls; reset 1; blocks 0; received 0 samples; no task error. |

```text
RENDERER_DIAGNOSTIC_COMPLETE scenario=c-baseline run_calls=191 elapsed_ms=468 reset_calls=1 add_block_calls=2 received_samples=256 output_samples=256 error=
RENDERER_DIAGNOSTIC_COMPLETE scenario=impulse-content run_calls=191 elapsed_ms=471 reset_calls=1 add_block_calls=2 received_samples=256 output_samples=256 error=
RENDERER_DIAGNOSTIC_COMPLETE scenario=long-source run_calls=191 elapsed_ms=465 reset_calls=1 add_block_calls=2 received_samples=256 output_samples=256 error=
RENDERER_DIAGNOSTIC_COMPLETE scenario=long-range run_calls=221 elapsed_ms=465 reset_calls=1 add_block_calls=32 received_samples=4096 output_samples=4096 error=
RENDERER_DIAGNOSTIC_COMPLETE scenario=event-position run_calls=221 elapsed_ms=467 reset_calls=1 add_block_calls=32 received_samples=4096 output_samples=4096 error=
RENDERER_DIAGNOSTIC_COMPLETE scenario=long-clip run_calls=221 elapsed_ms=468 reset_calls=1 add_block_calls=32 received_samples=4096 output_samples=4096 error=
RENDERER_DIAGNOSTIC_TIMEOUT scenario=without-resolver run_calls=708110 elapsed_ms=10000 reset_calls=1 add_block_calls=0 received_samples=0 error=
```

## Result

Root cause: the Phase E harness omitted the adapter-owned `Edit::filePathResolver`. Removing that resolver alone reproduces the bounded stall; restoring it completes the same long source/range and event-position baseline. This is a prototype adapter/harness defect, not evidence that `Renderer::RenderTask` requires a physical device, worker pool, GUI, or a Tracktion Engine change.

Phase E remains **Inconclusive**. The PDC fixture itself was intentionally not rerun, so `alignment error = 0 timeline samples` is still unmeasured. A narrowly scoped Phase E retry is now appropriate: add the same adapter-owned resolver used by Phases C/D, retain the 10-second guard, and run the existing deterministic PDC measurements before considering Phase F.
