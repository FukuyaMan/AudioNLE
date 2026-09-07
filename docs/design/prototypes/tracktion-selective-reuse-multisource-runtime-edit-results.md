# Option A2 Multi-source / Runtime-edit Results

## Final gate result

Gate: **Proceed with Constraints**. Subsystem: **Substantial adapter extension**. Overall Option A2 recommendation: **Continue with significant constraints**. This is feasibility evidence only, not a production decision or ADR.

Pinned Tracktion `b88a6ee51913668cb53e911e030ab736b13342cf` and JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576` were unchanged. Model A uses one bounded worker/cache runtime per MediaSource and multiple independent Clip views. Runtime identity remains outside the framework-free Domain.

M1 same-file identity/overlap, M2 different-file `SummingNode`, and M3's 12 density cells passed. M4 Move, M5 Trim/re-expand, M6 Split, M7 shared-media Delete/lifetime, and M8 rapid-edit stopped rebuild passed. M9 found a one-slot, no-dynamic-queue policy adequate only for bounded observations: identical ranges reused cache, overlapping pages reused naturally, and alternating ranges showed no starvation, but a later bounded multi-range policy is needed. M10 rebuilt fresh workers/caches/views/graph from the same edited Domain and obtained equal output with deleted output absent.

Callback reader/file, wait/block/spin, allocation/growth and synchronous fallback counts were zero; maximum timing error and stale outputs were zero. Full stopped/affected rebuild is the correctness reference. Persistent partial graph update is unresolved—not a pass or failure—because safe public low-level node replacement/ownership is unproven.

Workers, readers/files and caches scale with unique media; 32 distinct media reached 32 workers/caches despite only 64 KiB fixture PCM. Tracktion continues to supply graph execution, buffers, summing, processing, PDC, VST3, Tail and headless execution, so AudioNLE has not recreated most engine infrastructure. Future FFmpeg keeps the MediaSource/page/view boundary but makes simultaneous far-range reads, decoder contexts, seek/preroll, cancellation and packet arbitration a material risk.

Next gate: **worker-pool / shared decoder arbitration**, to reduce the explicit 32-distinct-media thread/file scaling risk before more format or live-edit work.
