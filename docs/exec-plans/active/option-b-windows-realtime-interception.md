# Execution Plan: Option B Windows Realtime Interception Harness

## Purpose

Build a test-only Windows/MSVC callback-region interception harness with proven positive/negative controls, then execute the fixed production-duration SRC matrix.

## Required implementation order

1. Thread-local non-allocating callback scope and bounded event counters.
2. Proven controls for CRT allocation, Windows heap, lock, sleep/wait and file I/O.
3. Static/runtime cross-check for prepared `SRC_SINC_BEST_QUALITY` process.
4. Separate low-overhead 10,000-block x three-run timing runner.

## Current status

Active. Existing source audit/local counters cannot satisfy this gate because they do not prove interception coverage or the required production-duration matrix. No release/ADR conclusion may be derived until the controls and timing rows pass.
