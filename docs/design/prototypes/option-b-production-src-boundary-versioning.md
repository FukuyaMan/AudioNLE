# Production SRC Boundary / Error / Versioning Contract

## Boundary policies

`PrerollPolicyV1` is `physicalStart=floor(max(0,floor(T0*Q/P)-1024)/Q)*Q`. `SourceStartPaddingPolicyV1` clamps physical start to zero, supplies only defined boundary history/padding, retains logical Source start, and prohibits negative reads. `SourceEndFlushPolicyV1` supplies `end_of_input` only at physical end; settling is physical-only. `OutputClipPolicyV1` clips exposure to authoritative logical Timeline end: SRC settling never becomes Effect Tail.

Truncated required history is an explicit `PreparedInputUnavailable` preparation failure unless the Source-start padding policy applies; the engine never silently shortens history and processes altered output.

## Engine-private failures and callback behavior

`ProviderUnavailable`, `ProviderReadFailure`, `PreparedInputUnavailable`, `StaleGeneration`, `CapacityExceeded`, `UnsupportedConfiguration`, `BackendPrepareFailure`, and `BackendProcessFailure` are engine failures, never Domain/backend numeric types. Callback-visible invalid prepared input produces deterministic silence/underrun plus a counter; it performs no read/decode, block, allocation, lifecycle operation or Domain mutation. Recovery is prepare off callback, discard contaminated state, retry current generation, then deterministic resume.

## Versioned reconstruction record

Record backend identifier/pin, converter mode, native/project rates, reduced P/Q, preroll/source-start/source-end/output policy versions, and capacity policy version. Its deterministic fingerprint is a canonical ordered serialization of these scalar/string fields, not a pointer or opaque state. Persisted project data contains logical source identity/range and integer Timeline coordinates; engine metadata contains the versioned reconstruction record; ephemeral runtime contains `SRC_STATE`, filter history, pages and buffers.

Matching versions reconstruct from logical coordinates plus record only. Known older policy requires explicit migration/revalidation; unknown future policy fails; backend-pin mismatch requires explicit revalidation. All six common 44.1/48/96 directions have V1 comparator validation; arbitrary rates remain `UnsupportedConfiguration` pending explicit validation. Configuration fingerprint participates with generation in prepared-state validity, preventing old configuration state promotion.

## Classification

**Versioning works with explicit migration constraints** and **Boundary/error reliability passes with explicit compatibility constraints**. Gate: **Production SRC boundary/versioning Proceed with Constraints**. Source replacement/relink behavior is deferred because this prototype does not model a provider relink lifecycle. The accepted SRC ADR constrains realtime tiers rather than treating all validated directions as uniform performance guarantees.
