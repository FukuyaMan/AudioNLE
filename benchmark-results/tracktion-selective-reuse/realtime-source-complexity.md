# Tracktion Selective Reuse — Realtime Source Complexity Review

## Classification

**Substantial adapter subsystem.** The realtime source work owns fixed PCM pages, request publication, a worker, generation invalidation, underrun policy, page ownership, and destruction. It is not custom-engine-scale: it does not implement graph traversal, graph buffer allocation, mixing, PDC, VST3 hosting, or Tail flow.

## Measured prototype surface

`realtime_source_main.cpp` is approximately 470 lines. Responsibility estimates overlap: fixed cache/page ownership ~110 lines; worker/reader/wakeup ~85; source Node/cross-page copy ~65; generation/request/cancellation ~65; fixture/assertion/render/instrumentation ~145. These are prototype measurements, not production estimates.

| Resource | Observed fixture value |
| --- | --- |
| Reader workers | One JUCE `Thread` per prototype source |
| Default pages | 8 × 256 mono float frames = 8192 bytes PCM |
| Boundary fixture | 257 frames/page, 128-frame process, cross-page pass |
| Control storage | Fixed atomics for source start, generation, sequence, page state, reader count |
| Events | Worker wake, held-read release, reader-release notification |
| Callback storage | Fixed 257-frame scratch; no dynamic allocation |
| Callback locks/waits/file reads | 0 / 0 / 0 |

JUCE is actually reused for WAV range decode, `Thread`, and `WaitableEvent`. Tracktion is actually reused for public low-level `Node`, `SimpleNodePlayer`, graph traversal/buffer flow, processing, `SummingNode`, PDC, hosted VST3 graph processing, and Tail propagation established in other A2 fixtures. Rejected high-level source scheduling is not reuse credit.

## Boundary, Option B comparison, and FFmpeg

AudioNLE owns authoritative integer scheduling, read planning, cache/prefetch, worker lifecycle, invalidation, underrun policy, and realtime source supply. Tracktion still saves graph topology/traversal, processing, summing, PDC, VST3 graph integration, Tail propagation, and headless graph execution. This keeps A2 viable but constrained; it is not an architecture decision.

**FFmpeg boundary: needs extension, not replacement.** A future decoder worker can retain decoded integer-source pages -> cache -> realtime Node without changing Domain, Timeline scheduling, or Tracktion graph use. It must add packet/frame metadata, keyframe/preroll, codec delay/delayed frames, varying decoded sizes, stream selection, cancellation, and source numbering. WAV byte offsets are not part of the cache contract.

Multiple simultaneous sources, shared media workers/caches, overlapping reads, rapid edits, and cache priority are unmeasured and are the dominant next scaling risk.
