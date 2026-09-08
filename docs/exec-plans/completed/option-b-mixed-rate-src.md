# Execution Plan: Option B Real Mixed-Rate Source-to-Timeline SRC

## Objective

Execute the bounded S0--S12 gate in [the mixed-rate SRC feasibility plan](../../design/prototypes/option-b-mixed-rate-src-feasibility.md). Determine whether real 44.1 kHz Source media can render into an integer 48 kHz Project Timeline through explicit AudioNLE-owned rational mapping and a bounded SRC wrapper, without hidden timing offsets, float-time authority, callback hazards, fractional edit drift, or runtime-state persistence.

Classify independently:

```text
Gate: Proceed | Proceed with Constraints | Reject | Inconclusive

SRC architecture:
  Public SRC primitive with AudioNLE mapping wrapper viable
  Custom SRC layer required
  Mixed-rate source architecture materially complex
  Inconclusive

Implementation burden:
  Bounded SRC extension
  Substantial SRC subsystem
  Custom-DSP warning
```

This is not FFmpeg/container work, device validation, live mutation, a new editor implementation, a Tracktion integration, or an ADR change.

## Established boundary

Retain without reclassification:

```text
Option B source handoff = Proceed with Constraints / Thin backend handoff
Option B multi-source = Proceed with Constraints / Bounded shared worker model viable
workers = 2
readers = unique MediaSources
caches = unique MediaSources
same-rate maximum timing error = 0
same-rate callback forbidden operations = 0
Tracktion linkage = 0
```

## Corrective ZOH timing gate (complete)

The floating JUCE `ZeroOrderHoldInterpolator` diagnostic deferred exact rational crossings at T160 and T320 for P=160/Q=147. This is classified as a floating exact-crossing accumulation defect, not as an edit mapping change. A fixture-local `IntegerRationalZoh` now owns ZOH Source identity: `HeldSource(T) = floor(T * Q / P)`. It uses checked signed-64-bit closed-form initialization and an equivalent quotient/remainder incremental schedule; no epsilon or Timeline float state is used.

Acceptance evidence is in `benchmark-results/option-b/mixed-rate-src-integer-zoh.md`, `mixed-rate-src-exact-boundary-correction.md`, `mixed-rate-src-worker-cache-identity.md`, and `mixed-rate-src-complexity.md`. Direct and worker/cache T0..400 traces are 401/401 exact; the T0..100000 schedule, all four required ratios, block partitions, 257-frame pages, nonzero starts, seek/reset, nonzero Source offsets, and callback counters pass. The implementation remains a **Tiny AudioNLE-owned ZOH timing primitive**. JUCE floating ZOH is not authoritative for Source-to-Timeline sample identity in this prototype.

Corrective classification: **Integer-rational ZOH correction validated**. S4--S12 may resume only using this timing path; timing-sensitive evidence from the floating JUCE ZOH path is superseded and must be rerun or explicitly superseded.

## S4--S12 result (complete)

The rerun uses real 44.1 kHz PCM16 WAV through a worker-owned reader, eight fixed 257-frame native pages, `ClipRuntimeView`, integer-rational ZOH, `SourceNode`, and the fixture graph. S4 exact step boundaries, S5 starts/partitions, S6 callback/cache invariants, S7 seek/reset, S8 trim/re-expand, S9 split seams, S10 1/3/6/12-hour arithmetic, S11 independent views, and S12 reconstruction pass. Detailed evidence: `benchmark-results/option-b/mixed-rate-src.md`.

The floating-JUCE-ZOH source file and its timing evidence remain historical root-cause evidence only; no S4--S12 timing conclusion is carried forward from them. Final gate classification: **Proceed with Constraints**. The plan remains active pending the next separately scoped boundary; its mixed-rate S4--S12 work is complete.

The historical Tracktion high-level source/resampler offset is a failure reference only. It is not an Option B result and it establishes no allowable tolerance. `Expected 1024; observed 1026` is failure, never acceptance.

## Fixed model, integer safety, and selected-policy phase

Use the primary model:

```text
ProjectRate = 48000
SourceRate = 44100
P = 160
Q = 147
ProjectRate / SourceRate = P / Q
```

All edit-domain calculations use reduced integer rational arithmetic. No float seconds, accumulated `double` phase, or DSP observation may become canonical Domain/edit/cache/reconstruction state.

Before real WAV or SRC output, compare at minimum:

```text
Candidate mapping 1:
  TimelineBoundary(S) = floor(S * P / Q)
  SourceBoundary(T)   = floor(T * Q / P)
```

against nearest and any other justified half-open policy. Select based on NLE edit semantics, not filter convenience. Document the exact signed sample assumptions, a checked multiplication/division strategy (including wide intermediate type if necessary), and supported long-form bounds. Never silently overflow `S * P`.

