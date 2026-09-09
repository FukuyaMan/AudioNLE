# SpeexDSP Alternative Quality Evidence

Quality-10 measurements use passband tones through 80% of lower Nyquist, sampled alias/image rejection, deterministic output, and finite checks. Existing AudioNLE thresholds are unchanged: gain error <=0.10 dB, ripple <=0.05 dB, rejection >=60 dB.

| direction | gain error dB | ripple dB | alias/image rejection dB | result |
| --- | ---: | ---: | ---: | --- |
| 44.1 -> 48 | 0.000003 | 0.000001 | -65.727 | PASS |
| 48 -> 44.1 | 0.052644 | 0.055429 | -124.601 | FAIL ripple |
| 44.1 -> 96 | 0.111345 | 0.111344 | -70.913 | FAIL gain/ripple |
| 96 -> 44.1 | 0.052644 | 0.055429 | -124.621 | FAIL ripple |
| 48 -> 96 | 0.163631 | 0.163630 | -134.137 | FAIL gain/ripple |
| 96 -> 48 | 0.000002 | 0.000001 | -125.455 | PASS |

No NaN/Inf or same-build nondeterminism was observed in the fixture. The candidate fails current production quality thresholds; thresholds were not relaxed. Consequently content-fixture expansion, production performance, source-service integration, and Windows realtime smoke are not valid gates to promote for this candidate.
