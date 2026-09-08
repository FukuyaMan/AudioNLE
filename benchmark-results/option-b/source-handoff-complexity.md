# Option B Source Handoff Complexity

The fixture re-specifies backend-independent concepts locally: fixed pages/cache, worker reader, bounded request publication, MediaGeneration, ClipRuntimeView, and integer Timeline-to-Source mapping. B-specific work is the narrow SourceNode fixed-buffer handoff, callback counters, graph render hookup, same-source sum, and reconstruction glue. WAV generation, held-worker control, assertions, working-set observation, and CLI are test-only.

Classification: **Thin backend handoff**. The runtime stores no Timeline or graph identity and required no Tracktion or worker-arbitration redesign. This is not a production LOC estimate and does not establish dense multi-source scaling.
