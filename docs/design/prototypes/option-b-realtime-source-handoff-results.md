# Option B Realtime Source Handoff Results

## Outcome

Gate: **Proceed with Constraints**. Integration: **Thin backend handoff**. This validates the ADR-0001 source-to-B-graph handoff only, not production source scheduling.

R0/R1 retained conceptual reuse only: MediaSource runtime owns Source pages, worker/reader, Source generation and bounded latest request; Clip views own Timeline mapping/revision; SourceNode owns fixed graph-buffer copying, zero underrun, and counters. No Timeline key, graph identity, JUCE plugin state, or runtime pointer entered descriptions/cache state.

R2 real WAV, R6 257/128 geometry, R7 Multiply(2), R8 shared-runtime two-view sum, and R9 fresh reconstruction passed at maximum timing error 0. R3 callback reader/file/decode, wait/block/spin, allocation/growth, and fallback counters were all 0. R4 miss emitted exact zero plus underrun then recovered. R5 discarded stale generation work (`discard=1`). R10 reached duration 172800001 samples with fixed 8224-byte cache, fixed 512-byte SourceNode scratch, and retained full-source PCM 0. Direct-run working sets were 8253440, 8253440, and 8249344 bytes; these are representative OS/runtime observations, not a hard bound, and show no duration-scaled source storage.

The B target uses JUCE WAV reader/worker primitives and has Tracktion source/header/linkage 0. It does not prove arbitrary plugin safety, OS dropout behavior, FFmpeg, SRC, device output, live mutation, or multi-range arbitration. The single latest-request slot is bounded fixture behavior only.

The common-core conclusion is **yes**: in this bounded fixture source/cache/worker remained backend-independent in practice, with SourceNode/buffer/lifecycle glue as the only B-specific handoff. Next gate: **B multi-source arbitration / worker scaling**.
