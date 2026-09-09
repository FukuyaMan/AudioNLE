# libsoxr quality evidence

Configuration: libsoxr 0.1.3, `SOXR_FLOAT32_I` input/output, one channel, `SOXR_VHQ`, one runtime thread.  Thresholds are unchanged: gain error <= 0.10 dB, ripple <= 0.05 dB, and sampled alias/image rejection >= 60 dB.  Output was deterministic and finite in all measurements.

| source -> project | gain error dB | ripple dB | rejection dB | result |
| --- | ---: | ---: | ---: | --- |
| 44.1 -> 48 | 0.0368682 | 0.0412528 | -79.3109 | PASS |
| 48 -> 44.1 | 0.00393842 | 0.00262196 | -176.476 | PASS |
| 44.1 -> 96 | 0.124277 | 0.130629 | -76.3293 | FAIL |
| 96 -> 44.1 | 0.00393844 | 0.00262192 | -187.910 | PASS |
| 48 -> 96 | 0.124277 | 0.130629 | -77.3010 | FAIL |
| 96 -> 48 | 0.0368682 | 0.0412529 | -177.248 | PASS |

The upsampling 2x directions violate both unchanged passband limits.  Rejection passes at the sampled points, but that does not compensate for the gain/ripple failures.  This is a candidate quality failure; thresholds were not relaxed and no DSP patch was applied.
