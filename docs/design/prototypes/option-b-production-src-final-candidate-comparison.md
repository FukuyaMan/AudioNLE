# Option B Production SRC final candidate comparison

| Candidate | Timing/Edit | Quality | RT | 8 / 16 views | Memory / adapter | Integration |
| --- | --- | --- | --- | --- | --- | --- |
| libsamplerate BEST | PASS | PASS | bounded blind spots | FAIL overall | existing 2048/512 | Moderate |
| libsamplerate MEDIUM | FAIL common phase/history | N/A | N/A | N/A | 320/147 one-start needs 2367 | no validated policy |
| libsamplerate FASTEST | FAIL common phase/history | N/A | N/A | N/A | 320/147 one-start needs 2367 | no validated policy |
| SpeexDSP Q10 | basic PASS | FAIL | N/A | N/A | low | Moderate |
| libsoxr VHQ | basic PASS | FAIL | N/A | N/A | bounded | Moderate |
| r8brain | FAIL current capacity | N/A | not proven | N/A | 2246--4617 required / double staging | integration complexity unacceptable |

## Decision

Recommended production direction: **`Keep libsamplerate BEST with constrained realtime policy`**.

This preserves 96 kHz media support.  “Supported media” is distinct from a guarantee of simultaneous realtime mixed-rate SRC: under current evidence, high-cost rate/concurrency combinations require reduced realtime concurrency or prepared/offline conversion.  This is a product-policy fallback, not a claim that BEST meets the original 8/16-view guarantee.

The policy now defines reference-hardware rate/concurrency tiers, prepared/offline fallback, and revisit triggers. The resulting accepted ADR is [0002](../../decisions/0002-production-src-backend-and-realtime-policy.md). Further random backend research is not justified by this evidence.
