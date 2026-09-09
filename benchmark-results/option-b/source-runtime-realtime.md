# Source Runtime Callback Evidence

The Windows IAT harness compiles the actual source-runtime implementation and calls `SourceNode::process` through `ClipRuntimeView`. Native, realtime BEST, prepared cold, prepared reload, and mixed callbacks pass with zero intercepted CRT/heap/lock/wait/file events. Native and prepared have zero realtime SRC calls; realtime has `src_process > 0`.

Prepared cold returns `Unavailable` with an exact zero caller buffer. Worker draining happens after the callback, and the retry returns `Ready` from the persistent artifact residency path.
