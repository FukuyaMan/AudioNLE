# Production SRC Supported-Tier Breakdown

The diagnostic fixture uses the production request policy: `nativeRequest = ceil(projectFrames / ratio) + 4` and `output_frames = projectFrames`; one `src_process` call is made per active view per callback. Its view contains only fixed 2048-frame input and 512-frame output arrays. It performs no source read/decode, allocation/growth, lifecycle, staging copy, or summing work in the measured loop. Existing callback counters remain zero; mandatory IAT and first/steady libsamplerate smoke remain the separate runtime proof.

At 256 frames, equivalent empty-callback timing was 0.0158203 us for 8 views and 0.0164062 us for 16 views, versus 810.940 us and 1650.180 us for 44.1->48 `src_process`. Harness/loop overhead is therefore negligible relative to backend work. The fixture's bytes copied and cleared in the measured loop are zero; it supplies caller-owned native input directly and does not clear/copy capacity-sized ranges.

The short matrix records p50/p95/p99/worst and threshold/hard counts in the runner output. 44.1->96 16 views produced 68 samples over threshold in product-like mode and 324 (including 8 hard-deadline samples) under fixed affinity. 96->44.1 16 views produced 35 over threshold product-like and 200 (51 hard) controlled. These are not rare timer-only spikes; the 16-view p50 values are already 1611.1 us and 3447.5 us respectively.

Warm-up comparison for failing 44.1->96 / 8 views did not improve performance: 256/1024/4096 warm-up blocks yielded worst headroom 0.655388/0.658838/0.742575. The fixed 256-block production protocol is retained.
