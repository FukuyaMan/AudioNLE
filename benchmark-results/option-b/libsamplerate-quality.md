# libsamplerate Quality Evidence (Pending)

`SRC_SINC_BEST_QUALITY` is configured, but no passband, stopband, alias, speech, or music measurement has yet run.
# libsamplerate LS20--LS22 quality evidence

Configuration is fixed: libsamplerate 0.2.2 (`b9c20b93660c3683fda12e3c2a01f0021bf96c56`), `SRC_SINC_BEST_QUALITY`, fixed ratio and caller-buffer `src_process` API. Results apply only to 44100->48000 and 48000->44100.

| Measurement | 44100->48000 | 48000->44100 |
| --- | ---: | ---: |
| Sweep range | 100 Hz--18 kHz | 100 Hz--18 kHz |
| Worst measured passband error | -0.00000083 dB at 18 kHz | -0.02878 dB at 100 Hz |
| Worst measured ripple | 0.00000085 dB | 0.00123 dB |
| Image/alias rejection | -65.12 dB at 13.9 kHz image | -163.63 dB, 23 kHz input / 21.1 kHz alias |
| IntegerRationalZoh baseline | -11.40 dB corresponding image | -3.58 dB corresponding alias |
| Impulse peak / output frames | 4354 / 17414 | 3675 / 14699 |
| Round trip max / RMS | 5.96e-08 / 1.46e-08 | 5.96e-08 / 1.52e-08 |

Impulse responses are finite, causal physical filter responses with observed post-ringing; they do not define logical edit latency. DC/low-frequency sweep includes 100 Hz and shows no drift; finite-value scans found no NaN or infinity. Deterministic synthetic spoken-like and music/BGM-like fixtures repeated byte-identically three times. Converted RMS/peak were respectively 0.112664 / 0.340927 and 0.284982 / 0.549676 for 44.1->48; 0.107815 / 0.316222 and 0.284980 / 0.549662 for 48->44. Round trip is a diagnostic, not a sample-identity requirement.

Project-local prototype criterion: no material passband defect in this sweep, at least material improvement over ZOH at the sampled alias/image point, finite stable output, and deterministic repeats. The measured result is clearly suitable for prototype continuation, not a general arbitrary-ratio or perceptual-quality claim.
