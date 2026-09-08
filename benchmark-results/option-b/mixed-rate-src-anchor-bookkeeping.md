# Option B ClipRuntimeView Native Source Anchor Bookkeeping

## Authoritative ZOH intervals (corrected)

With the fixed authority, native Source sample `S` occupies the project interval:

```text
[ ceil(S * 160 / 147), ceil((S + 1) * 160 / 147) )
```

Therefore Source 146 occupies `[159, 160)` and Source 147 occupies `[160, 162)`. A step from 146 to 147 must first expose its post-edge held value at Timeline 160.

The unique Source identity for project sample `T` is the largest `S` whose forward boundary is no later than `T`. Integer equivalence gives:

```text
HeldSource(T) = floor(T * 147 / 160)
```

For `T=0..400`, an independent interval search using `ceil(S*160/147) <= T < ceil((S+1)*160/147)` agrees exactly with this closed form. The same checked multiplication applies at long-form positions. The former `ceil(T*147/160)` expectation is superseded: it conflicts with the half-open interval definition (for example T161 belongs to Source 147, not 148).

## Reset-anchor trace (superseded global-shift interpretation)

The public JUCE loop starts reset state at `subSamplePos=1.0`, pushes `input[0]` before its first output, and pushes another input whenever its accumulated phase reaches one. With `srcInput[0]=Source 0`, the present wrapper's selected source identity is:

```text
expected(T) = floor(T * 147 / 160)
```

| Timeline T | Interval Source | Closed-form Source | Current ZOH Source | Match |
| ---: | ---: | ---: | :---: |
| 156 | 143 | 143 | not yet identity-traced | — |
| 157 | 144 | 144 | not yet identity-traced | — |
| 158 | 145 | 145 | not yet identity-traced | — |
| 159 | 146 | 146 | 146 (step pre-edge) | yes |
| 160 | 147 | 147 | 146 (step pre-edge) | no |
| 161 | 147 | 147 | 147 (step post-edge) | yes |
| 162 | 148 | 148 | not yet identity-traced | — |
| 163 | 149 | 149 | not yet identity-traced | — |
| 164 | 150 | 150 | not yet identity-traced | — |
| 165 | 151 | 151 | not yet identity-traced | — |

The measured step establishes only T159--161: the wrapper is correct before and after the boundary but emits the old held value at exact T160. It does not prove a global one-Source-sample anchor shift. `GenericInterpolator` advances input only in its public pinned loop `while (pos >= 1.0)` before producing each output; a full phase trace at the crossing remains required.

## Diagnostic status

No previous-history, shifted-input, trim, or phase correction has been applied.

Classification remains **Inconclusive**. The corrected interval semantics make an exact-boundary transition defect the leading candidate; raw identity traces across additional boundaries and generated-versus-worker/cache comparison are still required.

Logical Clip range and any future physical history/preroll range must remain separate; Source -1 must never be read implicitly.