For `[S0, S1)`, define Source start/end, Timeline start/end, empty/collapsed case, and non-bijective case. Define inverse **coverage** semantics rather than requiring `inverse(forward(S)) == S` for every sample:

```text
Timeline request -> integer Source read range -> bounded preroll/history -> SRC
-> output trim/alignment -> exact requested Timeline range
```

Maintain three separate values in all instrumentation and result tables:

```text
edit-domain mapped Timeline position
SRC primitive effective latency
final observed project-rate position
```

Latency compensation must not alter the edit-domain mapping rule.

## Scope and ownership

Use local 44.1 kHz PCM16 WAV and generated fixtures; retain 128-frame graph blocks and 257-frame source pages. A 96-to-48 exact-ratio observation is optional and cannot replace the primary non-integer ratio gate.

```text
MediaSourceRuntime: native-rate pages/cache, reader/decoder, generation, bounded requests
ClipRuntimeView / SourceNode: Timeline placement, Source range, rational planning,
                              independent SRC phase/history/reset and fixed buffers
Option B graph: project-rate audio, then existing Clip processing/PDC/Tail behavior
```

Two far-apart Clip views of one MediaSource share source reader/pages but must not silently share incompatible SRC history. Determine state ownership experimentally; the expected candidate is independent Clip-view/SourceNode state. Domain descriptions may persist native rate and integer edit values, never resampler objects, phase, history, preroll, latency caches, workers, pages, or graph identity.

Out of scope: FFmpeg, compressed/container media, device I/O, full PDC/Tail combination, plugin changes, persistence, GUI, live mutation, production priority tuning, Tracktion, dependencies/submodules, or custom DSP before public candidates fail an identified requirement.

## S0: public SRC API investigation and model selection

Inspect public APIs in the pinned JUCE source and record `benchmark-results/option-b/mixed-rate-src-api-investigation.md`. Compare:

| Candidate | Investigation required |
| --- | --- |
| A: direct public JUCE primitive | public type/status, ratio convention, streaming input/output consumption, fixed-block support, reset, latency API/effective delay, allocation, state and thread assumptions, buffer contract |
| B: AudioNLE wrapper around public primitive | rational mapping/planning, state/reset, latency metadata, bounded preroll/trim, fixed callback buffers, render-start independence |
| C: custom DSP | Consider only after A/B concrete hard failure; do not implement merely to control alignment |

Record only documented guarantees. If latency is not exposed, define a reproducible generated-signal observation before accepting the primitive. Do not infer concurrency, allocation, or realtime properties not stated by public API evidence.

Choose the smallest candidate meeting the gate. If candidate B is selected, its primitive remains implementation detail while AudioNLE owns timing authority, input request planning, compensation and reconstruction policy.

## S1: pure mapping arithmetic

Before audio conversion, test mapping independently for Source markers:

```text
0, 1, 100, 147, 148, 1000, 44100, 441000, 158760000
```

For each record Source sample, expected Timeline boundary, inverse-planning result, and documented round-trip/coverage result. Include arithmetic-only positions corresponding to 1, 3, 6 and 12 hours. Require exact integer expected values, defined overflow behavior, and no float drift.

## S2--S3: generated alignment and latency hard gate

### S2: generated SRC alignment

Before real WAV, use a deterministic generated step or another signal with robust filter-aware boundary observation. Do not use impulse-peak location unless the primitive's documented alignment makes it valid. Establish a fixed reference/edge/center rule and verify block-streamed conversion against it.

### S3: latency

Measure raw/effective algorithmic output delay in Timeline samples. Record primitive delay, AudioNLE preroll/compensation/output trim, and final observation error. Accept only:

```text
effective aligned latency = 0
```

or a known `L` held as runtime metadata and explicitly compensated. Unknown, path-dependent, render-start-dependent or uncompensated delay is a hard stop.

## S4: real 44.1 kHz WAV path

Run:

```text
44.1 kHz PCM16 WAV -> source-rate worker/cache -> ClipRuntimeView -> SRC wrapper/state
-> Option B SourceNode -> project-rate graph -> headless observation
```

Use known markers/edges; source cache keys remain native-rate MediaSource/Source-page/MediaGeneration keys. Do not cache project-rate audio in MediaSourceRuntime. Verify mapped boundaries and filter-aware amplitude/reference behavior without redefining Timeline placement from an impulse peak.

## S5--S7: partition, seek, and preroll

### S5: render-start independence

Render equivalent events from Timeline starts 0, 512 and 1000. After explicit reset/preroll/compensation, final observed mapped position must be identical with timing error 0.

### S6: block/page/SRC-chunk independence

Use 128-frame graph blocks and 257-frame source pages. Exercise markers around 127, 128, 129, 255, 256 and 257 plus relevant mapped positions. Process block partition, page partition, and input chunk partition must not alter authoritative Timeline placement.

### S7: seek/reset/preroll

