# Option A2 Realtime Source Results

## Final classification

**Proceed with Constraints.** Q0–Q9 establish bounded same-rate WAV evidence for an AudioNLE-owned realtime source adapter: fixed cache pages, worker-owned reader, zero-on-miss underrun policy, generation invalidation, page ownership, non-divisible/cross-page handling, bounded memory, destruction, reconstruction, and downstream Multiply(2). This is not a production realtime engine or final architecture decision.

Callback reader/file calls, waits/blocks, allocation/growth, and synchronous miss fallback were 0. Stale output was 0; required marker timing error was 0 Timeline samples. The default cache was 8 × 256 mono float frames = 8192 bytes; 257-frame pages with 128-frame cross-page processing also passed.

Q9 initially failed only because `0.875` PCM16 decoded as `0.874969`; Multiply output `1.74994` was exact decoded-source × 2. The corrected Q9 pass compares processing output with decoded source PCM, not pre-quantisation fixture values. See [Q9 diagnostic](../../../benchmark-results/tracktion-selective-reuse/realtime-source-q9-diagnostic.md).

## Complexity and boundary

The source subsystem is a **substantial adapter subsystem**, not trivial glue and not custom-engine-scale. AudioNLE owns scheduling, cache/prefetch, worker lifecycle, invalidation, and underrun policy; Tracktion still supplies graph traversal, processing, summing, PDC, VST3 graph integration, Tail flow, and headless execution. See [complexity review](../../../benchmark-results/tracktion-selective-reuse/realtime-source-complexity.md).

The FFmpeg boundary needs richer decode request/result metadata but the decoded-source-page consumer contract can remain. No FFmpeg, mixed-rate/SRC, device callback, production cache sizing, multi-source scaling, dense-Clip work, runtime edits, persistence, export, or ADR is included.

## Option A2 implication and next gate

**Continue with significant constraints.** The recommended single next gate is **dense / multi-source / runtime-edit source scheduling**: concurrent real-media Clips, same-file ranges, different files, shared versus per-source workers/caches, rapid edit invalidation, and bounded callback behavior.

## Verification

The corrected target CTest passed 1/1 and direct output ended `REALTIME-SOURCE PASS`. The known unrelated high-level Phase E PDC failure remains unchanged. `git diff --check` reports no whitespace error.
