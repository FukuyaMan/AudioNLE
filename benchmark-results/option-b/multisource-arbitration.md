# Option B Multi-source Arbitration Evidence

## Fixture and selected model

The fixture uses 48 kHz mono PCM16 WAV files, 128-frame Option B source/graph observations, 257-frame pages, eight pages per MediaSource, four fixed request slots per MediaSource, and two fixed worker threads. Model C was selected:

```text
per-MediaSource fixed page cache + fixed request slots + one reader context
                    -> two-worker shared service
```

This is bounded WAV-fixture evidence, not a throughput, device, FFmpeg, or production scheduler result.

## M0--M4

M0 reproduced one shared MediaSource with multiple Clip views and three independent MediaSources. Marker timing error was 0 and the source callback forbidden counters remained 0.

M1 used same-page, overlapping-page, adjacent-page, far-apart same-media, and cross-media demand. Same-page demand was deduplicated. Adjacent ranges remained separate page requests; no general interval scheduler was introduced. Far ranges at Source 0 and 10000 both became ready under alternating demand, while cross-media demand progressed.

M2 uses four fixed request slots per MediaSource. Four queued non-conflicting page requests were retained; a fifth request was deterministically rejected, rendered as the established zero/underrun path when requested by SourceNode, and later republished successfully after capacity drained. No callback waits or synchronous decode were used.

M3 uses the worker service's round-robin media scan. The deterministic far and hot/cold observations completed without permanent starvation. This records service-cycle progress only; it makes no wall-clock deadline claim.

M4 exercised 8 and 16 MediaSources through two workers, and the density matrix exercised 32 MediaSources through the same two workers. Each MediaSource reader is owned by its runtime and protected by a per-media atomic busy guard; observed maximum concurrent reader calls per MediaSource was at most one.

## M5--M8 resource, density, output, callback observations

Measured/configured formulas:

```text
workers = 2
reader contexts = unique active MediaSources
cache instances = unique active MediaSources
cache bytes = unique active MediaSources * 8224
pending request capacity = unique active MediaSources * 4
active decode tasks <= 2
```

The established handoff fixture has a 512-byte SourceNode scratch buffer. This multi-source fixture renders into caller-provided scalar observations and retains no additional SourceNode scratch or full-source PCM.

| Unique media | Clips | Workers | Readers/caches | Cache bytes | Pending capacity | Active tasks | Timing/output | Working set run 1 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| 1 | 4 | 2 | 1 / 1 | 8,224 | 4 | <= 2 | pass / 0 | 8,204,288 |
| 4 | 8 | 2 | 4 / 4 | 32,896 | 16 | <= 2 | pass / 0 | 8,228,864 |
| 8 | 16 | 2 | 8 / 8 | 65,792 | 32 | <= 2 | pass / 0 | 8,265,728 |
| 16 | 32 | 2 | 16 / 16 | 131,584 | 64 | <= 2 | pass / 0 | 8,331,264 |
| 32 | 32 | 2 | 32 / 32 | 263,168 | 128 | <= 2 | pass / 0 | 8,482,816 |

The three direct-run working-set vectors were:

```text
8196096 / 8216576 / 8249344 / 8327168 / 8474624
8204288 / 8232960 / 8265728 / 8335360 / 8491008
8200192 / 8220672 / 8253440 / 8323072 / 8470528
```

They are representative OS observations, not hard memory bounds. M7 verified same-source overlap, cross-media and dense linear sums from observed decoded PCM values, with no hard clipping at internal sums above 1.0. Maximum timing error was 0 samples. M8 callback reader/file/decode, wait/block/spin, allocation/growth, synchronous fallback, and mutex-acquisition counters were all 0.

## M9--M11

M9 queued stale work while workers were held, advanced MediaGeneration, then released service. The stale task was discarded; current-generation demand became ready; stale work did not publish a current page.

M10 held an in-flight worker read for MediaSource A, removed its weak registry entry, invalidated/destroyed A, and then released the worker. Completion could not publish into the destroyed runtime. MediaSources B and C continued to render exactly. Worker registry entries are weak references and workers hold a temporary `shared_ptr` only while servicing, avoiding raw task lifetime assumptions. This is fixture-local lifecycle evidence; it is not a production cancellation design.

M11 recreated fresh WAV readers, caches, arbitration state, two-worker service, Clip views and source/graph observations. The 32-media/32-Clip output was equal across reconstructions at timing error 0. OS scheduling history is intentionally not an equality requirement.

## Boundaries and conclusion

Tracktion source references, headers/types, and linkage are 0 for `option_b_multisource_arbitration`. The target links public JUCE audio-format support and Windows `Psapi` only; Tracktion Engine is not linked transitively.

**Is one worker per MediaSource necessary in the measured Option B source architecture? No, globally bounded workers demonstrated.** Reader contexts and caches still scale per unique MediaSource; cache bytes and pending capacity therefore have explicit per-media terms. File-handle count was not measured separately from reader contexts.

The ownership shape conceptually accommodates a persistent per-MediaSource decoder context, serialized bounded worker tasks, and a fixed decoded-page cache. That is an architectural inference only, not FFmpeg evidence.
