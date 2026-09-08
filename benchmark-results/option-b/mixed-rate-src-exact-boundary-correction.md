# Option B Exact-Boundary ZOH Advance Corrective Gate

## Accumulator finding

The pinned JUCE loop advances only while `pos >= 1.0`, then adds `147.0 / 160.0`. At the second exact crossing:

```text
T319: pos before = 1.0812499999999858, advance = yes
      pos after selection = 0.081249999999985834
      pos after update = 0.99999999999998579
T320: pos before = 0.99999999999998579
      pos >= 1.0 = false; advance deferred to T321
```

This is not an intended interval convention: the mathematical accumulator is exactly one at T320. The same mechanism accounts for T160. It is a floating exact-crossing accumulation defect; no epsilon is acceptable as a correction.

## Integer authority model

For output `T`, the authoritative selected source is `floor(T*147/160)`. An equivalent fixed-size integer schedule maintains an integer numerator phase and advances source exactly when its rational sum crosses denominator 160, with initial state/operation order chosen to emit Source `floor(T*147/160)` at output `T`. Checked integer multiplication proves equality with the closed form for T0..100000 and representative 1/3/6/12-hour arithmetic positions; no float accumulator is authority.

## Candidate selection

Implemented and validated candidate: **Tiny AudioNLE-owned ZOH timing primitive**. It computes `output[T] = input[floor(T*Q/P)]` from integer authority. JUCE ZOH may not remain timing authority because its public floating accumulator disagrees at exact crossings. This is a bounded ZOH timing operation, not a general SRC or a mapping change.

The fixture-local primitive stores only signed-64-bit `source` and `phase`, plus the positive reduced factors `P` and `Q`. `reset(T0)` derives `source = floor(T0*Q/P)` and `phase = (T0*Q)%P`; each output emits `source`, then advances by `(phase+Q)/P` and `(phase+Q)%P`. The closed form rejects `T*Q` overflow before multiplication. Tested factors keep `phase + Q` within signed 64-bit range; production factor validation must retain that precondition.

Corrected direct and worker/cache traces both match all 401 positions at T0..400, including T160 -> 147 and T320 -> 294. The 100001-position integer schedule, four rate ratios, partitions, page boundary, nonzero starts, repeated seeks, and nonzero Source offset all pass; see [mixed-rate-src-integer-zoh.md](mixed-rate-src-integer-zoh.md). S4--S12 may now use this timing path; prior JUCE-ZOH timing-sensitive evidence is superseded rather than accepted.

S4--S12 were rerun through the integer-rational path; their result is recorded in `mixed-rate-src.md`.
