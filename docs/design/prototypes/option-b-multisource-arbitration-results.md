# Option B Multi-source Arbitration / Worker Scaling Results

## Final classifications

| Area | Result |
| --- | --- |
| Gate | **Proceed with Constraints** |
| Arbitration architecture | **Bounded shared worker model viable** |
| Implementation burden | **Substantial source-service subsystem** |

The accepted Candidate B backend and the earlier realtime-source handoff classification remain unchanged. This is a bounded WAV fixture, not an assertion that production source scheduling, device behavior, FFmpeg, SRC, or live mutation is solved.

## Selected model and reader finding

Model C was selected after comparing per-media workers (Model A), a globally shared request service (Model B), and per-media bounded arbitration with a shared worker pool (Model C). The fixture uses four fixed request slots and eight fixed pages per MediaSource with two shared workers. Model A would preserve the known worker-per-media term; Model B would add an unnecessary global request scheduler to this experiment.

The available public JUCE `AudioFormatReader` declaration documents reads but does not establish concurrent-reader safety. Each reader stays owned by a stable MediaSource runtime and is serialized with a per-media atomic busy guard. No worker affinity is required; maximum observed concurrent reader calls per media was one. See [API investigation](../../../benchmark-results/option-b/multisource-arbitration-api-investigation.md).

## Gate evidence

M0 baseline passed for one media/multiple views and three media. M1 covered same-page deduplication, adjacent and far ranges, and cross-media demand. M2 demonstrated fixed request capacity 4 with deterministic reject/zero/re-publish behavior. M3 demonstrated deterministic far, hot/cold, and cross-media progress without permanent starvation.

M4 retained two workers while serving 8, 16, and 32 MediaSources. M5/M6 measured the 1/4, 4/8, 8/16, 16/32, and 32/32 matrix. Readers, caches and cache bytes scale per media; workers remain fixed at two. M7 exact overlap/cross-media/dense sums passed at maximum timing error 0 with no hard clipping. M8 callback forbidden-operation counters were all zero.

M9 discarded stale MediaGeneration work and progressed current work. M10 destroyed a runtime during a held in-flight read, prevented publication to its cache, and kept unrelated sources exact. M11 fresh reconstruction was equal. Full evidence, formulas, working-set observations, and constraints are in [the benchmark record](../../../benchmark-results/option-b/multisource-arbitration.md).

## Resource result

```text
workers = 2
readers = unique active MediaSources
caches = unique active MediaSources
cache bytes = unique active MediaSources * 8224
pending capacity = unique active MediaSources * 4
active tasks <= 2
```

Therefore, **one worker per MediaSource is not necessary in the measured fixture**. Reader/decoder contexts and cache resources still have explicit per-media scaling terms. File handles were not independently measured.

## Tracktion and JUCE boundary

The Option B target has Tracktion source references, headers/types, and linkage all equal to 0. It links JUCE public audio-format support and Windows `Psapi`; Tracktion Engine is absent from the target dependency closure.

## Remaining risks and next gate

The finite request slots and round-robin service are fixture policies, not a production priority/eviction system. Real decoder contexts, compressed seek/preroll, real mixed-rate media, device callback behavior, and live mutation remain unproven. The result supports the inference that a persistent per-media decoder context with serialized bounded tasks and fixed pages is a viable shape; it is not FFmpeg evidence.

The single next recommended gate is **real mixed-rate Source-to-Timeline SRC**.
