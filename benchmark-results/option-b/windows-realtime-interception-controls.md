# Windows Realtime Interception Controls Status

No controls were run. The repository has no verified IAT/detour interception layer for MSVC CRT allocation APIs, Kernel32 heap/synchronization/file APIs, or static-library call paths. Wrapper-only controls are deliberately rejected as invalid evidence because they cannot detect target calls that bypass wrappers.

Required before a valid result: non-allocating thread-local callback marker; recursion-safe bounded counters; verified interception coverage; CRT malloc/new/aligned controls; HeapAlloc control; critical-section/SRW/Sleep/Wait controls; CreateFile/ReadFile/WriteFile control; clean fixed-copy negative control; non-callback-thread isolation; and first/steady `src_process` smoke audit.
