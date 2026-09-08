# libsamplerate Timing-Contract Evidence (Partial)

| Item | Result |
| --- | --- |
| Pin | 0.2.2 / `b9c20b93660c3683fda12e3c2a01f0021bf96c56` |
| Linkage | static CMake FetchContent subproject, MSVC x64 |
| API | caller-buffer streaming `src_process` only |
| Converter | `SRC_SINC_BEST_QUALITY` |
| Fresh repeatability | 3/3 identical per ratio |
| 44100->48000 | 4096 input; 4302 first process output; 4302 after flush |
| 48000->44100 | 4096 input; 3619 first process output; 3619 after flush |

LS5--LS8 establishes edit-boundary and preroll evidence only; quality remains outside this gate.

## LS9--LS12 evidence

| Check | 160/147 | 147/160 |
| --- | --- | --- |
| Graph blocks 128 / 256 / irregular | PASS; `T4096..T4223`, 128 exposed frames | PASS; same range and count |
| Visible comparison | max abs and RMS <= `2e-5` | max abs and RMS <= `2e-5` |
| Graph-boundary lifecycle | `src_new=1`, `src_reset=0`, replacements `=0` | same |
| Native `src_process` chunks | large / 128 / 256 / irregular equivalent | same |
| Joint graph/native matrix | 128, irregular x large, irregular: PASS | same |
| Generation invalidation | stale observed 1, discarded 1; fresh state PASS | same |

The fixture records `input_frames_used` and `output_frames_gen` per `src_process` call. Intermediate production may differ by partition; after unchanged wrapper discard, final visible output is equivalent. Callback counters remain reader/file/decode `0`, wait/block/spin `0`, allocation/growth `0`, fallback `0`, and prepared-cache misses `0`. Cache is fixed at `8 x 257` float frames (8224 bytes), and duration-scaled retained PCM is `0`.

Classification: **LS9-LS12 PASS; proceed to edit/lifecycle gates**. Carried constraint: **AudioNLE callback contract proven; libsamplerate internal realtime safety partially unproven.**

## LS13--LS19 evidence

| Check | Result |
| --- | --- |
| Seek forward/backward/repeat | PASS; fresh state and bounded preroll match continuous reference |
| Trim left/right and re-expand | PASS; integer logical coordinates unchanged; no accumulated phase shift |
| Source-start | PASS at Source 0/1/small offsets; no negative read or Timeline shift |
| Split exact / fractional / interior | PASS at T4096 / T4097 / T5003; max/RMS <= `2e-5`, no gap/duplicate/shift |
| Physical history direction | Before-split history and future source input are DSP reads only; neither changes ownership |
| Reconstruction | PASS from Domain coordinates and fixed wrapper policy only |
| Multi-view / recreation | PASS; independent SRC states, deterministic sum, unaffected surviving view |
| Logical / physical end | PASS; logical exposure cap exact; final input uses `end_of_input=true` |

SRC settling is not AudioNLE Effect Tail. It is solely physical computation needed at source boundaries and creates no user-visible effect-tail semantics. Callback counters remain zero for all exercised edit cases; 8224-byte cache remains fixed and duration-scaled PCM remains zero. The empirical `1024 + phase rounding` preroll was sufficient without increase. Classification: **LS13-LS19 PASS; proceed to quality/final comparison**. LS20 may begin.

## LS20--LS22 classification

Quality sweeps, impulse, low-frequency, generated spoken/music, round-trip diagnostics, and repeatability are recorded in `libsamplerate-quality.md`. Measured spectral behavior is suitable for prototype continuation and materially exceeds the sampled IntegerRationalZoh alias/image baseline. Timing authority remains unchanged: integer Project Timeline, integer native Source, and exact rational mapping are authoritative; libsamplerate phase/history/output counts are physical runtime state only.

Candidate classification: **libsamplerate Proceed with Constraints**. Constraints are empirical preroll dependence on `SRC_SINC_BEST_QUALITY` and tested ratios, and partially unproven backend-internal realtime safety. Complexity is a **Moderate SRC integration subsystem**. This is prototype selection evidence only, not irreversible production commitment.

Diagnostic history: an earlier residual was caused by deriving discard from logical Source distance rather than authoritative Timeline position and phase-aligned physical Source start. Interior starts now match continuous output exactly with bounded physical history.

## LS5--LS8 acceptance

Exact rule: `logicalSource=floor(T0*Q/P)`, `physicalStart=floor(max(0, logicalSource-1024)/Q)*Q`, `outputDiscard=T0-(physicalStart/Q)*P`. The 160 upsample and 147 downsample phase classes pass. Three deterministic content classes and near-origin, T4096, same-phase, and T12000 starts pass with exact observed equality. Fresh and reset-prepared state agree. Source-start clamps to zero and never reads negative samples.
