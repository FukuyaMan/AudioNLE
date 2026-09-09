# Production SRC Mode Comparison

## Pinned libsamplerate modes

`include/samplerate.h` at libsamplerate 0.2.2 revision `b9c20b93660c3683fda12e3c2a01f0021bf96c56` defines `SRC_SINC_BEST_QUALITY = 0`, `SRC_SINC_MEDIUM_QUALITY = 1`, and `SRC_SINC_FASTEST = 2`, followed by non-production `SRC_ZERO_ORDER_HOLD` and `SRC_LINEAR`. The header supplies names/descriptions through `src_get_name` and `src_get_description`; it does not promise a public delay bound. Medium and Fastest are the only lower sinc candidates selected. Both preserve fixed-ratio caller-buffer `src_process`; ZOH/linear were excluded.

## Timing/preroll gate

The existing six-direction timing/edit fixture was made converter-configurable. It preserves integer/rational authority, `PrerollPolicyV1` (1024 native frames plus phase rounding), fresh reconstruction, phase coverage, seek/split/source-boundary, worker/cache, and partition paths. Both commands failed before quality/performance measurement:

| mode | command | result |
| --- | --- | --- |
| `SRC_SINC_MEDIUM_QUALITY` | `option_b_libsamplerate_timing_contract --medium` | `OB-LSR FAIL prepared cache` |
| `SRC_SINC_FASTEST` | `option_b_libsamplerate_timing_contract --fastest` | `OB-LSR FAIL prepared cache` |

Classification for each is **V1InsufficientForMode**. The failure occurred in the prepared cache range supplied under the unchanged V1 policy; no new preroll value was inferred and no timing policy was changed. Therefore neither mode reaches the quality, edit/reconstruction, or production-duration gates, and there is no valid BEST-vs-lower performance comparison.

`SRC_SINC_BEST_QUALITY` remains the only mode with existing V1 timing evidence. Its performance failure is recorded separately and is not repaired by lowering a threshold.
