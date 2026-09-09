# Production SRC Realtime Audit

Environment: Windows 11 x64, Release MSVC build, `steady_clock`, static libsamplerate 0.2.2 pin `b9c20b93660c3683fda12e3c2a01f0021bf96c56`, `SRC_SINC_BEST_QUALITY`; no affinity policy was applied. Prepared 44.1->48 calls used fixed 2048-native/512-project capacity and precreated states.

AudioNLE counters under all fixture workloads: reader/file/decode 0; wait/block/spin 0; allocation/growth 0; synchronous fallback 0; cache request creation 0; backend lifecycle 0; callback cache miss 0. Stress used 270 measured prepared blocks at 1/16/32 views with deterministic output/frame behavior and no local allocation/lock/wait event.

Source audit observes state allocation in lifecycle paths and `src_process` dispatch to existing vtable state. Observed absent: fixture-owned callback forbidden work. Unproven: exhaustive CRT heap/free interception and all OS synchronization/transitive calls. Result: **Backend process path acceptable with residual unproven internals**.
