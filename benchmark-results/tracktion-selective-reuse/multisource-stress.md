# Tracktion Selective Reuse — Multi-source M3 Dense Matrix

Execution date: 2026-09-07 (Asia/Tokyo)

| Clips | Unique media | Workers/caches | PCM bytes | Graph nodes | Max pending | Max error | Stale |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 4 | 1 / 3 / 4 | 1 / 3 / 4 | 2,048 / 6,144 / 8,192 | 5 | 1 | 0 | 0 |
| 8 | 1 / 3 / 8 | 1 / 3 / 8 | 2,048 / 6,144 / 16,384 | 9 | 1 | 0 | 0 |
| 16 | 1 / 3 / 16 | 1 / 3 / 16 | 2,048 / 6,144 / 32,768 | 17 | 1 | 0 | 0 |
| 32 | 1 / 3 / 32 | 1 / 3 / 32 | 2,048 / 6,144 / 65,536 | 33 | 1 | 0 | 0 |

All 12 cells passed exact marker placement with callback reader/wait/growth counters at zero. Model A shares runtime state for same-media views, but the all-distinct-media dimension grows workers and file handles linearly; this is a scaling warning, not a hard-stop failure in the bounded fixture.

## M9 arbitration follow-up

Request model: one latest-request slot per MediaSource runtime; maximum observable pending work 1; dynamic queue: no. Identical same-media requests used one worker read and then one cache reuse. Overlapping `[0,257)` / `[128,385)` range observations required two additional page reads (pages 128 and 256) after page 0, and reused those pages thereafter. Alternating disjoint pages for 16 cycles had no starvation; once cached, it needed zero additional reads. Competing 2, 4, and 8 views were exact.

This is **adequate for the bounded prototype**, with a constraint: a latest slot can overwrite an unserved still-needed range, so a production multi-range policy requires separate bounded arbitration investigation. Callback invariants remained zero.
