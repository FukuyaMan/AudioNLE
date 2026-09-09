# Option B Production SRC Component Skeleton

## Result

The engine-private skeleton is implemented in the Option B feasibility target, not in Domain or a production runtime target. It introduces `BackendConfig`, `ViewCoordinates`, `PhysicalInputPlan`, `PreparedNativeInput`, `FixedOutput`, `Generation`, `Capacity`, configuration status, and engine-private failure categories. `SRC_STATE` is confined to `BandlimitedSrcRuntime` implementation.

## Lifecycle boundary

`configure`, `plan`, `prepare`, `requiredCapacity`, `invalidate`, and `reconstruct` are stopped/control-thread seams. `process` accepts only `PreparedNativeInput` and `FixedOutput`; it rejects stale generation and has no route to configuration, planning, preparation, reconstruction, invalidation, allocation, or cache request. Fixed process staging is 512 project frames; planned native input is capped at 2048 frames per bounded unit.

## Configuration and policy identity

Configuration identity contains backend kind, pinned backend version, converter mode, native/project rate, reduced `P/Q`, preroll policy version and boundary policy version. Preroll v1 is exactly the established `1024 + phase rounding` calculation. Only 44.1 <-> 48 configurations are marked `ValidatedV1`; 44.1 <-> 96 and 48 <-> 96 are explicitly `UnvalidatedConfiguration` pending independent continuous-reference validation. No fallback, altered mode, larger preroll, or floating timing authority exists.

## Matrix and arithmetic result

The executable skeleton exercises 44.1->48, 48->44.1, 44.1->96, 96->44.1, 48->96, and 96->48. For every pair it derives reduced rational rates, iterates all output phase classes, checks phase-aligned source-start-clamped physical planning and discard, and computes 1/3/6/12-hour coordinates directly in `int64_t` without drift. 44.1->96 uses 320 phase classes; 96->44.1 uses 147; 48->96 uses 2; 96->48 uses 1.

## Generation and reconstruction

Prepared generation N is rejected after invalidation to N+1; stale input is never processed. Reconstruction creates a fresh adapter state from `BackendConfig` and `Generation`, then derives the same physical start/discard from `ViewCoordinates`. Opaque backend state is neither reconstructed nor persisted.

## Classification

**Production SRC skeleton Proceed with Constraints.** The boundary is clean and bounded, 44.1 <-> 48 retains the existing validated-v1 designation and compact skeleton process regression, common 96-kHz planning arithmetic works, and unvalidated pairs remain explicit. This does not upgrade backend internal realtime evidence or validate v1 for new rates. Complexity is **Moderate production SRC component**. The next separate gate is production SRC realtime audit plus density/performance.
