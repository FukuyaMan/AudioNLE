# Execution Plan: Option B Production SRC Density Optimisation

## Purpose

Identify bounded callback wrapper waste and remeasure validated-rate density without changing backend, mode, timing authority, fixed capacities or ownership.

## Scope

Decompose active processing, input/output utilisation and call count. Allow only bounded coalescing, copy/clear reduction or precomputed active iteration. Require output/frame/generation and forbidden-operation equivalence.

## Result — complete

The fixture exposed a block-size-independent 512-frame process request. It now uses only `ceil(projectFrames / ratio) + 4` prepared native frames and exactly requested project output, still one fixed-buffer call per active view. No allocation, lifecycle, cache request or synchronization was added to callback. 32-view worst headroom improved below one block at 128/256/512; retain explicit upper-density constraint. Classification: **Production SRC density optimisation Proceed with Constraints**. The production integration planning plan remains active; next gate is cache-pressure plus long-form reliability.
