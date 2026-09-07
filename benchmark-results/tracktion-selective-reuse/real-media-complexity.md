# Tracktion Selective Reuse — Real-media Complexity and Boundary Review

Execution date: 2026-09-06 (Asia/Tokyo)

## Result

**Proceed with Constraints.** The measured code is bounded adapter glue for an offline WAV feasibility fixture. It does not make JUCE's reader a custom decoder or a production source system, and it does not make Tracktion an authoritative media scheduler.

## Responsibilities and ownership

| Component | Prototype responsibility | Ownership/lifetime |
| --- | --- | --- |
| Framework-free `DomainState` | path identity, integer source length, Timeline start, Source offset | value state; no JUCE/Tracktion/runtime data |
| `WavReader` | opens public JUCE reader and performs requested absolute range reads | owns `AudioFormatManager` and `AudioFormatReader`; transient per source node |
| `RealMediaSourceNode` | derives range overlap from `ProcessContext::referenceSampleRange`, maps Timeline to Source integers, zero-fills non-overlap | owns reader; fixed 128-frame scratch buffer; request log belongs to test result |
| `GeneratedMarkerNode` | independently scheduled deterministic fixture B for the mix check | graph-owned transient test node |
| `MultiplyNode` | deterministic downstream processing check | owns its input node; graph-owned transient wrapper |
| `SummingNode` / `SimpleNodePlayer` | public graph traversal, buffer flow, mix, and offline output processing | Tracktion graph/player owns graph lifetime |

The custom source-node plus bounded reader/scheduling portion is approximately 120 focused lines in `real_media_main.cpp`; fixture generation, assertions, error tests, and working-set instrumentation are test harness rather than production adapter code. The processing helpers are approximately 70 additional test-only lines. No separate cache manager exists: the reader has no retained decoded PCM cache and the source node has one block-sized scratch array. “Reader buffer = 0” in the long-source result means no AudioNLE cache/preload was configured; it is not a claim about all internal operating-system or JUCE buffering.

## Reused public API surface

* JUCE: `AudioFormatManager`, `registerBasicFormats`, `createReaderFor`, `AudioFormatReader::read`, and `WavAudioFormat` only.
* Tracktion Graph: public `Node`, `ProcessContext::referenceSampleRange`, `SimpleNodePlayer`, and `SummingNode` only.
* No `Engine`, `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, Tracktion file/cache manager, private API, patch, or fork is used.

`WavReader` is deliberately synchronous and is invoked by `RealMediaSourceNode::process` in this headless/offline prototype. It may perform filesystem I/O, decoding/conversion, allocation, and blocking. That violates the repository's realtime constraints if reused unchanged on an audio thread; therefore this result neither claims realtime safety nor proposes that lifecycle.

## Coupling and future decoder boundary

AudioNLE retains integer Timeline/Source mapping, source-range scheduling, source identity, and runtime lifecycle policy. JUCE is behind the narrow operation “read this absolute source range into this caller-provided buffer.” Tracktion starts only after the source node has supplied its buffer, and its graph cannot change Domain scheduling authority.

A future FFmpeg adapter can occupy the same range-reader boundary, but that work is explicitly deferred. It must separately define stream selection, timestamp-to-integer source mapping, decoder seek/preroll, packet/frame cache bounds, EOF/error behavior, ownership, cancellation, and thread handoff. It must not inherit this WAV fixture as proof of compressed/container behavior.

## Comparison and retained constraints

This is smaller than custom engine-scale work because it does not implement WAV parsing, codec decode, graph traversal, graph mixing, buffer graph ownership, or a cache manager. It is also intentionally incomplete: production cache/prefetch, concurrent reader ownership, realtime-safe handoff, media probing, non-WAV formats, source changes, waveform/proxy generation, export, and persistence are outside scope.

The one-hour fixture showed construction and far seeks without duration-scaled decoded PCM retention in this path. It is bounded evidence for this fixture, not a memory guarantee or throughput benchmark for production media.
