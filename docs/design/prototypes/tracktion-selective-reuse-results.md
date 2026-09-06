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
