# Tracktion Selective Reuse — Multi-source / Runtime-edit Complexity Review

## Classification

**Substantial adapter extension.** AudioNLE now owns per-MediaSource reader/cache workers, fixed-page request publication, Clip views, edit-driven rebuild boundaries, and invalidation responsibility. It has not implemented a general graph scheduler, generic graph mutation, plugin lifecycle, PDC update engine, or graph-wide buffer scheduler.

## Prototype responsibility estimate

`multisource_main.cpp` is approximately 320 lines. Estimates overlap: MediaSource runtime/worker/pages ~90; request publication/latest-slot arbitration ~30; Clip runtime view/mapping ~35; edit/rebuild/lifetime/reconstruction ~100; Tracktion graph integration ~20; fixture/assertions/instrumentation ~120. They are not additive.

## Scaling

```text
workers = unique active MediaSource runtimes
reader/file handles ~= unique active MediaSource runtimes
PCM cache bytes = unique media * 4 pages * 128 frames * 1 channel * sizeof(float)
request slots = unique media
Clip views / graph source nodes = active Clips
```

The 32-distinct-media case used 32 workers/caches and 65,536 bytes PCM. Cache memory is small only because this is a tiny fixture; thread, file-handle, OS scheduling, decoder and lifecycle scaling remain the significant warning. Model B would scale these resources by Clip count even for one file and remains unfavorable. The next worthwhile investigation is **worker-pool / shared decoder arbitration**; no Model C implementation is justified yet.

## Reuse and implications

Tracktion still provides graph traversal/execution, buffer propagation, `SummingNode`, processing, PDC, VST3 integration, Tail flow, and headless execution. AudioNLE owns Domain/edit semantics, integer mapping, source scheduling/cache/worker lifecycle, runtime views and rebuild/invalidation. This is still material engine reuse, so Option A2 has not become a custom engine.

Full stopped/affected rebuild is correctness-proven; persistent partial graph update remains unresolved because safe public-API node replacement/ownership was not established. It is a practicality constraint, not a pass or failure.

The MediaSource -> decoded pages -> Clip views boundary can extend to FFmpeg, but compressed media adds stream selection, seek/preroll, packet/frame numbering, cancellation, shared cache and multi-range arbitration. One decoder servicing far-apart simultaneous ranges may be insufficient; this was not implemented or decided.
