# Execution Plan: Option B Production SRC Component Skeleton

## Purpose

Introduce and test engine-private production-oriented SRC boundary types for the selected prototype backend. This gate is limited to the skeleton, fixed-capacity contracts, configuration-versioned planning, and common-rate validation. It does not make a production commitment, perform the deep realtime audit, benchmark density, or create an ADR.

## Invariants

Integer Project Timeline, integer native Source, and exact reduced `P/Q` remain authoritative. Backend state is adapter-private physical runtime state and is never persisted in Domain data. Callback processing consumes only prepared input and fixed output staging.

## Scope

Implement engine-private `BackendConfig`, `ViewCoordinates`, `PhysicalInputPlan`, `PreparedNativeInput`, `FixedOutput`, and `Generation`; seams for configure/plan/prepare/process/capacity/invalidate/reconstruct; configuration status; and an executable matrix fixture for 44.1/48/96 kHz.

## Completion

Complete only with matrix planning arithmetic, explicit validation status, bounded capacity, stale-generation rejection, reconstruction, compact 44.1<->48 regression, and evidence. Keep the production-integration planning plan active.

## Result

The skeleton target and evidence are complete. All six common-rate rows have explicit status; only existing 44.1 <-> 48 evidence marks preroll v1 validated, while 96-kHz configurations remain explicitly unvalidated. The next gate is realtime audit plus density/performance. The broader production SRC integration planning plan remains active for that follow-on evidence programme.
