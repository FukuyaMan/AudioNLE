# Production-facing Source Runtime Skeleton

The engine-private source boundary is `SourceRuntime` (shared source/path state), `ClipRuntimeView` (integer timeline/source coordinates), `SourceNode` (common project-rate caller buffer), and `SourceRenderResult` (`Ready`, `Unavailable`, `Failed`, `Stale`, `Unsupported`). No backend object, file handle, decoder, or artifact detail enters the view or downstream node.

Admission is selected before callback. Native-rate uses no SRC state; GuaranteedRealtime uses the ADR 0002 BEST runtime; PreparedRequired uses the persistent artifact residency runtime and never falls back to realtime SRC. A cold prepared page returns `Unavailable` and exact zeroes, then a fixed round-robin shared worker services bounded prepared requests off callback.

The current skeleton has an explicit fixed worker registry (eight source runtimes), no worker per source/clip/artifact, and construction/reconstruction occurs outside callback. It validates source move, split logical view, and fresh prepared-runtime reconstruction. The existing Windows IAT harness executes the actual `SourceRuntime -> ClipRuntimeView -> SourceNode` route for native, BEST realtime, prepared cold/reload, and mixed rendering: intercepted CRT/heap/lock/wait/file events are zero. Only the realtime route calls `src_process`.

`PhysicalSrcRangePlanner` is now an engine-private, backend-neutral component. It owns exact P/Q physical-range planning, `PrerollPolicyV1`, source-end clamping and 2048-native/512-project capacity rejection. `NativeSourceService` has two fixed PCM pages (16 KiB mono) and an eight-record fixed request table; callback misses publish or coalesce requests and return silence/unavailable, while only `serviceOne()` invokes the provider.

`SharedNativeSourceRuntime` is the source-level native ownership boundary. It owns the fixture provider and `NativeSourceService`; each `SourceRuntime` owns only its fixed staging buffer and optional libsamplerate state. It is created or reconfigured outside callback and is reference-counted by active views. Prepared artifact residency now has an equivalent source/config-scoped shared binding: a supplied shared residency must match the artifact key (source, generation, config, channel count, and project rate) before it may be used. Views do not create a second cache or page-load request for the same key.

The native and prepared work queues are still serviced independently through the fixture worker; a common bounded fairness worker is active work.

The selected Option B low-level graph now has a `runtimeSource` node kind that delegates fixed project-rate blocks to the engine-private `SourceNode`. It neither accesses provider/artifact/worker details nor owns SRC state. Its normal N-input sum has been exercised with native, realtime, and prepared routes. A cold prepared route contributes exact zero through that node; after worker service, its contribution appears without graph topology mutation.

## Production target boundary

`audionle_engine_source_runtime` is the separately compiled engine-private static library. It owns the `SourceRuntime` implementation and prepared-residency implementation; `source_runtime_engine.hpp` exposes only the callback contract, source-node adapter, worker facade, and opaque prepared-residency handle. It exposes neither reader/file handles, `SRC_STATE`, residency slots, nor Windows IAT types.

The standard CMake/CTest scope builds this Option B runtime and its direct JUCE
hosting fixtures. Current `main` has no Tracktion Engine dependency; historical
Tracktion conclusions remain as Markdown evidence, while the rejected
executable implementation is recoverable from Git history. No production
`SourceRuntime` target links to Tracktion.

The graph target, Windows IAT harness, and focused route tests link this same library. No target includes another implementation `.cpp`; therefore the callback and IAT evidence execute the one linked runtime implementation.

`SharedSourceWorker` is a fixed eight-runtime/two-category round-robin scheduler. It services native and prepared demand one work item at a time, does not create threads, and leaves prepared prefetch priority inside the prepared residency after demand. Its bounded metrics expose submitted, coalesced, completed, queue-full, maximum service gap, and starvation status. The production validation fixture exercises G1/G2 reconstruction, 48/96 kHz identity reconstruction, admission changes, edit-view lifecycle, and a seeded three-hour integer-coordinate native/realtime/prepared graph. The final linked Windows IAT route covers native ready/miss/refill, realtime, prepared resident/cold/reload, stale generation, saturation, concurrent worker activity, and mixed rendering.
