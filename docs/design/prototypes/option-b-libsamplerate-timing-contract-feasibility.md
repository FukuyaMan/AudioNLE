# Option B libsamplerate Timing-Contract Feasibility

## In progress

LS0--LS12 passed through the source-service partition and generation gate. No production backend decision follows: edit/lifecycle gates LS13--LS19 and final classification gates LS20--LS22 remain active.

Pinned release: libsamplerate `0.2.2`, tag revision `b9c20b93660c3683fda12e3c2a01f0021bf96c56`, upstream BSD-2-Clause. It is fetched as a CMake subproject and linked statically to the fixture. On Windows/MSVC, upstream CMake configures successfully; its old CMake policy warnings are upstream warnings, not suppressed or patched.

The direct fixture uses caller-buffer `src_new`/`src_process`/`src_delete`, fixed `SRC_SINC_BEST_QUALITY`, and a fixed ratio per render. No callback/pull API or dynamic-rate operation is used.

Three independent fresh states for each direction produced byte-identical output and frame counts. With 4096 supplied impulse frames, before an explicit end-of-input flush the counts were 4302 frames (44100->48000) and 3619 frames (48000->44100); flushing produced no additional frames in this fixture. These are physical startup observations only, not Timeline placement or latency compensation.

The LS5--LS8 residual diagnostic found a wrapper input/output accounting defect, not a demonstrated non-vanishing backend state difference. Fresh output became bit-identical to the 4096-sample continuous slice at representative interior starts when the physical Source start was snapped to the reduced rational phase lattice and trim was derived from `T0 - physicalStart * ratio`. Large and 128-frame process calls were equal; `end_of_input=false` and exact physical input identity were confirmed. The gate remains active pending the complete start/content/source-boundary/reset sweep.

## LS5--LS8 result

For reduced `P/Q`, `logicalSource = floor(T0*Q/P)`. With nominal native preroll `N=1024`, the exact physical start is `max(0, logicalSource-N)` rounded down to a multiple of `Q`. The exact discard is `T0 - (physicalStart/Q)*P`; both divisions are integer divisions because phase alignment makes `physicalStart` a multiple of `Q`.

All 160 phase classes for 44100->48000 and all 147 phase classes for 48000->44100 pass with max and RMS error <= `2e-5` (the fixture observations are exact equality). Broadband, distributed-marker, and multitonal content pass at near-origin T160/T161, T512, T1000, T4096, same-phase T4096+P, and T12000. At source start, physical start clamps to zero with no negative read or synthetic history; it deterministically equals the continuous source-origin reference. Three independent fresh reconstructions and `src_reset()` followed by identical prepared input equal the fresh result. No replay from Timeline 0 is required for interior cases; only bounded physical history plus integer coordinates is used.

History classification: **Bounded but empirically chosen**. libsamplerate does not expose sufficient public filter-support/delay information to derive 1024 formally. Gate result: **LS5-LS8 PASS; proceed to source-service integration**.

## LS9--LS12 result

Both ratios passed canonical-vs-partitioned reconstruction for 128-frame, 256-frame, and deterministic irregular (`73, 251, 19, 128, 311`) graph/native partitions. The exposed range was `T4096..T4223` (128 frames); max absolute and RMS error were at most `2e-5`. Ordinary graph processing retained one prepared state: `src_new=1`, `src_reset=0`, state replacements `=0`. The small 128/irregular graph x large/irregular native-input matrix passed. The fixture retains input-used/output-generated counts per `src_process`; intermediate counts may vary while final exposed output remains equivalent.

Generation is stamped on every 257-frame page. After invalidation, one deliberately published old page was observed and discarded before it could prepare current `src_process` input. The stale SRC state is discarded rather than repaired; a fresh state then renders current-generation output without starvation or callback blocking. Memory per view is fixed: 8224-byte native cache, prepared caller/scratch buffers, one opaque prepared SINC state, and zero duration-scaled retained PCM.

