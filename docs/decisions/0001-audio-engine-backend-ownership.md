# 0001: Audio Engine Backend Ownership

## Status

Accepted

## Context

AudioNLE is a long-form, multi-track, audio-only NLE. Its Domain must remain framework-independent, non-destructive, and authoritative in integer Timeline and Source samples. The V1 product requires VST3 processing, PDC, finite Tail handling, deterministic reconstruction, and long-form source handling; it does not target MIDI/DAW breadth or video editing.

Two backend candidates were evaluated:

```text
Candidate A (A2)
Domain -> AudioNLE integer scheduling/source runtime/cache/worker/Clip views/custom source Nodes
       -> Tracktion low-level graph -> processing/summing/PDC/VST3/Tail

Candidate B (Option B)
Domain -> AudioNLE integer scheduling/source runtime/cache/worker/Clip views/source-node contract
       -> AudioNLE low-level graph -> JUCE direct hosting -> processing/summing/PDC/Tail
```

The [ADR comparison preparation](../design/prototypes/audio-engine-adr-comparison-preparation.md) records an intentional evidence asymmetry. A2 has stronger measured real-WAV, long-source, realtime-page-cache, multi-source, and source-edit/rebuild evidence. Option B has bounded measured graph, buffer flow, N-input summing, PDC, public direct VST3 hosting, plugin latency lifecycle, finite actual VST3 Tail, stopped reconstruction, and headless execution evidence. A2 source results are not evidence that B source integration already works.

## Decision

Select **Candidate B: AudioNLE-owned low-level graph with JUCE direct plugin hosting** as the audio-engine backend architecture.

AudioNLE owns graph traversal, fixed buffer flow, summing, PDC, direct plugin graph integration, finite Tail policy/extent/scheduling, and stopped-rebuild runtime construction. JUCE direct hosting is the VST3 plugin boundary. Tracktion Engine is not the selected production graph backend; its prototypes and their evidence remain retained and are not removed by this decision.

This decision accepts the architectural inference that the following source core is principally AudioNLE-owned and can sit above either graph backend:

```text
Domain -> integer source scheduling -> MediaSource runtime -> cache/prefetch
-> decoder worker -> Clip views -> source-node contract
```

Accordingly, A2 source evidence informs the source-core design but does not prove its B backend handoff. That handoff is a required post-decision validation gate.

## Build and CI scope

The standard build and CTest gate represent the accepted Candidate B
architecture only. Current `main` has no Tracktion Engine dependency,
submodule, target, or feasibility test. Candidate B obtains its direct JUCE
hosting dependency independently.

The historical Tracktion executable prototypes were removed from current
`main` after this accepted decision. Their Markdown comparison, results, and
benchmark evidence remain in the repository; the removed implementation and
its exact historical test harnesses are recoverable from Git history. This
preserves the decision record without retaining rejected backend code as a
current dependency or CI concern.

## Decision drivers

1. Integer authority and deterministic reconstruction remain framework-free in both candidates; B demonstrates them through bounded graph/VST3/Tail reconstruction fixtures.
2. B reproduced the principal measured Tracktion graph responsibilities as bounded extensions: **Small bounded graph core**, **Small hosting extension**, and **Small Tail extension**. This makes Tracktion's reuse value insufficient to outweigh its additional dependency, coupling, and debugging layer for this small OSS project.
3. The highest-complexity source/runtime responsibilities already remain AudioNLE-owned under A2. Selecting Tracktion would not remove source scheduling, cache, worker, Clip-view, invalidation, or arbitration work.
4. B provides a smaller external backend stack and direct deterministic ownership of graph behavior, while retaining public JUCE VST3 hosting.

## Evidence summary

A2 measured public Tracktion low-level traversal/buffer flow, `SummingNode`, ordered processing, deterministic and actual VST3 PDC, finite actual VST3 Tail, and headless execution. It also measured exact same-rate WAV ranges, one-hour bounded reads, fixed-page realtime source supply, and Model A multi-source stopped rebuild. See [A2 results](../design/prototypes/tracktion-selective-reuse-results.md), [realtime source](../design/prototypes/tracktion-selective-reuse-realtime-source-results.md), and [multi-source/edit](../design/prototypes/tracktion-selective-reuse-multisource-runtime-edit-results.md).

Option B measured fixed-buffer DAG traversal, ordered processing, N-input sum, graph-derived deterministic PDC, direct public JUCE VST3 lifecycle and actual latency/PDC, and finite actual VST3 Tail. The Tail fixture measured block-interior SourceEnd silence feed, integer `[SourceEnd, ProcessingEnd)` extent, downstream processing, overlap, Move/Delete stopped rebuild, Reported/CutAtSourceEnd, and reconstruction at maximum boundary/timing error zero. See [graph/PDC results](../design/prototypes/option-b-low-level-graph-pdc-results.md), [VST3/PDC results](../design/prototypes/option-b-vst3-pdc-results.md), and [VST3 Tail results](../design/prototypes/option-b-vst3-tail-results.md).

