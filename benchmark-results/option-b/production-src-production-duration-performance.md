# Production-Duration Performance Status

The Release runner now implements 256 warm-up blocks followed by 10,000 measured blocks for each run and emits mean/p50/p95/p99/worst/headroom. It stops on any tier violation. The first recorded primary `44100 -> 48000` attempt produced:

| views | block | completed runs | result |
| --- | ---: | ---: | --- |
| 8 | 128 / 256 / 512 | 3 / 3 / 3 | PASS; worst headroom range 0.238969--0.329475 |
| 16 | 128 / 256 / 512 | 3 / 3 / 3 | PASS; worst headroom range 0.447834--0.612338 |
| 32 | 128 | 1 | HARD DEADLINE FAIL: worst 2695.0 us, headroom 1.01063 |

The runner correctly aborted at the hard-fail row; remaining 32-view rows, reverse 48k->44.1k, and 96 kHz sanity rows were not run. Consequently there is no production-duration matrix PASS and no release threshold classification. The failure is a concrete performance gate result, not a harness failure.

A subsequent independent primary rerun also stopped: its 8-view / 512-frame run 1 was 0.545878, exceeding the 0.50 minimum threshold (not a hard deadline). A sanity-only mode then completed the remaining directions without relaxing thresholds. For 48->44.1 at 256 frames, 8 views passed all runs (0.317537 / 0.419760 / 0.443739); 16 views were 0.823068 / 0.645100 / 0.656315, so the representative recommended row fails. See `production-src-production-duration-96k-sanity.md` for the four 96 kHz directions.
