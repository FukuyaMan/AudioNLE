# Execution Plan: Option B Bandlimited SRC Backend Selection

## Objective

Compare production-quality bandlimited SRC backends for the selected Option B source runtime. This is a feasibility/design gate only: no dependency, CMake target, implementation, ADR, decoder, or production runtime change is authorised.

## Fixed invariants

* Timeline authority is integer Project samples; Source authority is integer native samples.
* Rates are reduced exact integer ratios. `IntegerRationalZoh` remains the authoritative Source identity/timing path until a successor proves the same contract.
* DSP latency/history may affect only a wrapper-owned physical read/preroll/trim plan. It must never redefine edit boundaries, cache keys, Source identity, or persisted state.
* Nonzero start, seek, split, trim, multi-view isolation, and reconstruction must be deterministic from integer Domain coordinates.
* Source worker/cache remains bounded and callback-safe: no reader/decode, wait/spin, allocation/growth, or synchronous fallback on callback.

## Candidates and decision question

Compare libsoxr, libsamplerate, SpeexDSP resampler, r8brain-free-src, and the existing JUCE public path. Evaluate documented latency/delay introspection, state and reset semantics, arbitrary-ratio support, process-time allocation/locking evidence, quality controls, licensing, and Windows/JUCE integration burden.

The decision question is not which library owns edit timing. AudioNLE retains that authority. It is whether a library can be wrapped as bounded bandlimited DSP with observable/compensable physical delay and deterministic fresh-state rendering.

## Planned comparison method

1. Record only public API and license evidence from upstream primary sources.
2. Reject APIs whose documented model requires callback pull, unbounded allocation/locking, or hides delay/reset semantics incompatible with the fixed invariants.
3. Rank remaining candidates for a later, separately authorised implementation gate.
4. That later gate must test 44.1->48, 48->44.1, nonzero starts, seeks, split/trim/rebuild, 128/256/irregular blocks, 257-frame pages, latency/preroll accounting, spectral quality, and callback instrumentation.

## Completion criteria

This planning gate completes when the comparison and evidence state a shortlist, a rejected-path rationale, the timing-wrapper contract, licensing/integration risks, and exact acceptance tests for the next gate. It must not claim runtime safety or audio quality without measurement.

## Files

* `docs/design/prototypes/option-b-bandlimited-src-backend-selection.md`
* `benchmark-results/option-b/bandlimited-src-backend-comparison.md`

## Risks and stop conditions

Stop before implementation if no candidate exposes a bounded, reconstructible latency/reset model without leaking float timing into authority. Do not respond by adding epsilon, changing integer mapping, or creating a general custom SRC subsystem. In that case recommend a narrowly scoped public-SRC timing-contract experiment.

## Final result — complete

The shortlist and runtime comparator evidence now satisfy completion criteria. libsamplerate is selected for the current Option B prototype, classified **Proceed with Constraints**; its completed feasibility plan supplies the exact timing/edit/cache/quality acceptance evidence. libsoxr is **Inconclusive** only because its pinned 0.1.3 CMake subproject blocked before runtime measurement, not because of measured DSP failure. r8brain remains a future comparator, not a required current gate.

The wrapper contract remains integer Project Timeline + integer native Source + exact rational mapping, with backend phase/history/delay physical-only. Integration is moderate. Carry forward: empirical 1024-native-frame plus phase-rounding preroll for `SRC_SINC_BEST_QUALITY` and measured 44.1 <-> 48 ratios; partially unproven libsamplerate internal realtime safety; incomplete production quality/performance matrix; and no irreversible production commitment. The completed libsamplerate plan is `docs/exec-plans/completed/option-b-libsamplerate-timing-contract.md`.

Revisit if preroll fails for new ratio/mode, formal delay/support query or stronger realtime guarantee is required, deeper instrumentation or production quality/performance fails, maintenance cost grows, libsoxr integration becomes viable, or r8brain/another backend simplifies deterministic reconstruction. Current selection is ADR 0001 implementation evidence; a dedicated ADR is appropriate before productionisation.
