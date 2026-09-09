# SpeexDSP Alternative Timing Evidence

The comparator uses quality 10, one channel, `speex_resampler_init_frac(1, sourceRate, projectRate, sourceRate, projectRate, ...)`, caller-owned 2048-frame native input and 512-frame project output. It uses actual input consumption returned by `speex_resampler_process_float`; no capacity was changed. Continuous and partitioned runs are deterministic and finite for all six common directions. Reset is available via `speex_resampler_reset_mem`; input/output latency is public and physical-only.

| source -> project | input latency | output latency | result |
| --- | ---: | ---: | --- |
| 44.1 -> 48 | 128 | 139 | PASS |
| 48 -> 44.1 | 140 | 129 | PASS |
| 44.1 -> 96 | 128 | 279 | PASS |
| 96 -> 44.1 | 280 | 129 | PASS |
| 48 -> 96 | 128 | 256 | PASS |
| 96 -> 48 | 256 | 128 | PASS |

This is a minimal timing comparator, not a validation of PrerollPolicyV1 or the complete PhysicalSrcRangePlanner/NativeSourceService edit lifecycle. Quality failure prevented further source-service, IAT smoke, and performance work.
