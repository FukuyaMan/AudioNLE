# Option B Production-Quality Bandlimited SRC Backend Comparison

## Status

Complete: select **libsamplerate for the current Option B prototype backend**, classified **Proceed with Constraints**. This is not an irreversible production commitment. The completed mixed-rate gate establishes integer-rational ZOH timing; the completed libsamplerate comparator adds measured bandlimited-DSP evidence without changing authority.

## Timing-wrapper contract

For every output Timeline sample `T`, AudioNLE keeps the authoritative rational Source coordinate and edit boundary. A bandlimited backend receives a bounded native-rate range plus wrapper-owned preroll/history. The wrapper records:

```text
integer requested Timeline range
integer/rational authoritative Source coordinate/range
backend physical input range
backend reported or measured output delay
wrapper output trim
final observed Timeline range
```

Backend floating phase, FIR history, delay, buffers, and reset state are runtime-only. A fresh view at a nonzero Timeline start must derive its state from these authoritative coordinates and a bounded physical preroll, never from replaying the full project.

## Candidate comparison

| Candidate | Latency / reset / state | Ratios and quality | Realtime and integration assessment | License / result |
| --- | --- | --- | --- | --- |
| libsoxr | Public `soxr_delay()` reports current output-sample delay; `soxr_clear()` resets to fresh signal with the same config. History is opaque but delay is observable. | Quality recipes through VHQ and phase options; variable ratio API exists. | C API accepts caller buffers. Allocation/locking during `soxr_process` is not yet proven: require instrumentation. New C dependency/build/package work on Windows. | LGPL-2.1-or-later; compatible with the project's GPL-family distribution direction but needs release audit. **Best first measured candidate.** |
| libsamplerate | `src_reset()` resets a streaming state. Public API does not provide a delay query, so wrapper must measure and prove delay/start behaviour. | Sinc converter modes and streaming/variable ratios; float interleaved buffers. | Caller-buffer process API; do not use callback API. Process allocation/locking remains unproven until measured. CMake/Win32 support exists. | BSD-2-Clause. **Second candidate; timing opacity is the principal risk.** |
| SpeexDSP | Has per-channel filter/history state and reset/rate-fraction APIs. Source shows allocation on initialise/reconfigure; process safety must be measured and reconfiguration stays off callback. | Arbitrary rational resampling and quality levels 0--10; speech-oriented pedigree requires quality measurement for spoken-word production. | Compact C dependency; likely simple Windows integration, but no documented delay query in this comparison. | BSD 3-Clause. **Third candidate, principally for footprint comparison.** |
| r8brain-free-src | Constructor configures maximum input length; APIs expose output-size and latency-related facilities. Header source indicates an internal output buffer, so ownership/copy/allocation and reset/reconstruction behaviour require careful measurement. | High-quality sinc-family design; arbitrary non-integer ratios; double-precision processing. | Header-only C++ is convenient for JUCE/MSVC, but its internal-buffer model is a potential callback/ownership mismatch. | MIT with requested attribution. **Strong quality comparator; not yet first integration choice.** |
| JUCE `ResamplingAudioSource` / interpolators | Existing floating ZOH is superseded for timing authority. `ResamplingAudioSource` is AudioSource-oriented, and prior investigation identified dynamic buffers/locks unsuitable for this source-node boundary. | Public interpolators are not a production-quality backend decision. | Already available, but it cannot satisfy the fixed timing contract as timing authority and adds no benefit over a dedicated wrapper. | AGPL/commercial under current JUCE route. **Rejected for this gate.** |

## Evidence and interpretation

libsoxr publicly exposes delay, reset, quality recipes, and variable-ratio controls, making it the clearest starting point for a wrapper that separates DSP latency from edit timing. This is an inference from API shape, not a realtime-safety result. [libsoxr API](https://github.com/chirlu/soxr/blob/master/src/soxr.h)

libsamplerate provides streaming and variable-ratio APIs over caller float buffers and `src_reset`, but the lack of a public delay query increases the required measurement burden. [libsamplerate API](https://libsndfile.github.io/libsamplerate/api.html), [reset guidance](https://libsndfile.github.io/libsamplerate/faq.html), [BSD-2 license](https://github.com/libsndfile/libsamplerate)

SpeexDSP is explicitly arbitrary-ratio code and exposes fractional-rate setup in source; it allocates state at initialisation, so all creation/reconfiguration must remain control-thread work. [SpeexDSP resampler source](https://github.com/xiph/speexdsp/blob/master/libspeexdsp/resample.c)

r8brain-free-src documents arbitrary rates, quality controls, and a C++ front-end, but the wrapper must prove bounded caller-visible processing behavior before it can be used on a callback path. [r8brain-free-src](https://github.com/avaneev/r8brain-free-src), [resampler interface](https://github.com/avaneev/r8brain-free-src/blob/master/CDSPResampler.h)

## Final comparison and selection

libsoxr remains **Inconclusive**, not rejected for DSP quality: pinned 0.1.3 CMake-subproject integration blocked before runtime measurement. Its API shape and public delay query remain a future revisit opportunity. r8brain-free-src remains a valid future comparator, especially for public latency/input-requirement facilities, but no further comparator is needed to answer the current prototype-selection question.

The completed libsamplerate feasibility evidence demonstrates phase-complete starts, worker/cache integration, edit/lifecycle reconstruction, bounded memory, measured fixed-mode quality, and deterministic behaviour for 44.1 <-> 48 kHz. Its integration is a **Moderate SRC integration subsystem**, and it carries two material constraints: the 1024-native-frame plus phase-rounding preroll is empirical for `SRC_SINC_BEST_QUALITY` and tested ratios, and libsamplerate internal realtime safety is partially unproven despite the demonstrated AudioNLE callback contract.

Timing authority is non-negotiable: integer Project Timeline, integer native Source positions, and exact rational mapping remain authoritative. Backend delay, phase, history and output count are physical runtime state only; no opaque backend state is persisted in Domain data.

## Revisit triggers

Revisit the selection if the empirical preroll fails for a new ratio/configuration; production requires a queryable delay/support bound or stronger callback guarantees; deeper instrumentation finds unsuitable backend behavior; production quality/performance or new rates fail; maintenance burden grows; libsoxr obtains an acceptable upstream-supported integration path; or r8brain/another backend materially simplifies deterministic reconstruction.

## ADR disposition

The current prototype selection is implementation evidence under ADR 0001. A dedicated SRC-backend ADR should be considered before productionisation or any irreversible backend commitment.
