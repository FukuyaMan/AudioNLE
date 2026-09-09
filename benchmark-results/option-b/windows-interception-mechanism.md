# Windows Interception Mechanism Assessment

Selected future architecture: bounded **IAT patching plus verified detours where an API is not imported through the executable IAT**. IAT patching alone is insufficient for static-library internal calls, CRT internal/inlined allocator paths, forwarded Kernel32/KernelBase/API-set exports, and dynamically resolved procedures. A hook is claimable only after the actual import/thunk/original target is resolved, patched, called, counted and behavior-preserved.

| Category | Expected route | Required mechanism | Current coverage |
| --- | --- | --- | --- |
| CRT common allocation/new | MSVC CRT/import or internal thunk | IAT where imported; detour/internal audit otherwise | Unproven |
| aligned CRT allocation | CRT internal/thunk | verified detour or documented blind spot | Unproven |
| HeapAlloc/ReAlloc/Free | Kernel32 forward to KernelBase/API-set | resolved forwarded target/IAT or detour | Unproven |
| lock/SRW/wait/sleep | Kernel32/KernelBase/API-set | resolved target/IAT or detour | Unproven |
| CreateFile/ReadFile/WriteFile | Kernel32/KernelBase/API-set | resolved target/IAT or detour | Unproven |
| thread creation | Kernel32/KernelBase | optional target/IAT or detour | Unproven |

CRT linkage mode and exact import map must be captured from the built diagnostic executable before installation. No mechanism is claimed active yet.
