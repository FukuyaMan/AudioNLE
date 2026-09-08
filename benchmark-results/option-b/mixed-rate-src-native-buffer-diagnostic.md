# Option B Native Buffer / Streaming Anchor Differential

## Measured failing render

```text
requested graph block: Timeline [128, 256)
authoritative Source boundary: 147
authoritative Timeline boundary: 160
inverse coverage anchor: Source 0
physical cache page start: Source 0
srcInput[0] absolute Source: 0
dstOutput[0] absolute Timeline: 0 (the wrapper produces 256 samples then exposes [128, 256))
ratio: 147 / 160 input samples per output sample
ZOH reset state: public reset(), sub-sample phase 1.0
```

## Native buffer evidence

| Absolute Source | PCM16 decoded/cache value |
| ---: | ---: |
| 143 | 0 |
| 144 | 0 |
| 145 | 0 |
| 146 | 0 |
| 147 | 0.5 |
| 148 | 0.5 |
| 149 | 0.5 |
| 150 | 0.5 |
| 151 | 0.5 |

The decoded edge is exactly at Source 147; cache extraction and native source origin do not show a one-sample displacement.

## Raw project-rate output

The raw ZOH output exposed at absolute Timeline samples 156--165 is:

```text
156..160 = 0
161..165 = 0.5
```

Thus the first measured divergence is after the native cache window and before graph summing: the current `ClipRuntimeView` streaming ZOH input/output anchor produces the post-edge value at 161 despite `srcInput[0]=0` and the correct native edge at 147.

## Classification

**Inconclusive.** The first measured mismatch is after native cache extraction and before graph summing, making ClipRuntimeView/SRC input-output anchoring the leading suspect. Root-cause acceptance additionally requires a bounded diagnostic local correction that demonstrates 161 -> 160 without changing mapping; this gate does not implement it.

Worker/cache callback counters remain reader/wait/allocation/fallback = 0. Generated worker-cache and prefilled-cache controls, page-boundary variants, and a direct raw-array side-by-side trace remain unexecuted; they are not claimed as passing evidence.

## Next corrective gate

**correct ClipRuntimeView native Source anchor bookkeeping**: derive and test the exact initial input/history/output relationship for the `reset()` stream before changing production behavior.
