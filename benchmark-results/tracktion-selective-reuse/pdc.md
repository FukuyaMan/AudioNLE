# Tracktion Selective Reuse — Phase E-A2 Deterministic Low-Level PDC

Execution date: 2026-09-06 (Asia/Tokyo)

Scope: deterministic low-level PDC retry only, after A2-1/A2-2 Pass and A2-3 Conditional Pass. This is not a VST3, Tail, Phase F, realtime-editing, real-media, or production-SRC test.

## Pins and environment

| Item | Value |
| --- | --- |
| Tracktion Engine | `b88a6ee51913668cb53e911e030ab736b13342cf` |
| nested JUCE | `37c894f83d379179b2070d437ccd0f1cd9af9576` |
| Compiler | MSVC 19.51.36256, C++20 |
| CMake / generator | CMake 4.3.1-msvc1 / Ninja 1.13.2 |
| Windows SDK | 10.0.26100.0 |
| Project / source rate | 48000 / 48000 Hz |
| Event / render range | Timeline 1024 / `[0,8192)` in 128-frame blocks |
| Runtime | public `Node`, `LatencyNode`, `SummingNode`, and `SimpleNodePlayer` |
| Device / GUI / message loop | none opened or required |

Pins are unchanged. No binary artifact, submodule update, patch, fork, private API, high-level `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, Tracktion source reader/resampler, or Tracktion Clip placement is used.

## Public latency API evidence

| Public source | Observed contract used by this fixture |
| --- | --- |
| `tracktion_graph/nodes/tracktion_LatencyNode.h` | `LatencyNode(input, numSamplesToDelay)` adds its `LatencyProcessor` delay to `NodeProperties::latencyNumSamples`; the same node writes and reads its audio delay line. Thus the deterministic actual delay and public reported latency are the same `numSamplesToDelay`. |
| `tracktion_graph/nodes/tracktion_SummingNode.h` | `getNodeProperties` reports the maximum input latency; `transform` calls `createLatencyNodes()` unless latency compensation is disabled. |
| `tracktion_graph/nodes/tracktion_SummingNode.h` | `createLatencyNodes` calculates `maxLatency - nodeLatency` and inserts public `LatencyNode` instances for shorter inputs. The fixture contains no corresponding adapter compensation code. |
| `tracktion_graph/tracktion_Node.test.cpp` | Pinned tests construct `LatencyNode` and `SummingNode` graphs and check coherent delayed outputs, including multiple latencies. |
| `tracktion_graph/players/tracktion_SimpleNodePlayer.h` | The public player owns/prepares the transient graph and executes its transformation/processing path headlessly. |

The test Domain owns only `ProcessorId`, `ProcessorType = TestLatency`, `LatencySamples`, `Enabled`, `Order`, and `Layer` as standard C++ values. The adapter maps enabled latency state one-way to the public `LatencyNode`; it does not calculate `maxLatency - pathLatency`, insert delay itself, persist a graph, or synchronize runtime state back into Domain state.

## Measurements

Timing uses integer Timeline samples only; no timing epsilon is permitted. Amplitude tolerance is `1e-6`. `alignment error` below means the observed common output position minus the expected compensated output position. Each two-path mix uses distinct amplitudes, so the expected sum at exactly one sample proves both contributions share that position.

| Test ID | Path/layer state | Reported graph latency | Expected / observed common output | Alignment error | Status |
| --- | --- | ---: | --- | ---: | --- |
| E-A2-T0 baseline | one custom-source path, no processor | 0 | 1024 / 1024, amp 0.25 | 0 | Pass |
| E-A2-T0.5 latency contract | one custom-source path, TestLatency 1024 | 1024 | 2048 / 2048, amp 0.25 | 0 | Pass |
| E-A2-T1 2-path | A: 0; B: TestLatency 1024 | 1024 | 2048 / 2048, amp 0.75 | 0 | Pass |
| E-A2-T2 values | A: 0; B: TestLatency 256, 1024, 2048 | 256, 1024, 2048 | 1280, 2048, 3072 / same, amp 0.75 | 0 each | Pass |
| E-A2-T3 Track | A: 0; B: Track-equivalent latency 1024 | 1024 | 2048 / 2048, amp 0.75 | 0 | Pass |
| E-A2-T4 Clip | A: 0; B: Clip-equivalent latency 1024 | 1024 | 2048 / 2048, amp 0.75 | 0 | Pass |
| E-A2-T5 mixed | A: 0; B: Clip 256 + Track 768; C: Clip 1024 | 1024 | 2048 / 2048, amp 0.70 | 0 | Pass |
| E-A2-T6 order | `Latency(1024) -> Multiply(2)` and reverse | 1024 | 2048 / 2048, amp 0.50 | 0 each | Pass |
| E-A2-T7 change | stopped/rebuild B latency 1024 -> 2048 | 1024 -> 2048 | 2048 -> 3072 / same, amp 0.75 | 0 each | Pass |
| E-A2-T8 bypass | B latency 1024, `Enabled = false` | 0 | 1024 / 1024, amp 0.75 | 0 | Pass |
| E-A2-T9 reconstruction | mixed E-A2-T5 Domain -> render -> destroy -> rebuild | 1024 | 2048 / 2048, amp 1.15 | 0 | Pass |

For E-A2-T1, B's uncompensated theoretical output is 2048 (1024 event + 1024 actual delay). `SummingNode` adds 1024 latency to A through its graph transformation, so both paths are observed at 2048. For 256 and 2048 the corresponding theoretical delayed positions are 1280 and 3072. There is no manually injected preroll, negative Timeline time, Domain coordinate shift, or output-buffer shift; the render always begins at zero and includes leading silence naturally.

The fixed public `LatencyNode` fulfils `actual signal delay == reported latency`: its constructor receives precisely the Domain `LatencySamples`, its `NodeProperties` adds that latency, and its internal `LatencyProcessor` delays signal by that same value. The observed root graph latency matches all path maxima: 0, 256, 1024, or 2048 as listed.

Direct executable output:

```text
PDC baseline event=1024 observed=1024 error=0
PDC latency-contract reported=1024 actual=1024 observed=2048 error=0
PDC two-path latency=256 pathA=1280 pathB-uncompensated=1280 compensated=1280 error=0
PDC two-path latency=1024 pathA=2048 pathB-uncompensated=2048 compensated=2048 error=0
PDC two-path latency=2048 pathA=3072 pathB-uncompensated=3072 compensated=3072 error=0
PDC clip-and-track latency=1024 observed=2048 error=0
PDC mixed-layer totals=0,1024,1024 observed=2048 error=0
PDC order latency-multiply/multiply-latency timing=2048 amplitude=0.5 error=0
PDC latency-change 1024->2048 observed=2048->3072 error=0 bypass=1024 error=0
PDC reconstruction output/ranges/latency/order=equal error=0
PDC PASS elapsed_ms=0.9656
```

Maximum observed alignment error: **0 Timeline samples**.

## Boundary, lifecycle, and exclusions

The custom source receives requested Timeline sample ranges and emits the unchanged Domain event. Runtime graphs are constructed from Domain, rendered, and destroyed; the fixture compares Domain state before/after each render and observes no mutation. `LatencyNode` and `SummingNode` are transient Tracktion graph objects, never persistence.

`Enabled = false` is an explicit fixture adapter policy: no latency Node is created, and therefore no latency metadata is reported. This does not establish any production plugin bypass policy. Latency change is stopped/rebuild only; it does not test playback-time dynamic latency changes.

No Synchronization Group/Member, Clip Group, Ripple semantics, project persistence, real-media read/seek, production SRC, VST3, scanner, Tail, Phase F, realtime playback, or GUI was run.

## Verification and result

- Standard `build.ps1` configure/build completed with the new PDC target.
- Targeted CTest `tracktion_selective_reuse_pdc`: passed, 1/1, 0.02 seconds.
- Device-free direct executable exited successfully.
- The known high-level Phase E baseline remains `Inconclusive`: `WaveAudioClip` / `WaveNodeRealTime` observes `1024 -> 1026`. It is neither repaired nor reused here.

**Conditional Pass.** The deterministic Option A2 low-level graph satisfies its PDC criteria with public Tracktion latency metadata and graph-owned compensation, with maximum `alignment error = 0 Timeline samples`. Actual VST3 latency, production bypass semantics, dynamic realtime changes, and production thread behavior remain unmeasured; this result does not authorize them.
