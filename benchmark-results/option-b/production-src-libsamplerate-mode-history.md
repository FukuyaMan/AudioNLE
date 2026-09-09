# libsamplerate MEDIUM / FASTEST mode-history resolution

The existing `PrerollPolicyV1` remains unchanged and is BEST-only.  This comparator uses each lower mode's own continuous reference, deterministic broadband signal, exact `P/Q`, fresh state, and the unchanged `2e-5` max/RMS threshold.  It searches native history `0..8192` in 64-frame steps, then linearly refines the first passing candidate.  This bound is diagnostic only, never a promoted policy.

The attempted physical policy was `physicalStart=max(0,floor(T0*Q/P)-history)` with output discard `T0-floor(physicalStart*P/Q)`.  It reports the exact V1 mismatch at T0=4096 before applying the search.  All phase classes (`P`) and source starts/seeks/split-adjacent starts are then required to pass.

| mode | P/Q | V1 first mismatch / max error | first one-start history | 512-frame capacity | exhaustive phases | result |
| --- | --- | --- | ---: | ---: | --- | --- |
| MEDIUM | 160/147 | 0 / 0.132475 | none <=8192 | — | FAIL | mode-specific timing contract failure |
| MEDIUM | 147/160 | 0 / 0.501585 | none <=8192 | — | FAIL | mode-specific timing contract failure |
| MEDIUM | 320/147 | 0 / 0.144389 | 1984 | 2367 | FAIL | capacity/phase failure |
| MEDIUM | 147/320 | 0 / 0.161432 | none <=8192 | — | FAIL | mode-specific timing contract failure |
| MEDIUM | 2/1 | no mismatch | 36 | 293 | PASS | partial only |
| MEDIUM | 1/2 | no mismatch | 70 | 1096 | PASS | partial only |
| FASTEST | 160/147 | 0 / 0.109883 | none <=8192 | — | FAIL | mode-specific timing contract failure |
| FASTEST | 147/160 | 0 / 0.493223 | none <=8192 | — | FAIL | mode-specific timing contract failure |
| FASTEST | 320/147 | 0 / 0.126415 | 1984 | 2367 | FAIL | capacity/phase failure |
| FASTEST | 147/320 | 0 / 0.145323 | none <=8192 | — | FAIL | mode-specific timing contract failure |
| FASTEST | 2/1 | no mismatch | 14 | 271 | FAIL | phase failure |
| FASTEST | 1/2 | no mismatch | 30 | 1056 | FAIL | phase failure |

`first one-start history` is explicitly not a policy: it only proves that a single T0 can be made to agree.  The exhaustive phase/edit gate rejects it.  The failure is therefore not safely attributable to merely “insufficient preroll”; it is an unresolved mode-specific phase/history reconstruction mismatch.  No policy identity, persisted fingerprint, source-service integration, quality, IAT, or performance evidence is promoted for either mode.
