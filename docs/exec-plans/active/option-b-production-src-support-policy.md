# Execution Plan: Production SRC Support Policy

## Objective

Turn the completed backend evidence into a rate-aware V1 realtime support policy and ADR, using only the selected libsamplerate BEST backend.

## Scope

Consolidate existing duration evidence and run only missing 1/2/4-view, 256-frame, three-run duration measurements. Define admission, fallback, export, native-rate, mixed-load, and revisit policy; draft the ADR if evidence is sufficient.

## Invariants

Keep BEST, PrerollPolicyV1, exact integer/rational coordinates, 2048/512 capacity, immutable source media, and callback prohibitions. No automatic quality downgrade, backend change, or 32-view optimisation.

## Completion

Measured configurations receive an evidence-confidence class. The final policy distinguishes guarantees from best effort and prepared fallback, is qualified to reference hardware, and records explicit revisit triggers.
