# Option B Mixed-Rate SRC Alignment Corrective Gate

## Status

**Inconclusive.** The corrective investigation reproduces a deterministic mismatch, but has not established a bounded reset/preroll/output-trim policy that preserves the fixed authoritative mapping.

## Fixed authority

```text
TimelineBoundary(S) = ceil(S * 160 / 147)
SourceBoundary(T)   = floor(T * 147 / 160)
```

Neither expression was changed for the probe.

## A0 state finding

`ZeroOrderHoldInterpolator` is `GenericInterpolator<ZeroOrderHoldTraits, 1>`. Reset fixes `subSamplePos` to `1.0` and clears its one-sample history. The first process output pushes and exposes `input[0]`; later input pushes occur when the accumulated `147/160` phase reaches one. The public API exposes reset, processing and base latency, but not an explicit phase-anchor setter. Base latency zero is not a statement about reset alignment.

## Reproduced observation

The fixture uses a fixed 257-source-sample page cache, one shared worker, 128-frame graph blocks, source-first cache identity, and a per-view ZOH instance. It supplies the inverse-mapped source sample at the 160-Timeline-sample anchor and discards only the bounded pre-anchor output.

| Source boundary | Authoritative Timeline boundary | Streaming observation | Error |
| --- | ---: | ---: | ---: |
| 147 | 160 | 161--162 | +1 to +2 |

The output differs from the required exact boundary despite `getBaseLatency() == 0`. Subsequent direct generated-impulse work showed that this is an invalid impulse first-nonzero observation for a ZOH edit-boundary test, rather than evidence that Model 1 is misaligned. See `mixed-rate-src-wrapper-anchor.md`.

## Phase-class characterization

The generated `ZeroOrderHoldInterpolator` probe covers `S=0..293` (two complete `S mod 147` periods), using the same `147/160` ratio, reset value and 160-Timeline-sample anchor arithmetic as the wrapper. Its reset-anchor histogram is `error 0: 294`; the continuous reference is `error 0: 293, error +1: 1` (the start transition is the exceptional first-output case). Thus the primitive's rational phase classes do not reproduce the real streaming mismatch, and no phase-class trim or preroll rule is derived.

This is a hard diagnostic distinction: the generated primitive model is exact under the selected mapping, while the current worker/cache/SourceNode fixture reports `S=147` at 161--162. Until the wrapper-level discrepancy is isolated with the same exhaustive instrumentation, a correction would be unsupported magic. Render-start, partition, seek/reset and real-WAV confirmation for a *corrected* wrapper were therefore not run.

## Model result

Model 2 (bounded preroll) and Model 3 (fixed output trim) cannot yet be selected. A global trim is not justified while the source-position phase classes and chunk-partition observations have not been measured. No magic one- or two-sample correction was applied.

## Resource and callback boundary observed so far

```text
native cache: 8 * 257 * sizeof(float) = 8,224 bytes per MediaSource
ZOH input/output scratch: 2 * 512 * sizeof(float) = 4,096 bytes per Clip view
ZOH history: one float; reset phase/history are internal to the primitive
callback reader/wait/allocation/fallback: 0
```

All stated buffers are fixed-size and duration-independent. These observations do not establish final alignment or S4--S12.

## Consequence

The existing mixed-rate execution plan remains active. S4--S12 may not resume until a generated phase-class probe proves error 0 across the prescribed source boundaries, render starts, page/block/chunk partitions and reset sequences, followed by real-WAV confirmation. If no such bounded wrapper policy exists, the sole next gate is evaluation of another public JUCE SRC primitive under this same mapping contract.
