# Completed Execution Plan: Option A2 Real-media / Long-source Streaming

Completed: 2026-09-06 (Asia/Tokyo)

## Objective

Establish a bounded feasibility result for an AudioNLE-owned real-media source node: explicit integer WAV range reads, exact same-rate Timeline placement, deterministic seek/reset/rebuild, long-source range locality, and minimum public Tracktion graph compatibility. This plan could not choose a production media architecture.

## Scope and invariants

The implementation is isolated to `prototype/tracktion-selective-reuse/src/real_media_main.cpp`. It uses a temporary mono 48 kHz PCM16 WAV, framework-free Domain state, public JUCE `AudioFormatReader`, and public Tracktion Graph `Node`, `SimpleNodePlayer`, and `SummingNode` only. Domain remains free of file/decoder/cache/Tracktion/JUCE runtime identity; Source and Timeline positions remain signed integer samples; every expected marker requires exact zero Timeline-sample error.

High-level `WaveAudioClip` / `WaveNodeRealTime`, Tracktion media scheduling/cache management, private API, patches/forks, whole-file decoded PCM, FFmpeg, mixed-rate real media, realtime behavior, production cache, export, UI, persistence, and ADR work were excluded.

## Completed work

* R0 selected public JUCE WAV range reading and recorded alternatives/API evidence in `benchmark-results/tracktion-selective-reuse/real-media-api-investigation.md`.
* R1/R2 passed all 11 small-fixture and block/render-start markers at error 0.
* R3 passed forward/backward/repeated read and destroy/rebuild equality.
* EOF/error tests explicitly failed construction for missing, invalid, and truncated files; past EOF returned exact zero.
* R4 passed a chunk-generated one-hour-plus-one-sample (345,600,106-byte) fixture without duration-scaled decoded PCM retention.
* R5 passed real source -> Multiply(2), public `SummingNode` mix with independently scheduled source, mix -> Multiply(2), far 10-minute processing, and processing-graph reconstruction at error 0.
* R6 recorded ownership, coupling, offline limitations, and the future FFmpeg adapter boundary in `benchmark-results/tracktion-selective-reuse/real-media-complexity.md`.
* R7 decided that a separately scoped 44.1 kHz Source -> 48 kHz Timeline real-media gate is justified but deferred.

## Verification and result

`build.ps1` configured and built the target. Targeted CTest and the direct executable passed, ending with:

```text
REAL-MEDIA R5 source=1024 input=0.4375 processed=0.875 mix=0.6875 mix-then-multiply=1.375 long=0.75 reconstruction=equal max-error=0
REAL-MEDIA PASS
```

Classification: **Proceed with Constraints.** Full results are in `docs/design/prototypes/tracktion-selective-reuse-real-media-results.md`. The known unrelated high-level Phase E PDC failure (`expected 1024`, observed `1026`) remains unchanged and is not evidence for this result.
