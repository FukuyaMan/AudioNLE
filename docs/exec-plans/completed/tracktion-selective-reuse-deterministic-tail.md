# Execution Plan: Option A2 Deterministic Effect Tail

## Objective

Measure whether the public low-level Tracktion graph can process a finite deterministic Effect Tail from the AudioNLE-controlled source path, without adopting a production Tail design.

## Scope

Use a 48 kHz, 128-frame, framework-free fixture: Clip start 1000, source duration 480, source end 1480, reported/actual tail 1024, and processing end 2504. Test `Reported` and `CutAtSourceEnd` TailPolicy, silence feeding, downstream multiply order, Track Mix overlap, move by +48000, delete, and destroy/rebuild.

## Invariants and stop conditions

Source End and Processing End remain separate integer Timeline-sample values. Runtime nodes/buffers do not enter or mutate Domain state. `SummingNode` performs mixing; no high-level Tracktion source scheduling, manual post-mix, private API, patch, fork, or custom graph scheduler is allowed. A non-exact sample observation, failed downstream tail processing/mix/move/delete/rebuild, or need for a forbidden path stops the phase.

## Verification

Add a prototype-only target and CTest, update raw/result/complexity records, run `build.ps1`, targeted CTest and direct executable, then review the diff. Actual VST3 Tail, unknown/infinite tails, realtime edits, production render planning, and ADR remain excluded.
