# Windows Interception Positive Controls Status

Not run. Required controls remain malloc/calloc/realloc/free, new/delete/new[]/delete[]/aligned allocation, HeapAlloc/ReAlloc/Free, critical section/SRW, Sleep/Wait, and CreateFile/ReadFile/WriteFile. A wrapper-only test is explicitly not accepted because it does not prove interception of target static-library/CRT/Kernel call paths.
