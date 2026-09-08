# Execution Plan: Option B libsoxr Timing-Contract Compatibility

## Purpose

Measure pinned libsoxr as a prototype-only bandlimited DSP backend without changing AudioNLE's integer/rational edit timing contract or selecting a production backend.

## Invariants

Timeline and Source are signed integer samples; P/Q is reduced integer authority. Backend filter history, delay, preroll, and transient state are per-view runtime-only state. `soxr_create`, clear, delete, and configuration are control-thread-only.

## Scope

Pin libsoxr, add one fixture-local CTest, and create the requested latency/quality/complexity evidence. Exercise direct state, fresh nonzero starts, worker/cache, callback counters, partitions/pages, seek, trim, split, rebuild, multi-view, and measured spectral characteristics. No production dependency decision, decoder change, device work, GUI, FFmpeg, or Domain change.

## Stop conditions

Classify `libsoxr Inconclusive; run libsamplerate comparator` if fresh-state output cannot be deterministically accounted for from bounded preroll/delay, callback instrumentation finds an unsafe operation, quality thresholds are not established, or the experiment requires a growing subsystem.

## Completion

Complete only with every L0--L18 result and one prescribed classification. The completed integer-ZOH plan remains unchanged.

## Outcome

**libsoxr Inconclusive; run libsamplerate comparator.** The pinned 0.1.3 source could not configure as an AudioNLE CMake subproject because its module lookup assumes a top-level source directory. No wrapper or dependency workaround was retained. See `docs/design/prototypes/option-b-libsoxr-timing-contract-feasibility.md` and `benchmark-results/option-b/libsoxr-*.md`.
