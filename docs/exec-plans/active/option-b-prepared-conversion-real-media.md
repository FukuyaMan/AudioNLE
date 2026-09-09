# Execution Plan: Prepared Conversion Real-Media V1

## Objective

Extend the prepared conversion slice with a real bounded decoder-provider, multi-channel persistent rebuildable artifact, bounded RAM cache, reopen validation, and callback-safe cold-page behavior.

## Scope

Use the existing JUCE `AudioFormatReader` boundary for controlled WAV fixtures. Persist only derived project-rate float pages under a temporary/session cache root. FFmpeg/compressed/container support remains explicitly outside this slice because no production FFmpeg boundary exists.

## Invariants

All source/timeline/cache offsets remain integer frames; source remains immutable; callback never reads disk, decodes, runs SRC, allocates, waits, or evicts. Disk artifacts include a versioned canonical identity and never serialize backend state.

## Verification

Exercise mono/stereo real WAV directions, artifact write/reopen/header rejection, cold-page unavailable behavior, bounded eviction/pinning model, generation/config invalidation, split/trim/seek sharing, and prepared callback IAT smoke.

## Implemented evidence

Implemented: JUCE worker-only WAV reader; mono/stereo 44.1/48/96 fixtures; per-channel BEST conversion; versioned interleaved float32 artifact header/page checksums; temporary-write/atomic-rename publication; restart/reopen; generation and corrupt-header rejection. Focused CTest passes.

Implemented in the residency evidence target: fixed-record arithmetic artifact reader; fixed request table with demand priority/coalescing; worker-only page I/O/checksum/private publication; 16-slot pin/version guarded cache; deterministic worker eviction; cold reload/reopen/corruption/generation handling; bounded prefetch; sparse three-hour addressing; and concurrent callback/worker stress.

Implemented in the existing Windows IAT interception harness: the exact artifact-backed runtime path is audited for resident stereo seam copy, cold miss request publication, worker reload, worker-I/O cross-thread attribution, and eviction/reload. Each callback portion reports zero intercepted CRT/heap/lock/wait/file events and no prepared callback SRC fallback.

Remaining: wiring this engine-private prototype into the production shared worker and SourceNode lifecycle, streaming writer without fixture-owned reference storage, compressed/container/selected-stream decoder integration, and UI progress. These remain active rather than being represented as completed.
