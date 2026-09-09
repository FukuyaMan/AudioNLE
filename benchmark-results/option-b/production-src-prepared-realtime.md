# Prepared SRC Realtime Evidence

The callback surface is bounded scans of 16 slots and 8 request entries plus caller-owned output copying. It has no file handle, decoder, libsamplerate state, mutex, wait, allocation, or eviction path. Cold calls produce silence/`Unavailable` and only publish a fixed request.

The existing Windows IAT harness now compiles the same artifact-backed residency runtime and verifies resident page, stereo seam, cold demand publication, worker reload, worker-I/O cross-thread attribution, and eviction/reload. Every marked callback has zero CRT, heap, lock, wait/sleep, and file-I/O events. Prepared callback `src_process` and `unexpectedRealtimeSrcFallback` remain zero. This is isolated residency proof; shared-worker/SourceNode integration remains active.
