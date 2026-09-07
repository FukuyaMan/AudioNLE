# Option A2 Real-media / Long-source Streaming Results

Execution date: 2026-09-06 (Asia/Tokyo)

## Classification

**Proceed with Constraints.**

This bounded feasibility gate establishes an AudioNLE-owned, framework-free same-rate WAV source boundary with exact integer sample placement, deterministic seek/reset/rebuild, range-local long-source behavior, and minimum public low-level Tracktion downstream processing compatibility. It does not select a production source architecture, select Tracktion as a media scheduler, or create an ADR.

## Scope and selected boundary

The fixture is temporary mono 48 kHz PCM16 WAV. Pinned JUCE public `AudioFormatManager` / `AudioFormatReader` reads explicitly requested absolute `int64` source ranges. The framework-free Domain contains only source identity, source length, Timeline start, and Source offset. The AudioNLE custom source `Node` maps `ProcessContext::referenceSampleRange` to that explicit read; public Tracktion graph objects then process its supplied audio.

No `Engine`, `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, Tracktion media/cache manager, private API, patch, fork, whole-file PCM preload, or high-level Tracktion scheduling is used. The reader is synchronous/offline and is not realtime-safe evidence.

## R0–R5 measurements

| Phase | Result |
| --- | --- |
| R0 reader selection | Public JUCE WAV reader was the smallest viable range-reader boundary; source investigation is recorded in [real-media-api-investigation.md](../../../benchmark-results/tracktion-selective-reuse/real-media-api-investigation.md). |
| R1 exactness | All 11 small WAV markers reached direct reader and custom-source/public-graph output at 0 Timeline-sample error. |
| R2 boundaries | Markers around 128, 1024, and 4096 with 128-frame processing and varied render starts retained absolute placement; post-start negative controls had no stale output. |
| R3 seek/rebuild | Forward, backward, repeated reads and Domain -> destroy -> rebuild produced exact values, equal output, and equal requested ranges. |
| EOF/errors | Missing, invalid, and truncated WAV construction failed explicitly; past EOF produced exact zero. |
| R4 long source | One-hour-plus-one-sample WAV (345,600,106 bytes) was written in chunks. Beginning, 1-minute, 10-minute, and 1-hour reads and far backward seek were exact; working set stayed about 9.1 MB and no decoded duration-scaled buffer was retained. |
| R5 processing | real WAV -> Multiply(2) gave `0.4375 -> 0.875`; public `SummingNode` mixed independent `0.25` at 1024 to `0.6875`; subsequent Multiply(2) gave `1.375`; the 10-minute marker gave `0.375 -> 0.75`; graph rebuild was equal. All timing errors were 0. |

The raw exactness and processing record is [real-media.md](../../../benchmark-results/tracktion-selective-reuse/real-media.md); the long-source record is [long-source.md](../../../benchmark-results/tracktion-selective-reuse/long-source.md).

## R6 boundary and complexity review

The adapter remains bounded glue: Domain controls identity and integer mapping; the transient reader owns file/decode state; the source node owns only a 128-frame scratch buffer; public Tracktion owns graph traversal and public mixing. There is no production cache or preload. The detailed ownership, public API, coupling, and deferred FFmpeg boundary review is [real-media-complexity.md](../../../benchmark-results/tracktion-selective-reuse/real-media-complexity.md).

The lack of a source-level cache is intentional for this gate, not a production cache design. The selected JUCE reader may access the filesystem, allocate, decode, or block during `process`; it must be moved behind a later prefetch/cache and thread-ownership design before realtime use.

## R7 mixed-rate follow-up decision

A separate **44.1 kHz Source -> 48 kHz Timeline real-media feasibility phase is justified but deferred.** The earlier generated-source A2-3 integer/rational mapping evidence does not establish real decoder/SRC behavior. A new gate must separately define and measure:

* exact Source/Timeline integer mapping and its rounding/end-boundary policy;
* SRC quality, phase, latency, and deterministic reconstruction;
* decoder seek/preroll and cache keys/range locality across rate conversion;
* channel/layout and container timestamp semantics;
* dependency choice and ownership boundary for FFmpeg or another decoder/SRC;
* prefetch, cancellation, lifecycle, and realtime-safe thread handoff.

No mixed-rate real media, FFmpeg, production cache, realtime playback, waveform/proxy, export, UI, persistence, or ADR is implemented by this result.

## Verification

`build.ps1` configured and built the prototype. The targeted CTest and direct executable passed:

```text
REAL-MEDIA R1-R3 markers=11 max-error=0 reconstruction=equal eof=zero errors=pass
REAL-MEDIA R4 duration-samples=172800001 file-bytes=345600106 reader-buffer=0 ... long-markers=4 max-error=0
REAL-MEDIA R5 source=1024 input=0.4375 processed=0.875 mix=0.6875 mix-then-multiply=1.375 long=0.75 reconstruction=equal max-error=0
REAL-MEDIA PASS
```

The full suite still has the pre-existing high-level Phase E PDC baseline failure (`expected 1024`, observed `1026`). It is unrelated to this custom source-node result, was not changed, and is not used as evidence here.
