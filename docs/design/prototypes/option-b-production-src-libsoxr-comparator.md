# Option B libsoxr comparator

## Decision status

Production SRC ADR is **not ready**.  The libsoxr candidate classification is **`libsoxr quality failure`**.  Integration complexity is **`Moderate production SRC component`**: the local static build adapter is bounded and reproducible, but a viable candidate would still require a backend-specific bounded history/reconstruction policy, prepared-input/source-service integration, and callback evidence.

## Evidence boundary

The comparator pin is libsoxr 0.1.3 annotated tag `6e80f8b597c92afd3edf7e714a3b8d7b4534213d`, peeled commit `945b592b70470e29f917f4de89b4281fbbd540c0`, LGPL-2.1-or-later.  It uses an AudioNLE-owned direct static source target and a generated configuration header; no algorithm or compatibility patch is retained.

Float32 interleaved caller buffers, fixed rational rate setup, single-thread runtime specification, caller-managed 2048/512 buffers, opaque per-instance state, error returns, clear, delay query, and explicit NULL-input flush were verified.  Basic six-direction fresh-stream timing passed.  Full arbitrary-start reconstruction/edit/cache validation was not started because the mandatory quality gate invalidated the candidate.

`SOXR_VHQ` passes four common directions but fails 44.1 -> 96 and 48 -> 96 (0.124277 dB gain error and 0.130629 dB ripple).  Realtime interception and 8/16-view performance are correctly skipped rather than presented as passing evidence.

## Recommendation

**`No current candidate satisfies production requirements`**.

This does not recommend revisiting libsamplerate MEDIUM/FASTEST: their known mode-specific timing failure remains independent and no evidence here shows that repairing it is lower risk.  The next ADR-ready candidate requires complete timing/edit, quality, realtime, 8/16-view performance, common-rate coverage, and licensing/build reproducibility evidence.
