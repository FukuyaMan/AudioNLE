# Execution Plan: Option B Production SRC Realtime Audit + Density

## Purpose

Measure prepared validated-rate SRC processing at density and strengthen callback safety evidence without changing Domain authority, adding backends, or making a production commitment.

## Scope

Use 44.1->48 as primary and 48->44.1 sanity workloads; fixed 2048 native/512 project capacities; 1/4/8/16/32 views; 128/256/512 scheduling cases; shared/distinct source buffers; generation/capacity/cache-pressure seams. Preparation is outside timing. Allocation/lock claims distinguish observed, source-audited and unproven.

## Completion

Complete with measurements, callback counters, deterministic stress, bounded memory/capacity evidence and one residual-risk classification. The production integration planning plan remains active.

## Result

Realtime callback counters and deterministic stress pass, with backend internals still partially unproven. However, 32-view worst-case density timing exceeds block duration in this environment. Classification is **Production SRC realtime/density Inconclusive**; retain this plan active and address density/performance before the cache-pressure/long-form reliability gate.

## Density optimisation follow-up

The bounded used-range/process-call optimisation corrects block-size-independent 512-frame input/output usage. Measured 32-view headroom is now below one block duration at all tested block sizes, with explicit upper-density constraint retained. The follow-up optimisation gate is **Proceed with Constraints**; deeper realtime blind spots remain unchanged. The next gate may be cache-pressure plus long-form reliability, carrying the 32-view concurrency constraint.
