# Tracktion Selective Reuse Feasibility Results (Option A2)

## Conclusion

**Proceed with Constraints.**

Option A high-level scheduling remains unsuitable as an authoritative integer-sample source scheduler: the prior high-level `WaveAudioClip` / `WaveNodeRealTime` path has a source/resampler onset offset that varies with event position, render start, source rate, and quality. That evidence is retained and is not reinterpreted here.

Option A2 has now passed its bounded public-API and custom-source baseline. A public low-level Tracktion graph can receive exact `int64_t` requested sample ranges from an AudioNLE-controlled source node, run headlessly without a physical device, and produce zero-error absolute Timeline placement over the required event and render-start matrix. The same unchanged framework-free Domain fixture also reconstructs to an identical observation.

The bounded A2-2 graph test also passed: public low-level graph traversal and buffer propagation preserve Domain-ordered Add/Multiply processing, separate Clip/Track/Master-equivalent layers, public `SummingNode` Track Mix, and a zero-error impulse onset through deterministic zero-latency wrappers. This is sufficient to continue bounded A2 investigation with constraints, but not enough to select Tracktion, select Option A2 over Option B, or define production architecture. Plugin compatibility, real-media reads/seeks, and runtime/thread complexity are intentionally unmeasured. No ADR is created.

The conditional A2-3 fixture also passed. It keeps 44.1 kHz Source samples and 48 kHz Timeline samples distinct in framework-free Domain state, maps sparse generated events using explicit bounded integer/rational arithmetic, and observes zero Timeline-sample error for the required anchors, nontrivial positions, reconstruction, and a zero-latency deterministic processing wrapper. This is evidence only for deterministic fixture scheduling; it is not a production SRC, real-media reader, quality, or PDC result.

The deterministic low-level Phase E-A2 PDC retry passed its bounded graph criteria. Public `LatencyNode` provides equal actual and reported latency, while public `SummingNode` propagates maximum path latency and inserts graph-owned balancing delay on shorter paths. The 256/1024/2048-sample, Clip-, Track-, mixed-layer, processor-order, stopped/rebuild latency-change, bypass, and reconstruction cases each produced `alignment error = 0 Timeline samples`. This does not change the high-level Phase E result, which remains Inconclusive because its high-level source path cannot establish the zero-error baseline.

The initial actual-VST3 V2 hard stop has now received a separately authorised root-cause diagnostic. The former adapter published `AudioPluginInstance::getLatencySamples()` before public `prepareToPlay`, when it was 0; after public rate/block setup plus prepare, pinned JUCE reports 1024. The repository-local fixture also had an off-by-one delay-ring read index: it declared 1024 but produced 1023 samples. Its fixture-only correction now produces an actual 1024-sample delay. No manual compensation, latency override, source/output shift, high-level scheduling, private API, patch, or fork was used. Actual VST3 PDC itself remains unrun and requires separate authorisation.

## Scope and environment

## Corrected actual-VST3 PDC update

The separately authorised corrected retry is a **Conditional Pass**. Generated repository-local VST3 fixtures at 256, 768, 1024, and 2048 samples now satisfy the declared, post-prepare hosted, measured actual, adapter, and root-graph latency contract. The two-path, Clip/Track-equivalent, mixed-layer (deterministic Clip 256 + VST3 Track 768), processing-order, adapter-policy bypass, stopped destroy/rebuild 1024 -> 2048, and reconstruction cases all measure `alignment error = 0 Timeline samples`.

Tracktion's public `SummingNode` owns branch balancing; the adapter exposes only the hosted post-prepare latency and has no `maxLatency - pathLatency` computation, delay injection, source/output shift, private API, patch, fork, or reverse Domain synchronization. This result is bounded: arbitrary third-party plugin compatibility, dynamic playback-time latency changes, scanner/state work, production bypass semantics, crash isolation, Tail, Phase F, realtime work, and production hosting remain unmeasured. See [the raw actual-VST3 PDC record](../../../benchmark-results/tracktion-selective-reuse/vst3-pdc.md).

## Phase F deterministic Tail update

**Conditional Pass.** The low-level graph preserves distinct integer Source End 1480 and Processing End 2504 for a finite reported 1024-sample tail. It feeds silence after the source, processes a downstream multiply, mixes a subsequent Clip through public `SummingNode`, moves the full processing output by +48000, removes it on delete, honors `CutAtSourceEnd`, and reconstructs identically. See [tail.md](../../../benchmark-results/tracktion-selective-reuse/tail.md). Actual VST3 Tail, unknown/infinite tails, realtime edits, and production render planning remain unmeasured.

## Actual VST3 Tail update

**Conditional Pass for the bounded fixture.** A repository-local VST3 fixture reports `1024 / 48000` through the pinned public JUCE VST3 host API after prepare; explicit `llround` conversion returns 1024 integer Timeline samples. With Source End 1480, the actual hosted plugin receives zero input after the source and emits non-zero output exactly over `[1480,2504)`. Its tail reaches a downstream Multiply(2), overlaps an independent source through public `SummingNode` at 0.75, moves by exactly +48000, disappears on delete or `CutAtSourceEnd`, and reconstructs identically. No high-level source scheduler, private API, patch/fork, hard-coded adapter tail, manual Tail mix, or Domain mutation was used. See [vst3-tail.md](../../../benchmark-results/tracktion-selective-reuse/vst3-tail.md).

