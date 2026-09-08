# Option B Streaming Wrapper Anchor Diagnostic

## Result

**Observation/instrumentation defect identified.** No mapping or SRC correction was applied.

## Differential result at Source 147

| Stage | Direct generated step | Direct generated impulse | Real WAV impulse through cache |
| --- | ---: | ---: | ---: |
| Authoritative Source boundary | 147 | 147 | 147 |
| Authoritative Timeline boundary | 160 | 160 | 160 |
| Observed first changed/non-zero output | 160 | 161 | 161--162 |
| Error under the old impulse observation | 0 | +1 | +1 to +2 |

The generated phase probe uses a reset `ZeroOrderHoldInterpolator` with the same `147/160` ratio and finds the step edge at the authoritative `ceil` boundary. A direct generated *single-sample impulse* instead first appears at 161. This reproduces the supposed cache failure without a worker, cache, page extraction, WAV reader or Clip runtime view.

The old real-WAV fixture writes isolated impulses and observes the first `> 0.24` output. That is a measurement of a ZOH hold interval's first non-zero sample, not a valid measurement of an edit-domain source-boundary transition. It therefore cannot disprove the fixed edit mapping.

## Coordinate identity

The generated step probe starts `srcInput[0]` at absolute Source 0 and `dstOutput[0]` at absolute Timeline 0. The current cache wrapper uses the same identities for the `S=147` case: its 160-sample Timeline anchor maps to Source 0 and its pre-anchor output discard is 128 when rendering the block beginning at Timeline 128. The direct impulse reproduction establishes that the first divergence occurs before cache extraction, so a worker/page range displacement is not the measured cause.

## Page and worker controls

No cache correction was tested or needed: the direct primitive control already reproduces the impulse position. Consequently, prefilled-versus-worker and page-boundary comparisons cannot establish a cache anchoring root cause for this observation. They remain appropriate only after replacing the fixture with a step/edge observation rule.

## Callback boundary

The existing actual worker/cache path retained callback reader/wait/allocation/fallback counters of zero. The direct primitive controls are diagnostic-only and are not realtime evidence.

## Consequence

The old `S=147` WAV result is withdrawn as evidence of an SRC wrapper timing defect. It does not, by itself, establish a bounded correction policy or complete mixed-rate S4--S12. The next narrow gate is **replace the mixed-rate fixture's impulse first-nonzero observation with an exact step-edge transition observation**, then rerun direct/cache/WAV differential, page, worker, render-start and partition checks under that observation rule.
