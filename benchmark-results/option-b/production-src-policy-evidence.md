# Production SRC V1 policy evidence

Reference environment: Windows 11, Ryzen 9 7900X (24 logical processors), 31.12 GiB RAM, High Performance power scheme. These are repeatable reference-hardware observations, not a certification for all Windows PCs. All duration rows are Release, 256 warm-up blocks, 10,000 measured blocks, three runs, fixed 2048-native/512-project buffers, and libsamplerate BEST.

## Existing 8/16-view evidence at 256 project frames

| source -> project | views | worst headroom per run | confidence |
| --- | ---: | --- | --- |
| 44.1 -> 48 | 8 | earlier primary PASS; independent 512-frame rerun 0.545878 | ObservedPassButVariable |
| 44.1 -> 48 | 16 | earlier primary 0.447834--0.612338 | ObservedPassButVariable |
| 48 -> 44.1 | 8 | 0.317537 / 0.419760 / 0.443739 | StablePass |
| 48 -> 44.1 | 16 | 0.823068 / 0.645100 / 0.656315 | Fail |
| 44.1 -> 96 | 8 | 0.638888 / 0.592800 / 0.566250 | Fail |
| 44.1 -> 96 | 16 | 1.416710 / 1.018730 / 1.145550 | HardFail |
| 96 -> 44.1 | 8 | 0.611457 / 0.524187 / 0.588477 | Fail |
| 96 -> 44.1 | 16 | 1.056660 / 1.055920 / 1.216280 | HardFail |
| 48 -> 96 | 8 | 0.367200 / 0.497213 / 0.408338 | ObservedPassButVariable |
| 48 -> 96 | 16 | 1.680750 / 0.678300 / 0.942863 | HardFail |
| 96 -> 48 | 8 | 0.365025 / 0.447919 / 0.392794 | StablePass |
| 96 -> 48 | 16 | 0.729675 / 0.654000 / 0.673406 | Fail |

The configured 8-view threshold is `<0.50`, the 16-view threshold `<0.70`, and any `>=1.0` is a hard failure.  32 views remains a stress-only unsupported tier.

## Added low-concurrency evidence

The missing policy boundary was measured at 256 project frames only.  All rows below are `StablePass` relative to the conservative `<0.50` worst-headroom ceiling: no run exceeded 0.365.

| source -> project | 1 view (three runs) | 2 views | 4 views |
| --- | --- | --- | --- |
| 44.1 -> 48 | 0.035381 / 0.036038 / 0.034200 | 0.088688 / 0.061181 / 0.061538 | 0.129788 / 0.145238 / 0.138881 |
| 48 -> 44.1 | 0.034694 / 0.096951 / 0.075642 | 0.065220 / 0.064737 / 0.101981 | 0.165564 / 0.279311 / 0.205961 |
| 44.1 -> 96 | 0.107925 / 0.159638 / 0.090038 | 0.120825 / 0.150975 / 0.189337 | 0.278737 / 0.294263 / 0.364725 |
| 96 -> 44.1 | 0.101017 / 0.063824 / 0.061361 | 0.179449 / 0.222912 / 0.171094 | 0.317227 / 0.293678 / 0.281189 |
| 48 -> 96 | 0.049725 / 0.086250 / 0.061950 | 0.099900 / 0.108000 / 0.151538 | 0.188400 / 0.229612 / 0.176175 |
| 96 -> 48 | 0.047738 / 0.052725 / 0.057994 | 0.090169 / 0.100106 / 0.087919 | 0.199106 / 0.177525 / 0.205856 |

No statistical hardware-independent guarantee is inferred. The policy adopts four active mixed-rate SRC views as the common measured guarantee because it is the highest common low tier with three-run evidence for every validated direction.
