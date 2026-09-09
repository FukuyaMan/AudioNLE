# Option B Alternative Backend: SpeexDSP Comparator

SpeexDSP 1.2.1 is a closer runtime-shape comparator than r8brain: it provides per-instance, float caller-buffer processing, rational rate setup, reset, and latency queries. The minimal bounded adapter built and executed on Windows/MSVC Release using the existing 2048/512 capacities. Its quality-10 output passes the small timing comparator but fails the existing common-rate production quality gate in four of six directions.

**Candidate classification: Alternative backend quality failure.** The failure is not an integration, timing-capacity, or performance classification; those gates were deliberately not promoted after quality failure.

**Architecture recommendation: Revisit libsoxr integration with an explicit build-adapter gate.** SpeexDSP does not satisfy current quality requirements, lower libsamplerate modes lack a valid V1 timing policy, and r8brain needs a larger bounded adapter. No current candidate satisfies production requirements; production SRC ADR is not ready. The independent policy fallback remains: retain 44.1/48 realtime only at demonstrated tiers and constrain 96 kHz to reduced concurrency or prepared/offline conversion without removing 96 kHz import/edit support.
