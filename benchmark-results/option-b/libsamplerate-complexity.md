# libsamplerate Complexity (Partial)

The dependency adds one static C CMake subproject and one fixture target. Wrapper, preroll, cache, lifecycle, and callback complexity remain unmeasured.

The diagnostic identifies one necessary wrapper responsibility: snap the physical native start to the reduced rational phase lattice, then derive output discard from the requested Timeline coordinate and that physical start. This remains physical DSP accounting and does not alter Source or Timeline authority.

The bounded wrapper needs per-view prepared SRC state, up to `1024 + Q - 1` physical native history, phase-aligned input planning, and exact integer discard accounting. This is still unmeasured on the callback/cache path; LS9 is now authorised to measure that path.

## LS9--LS12 accounting and audit

The timing-contract fixture's active-view cache is 9252 bytes (nine fixed
257-frame float pages); this is the minimum capacity that retains every page
in the 320/147 invalidation range without modulo overwrite. Caller/scratch
buffers are prepared before callback processing; libsamplerate owns one opaque
prepared SINC state. Two views were exercised sequentially. Retained PCM does
not scale with source duration (`0`).

The pinned source allocates SINC state/private buffer in `sinc_state_new`/`sinc_filter_new`, resets it through lifecycle APIs, and frees it on close. `src_process` in `samplerate.c` validates and dispatches to the existing state vtable. No allocation/free/realloc, mutex/critical section, wait, sleep, or blocking OS operation was observed in that dispatch/source path. Full Windows CRT/OS interception was not performed; therefore backend internal allocation/lock freedom is partially unproven. The wrapper remains a bounded source/SRC extension, not a Timeline-authority change.

## LS13--LS19 lifecycle burden

Seek, trim, split, reconstruction, and view recreation require control-thread disposal/preparation of a per-view opaque state; no SRC history becomes persisted Domain data. Independent views cannot share that state, while their bounded native cache service can be shared. Sinc future-source reads may be needed to calculate edge-adjacent samples, but remain physical input planning and do not alter logical Clip boundaries. No additional preroll beyond the existing 1024 native frames plus phase rounding was required.

## LS20--LS22 disposition

Demonstrated integration responsibilities are dependency pinning/static linkage; control-thread state creation/reset/destruction; empirical `1024 + phase lattice` preroll; integer output discard; fixed caller buffers; worker/cache physical-range preparation; generation invalidation; per-view state; seek/split/trim reconstruction; end-of-input clipping; and callback counters. Classification: **Moderate SRC integration subsystem**. It is bounded, but materially more than a thin DSP wrapper.

Persistent risks: the preroll bound is experimental for this converter mode and two ratios, not a public support/delay contract; changing converter mode must be treated as invalidating it. AudioNLE callback safety is demonstrated, while libsamplerate internal allocation/lock safety remains partially unproven. No libsamplerate phase/history/output count becomes persisted Domain authority.