Source audit: pinned `samplerate.c` validates and dispatches `src_process` to a prepared state vtable. SINC state/private buffers are allocated in `sinc_state_new`/`sinc_filter_new` and freed on close; no allocation, realloc, lock, wait, sleep, or OS-I/O call was observed in the examined process dispatch. This is not full CRT/OS interposition proof. Runtime classification: **libsamplerate process path acceptable but partially unproven**. LS9--LS12: **PASS; proceed to edit/lifecycle gates** with the carried constraint: AudioNLE callback contract proven; libsamplerate internal realtime safety partially unproven.

## LS13--LS19 result

At both ratios, fresh-state seek forward, backward, and return-to-prior-position output matches the continuous reference at `T4096`, `T12000`, and phase-shifted targets. Repeating targets from different prior histories is identical because each seek prepares bounded physical history and a new/reset state outside callback. Trim-left preserves integer Source origin and integer Timeline placement while allowing physical preroll before the logical Clip boundary; trim-right is an exposure cap at the authoritative end; re-expansion returns to the original continuous slice without phase shift.

Source 0, Source 1, and small positive logical offsets clamp physical start to zero, never issue a negative read, and retain stable accounting. Exact rational (`T4096`), fractional-phase (`T4097`), and ordinary (`T5003`) split starts each use an independent fresh runtime state and match the unsplit reference around the seam (max and RMS <= `2e-5`, no first differing sample). Physical reconstruction requires history before the logical split and, for sinc output at the edge, prepared source samples after it; these reads are DSP input only and do not change Clip ownership or logical ranges.

Destroying runtime state/buffers and reconstructing only from Source identity/origin, Timeline placement, `P/Q`, converter mode, 1024-frame empirical preroll, and phase rule passes. Overlapping and disjoint views sharing input own independent states; deterministic sum/recreation checks leave the surviving view unchanged. A logical Clip end exposes no extra sample. At physical MediaSource end, `end_of_input=true` is supplied on the final input call; final generated frames are consumed only under the explicit wrapper exposure range. Filter settling is physical SRC computation, **not AudioNLE Effect Tail**.

All exercised worker/cache edit cases retain reader/file/decode, wait/block/spin, AudioNLE allocation/growth, synchronous fallback, and prepared-cache misses at zero. The unchanged `1024 + phase rounding` policy is sufficient for every LS13--LS19 case. Classification: **LS13-LS19 PASS; proceed to quality/final comparison**. LS20 may begin; backend runtime remains acceptable but partially unproven.

## LS20--LS22 result and disposition

Fixed-mode deterministic spectral measurements are clearly suitable for prototype continuation: sweep worst/ripple were -0.00000083 / 0.00000085 dB (44.1->48) and -0.02878 / 0.00123 dB (48->44); sampled image/alias rejection was -65.12 dB and -163.63 dB, versus IntegerRationalZoh -11.40 dB and -3.58 dB. Impulse, low-frequency, spoken-like, music/BGM-like, and round-trip diagnostics were finite and three-run deterministic. These results are limited to the two measured ratios and do not claim perceptual superiority or arbitrary-ratio quality.

Integration is a **Moderate SRC integration subsystem**: pinned dependency, lifecycle state, empirical preroll/discard accounting, fixed buffers, physical cache planning, invalidation, per-view reconstruction, source-end clipping, and instrumentation are all required. The candidate is **libsamplerate Proceed with Constraints**: 1024 plus phase rounding is an experimental converter-mode/ratio-specific bound, and backend-internal realtime allocation/lock freedom remains partially unproven.

Integer/rational Timeline and Source authority remains unchanged. Filter phase/history/output counts are physical runtime state; no libsamplerate state is persisted in Domain data. Relative to libsoxr, this is measured runtime/timing/edit/quality evidence; libsoxr remains inconclusive solely because pinned 0.1.3 CMake subproject integration blocked runtime measurement, not because of measured DSP quality. Recommendation: **Select libsamplerate for current Option B prototype backend**, explicitly not an irreversible production backend commitment.
