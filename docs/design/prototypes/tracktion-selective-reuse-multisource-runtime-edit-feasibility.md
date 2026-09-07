# Option A2 Multi-source / Runtime-edit Source Scheduling Feasibility

## Status

Planned feasibility gate only. This document authorises no implementation and makes no production architecture decision. It follows the completed realtime-source gate, whose bounded one-source result is **Proceed with Constraints** and whose source subsystem is a **substantial adapter subsystem**.

## Question and retained boundary

This gate answers one question:

> Can Option A2 scale from one realtime source adapter to realistic multi-Clip editing workloads while preserving exact integer scheduling, bounded callback behavior, cache/worker ownership, and deterministic runtime edit invalidation without the source subsystem growing into custom-engine-scale scheduling infrastructure?

The authoritative direction remains one-way:

```text
AudioNLE Domain
  -> AudioNLE integer Timeline / Source scheduling
  -> AudioNLE source request planning
  -> AudioNLE realtime prefetch/cache/worker subsystem
  -> custom realtime source Node(s)
  -> public Tracktion low-level graph
  -> mix / processing / PDC / VST3 / Tail
```

AudioNLE owns editing semantics and integer sample positions. Tracktion high-level Clip/source scheduling remains excluded: no `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, high-level source scheduler, private API, patch, or fork may become an authority.

This gate does not re-prove the already established framework-free Domain; same-rate integer mapping; low-level graph processing and `SummingNode`; deterministic and actual VST3 PDC/Tail; offline WAV reads; one-hour bounded reads; or one-source realtime pages, miss policy, generation invalidation, destruction, reconstruction, and downstream Multiply processing.

## Scope

Included fixture constraints:

* mono, local PCM16 WAV at 48 kHz Source and Timeline rate;
* existing realtime source adapter and JUCE reader-worker boundary;
* public Tracktion low-level graph with a 128-frame process baseline;
* controlled/stopped runtime edits without GUI;
* bounded same-file and different-file, overlapping, and progressively dense Clip cases.

Excluded: FFmpeg, compressed/container media, mixed-rate SRC, audio devices, GUI, waveform/proxy, export, persistence, full ripple, Synchronization Groups, production cache sizing, ADR, and final architecture selection. VST3/PDC/Tail are not revalidated unless an edit-invalidation interaction specifically requires it.

## Vocabulary and invariants

These identities must remain separate:

| Concept | Meaning | Must not be conflated with |
| --- | --- | --- |
| `MediaSource` runtime | runtime representation of a media identity and its decoder/cache service | Clip placement |
| Clip runtime view | transient consumer view of one Domain Clip and its revision | media decoder/cache identity |
| Timeline placement | integer Timeline range of a Clip | Source sample identity |
| Source range request | integer Source range required by a view | Timeline position |
| Cache ownership | owner of bounded decoded PCM pages | Domain ownership |
| Worker ownership | owner of reader/decoder access and worker lifecycle | callback ownership |

The editing model remains authoritative: a Clip is a non-destructive mapping of a Source range to a Timeline range; same-track overlaps sum; Move preserves Source reference/range; Trim changes corresponding Source/Timeline boundaries and may re-expand; Split produces same-SourceReference segments whose Timeline and Source unions equal the original; Delete removes only the Clip and never mutates media.

Hard invariants:

1. Source and Timeline positions are signed integer sample domains; required timing error is exactly 0 Timeline samples.
2. Two Clip views of identical Source samples at distinct Timeline placements must produce the correct samples at both placements. Source identity is never a cache key substitute for placement.
3. Domain persists no worker, file handle, cache page, queue, graph object, runtime revision, or Tracktion/JUCE type.
4. Callback consumption only reads published fixed storage and writes its supplied output. It performs no file/decode access, allocation/growth, blocking/wait/spin, or synchronous miss fallback.
5. A miss is exactly `zero output + underrun`; it cannot expose stale or uninitialised audio.
6. Request storage, page storage, pending work, and worker count all have explicit bounds.
7. Edit-invalidated or deleted runtime state cannot publish or emit later; shared MediaSource state remains live while any current Clip view needs it.
8. Same final Domain reconstructed into a fresh runtime yields the same defined output observations.

## M0: architecture and ownership investigation

Before implementation, inspect pinned Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`, nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`, and the existing realtime adapter. Record public/private status and source locations in `benchmark-results/tracktion-selective-reuse/multisource-api-investigation.md`.

Answer:

* whether the custom realtime adapter/cache can serve multiple Clip Nodes safely;
* `SimpleNodePlayer` and graph-processing restrictions for many source Nodes;
* how Nodes request one shared cache without callback arbitration work;
* whether one worker-owned `AudioFormatReader` can safely service shared-file reads;
* which public low-level Tracktion primitives help with many Nodes; and
* whether any public Tracktion cache/scheduler primitive can be reused without surrendering Timeline authority.

Compare these models before selecting a fixture implementation:

| Model | Shape | Required evaluation |
| --- | --- | --- |
| A: per media | one worker/cache per MediaSource; many Clip views | duplicate reads, range sharing, worker/cache count, independent seeks, ownership complexity |
| B: per Clip | one worker/cache per active Clip | simple isolation versus duplicate handles/PCM, worker explosion, same-file overlap cost |
| C: shared service | bounded arbitration over shared media workers/caches | priority, coalescing, fairness/starvation, bounded queueing, invalidation, custom-scheduler complexity |

Model C is not preferred merely for scalability. Record why the smallest viable model was selected, what it cannot prove, and whether a later model is warranted.

## M1: same-file multiple Clip views

Use one WAV with at least:

```text
Clip A: Source X -> Timeline T1
Clip B: Source Y -> Timeline T2
Clip C: Source X or overlapping X -> Timeline T3
```

Exercise disjoint, overlapping, and identical Source ranges at different Timeline placements. Markers and amplitudes must be exact, no Clip may see another view's stale PCM, and callback counters remain zero. The identical-range/different-placement case is a hard identity test.

## M2: different-file multiple sources

Use at least Media A, B, and C with overlapping Timeline output. Feed public `SummingNode` (or an already established equivalent) and verify the expected linear sum at every overlap.

Record worker count, cache count and bytes, open file handles where observable, request volume, pending request bound, underruns, and callback counters. State whether the selected model has one worker/cache per media, per Clip, or another bounded formula.

## M3: dense workload matrix

Use progressive active-Clip levels of 4, 8, 16, and 32 unless an earlier hard stop makes continuation unsafe. Test each density dimension separately:

```text
many Clips / one media
many Clips / several media
many Clips / all distinct media
```

This is responsibility/scaling evidence, not a dropout or throughput benchmark. At each point measure active Nodes, unique media, workers, cache memory, pending requests, underruns, callback forbidden-operation counters, marker error, stale output, working set, and graph complexity. Explain scaling with formulas, not only measurements.

## M4: runtime edit — Move

Perform a controlled running or stopped-state edit without GUI:

```text
Clip A Timeline 10000
move
Clip A Timeline 50000
```

The old placement must be silent; the new placement and Source mapping must be exact; old prefetched output must not leak. The Domain change, not a runtime cache, is authoritative.

## M5: runtime edit — Trim

Mutate/rebuild framework-free Domain fixture state rather than implementing full editor commands. For Source `[S0,S1)` mapped to Timeline `[T0,T1)`, test left trim and right trim; test re-expansion if feasible. Check exact changed boundaries, no output outside the revised range, and correct retained mapping. Preserve the editing model's non-destructive semantics.

## M6: Split

Split a same-rate logical Clip at an integer internal Timeline sample. Verify the resulting left/right views use the same MediaSource, preserve the original Timeline and Source unions, map the split boundary exactly, and do not duplicate or lose a sample. Do not invent a distinct split or synchronization model for the fixture.

## M7: Delete and shared-media lifetime

Delete one active Clip while unrelated Clip views, including another view of the same media, continue. The deleted output must disappear; unrelated output remains exact; pending deleted-view requests/pages cannot leak; and shared media worker/cache lifetime survives until its final current consumer releases it. Then destroy the last view and verify bounded worker/cache teardown.

## M8: rapid edit invalidation and edit strategy comparison

With worker activity deliberately pending, execute a bounded sequence:

```text
move A -> trim B -> delete C -> move A again
```

Record Domain revision, affected Clip-view revision, reader/request generation, issued/completed/discarded work, underruns, and final exact observations. Do not merge all invalidation into one global counter without explaining loss of scope or lifetime information.

Compare:

| Strategy | Required assessment |
| --- | --- |
| Full stopped-state graph rebuild | correctness, teardown/recreation bounds, retained Tracktion graph value |
| Partial runtime update | changed-view isolation, stale state, graph mutation complexity, public API sufficiency |

For the initial fixture, a full rebuild is an acceptable control. If reliable editing requires a general graph scheduler/mutator duplicating Tracktion's low-level graph value, record a major A2 warning rather than concealing it with fixture code.

## M9: cache/request arbitration

If a shared MediaSource cache is selected, instrument maximum outstanding requests, fixed request capacity, overwrite/coalescing policy, duplicate and overlapping range treatment, page reuse/eviction, priority ordering, and starvation. A bounded request slot or fixed queue is required; callbacks may publish a bounded request signal but cannot wait for its servicing. A per-media or per-Clip model must still record its request bound and explain duplicate-read behavior.

Retain the existing miss semantics:

```text
miss -> exact zero + underrun
```

No callback I/O or synchronous decode fallback is permitted.

## M10: reconstruction

From the same initial Domain, perform dense observations and edits, destroy runtime state, then build a fresh runtime from the same final Domain. Compare final output, marker locations, source requests, clip presence/absence, and intentional miss/underrun observations. Cache history, worker state, and graph mutation must not become hidden authority.

## M11: complexity and scaling review

Create `benchmark-results/tracktion-selective-reuse/multisource-runtime-edit-complexity.md` with separate LOC estimates for custom scheduling, MediaSource runtime, Clip runtime views, arbitration, edit invalidation, lifecycle, and Tracktion integration. Record worker strategy, cache scaling, file-handle strategy, fixed request bounds, synchronization primitives, callback operations, retained public Tracktion reuse, and future FFmpeg implications.

State explicit formulas, for example:

```text
cache PCM bytes = unique MediaSource runtimes * pages per runtime * frames per page * channels * sizeof(float)
workers = unique MediaSource runtimes                 (Model A)

