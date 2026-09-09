# Windows SRC Harness Import-Map Audit

Audited final Release harness: `build/prototype/option-b-low-level-graph/option_b_windows_iat_crt_harness.exe`, PE x64. The runtime loaded-image parser found and patched these IAT imports: `malloc`, `calloc`, and `free` from `api-ms-win-crt-heap-l1-1-0.dll`; `HeapAlloc`, `HeapReAlloc`, `HeapFree`, required critical-section/SRW APIs, required wait/sleep APIs, and `CreateFileW`/`ReadFile`/`WriteFile` from `KERNEL32.dll`.

Because the replacement is installed in the caller executable's IAT slot, Kernel32 -> KernelBase/API-set forwarding is outside the audited call route: each named caller import reaches the hook before any export forwarding. Runtime positive controls proved each mandatory slot fires inside the callback marker; full negative and cross-thread controls remained zero.

The same loaded-image parser checked `realloc`, `_aligned_malloc`, `_aligned_free`, and the MSVC x64 scalar/array `operator new`/`operator delete` import names. None is present in this executable's IAT, so each is classified `NotPresentInRelevantTargetPath` for this exact Release binary. This is not a claim about arbitrary modules or future link configurations.
