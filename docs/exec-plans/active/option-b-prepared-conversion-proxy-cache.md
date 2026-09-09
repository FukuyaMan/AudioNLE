# Execution Plan: V1 Prepared Conversion / Proxy Cache

## Objective

Implement the smallest engine-private prepared project-rate PCM cache that makes ADR 0002 `PreparedRequired` observable and safe at the callback boundary.

## Components

Add static V1 admission, bounded page-pool conversion/cache fixtures, prepared callback smoke, and evidence/design documents. Reuse libsamplerate BEST only on the preparation worker path.

## Invariants

Timeline and source coordinates remain integer samples with P/Q mapping. Source remains immutable. Prepared PCM is derived, generation/config-keyed cache material. Callback has no source I/O, decode, SRC, allocation, lock/wait, or lifecycle.

## Tests

Validate admission table, generation/config rejection, partial publication, deterministic unavailable silence, shared page lookup, seek/trim/split semantics, page seams against a direct BEST reference, and a bounded long-form coordinate/cache fixture. Extend the existing IAT harness with prepared playback.
