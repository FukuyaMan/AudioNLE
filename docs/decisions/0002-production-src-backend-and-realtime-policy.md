# 0002: Production SRC Backend and Realtime Policy

## Status

Accepted

## Context

AudioNLE must preserve integer Timeline/native sample authority, exact P/Q mapping, non-destructive media, long-form streaming, and a callback path without allocation, blocking, source I/O, decode, or lifecycle. Backend comparison established libsamplerate BEST as the only current candidate with common-rate timing/edit and quality evidence, while its performance is rate/concurrency dependent.

## Decision

Use libsamplerate 0.2.2, revision `b9c20b93660c3683fda12e3c2a01f0021bf96c56`, BSD-2-Clause, with `SRC_SINC_BEST_QUALITY` and `PrerollPolicyV1`. Apply the rate-aware V1 realtime support policy in [production-src-support-policy](../design/production-src-support-policy.md): the common measured guarantee is <=4 active mixed-rate SRC views at 256 project frames on the declared reference hardware. Outside that envelope use prepared/proxy/offline conversion without changing SRC quality.

All validated common 44.1/48/96 directions remain supported for import, editing and offline export. Native-rate clips bypass SRC and do not consume SRC admission capacity.

## Alternatives

MEDIUM/FASTEST lack a validated common phase/history policy. SpeexDSP Q10 and libsoxr VHQ fail quality. r8brain requires a material capacity/adapter redesign. Keeping BEST with a uniform 8/16-view policy contradicts measured failures.

## Consequences

Positive: known timing/edit correctness and production quality; existing source-service and bounded callback evidence; a bounded implementation and licensing record.

Negative: not all mixed-rate workloads meet realtime deadlines; 96 kHz can require preparation; guarantees are reference-hardware/evidence dependent; prepared cache/proxy workflow is product-visible.

This decision does not commit to a 32-view guarantee, arbitrary future sample rates or blocks, automatic quality reduction, permanent rejection of future backends, r8brain capacity expansion, or a custom SRC implementation.

## Revisit

Use the explicit triggers in the support policy. Release materials retain libsamplerate BSD-2-Clause notice/disclaimer and provenance as recorded in [release licensing evidence](../../benchmark-results/option-b/production-src-release-licensing.md).
