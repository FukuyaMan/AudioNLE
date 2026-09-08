# Option B Realtime Source Handoff Feasibility

## Status and objective

This is the first post-ADR-0001 validation plan. It does not implement a source subsystem, extract a shared library, change CMake/dependencies, or alter the selected backend. It asks one bounded question:

> Can the AudioNLE-owned realtime source/cache/worker model feed the selected Option B graph through a narrow source-node contract while preserving integer scheduling, callback invariants, bounded cache behavior, reconstruction, and backend independence?

The selected architecture remains:

```text
Domain -> integer Timeline/Source scheduling -> MediaSource runtime/cache/worker
-> Clip runtime view -> source-node contract -> Option B graph
-> processing/summing/PDC/VST3/Tail
```

This is a handoff validation, not a redesign of source arbitration, worker pools, FFmpeg, mixed-rate SRC, devices, live mutation, plugin management, persistence, or GUI.

## ADR context and evidence discipline

[ADR 0001](../../decisions/0001-audio-engine-backend-ownership.md) selects the AudioNLE low-level graph with JUCE direct hosting. It accepts that the source core is principally AudioNLE-owned, but explicitly requires validation of its B backend handoff. The prior A2 source results are design reference only, not evidence that B integration already works.

The A2 realtime fixture established worker-owned JUCE WAV range reads, fixed pages, bounded capacity, page ownership, nonblocking callback supply, exact-zero underrun, generation invalidation, awkward page/block geometry, bounded long-source behavior, and reconstruction. See [A2 realtime source results](tracktion-selective-reuse-realtime-source-results.md), [real-media results](tracktion-selective-reuse-real-media-results.md), and [multi-source/edit results](tracktion-selective-reuse-multisource-runtime-edit-results.md). Option B independently established a bounded graph/PDC, direct VST3 host/PDC, and finite Tail backend; see its [graph](option-b-low-level-graph-pdc-results.md), [VST3/PDC](option-b-vst3-pdc-results.md), and [Tail](option-b-vst3-tail-results.md) results.

Every result must state whether it is newly measured for B, inferred as a common-core design rule, or still untested.

## R0: existing source subsystem audit and reuse decision

The existing `prototype/tracktion-selective-reuse/src/realtime_source_main.cpp` is a monolithic feasibility fixture, not a reusable component.

| Classification | Existing fixture surface | Handoff treatment |
| --- | --- | --- |
| Backend-independent candidate by concept | fixed `Page` state, page identity by Source sample, cache capacity, `WorkerReader`, worker ownership, request publication, generation discard, reader-count ownership, exact-zero underrun rule | Re-specify and remeasure behind B contract |
| Tracktion-specific | `RealtimeSourceNode`, `tracktion::graph::Node`, `ProcessContext`, Tracktion buffer/MIDI access, `SimpleNodePlayer` integration | Replace with Option B source node only |
| Test-only | temporary WAV writer, held-worker controls, waits used by test coordination, working-set observation, assertions/counters/CLI | Keep local to B fixture as needed |

`DomainState` in that fixture mixes source identity/range with Timeline placement for the A2 node. The B plan must split this: MediaSource runtime receives only Source identity/address; Clip runtime view owns Timeline placement and Timeline-to-Source mapping.

**Reuse recommendation: Conceptual reuse only; keep prototype code separate for now.** The A2 cache/worker model is a credible common-core design, but its current implementation is intentionally fixture-shaped, uses JUCE reader/thread primitives, and embeds test controls. Copying it would duplicate accidental fixture structure; extracting it before the B handoff is proven would create a premature production abstraction. R0 must record the future audit in `benchmark-results/option-b/source-handoff-api-investigation.md`.

## Ownership and narrow source-node contract

The following runtime contract is the maximum shared boundary proposed for this gate:

```text
MediaSourceRuntimeView
  publishRequest(SourceSample start, FrameCount frames, MediaGeneration generation) -> void
  copyPreparedRange(SourceSample start, FrameCount frames, MediaGeneration generation,
                    callerProvidedOutput) -> Complete | NotReady
```

`SourceSample`, `FrameCount`, and `MediaGeneration` are integer values. `callerProvidedOutput` is a fixed source-node-owned buffer or graph-provided fixed buffer; the contract neither allocates nor stores graph buffer identity. `copyPreparedRange` must not read/decode files, wait/block/spin, or synchronously fill a miss. It may copy across fixed pages only when all required page ownership/generation checks pass. `NotReady` causes exact zero for the uncovered frames, an underrun counter increment, and a bounded nonblocking request publication.

The contract has no float seconds, Timeline position, graph Node pointer, JUCE plugin object, PDC metadata, Tail state, or persistent Domain mutation.

```text
ClipRuntimeView owns: Timeline placement, Source range, integer Timeline-to-Source mapping,
                      clip revision, and segmentation of a graph request.
MediaSourceRuntime owns: Source identity, decoded pages, reader/worker, media generation,
                          fixed cache identity, and request readiness.
Option B SourceNode owns: caller buffer, call instrumentation, zero/underrun rendering,
                          and forwarding prepared samples into graph buffer flow.
```

Cache keys must be Source sample ranges and MediaSource identity, never Timeline position. Media generation invalidates decoded-page publication after source/seek changes; Clip revision is separately owned by the Clip view for placement/range changes. No global Domain generation is introduced unless a later design demonstrates why the two scopes cannot remain separate.

## Fixed feasibility model and instrumentation

Use a temporary mono 48 kHz PCM16 WAV and the existing marker pattern: beginning, page and block boundaries, far seek, and the one-hour marker. Use 128-frame Option B graph processing and include a non-divisible geometry of 257 frames/page to exercise cross-page reads. All expected amplitudes after PCM16 decoding must use observed decoded samples, not pre-quantisation fixture literals.

