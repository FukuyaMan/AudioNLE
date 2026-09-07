# Audio Engine Architecture Comparison / Decision Preparation

## Status and scope

This is decision-preparation material, not an ADR or a production architecture selection. It compares established Option A2 evidence with the conceptual, unproven Option B shape. It makes no implementation change or claim that Option B has equivalent prototype evidence.

The product target remains a long-form, multi-track, audio-only NLE with framework-independent, non-destructive editing semantics and integer Timeline/Source sample authority. MIDI/DAW breadth and video editing remain out of scope.

## Compared shapes

```text
A2: Domain -> integer scheduling -> source request planning -> MediaSource runtime/cache/worker
    -> Clip views -> custom source Nodes -> Tracktion low-level graph -> mix/processing/PDC/VST3/Tail

B:  Domain -> integer scheduling -> source runtime/cache/decoder -> AudioNLE graph/scheduler
    -> JUCE/direct plugin hosting -> mix/processing/PDC/Tail
```

Option B is conceptual/unproven. A2 measurements must not be attributed to it.

## A2 evidence summary

The Domain is framework-free; same-rate scheduling is exact, and a deterministic mixed-rate integer mapping fixture exists. Public Tracktion low-level `Node`, `SimpleNodePlayer`, graph traversal/buffer flow and `SummingNode` were exercised with ordered processing, deterministic and actual-VST3 PDC, deterministic and actual-VST3 finite Tail, and headless output.

The source evidence covers exact WAV range reads, one-hour bounded-memory observation, fixed realtime pages, zero callback reader/wait/allocation counters, zero-plus-underrun miss policy, generation invalidation, page ownership, rapid seek, destruction/reconstruction, and non-divisible/cross-page access. The realtime source subsystem is a **substantial adapter subsystem**.

For multi-source/edit, Model A was selected: one worker/cache per MediaSource with many Clip runtime views. Same-file identity/overlap, different-file summing, 4/8/16/32 Clip density, Move, Trim/re-expand, Split, Delete, rapid-edit stopped rebuild, bounded one-slot arbitration, and reconstruction passed. The multi-source/edit work is a **substantial adapter extension**.

## Known A2 constraints

* Tracktion high-level source scheduling is unsuitable for this strict source-timing boundary; AudioNLE owns source timing, cache and prefetch.
* Model A scales workers, readers/files and caches with unique media: the 32-distinct-media fixture reached 32 workers/caches.
* One latest-request slot per media can overwrite a still-needed unserved range; production bounded multi-range arbitration is unresolved.
* Persistent partial graph update is unresolved; stopped/affected rebuild is the correctness reference.
* Device callback, real-media 44.1→48 SRC, FFmpeg/compressed/container media, and production worker-pool behavior are untested.

## Responsibility matrix

| Responsibility | A2 owner | A2 evidence | Option B likely owner | B evidence level |
| --- | --- | --- | --- | --- |
| Domain | AudioNLE | Strong, framework-free | AudioNLE | Conceptual / unproven |
| Edit semantics | AudioNLE | Move/Trim/Split/Delete fixture | AudioNLE | Conceptual / unproven |
| Timeline scheduler | AudioNLE | Exact integer fixture | AudioNLE | Conceptual / unproven |
| Source cache | AudioNLE | Fixed pages, bounded Model A | AudioNLE | Conceptual / unproven |
| Decoder worker | AudioNLE + JUCE | WAV worker only | AudioNLE + decoder | Conceptual / unproven |
| Graph traversal | Tracktion | Public graph exercised | AudioNLE | Conceptual / unproven |
| Mixing | Tracktion | `SummingNode` | AudioNLE | Conceptual / unproven |
| Processing stack | Tracktion graph + adapter | Ordered processing | AudioNLE/JUCE | Conceptual / unproven |
| Plugin hosting | Tracktion path | Bounded actual VST3 | AudioNLE/JUCE/direct | Conceptual / unproven |
| PDC | Tracktion | Deterministic + actual VST3 | AudioNLE | Conceptual / unproven |
| Tail propagation | Tracktion | Deterministic + actual VST3 finite tail | AudioNLE | Conceptual / unproven |
| Runtime graph mutation | AudioNLE boundary / unresolved | Rebuild only | AudioNLE | Conceptual / unproven |
| Persistence boundary | AudioNLE | Domain/runtime separation | AudioNLE | Conceptual / unproven |

