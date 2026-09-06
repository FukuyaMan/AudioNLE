# Tracktion Selective Reuse — Phase 0 Public API Investigation

Execution date: 2026-09-06 (Asia/Tokyo)

Scope: Option A2 public-API investigation only. This record does not adopt Tracktion, select a production architecture, or revisit the rejected high-level `WaveAudioClip` scheduling path.

## Environment and pins

| Item | Value |
| --- | --- |
| OS | Windows 11 Home 10.0.26200, x64 |
| Compiler | MSVC 19.51.36256, C++20 |
| CMake / generator | CMake 4.3.1-msvc1 / Ninja 1.13.2 |
| Windows SDK | 10.0.26100.0 |
| Tracktion Engine | `b88a6ee51913668cb53e911e030ab736b13342cf` |
| nested JUCE | `37c894f83d379179b2070d437ccd0f1cd9af9576` |

No revision, submodule, binary artifact, patch, or fork changed.

## Inspected public declarations and evidence

| Need | Public source and symbol | Finding | A2 implication |
| --- | --- | --- | --- |
| Custom source node | `modules/tracktion_graph/tracktion_graph/tracktion_Node.h`: `tracktion::graph::Node` | Public polymorphic base class; a subclass implements `getNodeProperties`, `isReadyToProcess`, and protected `process(ProcessContext&)`. | AudioNLE can own a custom runtime-only source node. |
| Exact requested sample range | same file: `Node::ProcessContext::referenceSampleRange` | `juce::Range<int64_t>` is explicitly documented as the monotonic stream-time range used by nodes for file-reading positions; `Node::process` forwards it. | Adapter can pass/observe integer absolute Timeline positions without converting Domain state to Tracktion clip timing. |
| Graph ownership and preparation | same file: public `createNodeGraph(std::unique_ptr<Node>, bool)` and `NodeGraph::rootNode` | Root ownership is a `unique_ptr`; graph preparation/transform is public. | Runtime graph can be transient and adapter-owned. |
| Headless single-threaded output | `modules/tracktion_graph/tracktion_graph/players/tracktion_SimpleNodePlayer.h`: `SimpleNodePlayer` | Public player accepts `unique_ptr<Node>`, sample rate, and block size; `process(ProcessContext)` writes root output into caller-supplied audio/MIDI buffers. No device, `Edit`, GUI, or message loop is required. | Suitable as A2-1 offline receiver/output observation. |
| More general player | `tracktion_graph/tracktion_NodePlayer.h`: `NodePlayer` | Public graph player prepares and processes a root node; its documented implementation iterates nodes single-threaded and adds root output into caller buffers. | Confirms the graph has a supported player path beyond the minimal A2-1 player. |
| Public latency property | `tracktion_Node.h`: `NodeProperties::latencyNumSamples`; `nodes/tracktion_LatencyNode.h`; `nodes/tracktion_SummingNode.h` | Node properties publicly expose latency. The public `LatencyNode` adds its latency and `SummingNode` reads latency to insert balancing nodes during graph transformation. | Latency propagation has a public low-level representation; it is not exercised as PDC in this scope. |
| Renderer-compatible graph input | `modules/tracktion_engine/model/export/tracktion_Renderer.h`: public `Renderer::RenderTask` constructor accepting `unique_ptr<tracktion::graph::Node>`, `PlayHead`, `PlayHeadState`, and `ProcessState` | A public existing-graph render-task constructor exists. The convenience renderer APIs construct their own `Edit` graph, but that convenience route is not required to create or play a custom graph. | Custom graph is renderer-compatible in API shape; A2-1 deliberately uses `SimpleNodePlayer` to avoid introducing an unnecessary transient `Edit`. |
| Supported examples/tests | `tracktion_graph/tracktion_graph/tracktion_TestNodes.h`: `SinNode`, `SilentNode`; `tracktion_TestUtilities.h`: `TestProcess` | Pinned source contains custom `Node` subclasses and headless players that build `ProcessContext` with a monotonic `Range<int64_t>` and collect the caller buffer. | Direct precedent for custom source construction and headless observation. |

## Boundary and negative findings

- The A2 route imports `tracktion_graph/tracktion_graph.h` explicitly; `tracktion_engine.h` alone only forwards some graph declarations.
- The source node path uses none of `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, an audio-file reader, a resampler reader, `filePathResolver`, Tracktion clip placement, or Tracktion serialization.
- `PlaybackInitialisationInfo::enableNodeMemorySharing` and `Node::internal` are marked `@internal`; neither is used. No private/internal symbol is required by the A2-1 path.
- `Renderer::RenderTask` still needs renderer `Parameters` and runtime support objects. That is evidence of compatibility, not a justification to use high-level renderer convenience APIs or make an `Edit` authoritative.

## Lifecycle established for A2-1

```text
framework-free DomainState (int64 Timeline sample + generated source data)
  -> adapter constructs DomainControlledSourceNode (unique_ptr)
  -> SimpleNodePlayer owns/prepares transient NodeGraph
  -> caller supplies each absolute Range<int64_t> and receives a float buffer
  -> runtime/player/node destroyed
  -> unchanged DomainState may construct an equivalent new runtime
```

There is no Runtime → Domain synchronization, runtime persistence, Tracktion/JUCE type in `DomainState`, or delegated Synchronization Group / Clip Group / Ripple behavior.

## Phase 0 decision

**Pass (A2-T001).** The pinned public low-level graph API can create and run an AudioNLE-controlled source node in headless offline processing without a high-level Clip scheduling path. A2-1 is authorized. This does not yet establish processing/plugin reuse or a production choice.
