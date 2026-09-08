# Option B Integer-Rational ZOH Timing Primitive

Classification: **Integer-rational ZOH correction validated**.

The fixture-local `IntegerRationalZoh` has four signed-64-bit values: reduced positive `P`, `Q`, current absolute-relative Source identity, and phase. It selects an identity only; it has no filter, interpolation kernel, history buffer, I/O, allocation, locks, waits, Timeline float seconds, or dynamic-ratio machinery.

`ReferenceHeldSource(T) = floor(T * Q / P)` is checked before the signed-64-bit multiplication. The supported input bound is `0 <= T <= INT64_MAX / Q`; construction also rejects factors for which `phase + Q` could overflow, and each advance checks the resulting Source identity before adding it. Incremental reset derives `source=floor(T0*Q/P)` and `phase=(T0*Q)%P`; after emitting it advances `source += (phase+Q)/P`, `phase=(phase+Q)%P`. This is the quotient/remainder recurrence for `(T+1)Q/P`, so it equals the closed form at every output.

| Check | Result |
| --- | --- |
| Closed form vs incremental, T0..100000 | 100001 matched; 0 mismatched |
| Direct generated `float(S)`, T0..400 | 401 matched; 0 mismatched |
| Worker/cache/view/node/graph trace, T0..400 | 401 matched; 0 mismatched |
| Exact/fractional mismatch counts through T100000 | exact 0; fractional 0 |
| 44100 -> 48000 (`P=160,Q=147`) | pass |
| 48000 -> 44100 (`P=147,Q=160`) | pass |
| 32000 -> 48000 (`P=3,Q=2`) | pass |
| 48000 -> 96000 (`P=2,Q=1`) | pass |
| 128, 256, irregular process partitions | identical identity sequence |
| 257-frame native pages; Source 255..258 | exact; no page-dependent timing |
| Direct nonzero starts T160, T161, T213 | continuous-reference slices match |
| Seek/reset T160 -> T213 -> T160 | first/second T160 sequences and reference match |
| Nonzero logical Source origin S0=1000 | absolute identities match |
| Worker/cache callback counters | generator 0; wait/block/spin 0; allocation/growth 0; synchronous fallback 0 |

Representative corrected trace:

```text
T159 -> 146
T160 -> 147
T161 -> 147
T162 -> 148
T319 -> 293
T320 -> 294
T321 -> 294
```

JUCE `ZeroOrderHoldInterpolator` is not authoritative for Source-to-Timeline sample identity in this mixed-rate prototype. This finding makes no broader claim about JUCE SRC quality.