The PDC-plus-Tail combination is deliberately deferred: prior bounded actual-VST3 PDC and this Tail result are separate evidence. Third-party VST3 tail behavior; zero, unknown, or infinite production policies; dynamic reports; realtime behavior; production render planning; scanner/state/crash isolation; GUI; and ADRs remain unmeasured. Option A2 therefore remains **Proceed with Constraints**, not a production-engine selection.

| Item | Value |
| --- | --- |
| Execution date | 2026-09-06 (Asia/Tokyo) |
| Tracktion Engine | `b88a6ee51913668cb53e911e030ab736b13342cf` |
| nested JUCE | `37c894f83d379179b2070d437ccd0f1cd9af9576` |
| Compiler | MSVC 19.51.36256, C++20 |
| CMake / generator | CMake 4.3.1-msvc1 / Ninja 1.13.2 |
| Windows SDK | 10.0.26100.0 |
| Executed phases | Phase 0, A2-1, A2-1 reconstruction, A2-2, conditional A2-3, and deterministic low-level Phase E-A2 PDC |
| Explicitly not run | actual-VST3 PDC, Tail, Phase F, scanner, GUI, persistence, long-source/dense-Clip work, realtime editing, real-media reader/resampler, production SRC selection |

Pins are unchanged. No floating revision, binary artifact, submodule update, patch, or fork was used.

## Results

| Test | Status | Evidence |
| --- | --- | --- |
| A2-T001 public API/source investigation | Pass | Public `Node`, `ProcessContext::referenceSampleRange`, graph ownership, `SimpleNodePlayer`, latency property, and renderer existing-graph input were identified; no high-level Clip path or private API is needed. See [api-investigation.md](../../../benchmark-results/tracktion-selective-reuse/api-investigation.md). |
| A2-T002 event-position matrix | Pass | Events 1, 100, 1024, 2048, and 3000 all observe their requested absolute Domain event; maximum error is 0 Timeline samples. |
| A2-T003 render-start matrix | Pass | Event 1024 remains absolute 1024 for `[0,4096)`, `[512,4608)`, and `[1000,5096)`; no coordinate or buffer correction is used. |
| A2-T004 destroy/rebuild | Pass | Destruction and reconstruction from unchanged Domain state produces identical 4096-sample output, requested ranges, and zero error. |
| A2-T005 processing compatibility | Pass | Public low-level graph preserves ordered deterministic processing, Clip/Track/Master-equivalent boundaries, public `SummingNode` Track Mix, and zero-error onset; see [a2-2.md](../../../benchmark-results/tracktion-selective-reuse/a2-2.md). |
| A2-T006 mixed-rate fixture | Conditional Pass | Explicit 44.1/48 kHz framework-free integer/rational event mapping observes zero error for required positions, one-second duration, ten rebuilds, and a Multiply(2) wrapper. It does not evaluate production SRC. See [a2-3.md](../../../benchmark-results/tracktion-selective-reuse/a2-3.md). |
| E-A2-T0 to E-A2-T9 deterministic PDC | Conditional Pass | Baseline plus all latency, layer, order, stopped/rebuild change, bypass, and reconstruction matrices use public `LatencyNode`/`SummingNode`; maximum `alignment error = 0 Timeline samples`. VST3 and realtime behavior are unmeasured. See [pdc.md](../../../benchmark-results/tracktion-selective-reuse/pdc.md). |
| V0–V2 actual VST3 PDC | Conditional Pass / retry ready | The initial V2 hard stop was an adapter lifecycle read before prepare; after the public prepare lifecycle, host-visible and actual fixture latency are both 1024. No PDC retry was run. See [vst3-latency-diagnostic.md](../../../benchmark-results/tracktion-selective-reuse/vst3-latency-diagnostic.md). |
| A2-T007 complexity decision | Proceed with Constraints | Public graph supplies ownership, traversal, buffers, summing, latency metadata, delay lines, and graph-wide PDC for the measured cases; custom source scheduling and framework-free processor descriptions remain adapter-owned. See [complexity.md](../../../benchmark-results/tracktion-selective-reuse/complexity.md). |

The complete A2-1 raw matrix and command outcomes are in [a2-1.md](../../../benchmark-results/tracktion-selective-reuse/a2-1.md).
The processing API investigation and A2-2 raw measurements are in [a2-2.md](../../../benchmark-results/tracktion-selective-reuse/a2-2.md). The conditional mixed-rate record is in [a2-3.md](../../../benchmark-results/tracktion-selective-reuse/a2-3.md). The deterministic PDC record is in [pdc.md](../../../benchmark-results/tracktion-selective-reuse/pdc.md).

