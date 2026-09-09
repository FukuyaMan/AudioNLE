# Execution Plan: Option B Production SRC Remaining Candidates

## Objective

Resolve libsamplerate MEDIUM/FASTEST with mode-specific bounded reconstruction evidence, then evaluate r8brain only if neither mode completes the mandatory gate.

## Components

Add comparator fixtures and evidence under `benchmark-results/option-b`; retain `PrerollPolicyV1` unchanged for BEST.  Reuse the existing fixed 2048-native/512-project boundary and Windows IAT harness.

## Invariants

Timeline and source positions remain integer samples; mappings remain reduced rationals; backend state/history/delay stay physical-only.  No callback allocation, lifecycle, decode, lock, wait, or I/O is introduced.

## Strategy and stop conditions

Search a declared finite native-history range against each mode's own continuous reference and exhaustive phase cases.  A history value is not promoted unless it passes reconstruction and fits the existing bounded capacity.  Only timing-qualified modes proceed to quality, then IAT and performance.  If neither completes, inspect/build a bounded r8brain adapter only through its global-cache realtime gate; any mandatory failure stops that candidate.

## Verification

Run `build.ps1`, focused CTest targets, and record the known unrelated Phase E failure separately.  Preserve all existing unrelated worktree changes.
