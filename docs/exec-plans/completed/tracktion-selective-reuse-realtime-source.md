# Completed Execution Plan: Option A2 Realtime Prefetch / Cache / Thread Ownership

Completed: 2026-09-07 (Asia/Tokyo)

## Objective and boundary

Evaluate a bounded AudioNLE-owned same-rate realtime source adapter: integer request scheduling, fixed PCM pages, background JUCE reader worker, runtime-only generation invalidation, callback-safe page supply, and public low-level Tracktion graph consumption. This plan did not select a production architecture.

## Chronology and result

* Q0 selected AudioNLE fixed pages plus a dedicated public JUCE `Thread`/`WaitableEvent` reader worker.
* Q1–Q4 passed prefetched playback, zero-on-miss recovery, generation invalidation, and rapid seeks.
* Q5 passed 257-frame non-divisible pages, 128-frame cross-page processing, and page ownership exclusion.
* Q6–Q8 passed fixed-memory observation, idle/pending worker destruction, and reconstruction.
* Q9 initially failed because PCM16 `0.875` decoded to `0.874969`; the source and Multiply output were correct. Corrected decoded-PCM × 2 expectations passed near and one-hour cases at error 0.
* Q10 classified the source subsystem as a substantial adapter subsystem and the gate as **Proceed with Constraints**.

Callback reader/file calls, waits/blocks, allocation/growth, and synchronous miss fallback were 0; stale output was 0; required timing error was 0 Timeline samples. The default cache was 8192 bytes PCM.

## Constraints and next gate

No audio-device callback, OS dropout guarantee, production cache sizing, multi-source/dense-Clip scaling, runtime edits, mixed-rate/SRC, FFmpeg, compressed media, persistence, export, ADR, or production architecture decision was made. The recommended next gate is dense / multi-source / runtime-edit source scheduling.

See `docs/design/prototypes/tracktion-selective-reuse-realtime-source-results.md` and the realtime-source benchmark records for evidence.
