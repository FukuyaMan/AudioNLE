# Audio Engine ADR Comparison Preparation: A2 vs Option B

## Status and scope

This is ADR-level comparison preparation, not an ADR and not a production architecture selection. It organizes completed feasibility evidence for a long-form, multi-track, audio-only NLE whose Domain remains framework-independent, non-destructive, and authoritative in integer Timeline and Source samples. MIDI/DAW breadth and video remain outside scope.

The comparison is deliberately asymmetric. A2 has substantially stronger measured source/runtime evidence. Option B now has comparable *bounded* evidence for its low-level graph, PDC, public JUCE VST3 hosting, finite actual VST3 Tail, stopped reconstruction, and headless output. No inference below is presented as a measurement.

## Compared backend shapes

```text
A2: Domain -> integer scheduling -> source request planning -> MediaSource runtime/cache/worker
    -> Clip runtime views -> custom source Nodes -> Tracktion low-level graph
    -> processing/summing/PDC/VST3/Tail

B:  Domain -> integer scheduling -> AudioNLE source/runtime layer -> AudioNLE low-level graph
    -> JUCE direct VST3 hosting -> processing/summing/PDC/Tail
```

Option B has not yet integrated a real reader, cache, worker, or multi-source scheduler into its graph. It must not inherit A2 source measurements merely because the intended upstream ownership is similar.

## Responsibility and evidence matrix

Evidence levels: **Strong measured**, **Bounded measured**, **Partial measured**, **Conceptual**, and **Untested**.

| Responsibility | A2 owner | A2 evidence | B owner | B evidence | Decision significance |
| --- | --- | --- | --- | --- | --- |
| Domain | AudioNLE | Strong measured, framework-free | AudioNLE | Strong measured in bounded descriptions | Common core |
| Timeline authority | AudioNLE | Strong measured integer scheduling | AudioNLE | Bounded measured generated-source placement | Common core |
| Source mapping | AudioNLE custom Node | Strong measured same-rate WAV; rational generated mixed-rate fixture | AudioNLE source layer | Conceptual for real source | Major asymmetry |
| Source cache/prefetch | AudioNLE | Bounded measured fixed realtime pages | AudioNLE | Untested | Major asymmetry |
| Decoder worker | AudioNLE/JUCE reader | Bounded measured worker-owned WAV reader | AudioNLE/decoder | Untested | Major asymmetry |
| Multi-source sharing | AudioNLE Model A | Bounded measured; 4/8/16/32 density | AudioNLE | Untested | Major asymmetry |
| Graph traversal | Tracktion | Bounded measured public low-level graph | AudioNLE | Bounded measured DAG preparation/traversal | Primary difference |
| Buffer flow | Tracktion | Bounded measured | AudioNLE | Bounded measured fixed 128-frame buffers | Primary difference |
| Summing | Tracktion `SummingNode` | Bounded measured | AudioNLE | Bounded measured N-input sum | Primary difference |
| Ordered processing | Tracktion graph | Bounded measured | AudioNLE graph | Bounded measured | Comparable |
| PDC | Tracktion | Bounded measured deterministic and actual VST3 | AudioNLE graph | Bounded measured deterministic and actual VST3 | Comparable |
| VST3 hosting | Tracktion/JUCE path | Bounded measured | JUCE direct | Bounded measured public headless host | Comparable |
| Plugin latency lifecycle | Tracktion runtime | Bounded measured | AudioNLE/JUCE direct | Bounded measured create/configure/prepare/query | Comparable |
| Tail query | Tracktion/JUCE path | Bounded measured finite actual VST3 | JUCE direct | Bounded measured public query | Comparable |
| Tail scheduling | Tracktion execution plus AudioNLE boundary | Bounded measured finite Tail | AudioNLE runtime | Bounded measured integer extent/silence feed | Primary difference |
| Runtime edits | AudioNLE plus stopped rebuild | Bounded measured Move/Trim/Split/Delete | AudioNLE plus stopped rebuild | Bounded measured Move/Delete fixture only | B remains partial |
| Reconstruction | AudioNLE/runtime boundary | Strong/bounded measured across source and edit fixtures | AudioNLE graph/host runtime | Bounded measured graph/VST3/Tail | Comparable at fixture scope |
| Headless execution | Tracktion player | Bounded measured | AudioNLE/JUCE targets | Bounded measured | Comparable |

## Established evidence

### A2 strengths and constraints

A2 demonstrates exact same-rate WAV range access, one-hour bounded-memory observation, fixed-page realtime source supply, callback reader/wait/allocation counters at zero, exact-zero underrun policy, generation invalidation, page ownership, seek, destruction/reconstruction, and non-divisible/cross-page access. Model A shares one worker/cache runtime per MediaSource across Clip views. Same-file overlap, different-file summing, 4/8/16/32 Clip density, Move, Trim/re-expand, Split, Delete, rapid stopped rebuild, and reconstruction are measured.

