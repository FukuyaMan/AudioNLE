# Production SRC Performance Thresholds

| Requirement | Threshold | Current evidence | Status |
| --- | --- | --- | --- |
| 8 views worst headroom | <0.50 | corrected fixture ~0.19 | PASS; duration rerun needed |
| 16 views worst headroom | <0.70 | corrected scaling supports direction | Needs 10k-block rerun |
| 32 stress worst headroom | <0.85 | 0.69 / 0.64 / 0.74 | PASS; duration rerun needed |
| Forbidden callback work | 0 | 0 | PASS; deeper audit remains |
| Duration PCM | 0 | 0 | PASS |
| Benchmark protocol | 10k blocks, 3 runs | 270 blocks | Needs rerun |
