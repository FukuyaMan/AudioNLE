# Option B Bandlimited SRC Backend Comparison Evidence

This is desk-research evidence, not a runtime benchmark or selection.

| Candidate | Upstream evidence | Comparison disposition |
| --- | --- | --- |
| libsoxr | [`soxr_delay`, `soxr_clear`, quality recipes, variable-rate API](https://github.com/chirlu/soxr/blob/master/src/soxr.h) | First compatibility experiment candidate. |
| libsamplerate | [streaming/caller-buffer API](https://libsndfile.github.io/libsamplerate/api.html), [`src_reset`](https://libsndfile.github.io/libsamplerate/faq.html), [BSD-2](https://github.com/libsndfile/libsamplerate) | Fallback comparator; delay must be independently measured. |
| SpeexDSP | [arbitrary resampling and fractional-rate implementation](https://github.com/xiph/speexdsp/blob/master/libspeexdsp/resample.c) | Footprint comparator; quality and delay need measurement. |
| r8brain-free-src | [MIT, arbitrary rates, quality claims](https://github.com/avaneev/r8brain-free-src) | Quality comparator; internal-buffer/callback contract needs measurement. |
| JUCE path | [public ResamplingAudioSource reference](https://docs.juce.com/develop/classjuce_1_1ResamplingAudioSource.html) | Not selected: superseded floating timing authority and unsuitable source-node ownership model. |

The authoritative timing constraint is unchanged: integer Project Timeline + integer native Source + exact rational mapping. DSP state may be physical runtime state only.

## Measured libsamplerate consequence

The separately authorised libsamplerate comparator completed with **Proceed with Constraints**. It measured both required ratios across timing, cache/callback, seek/trim/split/reconstruction, multiview, source-end, and fixed-mode spectral fixtures. Sampled image/alias rejection (-65.12 dB upsample image; -163.63 dB downsample alias) materially improves on IntegerRationalZoh (-11.40 dB / -3.58 dB). Integration is moderate, not thin, because it needs bounded empirical preroll, per-view lifecycle and physical range accounting.

libsoxr remains **Inconclusive**: its API shape/public delay query remain attractive, but its pinned 0.1.3 CMake-subproject blocker prevented runtime measurement. This is not a DSP-quality failure. Recommendation for the current Option B prototype: **Select libsamplerate for current Option B prototype backend**, carrying the empirical-preroll and partially-unproven backend-realtime-safety constraints. This does not complete the parent selection plan or make an irreversible production commitment.

## Final backend-selection disposition

Parent classification: **Proceed with Constraints**. The parent plan is complete because the shortlist, rejected/inconclusive rationale, wrapper contract, licensing/integration considerations, and exact measured successor acceptance evidence are recorded. libsamplerate is static BSD-2-Clause, `SRC_SINC_BEST_QUALITY`, caller-buffer streaming, fixed-ratio prototype evidence; its integration is a **Moderate SRC integration subsystem**.

Scope remains 44.1 <-> 48 kHz and the validated runtime/edit matrix. Carry forward the empirical 1024-native-frame plus phase-rounding preroll, partly unproven backend-internal realtime safety, incomplete production performance/quality matrix, and non-production-commitment status. Integer Project/Source/rational authority is unchanged; filter state and output production are physical runtime state only.

r8brain remains a future valid comparator, particularly for its latency/input-requirement facilities, but is not needed to select the current prototype backend. Revisit under the documented preroll, formal-latency, realtime, quality/performance, maintenance, libsoxr-integration, or simplification triggers. ADR disposition: current selection fits ADR 0001 implementation evidence; a dedicated decision should precede productionisation.
