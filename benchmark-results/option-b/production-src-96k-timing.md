# 96 kHz Timing Evidence

| Pair | P/Q | phase classes | V1 | status |
| --- | --- | ---: | --- | --- |
| 44100->96000 | 320/147 | 320 | 1024 + phase rounding PASS | ValidatedV1 |
| 96000->44100 | 147/320 | 147 | PASS | ValidatedV1 |
| 48000->96000 | 2/1 | 2 | PASS | ValidatedV1 |
| 96000->48000 | 1/2 | 1 | PASS | ValidatedV1 |

All comparisons use integer physical start/discard and max/RMS tolerance <=2e-5. Content includes broadband, markers, multitone, generated spoken-like and music-like fixtures; source starts, T4096, same-phase, T12000 and independent seek/trim/split/reconstruction are covered.
