# Execution Plan: Source Runtime Integration

## Objective

Establish an engine-private common project-rate source runtime for native, realtime BEST SRC, and prepared artifact PCM.

## Implemented slice

`SourceRuntime`, `ClipRuntimeView`, `SourceNode`, typed result, fixed shared-worker registry, and mixed native/realtime/prepared route fixture. Prepared cold pages are silent/unavailable until worker service; runtime reconstruction uses persistent artifact identity.

## Invariants

Integer coordinates remain authoritative. Callback has no artifact I/O, allocation, locking, waiting, or lifecycle; PreparedRequired never invokes realtime SRC. Views do not own shared cache/artifact state.

## Runtime IAT evidence

The existing Windows interception harness directly executes this runtime route. Native Ready, GuaranteedRealtime Ready, PreparedRequired cold Unavailable, prepared reload Ready, and a mixed callback all pass with zero CRT/heap/lock/wait/file events. Only the realtime route invokes `src_process`; native and prepared routes do not.

## Completed increment

The reusable integer-only `PhysicalSrcRangePlanner` and an eight-record, generation-tagged `NativeSourceService` request table are now in the runtime fixture. Native page misses are callback-safe and provider reads remain worker-side. Direct CTests cover all six 44.1/48/96 directions, start/interior/end/capacity planning, request coalescing, saturation and post-drain recovery.

`SharedNativeSourceRuntime` now owns native provider/cache lifetime. Two same-source realtime views share it while retaining separate libsamplerate states; after one view is destroyed, the remaining view still renders through the same shared service.

PreparedRequired views can now receive one shared persistent artifact residency. The artifact key is checked before binding; cold demand coalesces in the one residency queue, and one view's destruction leaves that residency available to the other.

The SourceNode route is connected to the selected AudioNLE-owned Option B graph fixture. Its normal summing path renders native, realtime, and prepared inputs together; prepared cold is zero-only and reload requires no graph rebuild.

## Completed extraction increment

`audionle_engine_source_runtime` is now a separately compiled engine-private library. `SourceRuntime` uses pImpl to keep `SRC_STATE`, staging storage, and residency internals out of its header. Prepared artifact identity and residency are represented by an opaque handle plus narrow engine-private functions. The graph, Windows IAT harness, and focused route tests link that library; all direct implementation `.cpp` includes have been removed.

Focused CTest coverage passes for graph routing, native/realtime/prepared source routes, lifecycle/shared ownership aliases, prepared disk residency, and the Windows IAT route audit.

## Completed production validation gate

Implemented a fixed eight-runtime/two-category round-robin scheduler in the linked production library. Native and prepared demand are serviced one item at a time; prepared residency retains demand-before-prefetch priority. The worker creates no thread and reports bounded submitted/coalesced/completed/queue-full/service-gap/starvation metrics.

`option_b_production_source_runtime_validation` passes same-generation recreation, G1-to-G2 queued-work isolation, 48-to-96 and 96-to-48 identity reconstruction, GuaranteedRealtime/BestEffortRealtime/PreparedRequired routes, seek/trim/re-expand/split/move/shared-sibling lifecycle, saturation/recovery, and a seeded three-hour (518,400,000 project-sample) mixed graph. Its fixture uses sparse persistent artifact records and synthetic native pages, so RAM and prepared page indexing remain fixed rather than duration-scaled.

The linked Windows IAT harness now audits native ready/miss/refill, realtime, prepared resident/cold/reload, stale generation, prepared saturation, concurrent worker service, and mixed rendering. All callback intervals report zero CRT, heap, lock, wait/sleep, file I/O, and provider reads. Only realtime increments `src_process`; PreparedRequired has no realtime fallback.

All eight focused CTests pass. This plan is complete; the full suite's unrelated historical failure remains `tracktion_feasibility_phase_e_pdc` (`expected=1024`, `observed=1026`).
