# Tracktion Selective Reuse — Complexity and Boundary Review

Execution date: 2026-09-06 (Asia/Tokyo)

Scope: evidence after Phase 0, A2-1, A2-2, conditional A2-3, deterministic low-level PDC, and the actual-VST3 hard-stop investigation. This is not an estimate of a production engine.

## Measured prototype surface

| Measure | Observation |
| --- | --- |
| A2-specific implementation | A2-1: 1 C++ source file, 221 lines; A2-2: 1 C++ source file, 338 lines; A2-3: 1 C++ source file, 368 lines; PDC: 1 C++ source file, 411 lines; 1 CMake file, 43 lines, with 4 targets |
| Custom components | framework-free source/processor/layer Domain values; custom source Node; deterministic runtime wrapper Node; explicit integer/rational Source-to-Timeline mapper; fixture-only latency processor description; adapter-owned offline receiver loop |
| Public Tracktion concepts directly used | `Node`, `NodeProperties`, `Node::ProcessContext`, `SimpleNodePlayer`, `SummingNode`, `LatencyNode` (6) |
| Public Tracktion concept inspected but not instantiated | `Renderer::RenderTask` existing-graph constructor (1) |
| Private/internal API count | 0 |
| Patch/fork count | 0 |
| Actual VST3 diagnostic | Public host reports 0 before prepare and 1024 after public prepare; actual fixture delay is 1024 after a fixture-only index correction. Actual PDC remains unrun. |
| High-level Clip/source scheduling, source reader, or plugin components | 0 |
| Production-directory changes | 0 |

## Ownership and responsibility split

| Responsibility | Current owner | Evidence / limitation |
| --- | --- | --- |
| Authoritative source identity/data and Timeline event | AudioNLE framework-free Domain fixture | `int64_t` event remains unchanged over destroy/rebuild. |
| Source scheduling and direct sample mapping | AudioNLE custom source node | Node consumes the requested range and writes source output directly. A2-3 additionally maps sparse 44.1/48 kHz events with bounded `int64_t` rational arithmetic. This is deliberate A2 scope, not Tracktion clip scheduling or a production SRC decision. |
| Processing behavior / processor descriptions | AudioNLE Domain descriptions plus runtime-only deterministic wrapper | Required test instrumentation only; it does not select a production processor abstraction. |
| Node graph ownership/preparation, dependency traversal, and block processing | Tracktion public graph/player | `SimpleNodePlayer` owns/prepares the root `NodeGraph`, orders input Nodes, and processes it. |
| Track Mix / summing | Tracktion public `SummingNode` | Two sources `0.25 + 0.5` observe as `0.75`, then Track Multiply observes 1.5. |
| Offline output-buffer ownership | AudioNLE test harness | Caller supplies the output buffer; this makes sample observation explicit. |
| Latency property/graph transformation | Tracktion public graph API | `NodeProperties::latencyNumSamples`, `LatencyNode`, and `SummingNode` public transformation were exercised in the bounded deterministic PDC fixture. Hosted plugin latency is 1024 after public prepare; actual plugin PDC remains unmeasured. |
| PDC metadata, delay, and branch balancing | Tracktion public `LatencyNode` / `SummingNode` | Measured for 256, 1024, and 2048 samples, Clip/Track-equivalent paths, accumulated layers, reorder, stopped/rebuild change, bypass, and reconstruction. `SummingNode` performs `maxLatency - pathLatency` insertion; the adapter has no manual compensation calculation. |
| Ordered deterministic processing graph behavior | Measured for bounded A2-2 | Add/Multiply order, layers, timing, and reconstruction pass through public Node graph. Plugin lifecycle remains unmeasured. |
| Source read/seek/reset and resampling for real media | Unmeasured | A2-3 proves only sparse-event integer placement; it does not provide an audio resampler, reader, seek/reset policy, or quality result. |
| Thread scheduling | Single-threaded Tracktion `SimpleNodePlayer`, unmeasured beyond headless run | No realtime/thread guarantee is inferred. |

## Boundary review

The source code under `prototype/tracktion-selective-reuse/` contains Tracktion types only in the runtime node/player layer. `DomainState`, `SourceData`, Source positions, Timeline positions, rates, and durations use standard C++ strings/vectors/`int64_t`. A2-3's Source and Timeline domains remain distinct; no floating time is authoritative. No Tracktion graph is persisted, no graph pointer or JUCE/Tracktion type enters authoritative state, and no reverse synchronization exists.

The prototype does not create or delegate Synchronization Group/Member, Clip Group, Ripple semantics, or project persistence. It also does not use high-level Tracktion `Edit`/Clip semantics as a scheduling authority.

## Assessment

The measured glue burden remains materially below a custom graph runner for the bounded generated-source, integer-rational event-mapping, ordered-processing, Track Mix, and deterministic PDC cases. The public host provides the fixed-fixture latency-report contract after the required lifecycle, but actual VST3 PDC remains untested. No conclusion about production plugin hosting can be inferred, and a manual compensation adapter would violate this prototype's hard-stop rule.