## Architecture and leakage review

The A2 code is isolated to `prototype/tracktion-selective-reuse/`; root CMake adds only that prototype directory. No production AudioNLE source or production dependency boundary changed.

`DomainState`, source identity/data, Source samples, Timeline samples, rates, and durations are framework-free and authoritative. A2-3 maintains a distinct Source Time Domain (44100 Hz) and Timeline Time Domain (48000 Hz) through an explicit integer/rational fixture mapper. The direction is strictly Domain → adapter-created custom source node → transient Tracktion graph/player → output observation. Runtime destruction does not mutate the Domain fixture, Tracktion object graphs are never persisted, and no reverse synchronization occurs.

No Tracktion/JUCE type leaks into the fixture Domain state. Source and Timeline domains are not conflated. No Synchronization Group/Member, Clip Group, or Ripple semantics is delegated to Tracktion.

## Headless/build status

The A2-1, A2-2, A2-3, deterministic PDC, and VST3 latency-diagnostic executables/targeted CTests pass device-free. The standard `build.ps1` configures and builds them; the diagnostic targeted CTest is test 11 and passes 1/1. The full suite still has the already documented high-level Phase E PDC baseline failure at `1024 → 1026`; that result was neither changed nor used as A2 evidence.

## Reuse versus custom responsibilities

Tracktion is actually reused for the public node graph, graph preparation, dependency traversal, node-buffer allocation/flow, public `SummingNode` mixing, public `LatencyNode` delay/metadata, graph-wide branch balancing, and headless single-thread processing. AudioNLE still owns authoritative source scheduling, explicit Source/Timeline integer mapping, source read/seek/resampling policy, framework-free processor descriptions, output observation, and all product editing semantics. The observed glue remains bounded: no adapter-side `maxLatency - pathLatency` calculation or manual compensation exists. Later plugin, realtime-latency, real-media, and thread work could still change that assessment.

## PDC readiness and next permitted action

The deterministic low-level PDC gate has now been exercised and is **Conditional Pass**. It uses only the Option A2 custom-source path, public `LatencyNode`, public `SummingNode`, and `SimpleNodePlayer`; automatic graph balancing belongs to Tracktion, while Domain processor state remains authoritative and one-way. The high-level Phase E PDC result remains independently **Inconclusive** and cannot be repaired or replaced by this result.

Actual VST3 PDC retry may be considered only through separate authorization; it is now technically ready because public host-visible and actual fixture delay are both 1024. Tail, Phase F, production implementation, realtime dynamic latency change, production bypass semantics, and ADR work remain unauthorized by this result.

## Real-media / long-source update

**Proceed with Constraints.** The Option A2 custom source boundary now has bounded same-rate real-media evidence: public JUCE WAV range reads behind AudioNLE-owned integer scheduling preserve all small-fixture and block/render-start markers at 0 Timeline-sample error; seek/reset/rebuild is deterministic; EOF/error behavior is explicit; and a one-hour-plus-one-sample WAV is read at beginning, 1 minute, 10 minutes, and 1 hour without retaining duration-scaled decoded PCM. The real WAV source also reaches public downstream `MultiplyNode` and `SummingNode` processing with exact amplitude and timing, including a graph rebuild.

This is not evidence for realtime behavior, compressed/container media, production cache/prefetch, FFmpeg, or mixed-rate real media. The selected JUCE reader is synchronous and may perform I/O, allocation, decode, or blocking in the offline source node. A separately scoped 44.1 kHz Source -> 48 kHz Timeline real-media gate is justified but deferred; SRC mapping/quality/latency, decoder seek/preroll, cache, ownership, and realtime handoff remain open. See [real-media results](tracktion-selective-reuse-real-media-results.md), [raw measurements](../../../benchmark-results/tracktion-selective-reuse/real-media.md), and [complexity review](../../../benchmark-results/tracktion-selective-reuse/real-media-complexity.md).

## Realtime source update

**Proceed with Constraints.** The bounded Option A2 realtime-source gate now separates reader work from callback supply using fixed pages, a worker-owned public JUCE reader, runtime-only generations, exact-zero underrun output, and page ownership that excludes worker overwrite while callback readers copy. Same-rate markers, rapid seek, 257-frame non-divisible cross-page processing, bounded-memory observation, worker destruction/rebuild, and downstream Multiply(2) pass at zero Timeline-sample error. Q9’s initial failure was a PCM16 expected-value bug, not a graph-path failure; corrected processing compares against decoded source PCM. The source subsystem is a substantial adapter subsystem, while Tracktion still provides graph traversal, processing, summing, PDC, VST3 graph integration, Tail flow, and headless execution. See [realtime-source results](tracktion-selective-reuse-realtime-source-results.md).

This does not establish device-callback behavior, OS dropout guarantees, production cache sizing, multiple-source/dense-Clip scaling, runtime edits, mixed-rate/SRC, FFmpeg/compressed media, persistence, or a final architecture decision. The next recommended feasibility gate is dense / multi-source / runtime-edit source scheduling.