The selected architecture does not reinterpret bounded fixtures as production guarantees. In particular, B has not repeated A2's real-source/realtime/multi-source gate.

## Consequences

### Accepted

* AudioNLE is responsible for graph traversal, buffers, summing, PDC, plugin graph integration, and finite Tail scheduling, in addition to its source core.
* JUCE direct hosting is the plugin boundary. Plugin descriptions and Tail policy remain framework-free values; JUCE instances, buffers, and host reports remain runtime state.
* The initial runtime-edit correctness strategy is `Domain edit -> stopped/control boundary -> affected or full runtime rebuild`. Persistent live graph mutation is deferred.
* The source scaling risk remains: Model A currently scales workers/readers/caches with unique MediaSource, reached 32 workers/caches for 32 distinct media, and needs bounded multi-range arbitration. Choosing B does not resolve it.
* Tracktion-specific coupling and its graph API compatibility risk are avoided in the selected backend.

### Costs and constraints accepted

* B's small prototype classifications do not make production engine work trivial. The project bears ongoing ownership of graph/PDC/Tail behavior and direct JUCE/VST3 lifecycle expertise.
* B source/cache/worker integration, multi-source scaling, and production graph/runtime composition are unproven and must be validated.
* Unknown/infinite/dynamic Tail, dynamic latency, arbitrary plugin scanning/state/UI/crash isolation, device callback behavior, real mixed-rate media, FFmpeg/container media, and live mutation remain unproven.

## Deferred validation gates

Required for the selected backend:

1. Integrate the AudioNLE source/cache/worker contract with the B graph, including realtime-safe buffer handoff and exact source-node scheduling.
2. Exercise B with the common source subsystem under multi-source density, then define bounded multi-range arbitration/worker strategy; the known 32-unique-media scaling risk remains active.
3. Establish real mixed-rate Source-to-Timeline SRC policy and behavior, then define the FFmpeg/container boundary.
4. Validate device callback/realtime output behavior and retain stopped rebuild until a live-mutation policy is separately justified.
5. Broaden direct-host validation for plugin state/scanning, missing plugins, dynamic latency, Tail policy breadth, and crash isolation as required by product requirements.

## Rejected alternative: Candidate A (A2)

A2 remains technically viable and has the strongest current real-source, realtime-page-cache, multi-source, and source-edit evidence. It also reuses mature Tracktion graph traversal, buffer propagation, summing, processing execution, PDC, VST3 integration, finite Tail propagation, and headless graph execution.

It is not selected because the measured B fixtures demonstrate that the core graph/VST3/PDC/finite-Tail responsibilities can remain bounded under project ownership, while A2 still requires the substantial AudioNLE source subsystem and adds Tracktion dependency, low-level API coupling, an extra debugging layer, and a high-level source path that is unsuitable for the required timing boundary. This is a project-specific ownership decision, not a conclusion that Tracktion is technically inadequate.

Revisit Candidate A if B source/backend integration requires materially more adaptation or maintenance than this comparison predicts, if B exposes unacceptable device-callback behavior, or if dependency/API/distribution conditions materially change.

## Licensing and distribution considerations

This ADR accepts the project's GPL-family OSS distribution policy, including the AGPLv3 obligations applicable to the selected direct JUCE route, subject to a release-specific compliance audit. Repository-recorded license files identify Tracktion Engine as GPLv3-or-later/commercial and bundled JUCE as AGPLv3/commercial; the measured B targets use direct JUCE hosting and a repository-local build-only VST3 fixture, with no Tracktion target linkage. The bundled VST3 SDK/header license is recorded as MIT.

This is not legal advice and does not determine a release's compliance. Before distribution, the project must inventory the exact JUCE modules and bundled third-party material in release artifacts, preserve required notices/license texts, verify the actual build configuration and linkage, and separately audit future FFmpeg/codecs and platform dependencies. The decision is to accept that audit obligation, not to claim it is already complete.

## Reversibility and revisit triggers

The following remain backend-independent: Domain/editing semantics, integer Source/Timeline mapping, the intended source runtime/cache contract, Clip views, persistent processor descriptions, and Tail policy. B graph implementation, JUCE hosting adapters, PDC scheduling, and runtime graph construction are backend-specific.

Revisit this decision if the selected backend requires materially more custom adaptation than predicted; the source core contradicts its assumed backend-independent contract; licensing/distribution or framework API conditions change; device callback behavior is unacceptable; live mutation cannot be met with a reasonable strategy; or maintenance/contributor burden materially exceeds the comparison assumptions.
