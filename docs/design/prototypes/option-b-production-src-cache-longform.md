# Production SRC Cache-Pressure + Long-Form Reliability

Sparse deterministic native data indexed by integer Source coordinate exercised 1/3/6/12-hour planning without retaining decoded-duration PCM. A fixed 8 x 257-page cache repeatedly served 200 seeded nearby/far/return seeks, pressure eviction and republication. Prepared range is 1500 native frames for the representative 512-project-frame workload, fitting the fixed page service without shortening v1 preroll.

Stale generation pages are rejected, evicted ranges republish before use, and reconstruction derives the same integer plan. Capacity excess is rejected before callback; unavailable pages remain a bounded prepared-input failure and recover after preparation. Classification: **Reliability passes with cache/concurrency constraints**; gate **Production SRC cache/long-form Proceed with Constraints**. Complexity remains Moderate. The 32-view stress point remains a constraint, not a guarantee.
