# libsoxr timing evidence

`option_b_libsoxr_comparator` uses `soxr_create(sourceRate, projectRate, 1)`, `SOXR_FLOAT32_I` caller buffers, `SOXR_VHQ`, and `soxr_runtime_spec(1)`.  Each fresh stream uses 2048 native input and 512 project output capacity.  Input/output ownership remains with the caller; libsoxr owns its opaque state and internal filter/buffer storage.  `soxr_process(NULL, 0, ...)` is used for end-of-input flushing; `soxr_delay` reports output-domain physical delay and `soxr_clear` is available for reset.

Fresh-state deterministic and partitioned stream smoke results:

| source -> project | result | partitioned frames |
| --- | --- | ---: |
| 44.1 -> 48 | PASS | 34830 |
| 48 -> 44.1 | PASS | 29400 |
| 44.1 -> 96 | PASS | 69660 |
| 96 -> 44.1 | PASS | 14700 |
| 48 -> 96 | PASS | 64000 |
| 96 -> 48 | PASS | 16000 |

This establishes six-direction basic streaming, fresh-state determinism, finite output, flush, and the existing capacities.  It does **not** establish a libsoxr physical reconstruction policy for arbitrary timeline starts, exhaustive rational phase classes, cache/source-service integration, or the full edit matrix.  No libsamplerate `PrerollPolicyV1` was reused.  Timeline authority remains integer project/native coordinates and the exact rate rational; libsoxr delay/output count is physical-only.

The candidate failed the subsequent mandatory quality gate, so developing and validating a backend-specific history/reconstruction policy is intentionally stopped.  Logical clip end would remain AudioNLE-owned; any libsoxr filter settling would be physical preparation only, never an Effect Tail.