## Implementation burden and Tracktion value

Measured A2 prototype surfaces are not production LOC estimates: the realtime source fixture is ~470 lines, with fixed cache/page ownership ~110, worker/reader ~85, source node ~65, and invalidation/request/cancellation ~65; the multi-source fixture is ~320 lines, with MediaSource runtime ~90, request handling ~30, Clip mapping ~35, and edit/rebuild/lifetime ~100. Fixture, assertions and instrumentation (~145 and ~120 respectively) are deliberately separate and estimates overlap.

Option B LOC must not be invented. It would likely add AudioNLE responsibility for graph traversal, graph buffer scheduling, summing, ordered processing graph lifecycle, PDC, plugin graph integration, Tail scheduling and runtime lifecycle—work A2 presently reuses from Tracktion. Those are engine-critical, not superficial convenience. A2 therefore still avoids most graph-engine infrastructure even though it owns a substantial source subsystem.

Tracktion also costs a framework dependency, low-level graph API constraints, inability to use its high-level source path, JUCE/Tracktion coupling, unresolved persistent mutation behavior, and licensing/distribution review. Option B might unify source and graph scheduler ownership and permit a domain-designed live mutation model, but these are hypotheses rather than measured benefits. Its likely cost is rebuilding the engine-critical responsibilities above.

## Decisions needed before an ADR

| Area | A2 evidence | B evidence | Architecture decision blocker? |
| --- | --- | --- | --- |
| Domain | Strong | Conceptual | No |
| Source timing / realtime cache | Strong | Conceptual | No, comparison-ready |
| Multi-source | Bounded strong; scaling constraint | Conceptual | Yes, material risk |
| Processing / PDC / VST3 / Tail | Strong or bounded strong | Conceptual | Yes, B lacks comparison evidence |
| Live mutation | Constrained | Conceptual | No; useful follow-up |
| Mixed-rate real media / compressed media | Weak | Weak | No for architecture preparation; later product gate |

**Worker-pool/shared decoder arbitration before comparison: No.** Existing evidence is enough to prepare an architecture comparison, and implementing it first would be significant A2-specific investment. A small design investigation may refine its requirements, but no implementation should precede the comparison.

**Persistent partial graph mutation: useful but not required.** Stopped rebuild establishes a viable correctness strategy; the unresolved live-edit practicality should be evaluated as a decision criterion rather than silently solved in A2 first.

**Actual 44.1→48 real-media SRC: useful but not required.** Both options lack it, so it does not distinguish them yet.

## Future ADR criteria

Compare exact sample authority, implementation complexity, realtime source design, multi-source scaling, edit mutation, plugin hosting, PDC, Tail, long-form media, FFmpeg integration, debugging/testing, dependency surface, licensing, and small-OSS maintainability. The evaluation must distinguish fixture evidence from production guarantees.

## Selected outcome and next bounded work

**Outcome B — Run one narrowly bounded Option B prototype before decision preparation.** A2 evidence is strong enough to identify what Tracktion saves, but no evidence measures the cost of replacing it. The prototype should be limited to framework-free Domain -> custom low-level graph -> two sources -> summing -> multiply -> deterministic latency/PDC -> headless output. It must not include VST3, FFmpeg, a full editor, or a production scheduler.

This is not an A2 rejection. It is the smallest experiment that can convert Option B from inference into comparable evidence; worker-pool arbitration remains the highest subsequent A2 scaling risk.