A2 also exercises public Tracktion low-level graph traversal/buffer flow, ordered processing, summing, deterministic and actual VST3 PDC, deterministic and actual finite VST3 Tail, and headless execution. The source subsystem is nevertheless a **substantial adapter subsystem** and the multi-source extension is substantial.

Its constraints are material: Tracktion high-level source scheduling is unsuitable for the strict source boundary; 32 distinct media reached 32 workers/readers/caches; single latest-request-slot arbitration is inadequate for production multi-range demand; persistent partial graph mutation is unresolved; and device callback, real 44.1-to-48 kHz media SRC, FFmpeg/container media, and production worker pools remain untested.

### Option B strengths and constraints

Option B's standard-C++ graph fixture measures generated-source placement, ordered processing, N-input sum, deterministic declared/actual latency, graph-derived PDC, reconstruction, and process growth zero: **Small bounded graph core**. Its public-JUCE VST3 fixture measures post-prepare host latency, declared=host=actual delay for multiple fixtures, graph-derived mixed PDC, processing order, reconstruction, Tracktion linkage zero, and wrapper growth/waits zero: **Small hosting extension**.

Its finite actual VST3 Tail fixture measures public Tail query, integer conversion, SourceEnd 1480 inside a 128-frame block, exact silence feed, `[1480,2504)` processing extent, downstream multiply, ordinary overlap/summing, stopped-rebuild Move/Delete, Reported/CutAtSourceEnd, and reconstruction at maximum boundary/timing error zero: **Small Tail extension**.

Option B still lacks equivalent real WAV reader integration, realtime cache/prefetch, multi-source worker/cache scaling, FFmpeg/container media, real mixed-rate SRC, device callback, production edit integration, persistent live mutation, arbitrary plugin scanning/state/crash isolation, dynamic plugin latency, and unknown/infinite/dynamic Tail evidence.

## Source subsystem: the decisive asymmetry

The upstream product responsibilities are likely common regardless of backend:

```text
Domain -> integer scheduling -> MediaSource runtime -> cache/prefetch -> decoder workers
-> Clip views -> source-node contract -> graph backend
```

A2 proves much of this chain; B proves none of its real-media/realtime integration. Therefore the architecture decision is most meaningfully a decision *below the source-node boundary*: reuse Tracktion's graph runtime (A2) or own the graph runtime and use JUCE directly (B). This is an informed architectural inference, not B source evidence.

**Option B realtime source integration is useful but not required before an ADR.** The existing A2 source design is substantially AudioNLE-owned already, and its source/cache/worker responsibilities would be required by either candidate. A B integration gate is still an architecture-specific post-ADR implementation gate because backend buffer/Node integration could expose new cost; it is not presently shown likely to reverse the high-level ownership tradeoff more than the already-measured graph/VST3/Tail difference.

## Complexity, dependencies, and maintainability

Prototype LOC must not be summed into fake production totals. A2 owns source mapping, cache/prefetch, worker lifecycle, MediaSource runtime, Clip views, edit invalidation/rebuild, and source arbitration. Tracktion supplies traversal, buffer flow, summing, processing graph execution, PDC, VST3 integration, Tail propagation, and headless execution.

B would retain the source responsibilities if it adopts the same source model, but additionally owns graph traversal, buffer scheduling, summing, PDC, plugin graph integration, and Tail extent/scheduling. JUCE supplies direct hosting primitives, VST3 APIs, and basic audio buffers where used. B's measured slices are small, but the unmeasured composition of those responsibilities remains a maintenance cost rather than a production estimate.

A2 brings Tracktion Engine, nested JUCE, Tracktion API compatibility, and licensing/distribution review. Its high-level source path is not usable, so the dependency does not eliminate source work. B removes Tracktion-specific coupling but increases internal engine ownership and requires direct JUCE/VST3 host expertise. Repository evidence supports neither a legal conclusion nor a distribution-policy conclusion; both need explicit ADR review.

For a small OSS project, A2 offers a narrower owned graph surface but more runtime layers to debug:

```text
Domain -> AudioNLE source subsystem -> Tracktion graph -> JUCE/plugin
```

B has fewer external abstraction layers but owns the graph boundary itself:

```text
Domain -> AudioNLE source subsystem -> AudioNLE graph -> JUCE/plugin
```

Fewer layers may improve failure localization and deterministic fixture control; owning the graph increases code, versioning, and contributor burden. Neither benefit is fully measured at production scale. Both have deterministic stopped reconstruction evidence; B does not thereby solve live mutation.

