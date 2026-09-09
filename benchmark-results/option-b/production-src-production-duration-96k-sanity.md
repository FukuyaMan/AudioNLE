# Production-Duration 96 kHz Sanity Status

Release duration sanity used 256 warm-up blocks and 10,000 measured blocks for each of three runs, at 256 project frames. It completed all four directions for the defined 8/16 tiers. Thresholds are unchanged: 8 views <0.50, 16 views <0.70, hard deadline <1.0.

| direction | 8 views | 16 views | classification |
| --- | --- | --- | --- |
| 44.1 -> 96 | 0.638888 / 0.592800 / 0.566250 | 1.416710 / 1.018730 / 1.145550 | 8 fails; 16 hard-deadline fails |
| 96 -> 44.1 | 0.611457 / 0.524187 / 0.588477 | 1.056660 / 1.055920 / 1.216280 | 8 fails; 16 hard-deadline fails |
| 48 -> 96 | 0.367200 / 0.497213 / 0.408338 | 1.680750 / 0.678300 / 0.942863 | 8 passes; 16 fails, including one hard-deadline fail |
| 96 -> 48 | 0.365025 / 0.447919 / 0.392794 | 0.729675 / 0.654000 / 0.673406 | 8 passes; 16 fails |

These are performance results only. Existing timing/configuration correctness remains validated; this duration runner does not replace sample-by-sample reconstruction/determinism fixtures.
