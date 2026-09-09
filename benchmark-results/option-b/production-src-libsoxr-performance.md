# libsoxr performance evidence

Not run.  P0--P5 are gated behind a passing production quality configuration.  `SOXR_VHQ` fails 44.1 -> 96 and 48 -> 96 passband gain/ripple thresholds, so 8/16-view performance would not alter the architecture decision.

Consequently there is no valid libsoxr versus libsamplerate BEST/SpeexDSP timing-cost comparison.  Existing context remains: libsamplerate BEST passes quality/timing but misses supported-tier performance; SpeexDSP quality 10 has already failed quality and is only a failed-reference comparator.