cache PCM bytes = active Clip views * pages per view * frames per page * channels * sizeof(float)
workers = active Clip views                            (Model B)
```

For Model C, define bounds in terms of configured media workers, page pools, and request slots. Do not hide a scaling term in a dynamic container.

Classify the resulting subsystem as **Bounded adapter extension**, **Substantial adapter extension**, or **Custom-engine-scale warning**, then classify the gate as **Proceed**, **Proceed with Constraints**, **Reject**, or **Inconclusive**.

## Hard rejection signals

Reject on callback file/decode access, callback blocking/spin, callback allocation/growth, unbounded backlog, stale audio after edit, wrong Source/Timeline mapping, nonzero sample error, deleted Clip reappearance, shared-media lifetime corruption, unintended cache growth with Clip count, clearly impractical per-Clip workers, custom-engine-scale global scheduling, private Tracktion API, necessary high-level Tracktion scheduling, reversed Domain/runtime authority, or graph mutation complexity that removes material Tracktion reuse value.

## Evidence and later result document

Later implementation, if separately authorised, must produce:

```text
benchmark-results/tracktion-selective-reuse/
  multisource-api-investigation.md
  multisource.md
  runtime-edit.md
  multisource-stress.md
  multisource-runtime-edit-complexity.md

docs/design/prototypes/
  tracktion-selective-reuse-multisource-runtime-edit-results.md
```

The results document must cover recommendation; pins/scope; ownership model; MediaSource versus Clip-view distinction; same-file and different-file cases; density; worker/cache formulas; Move, Trim, Split, Delete, and rapid invalidation; arbitration; callback invariants; reconstruction; complexity; Tracktion reuse; FFmpeg/mixed-rate implications; remaining risks; and next gate.

## Completion criteria for this planning task

This planning task is complete only when the later gate has an unambiguous M0–M11 sequence, hard-stop signals, explicit bounded ownership/scaling evidence, editing-model-consistent operations, and decision criteria. It changes no implementation, CMake, dependency, submodule, benchmark result, ADR, production source, or final architecture decision.
