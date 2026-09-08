# Option B Source Handoff R0/R1 Audit

The A2 realtime fixture was reused by concept only. Fixed Source-page identity, pages, bounded request slot, worker-owned reader, MediaGeneration discard, reader/page readiness, and zero underrun are backend-independent candidates. Tracktion Node/ProcessContext/buffer/player are replaced locally; WAV generation, holds, counters, assertions, and CLI remain test-only. No shared extraction or source copy was made.

The B fixture contract is `publishRequest(SourceSample, FrameCount, MediaGeneration)` plus `copyPreparedRange(..., callerProvidedOutput) -> Complete|NotReady`. It accepts no Timeline, graph identity, plugin/PDC/Tail state, or float seconds. Clip view owns Timeline mapping/revision; runtime owns Source pages/reader/generation. `NotReady` zeroes output, increments underrun, and publishes only the bounded latest request.