Run forward render, backward seek/render, far-forward seek/render, and identical repeated seek. Define and instrument the requested Timeline start, mapped Source start, bounded preroll range, SRC warmup/reset, output trim, and final Timeline observation. Require no stale filter history or phase, no duration-scaled preroll, and no float phase authority.

## S8--S10: edit semantics and long-form state

### S8: trim and re-expand

Use one real mixed-rate Clip and stopped rebuild. Render original Source range, trimmed source start/end, then re-expanded range. Source integers remain authoritative; Timeline boundary behavior follows the selected mapping; repeated changes show no cumulative fractional drift; SRC state is rebuilt fresh.

### S9: split semantics

Choose Timeline split `T` where `T * Q / P` is non-integer. Define explicit inverse selection (for example `floor(T * Q / P)` only if selected) and resulting left/right Source and Timeline half-open ranges. Compare left-plus-right rendered reconstruction with unsplit reference under the defined SRC/reset/preroll rule. If independent filter resets create an unavoidable seam difference, specify and classify the required seam/preroll semantics; do not hide it in metadata-only checks.

### S10: long-form drift and memory

Observe real/generated positions at 1 minute, 10 minutes and 1 hour; use arithmetic-only 3/6/12-hour checks where real media is impractical. Record expected/observed mapping and zero authoritative mapping error. Record source cache bytes, SRC input/output scratch, filter/history state, preroll bound, and per-Clip/per-source formulas. Reject duration-scaled decoded/resampled PCM, scratch, history, or preroll.

## S11: callback safety, mixed-rate coexistence, and multi-Clip state

Across all relevant cases retain:

```text
callback reader/file/decode = 0
callback wait/block/spin = 0
callback allocation/growth = 0
synchronous fallback = 0
```

SRC callback work must use fixed/preallocated state and buffers. Then sum at least one 44.1 kHz and one 48 kHz MediaSource into the project-rate graph; the native-rate path remains exact and the converted path uses the defined reference/tolerance. Do not repeat the full 32-media matrix.

Mandatory state-ownership case: render two far-apart Clip views of the same 44.1 kHz MediaSource, reset/seek one view, and require the other view's SRC output/history to remain correct. This determines whether state is per Clip view/SourceNode rather than shared by MediaSource.

## S12: reconstruction

From the same framework-free description containing MediaSource identity, native rate, integer Source range, integer Timeline placement and edit description, create fresh reader/cache/shared-worker service, Clip SRC states, SourceNodes and B graph twice. Require equal defined output, mapping observations, latency alignment, trim/split semantics and maximum timing error 0. Queue, cache, phase, history, preroll and latency state must not persist.

## Implementation surface and evidence

Keep implementation fixture-local under `prototype/option-b-low-level-graph/`. Expected names follow S0; no A2 source or production Domain code may change. Add a distinct Option B CTest target. Expected evidence:

```text
benchmark-results/option-b/mixed-rate-src-api-investigation.md
benchmark-results/option-b/mixed-rate-src.md
benchmark-results/option-b/mixed-rate-src-complexity.md
docs/design/prototypes/option-b-mixed-rate-src-results.md
```

The complexity record separates existing worker/reader/cache/arbitration, rational mapping and split/trim policy, SRC wrapper/state/reset/preroll/latency/fixed buffers, B graph handoff/reconstruction, and fixture-only generation/assertion/instrumentation. Do not create a fake production LOC total.

## Hard stops

Stop and classify `Reject` or `Inconclusive` before scope expansion if float seconds becomes authority; mapping drift/overflow is undefined; latency is unknown/uncompensated; render start or block/page partition changes timing; seek leaks history; far Clip views share state incorrectly; trim/split semantics remain ambiguous; duration-scaled state appears; callback waits/allocates/decodes; SRC state leaks to Domain; Timeline enters MediaSource cache identity; Tracktion enters the target; or passing requires custom DSP without evidenced A/B rejection, FFmpeg, devices, or live mutation.

## Verification and completion

After implementation run:

```powershell
.\build.ps1
ctest --test-dir build -R '<actual-option-b-mixed-rate-src-regex>' --output-on-failure
```

Also directly run the fixture at least three deterministic times; run `git diff --check`; review Tracktion source/header/linkage, JUCE dependency boundary, Domain/runtime leakage, rational arithmetic, mapping/latency tables, and memory/state formulas. The known unrelated Tracktion high-level Phase E 1024-versus-1026 failure remains separate; any new compile/link failure is a gate failure.

Pass requires S0--S12, exact integer mapping, known/compensated latency, render-start and partition independence, zero long-form mapping drift, retained callback invariants, bounded SRC state, independent multi-Clip state where required, reconstruction, and Tracktion linkage 0. Move this plan to `docs/exec-plans/completed/option-b-mixed-rate-src.md` only then.

If the gate passes, recommend exactly **FFmpeg/container boundary**. Otherwise recommend one narrowly named SRC corrective gate.