At the B SourceNode boundary, counters must be owned by the fixture and reset/read only outside the process path:

```text
callback/source-node calls
callback reader/file/decode calls
callback waits/blocks/spins
callback allocations/growth
synchronous miss fallback
cache hits/misses/underruns
worker reader calls
old MediaGeneration discards
```

Required callback values are reader/file/decode = 0, waits/blocks/spins = 0, allocations/growth = 0, and synchronous miss fallback = 0. The worker may read/decode only after a published request. Counters do not claim that arbitrary graph processors or plugins allocate-free.

## Planned bounded gates

### R1: contract-only audit

Before integration, compile the contract into a fixture-local B source runtime/view/node split. Confirm that only the Clip view receives Timeline samples, only the runtime receives Source addresses, and B graph code sees neither reader nor cache internals. Record exact API, ownership, lifecycle, and no-Tracktion boundary in `source-handoff-api-investigation.md`.

### R2: one real WAV through the B graph

Run:

```text
real WAV -> worker/cache -> Clip runtime view -> Option B SourceNode -> headless output
```

Verify markers near beginning, page boundary, block boundary, a far seek, and the one-hour marker when the fixture remains practical. Each required marker must have maximum Timeline timing error 0. The source node must derive requested Source range only from its Clip view's integer mapping.

### R3: callback invariants and R4 miss/recovery

For normal ready pages, assert all callback invariants above. Then deliberately hold a worker completion:

```text
publish request -> page NotReady -> exact zero + underrun -> worker fills page
-> later process call obtains exact decoded source sample
```

No source-node path may call the reader, wait for readiness, spin, allocate, or perform a synchronous fallback. The test harness may coordinate the hold outside the process path only.

### R5: seek and generation invalidation

Exercise request A, held/pending worker work, seek to B, MediaGeneration increment, then old A completion. The old completion must be discarded rather than published as current audio; B must subsequently render exactly. Independently change a Clip view revision/placement in a stopped rebuild case to prove that a clip edit does not redefine MediaSource page identity.

### R6: cross-page and block-interior access

With 257-frame pages and 128-frame graph blocks, test a request that crosses one or more page boundaries and a marker whose Timeline placement lies inside a graph block. Require exact decoded samples and no alignment assumption in the B SourceNode.

### R7: downstream graph processing

Run `real source -> Option B Multiply(2) -> headless output`. At a decoded PCM16 marker, require observed downstream amplitude = observed decoded source amplitude × 2 and timing error 0. This validates source-buffer handoff into existing B graph flow; it does not re-prove PDC, hosting, or Tail.

### R8: same-source multi-view summing

Use one MediaSource runtime with two distinct Clip runtime views and placements, including one overlap when practical:

```text
MediaSource X -> Clip view A -> B SourceNode A
              -> Clip view B -> B SourceNode B -> Option B sum
```

Verify source-view identity, exact linear output in unique and overlap regions, and no Timeline location in the MediaSource cache key. Do not repeat the 4/8/16/32 density matrix or redesign arbitration in this gate.

### R9: reconstruction and R10 bounded long-source observation

From framework-free descriptions containing MediaSource identity, Clip identity, Source range, and Timeline placement, construct fresh reader/worker/cache, Clip views, B nodes, and graph. Render observation A, destroy all runtime state, reconstruct from the unchanged description, and require equal output and timing.

For the one-hour-plus fixture, record fixed source cache bytes, B source-node scratch bytes, and working-set observation. Reject duration-scaled decoded PCM, graph-sized source cache, or duration-scaled scratch. Do not claim OS I/O/dropout guarantees.

## Exclusions and hard stops

Excluded: production multi-range arbitration, 32-distinct-media stress, worker pools, FFmpeg/compressed/container media, mixed-rate SRC, device output, live mutation, plugin scanning/state, persistence, GUI, and any Tracktion use.

Stop and classify the gate as Reject or Inconclusive before broadening scope if exact timing fails; callback reader/file/decode work, waiting/spinning, growth/allocation, or synchronous miss fallback occurs; cache identity receives Timeline placement or graph internals; stale generation audio appears; unbounded request storage is required; source memory scales with duration; Tracktion enters the B target; or Domain receives runtime identity.

## Results, complexity, and completion criteria

Later implementation must produce:

```text
benchmark-results/option-b/source-handoff-api-investigation.md
benchmark-results/option-b/source-handoff.md
benchmark-results/option-b/source-handoff-complexity.md
docs/design/prototypes/option-b-realtime-source-handoff-results.md
```

Complexity must separate source-independent candidate responsibilities (pages/cache, worker/reader, requests/generation, Clip view) from B-specific responsibilities (SourceNode adapter, fixed-buffer handoff, graph lifecycle/reconstruction), and both from fixture/assertion/instrumentation code. The central result is how much is backend-independent versus newly required for B, not a fake production LOC total.

Classify the gate as `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`; integration as `Thin backend handoff`, `Substantial backend adaptation`, or `Common-core assumption challenged`. A pass requires timing error 0, callback invariants preserved, exact miss/recovery, stale-generation rejection, cross-page correctness, downstream processing, same-source multi-view summing, deterministic reconstruction, bounded long-source observation, and zero Tracktion linkage.

If the result is a thin handoff at zero error, the single next recommended gate is **B multi-source arbitration / worker scaling**, because the selected architecture inherits the known per-MediaSource worker/cache risk. Otherwise its next action must address the failed handoff boundary rather than start a different deferred gate.