## Runtime edits, reversibility, and common core

A2 proves stopped/affected rebuild while persistent Tracktion graph mutation remains unresolved. B proves stopped reconstruction for bounded graph/VST3/Tail fixtures, while full production edit integration and live mutation remain unresolved. Stopped rebuild is a viable shared correctness reference, not a claim of equivalent live-edit capability.

Domain can remain unchanged under either candidate. Source/runtime, Clip views, source-node contract, deterministic processor descriptions, plugin descriptions, Tail policy, and graph-description values are promising decision-independent investments. A narrow backend adapter may make a later swap possible, but should not be designed prematurely. Stranded work is primarily A2 Tracktion integration on one path versus B custom graph runtime on the other; Domain and most source ownership should remain reusable.

## ADR criteria and risk classification

Ranked ADR criteria, without fabricated numeric weights:

1. Exact integer sample authority and deterministic reconstruction.
2. Source subsystem viability and realtime safety for long-form media.
3. Engine implementation/maintenance burden.
4. Dependency, coupling, licensing, and distribution review.
5. VST3/PDC/Tail lifecycle complexity.
6. Runtime edit/rebuild model and deferred live mutation.
7. FFmpeg/container and mixed-rate/SRC boundary.
8. Deterministic testability, debugging ownership, and contributor accessibility.

| Item | Classification | Rationale |
| --- | --- | --- |
| License/distribution review for both candidate dependency sets | Must resolve before ADR | An ADR cannot responsibly accept a dependency without its project-specific policy review. |
| Explicit backend ownership boundary and accepted source asymmetry | Must resolve before ADR | This is the decision itself, not an implementation detail. |
| A2 worker-pool/shared decoder arbitration | Architecture-specific follow-up | Material A2 scaling risk; it does not erase the already-known backend tradeoff. |
| B realtime source integration | Architecture-specific follow-up | Useful but not required before ADR; validates B's backend handoff. |
| Real mixed-rate SRC, FFmpeg, device callback | Post-ADR implementation gates | Both candidates require these product capabilities. |
| Persistent graph mutation | Post-ADR implementation gate | Both rely on stopped rebuild today. |
| Plugin scanning/state, dynamic latency, unknown/infinite Tail | Post-ADR implementation gates | Bounded fixtures do not establish production plugin breadth for either path. |

## Candidate ADR decisions (not selected)

### Candidate A — Adopt A2

**Decision shape:** AudioNLE-owned source/runtime with a Tracktion low-level graph backend.

Expected benefits: reuse of established traversal, buffer flow, summing, processing execution, PDC, VST3 integration, and Tail flow; stronger end-to-end real-source evidence today.

Accepted costs: Tracktion dependency/coupling, low-level API constraints, unusable high-level source path, source adapter ownership anyway, and dependency/licensing review.

Known constraints: Model A per-media worker/cache scaling; inadequate single-slot arbitration; unresolved partial mutation; untested mixed-rate real media, FFmpeg, and device callback.

Required post-decision gates: worker-pool/shared-decoder arbitration, real mixed-rate source/SRC, FFmpeg boundary, device callback behavior, and live-mutation decision.

### Candidate B — Adopt Option B

**Decision shape:** AudioNLE-owned source/runtime with an AudioNLE low-level graph and JUCE direct hosting.

Expected benefits: Tracktion-free backend, direct ownership of graph scheduling/debugging, bounded measured graph/PDC/VST3/Tail slices, and a domain-designed backend boundary.

Accepted costs: AudioNLE owns traversal, buffers, summing, PDC, plugin graph integration, and Tail scheduling; more internal engine maintenance; direct JUCE/VST3 host expertise; and direct JUCE licensing/distribution review.

Known constraints: no equivalent B real-source/realtime/multi-source integration, no production plugin breadth, and unresolved live mutation.

Required post-decision gates: real source/cache/worker handoff into B, multi-source scaling/arbitration, real mixed-rate/SRC, FFmpeg boundary, device callback behavior, plugin lifecycle breadth, and live-mutation decision.

## Decision readiness

**Ready for ADR with explicit deferred gates.** The former decisive B evidence gap for graph, actual VST3/PDC, and finite actual Tail has been narrowed by bounded measurements. The remaining source-runtime asymmetry is explicit and should be accepted or rejected as part of the ADR rather than obscured by more unrelated prototype work.

Evidence confidence is **High for A2** within its measured source/graph scope and **Medium for B**: B has strong bounded backend evidence but no equivalent real-source/realtime/multi-source evidence. Neither confidence rating implies production readiness.

## Recommended next action

**Write ADR comparing A2 and B.** It must choose an ownership/dependency direction, record the accepted asymmetry and deferred gates above, and avoid treating either prototype as a production guarantee.
