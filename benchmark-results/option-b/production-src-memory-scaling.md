# Production SRC Memory Scaling

Fixed MediaSource cache remains 8 x 257 float frames = 8224 bytes per source runtime. Each active view has one opaque libsamplerate state, fixed 2048-float prepared native capacity (8192 bytes) and 512-float staging (2048 bytes), plus bounded metadata. Shared-source workloads share the 8224-byte cache; distinct-source workloads multiply only that fixed source-service cost. No duration-scaled decoded PCM was retained. Working-set OS measurement is deferred to the performance optimisation gate.
