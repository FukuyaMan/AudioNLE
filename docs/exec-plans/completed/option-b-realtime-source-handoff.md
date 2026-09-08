# Execution Plan: Option B Realtime Source Handoff

## Objective

## Completion

Completed. R0--R10 pass in the bounded fixture. R10 observed duration 172800001 samples, fixed 8224-byte cache, fixed 512-byte SourceNode scratch, full-source PCM retained 0, and representative working sets 8253440/8253440/8249344 bytes. Gate `Proceed with Constraints`; integration `Thin backend handoff`; maximum timing error 0. The bounded latest-request limitation and production multi-range arbitration remain deferred. Standard build compiled/linked the target; full suite retained only the known unrelated Tracktion Phase E failure.

Execute the post-ADR-0001 bounded handoff gate described in [the feasibility plan](../../design/prototypes/option-b-realtime-source-handoff-feasibility.md): prove or reject that an AudioNLE-owned realtime source/cache/worker model can feed the selected Option B graph through a narrow source-node contract without losing integer authority, callback safety, bounded memory, reconstruction, or backend independence.

This is not an architecture comparison. ADR 0001 remains accepted and unchanged. The gate must classify both the outcome (`Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`) and handoff complexity (`Thin backend handoff`, `Substantial backend adaptation`, or `Common-core assumption challenged`).

## Scope and fixed model

Use a repository-local temporary mono PCM16 WAV fixture at 48 kHz, Option B graph blocks of 128 frames, and fixed source cache pages of 257 frames. Decode expectations use observed PCM16-decoded samples, not fixture literals. Use a bounded single latest-request publication slot only as a handoff-fixture mechanism; production multi-range arbitration remains deferred.

In scope:

```text
real WAV -> worker/cache -> ClipRuntimeView -> Option B SourceNode -> B graph
-> optional Multiply(2) -> B sum -> headless observation
```

Include source readiness/miss behavior, generation invalidation, page ownership, cross-page reads, same-source two-view sum, stopped reconstruction, and one-hour boundedness observation.

Exclude production worker pools, multi-range arbitration redesign, 32-media stress, FFmpeg/compressed/container media, mixed-rate SRC, devices, live mutation, plugin state/scanning, persistence, GUI, and Tracktion.

## Required ownership boundary

Persistent/framework-free descriptions contain MediaSource identity, Clip identity, Source range, Timeline placement, and Clip revision only. They do not contain runtime readers, workers, page/cache addresses, graph Nodes/buffers, JUCE plugin objects, PDC/Tail state, host reports, or any other runtime identity.

```text
ClipRuntimeView
  owns Timeline placement, Source range, integer Timeline-to-Source mapping, Clip revision

MediaSourceRuntime
  owns Source identity, fixed decoded pages, worker/reader, MediaGeneration, readiness

Option B SourceNode
  owns fixed graph-facing scratch/output, nonblocking runtime calls, zero/underrun output,
  instrumentation, and graph-buffer forwarding
```

MediaGeneration invalidates reader/cache publication after source/seek changes. Clip revision changes placement/range/view state only. Cache keys are MediaSource identity plus Source-sample page range; Timeline positions and graph identities must not enter them.

## R0: audit and implementation strategy

Audit `prototype/tracktion-selective-reuse/src/realtime_source_main.cpp` before writing B code. Record in `benchmark-results/option-b/source-handoff-api-investigation.md`:

| Classification | Expected treatment |
| --- | --- |
| Fixed pages, Source-page identity, capacity, worker/reader ownership, request publication, generation discard, reader counts, zero underrun | backend-independent by concept; re-specify and remeasure |
| `RealtimeSourceNode`, Tracktion `ProcessContext`, Tracktion buffer/MIDI access, `SimpleNodePlayer` | Tracktion-specific; replace with B adapter |
| WAV creation, worker hold control, observations/assertions/CLI | test-only |

The implementation strategy is fixed: **conceptual reuse only; keep prototypes separate**. Do not copy the monolithic A2 fixture and do not extract a shared production component before handoff evidence exists.

## R1: fixture-local runtime contract

Define and document a fixture-local equivalent of:

```text
MediaSourceRuntimeView
  publishRequest(SourceSample, FrameCount, MediaGeneration) -> void
  copyPreparedRange(SourceSample, FrameCount, MediaGeneration, callerProvidedOutput)
    -> Complete | NotReady
```

