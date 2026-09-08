# Option B Multi-source Arbitration Complexity

## Classification

Implementation burden: **Substantial source-service subsystem**.

The selected fixture remains bounded and is not a general-purpose scheduler, but multi-range demand adds explicit request-state, fairness, cancellation, worker dispatch, reader serialization, and lifetime responsibilities beyond the thin one-source handoff.

## Responsibility split

| Area | Scope in the fixture |
| --- | --- |
| Existing/common source core | MediaSource page cache, Source identity, MediaGeneration, Clip mapping, zero-on-miss SourceNode handoff |
| New arbitration | Four fixed request states per media, same-page deduplication, deterministic overflow/republication, round-robin service and stale cancellation |
| New worker service | Two fixed workers, weak registry, per-reader serialization, temporary service lifetime, completion publication |
| Fixture/test only | WAV creation, held worker/read controls, deterministic observations, counters, working-set sampling, matrix assertions |

No production LOC estimate is implied. The meaningful result is that a bounded worker pool can remove the worker-thread-per-media term while readers, cache instances, cache bytes and pending request capacity still scale with unique MediaSources.

## Retained risks

The fixture has no FFmpeg decoder contexts, source-rate conversion, device callback, real-time deadline, cache eviction policy beyond its fixed page ring, compressed-media seek/preroll, production priority policy, or live mutation. Those remain separate work.