The contract accepts integer Source samples and a caller-provided fixed buffer. It accepts no Timeline sample, Node pointer, graph buffer identity, plugin object, PDC/Tail state, or float seconds. `copyPreparedRange` cannot read/decode, wait/block/spin, allocate/grow, or synchronously fill a miss. On `NotReady`, SourceNode emits exact zero for uncovered frames, increments underrun, and nonblockingly publishes bounded demand.

## R2–R10 implementation and assertions

1. **R2 — one real WAV:** Connect a fresh worker/cache, ClipRuntimeView, B SourceNode, and headless B graph. Check beginning, page boundary, block boundary, far seek, and one-hour marker when practical. Require maximum timing error 0 Timeline samples.
2. **R3 — callback invariants:** At the B SourceNode boundary count source-node calls, callback reader/file/decode calls, waits/blocks/spins, allocations/growth, synchronous miss fallbacks, hits/misses/underruns, worker reader calls, and old-generation discards. Require callback reader/file/decode, waits/blocks/spins, allocations/growth, and fallback all zero.
3. **R4 — deliberate miss/recovery:** Hold the worker outside process execution; verify `NotReady -> exact zero + underrun`, release worker, then verify a later call produces the exact decoded value. No synchronous read or callback wait is permitted.
4. **R5 — generation and clip revision:** Hold request A, increment MediaGeneration for B, allow A completion, require discard/no stale audio, then render B exactly. Separately change only Clip placement/revision and show page identity stays Source-based.
5. **R6 — geometry:** With 257-frame pages and 128-frame graph blocks, exercise boundary-crossing and block-interior marker reads. Do not assume a page-aligned block or one page per call.
6. **R7 — downstream processing:** Route real source through existing B Multiply(2). Require output = observed decoded PCM × 2 at zero timing error. Do not re-prove PDC/VST3/Tail.
7. **R8 — same-source multi-view sum:** Attach two B SourceNodes/ClipRuntimeViews to one MediaSource runtime and existing B summing. Check A-only, overlap, B-only, linear sum, shared runtime identity, and Source-based cache keys. Do not run density/scaling matrices.
8. **R9 — reconstruction:** From unchanged framework-free descriptions, build fresh reader, worker/cache, views, nodes, and B graph twice. Compare relevant complete output, integer timing, Clip mapping, and cache-independent logical observations.
9. **R10 — long source:** Use a one-hour-plus WAV when feasible. Record duration, fixed page count/cache bytes, SourceNode scratch bytes, working set, and full-source PCM retained bytes. Require retained full-source PCM = 0 and no duration-scaled cache or scratch.

## Process-time invariants and hard stops

At the SourceNode boundary, filesystem/reader/decode calls, waiting/blocking/spinning, dynamic allocation/growth, and synchronous miss fallback are each zero. Fixed copying from ready pages is allowed. Request storage is bounded; no queue/vector/list may grow to retain demand.

Stop before adding scope if any required timing error is nonzero; callback performs reader/file/decode work, waits/spins, grows/allocates, or uses fallback; stale-generation samples appear; Timeline or graph identity enters MediaSource cache/runtime; request storage becomes unbounded; full-source PCM or duration-scaled scratch appears; Domain gains runtime identity; Tracktion enters the B target; or passing would require a production arbitration redesign.

## Planned files

Expected implementation files are limited to Option B prototype CMake/source and test targets under `prototype/option-b-low-level-graph/`; exact names follow the R1 contract. Do not modify A2 sources, dependencies, submodules, production source, or ADRs.

Expected evidence files:

```text
benchmark-results/option-b/source-handoff-api-investigation.md
benchmark-results/option-b/source-handoff.md
benchmark-results/option-b/source-handoff-complexity.md
docs/design/prototypes/option-b-realtime-source-handoff-results.md
```

The complexity record separates common-source candidates (pages/cache, worker/reader, request/generation, Clip view/mapping) from B-specific source node/buffer handoff/graph lifecycle/reconstruction, then from fixture/assertion/instrumentation. Do not total them into production LOC.

## Verification and completion

Run `build.ps1`, targeted source-handoff CTest, direct executable, repeated deterministic execution, `git diff --check`, Tracktion reference/linkage review, and Domain/runtime-leakage review. The existing unrelated high-level Tracktion Phase E failure remains separate.

Pass requires all R0–R10 assertions, timing error 0, callback invariants preserved, bounded request/cache/scratch behavior, no Tracktion linkage, and documented source-reuse classification. Write the result and complexity records, then move this plan to `docs/exec-plans/completed/option-b-realtime-source-handoff.md`.

If the result is a thin handoff at zero error, recommend exactly **B multi-source arbitration / worker scaling**. Otherwise recommend one narrowly named correction for the failed handoff boundary.
