# Option B Multi-source Arbitration / Worker Scaling Feasibility

## Status and objective

This is a planned feasibility gate only. It authorises no implementation, dependency change, CMake change, ADR, or architecture decision. It follows the accepted [ADR 0001](../../decisions/0001-audio-engine-backend-ownership.md) and the completed [Option B realtime source handoff](option-b-realtime-source-handoff-results.md).

It asks one bounded question:

> Can the selected Option B source subsystem serve concurrent multi-Clip and multi-MediaSource demand through bounded request arbitration and bounded worker ownership, while preserving exact integer scheduling, callback safety, bounded cache state, stale-work invalidation, and deterministic reconstruction?

This gate separates two scaling problems which must not be collapsed into one:

1. **Request-arbitration scaling:** one MediaSource has several simultaneous, far-apart Clip demands. A bounded request mechanism must neither grow without limit nor silently lose a needed range forever.
2. **Worker-ownership scaling:** independent MediaSources can otherwise create one worker, reader/decoder context, and cache per source. The A2 Model A fixture reached 32 workers/readers/caches for 32 distinct media; that measurement is an A2 risk reference, not an Option B result.

The previous B handoff gate is retained as **Proceed with Constraints**, **Thin backend handoff**, and maximum timing error 0. Its fixed cache, callback-zero counters, generation discard, one shared-media two-view sum, and long-source observation are a one-source baseline; its single latest-request slot does not establish multi-range arbitration.

## Evidence discipline and retained boundary

The selected shape remains:

```text
Domain -> integer Timeline/Source scheduling -> Clip runtime view
       -> MediaSource runtime/cache/arbitration -> worker/decoder service
       -> Option B SourceNode -> Option B graph -> processing/summing/PDC/Tail
```

The A2 realtime-source and multi-source/runtime-edit results inform requirements and risks only. They must never be reported as Option B measurements. In particular, A2's bounded Model A multi-source/edit evidence and its 32-distinct-media scaling observation do not prove B worker pooling, request arbitration, source-node integration, or lifecycle behavior.

The ownership boundary is retained:

| Owner | Responsibility | Must not own |
| --- | --- | --- |
| Clip runtime view | Timeline placement, integer Source mapping, Clip revision, range demand | reader, decoder, cache pages, worker lifecycle |
| MediaSource runtime | media identity, page/cache ownership, media generation, request-arbitration state | Timeline placement or graph-buffer identity |
| Worker service | serialized reader/decoder execution and completion publication | callback work or Domain authority |
| Option B SourceNode | consume ready pages, exact-zero miss, fixed buffer handoff, callback counters | file/decode/wait/allocation or arbitration locks |

All Timeline and Source positions, ranges, priorities, and generations remain integer values. Cache identity is `(MediaSource identity, Source page/range, MediaGeneration)`, never Timeline placement. Clip revision and MediaSource generation remain distinct because a placement edit is not necessarily a media/cache invalidation.

## Scope and exclusions

Included: local mono PCM16 WAV at 48 kHz, existing 128-frame graph processing baseline, fixed pages including 257-frame non-divisible geometry, bounded source request/cache/worker models, stopped/control-boundary edits only where needed for invalidation, headless exact-output checks, and resource observations.

Excluded: FFmpeg implementation, compressed/container media, mixed-rate real-media SRC, device callback/output, GUI, persistence, plugin scanning/state/UI, live graph mutation, production priority tuning, throughput/dropout claims, and a production scheduler. FFmpeg is recorded only as a future reason to avoid assuming that decoder contexts are cheap, movable, concurrently usable, or interchangeable.

## Candidate models to compare before selection

No model is preselected. A later implementation may use the smallest model that passes the gates, but must document why rejected alternatives are not needed yet.

| Model | Shape | Benefits | Risks and required evidence |
| --- | --- | --- | --- |
| A: fixed per-media arbitration | One MediaSource has a fixed request slot set/ring, fixed page cache, and reader context; its worker is per-media or otherwise assigned | Local ownership, page locality, straightforward media generation | Far-apart demand can exceed capacity. Show fixed capacity, dedup/merge, priority, deterministic overflow, cancellation, fairness, and no permanent starvation. |
| B: shared bounded request service | A runtime-wide service owns global fixed request capacity and schedules media work | Can bound global queue and coordinate cross-media demand | Show per-media fairness, global overflow, deduplication, generation/cancel handling, lifetime safety, no callback lock/wait, and no starvation. |
| C: per-media bounded arbitration plus small shared worker pool | Each MediaSource retains cache, arbitration and reader context; a fixed worker service executes tasks | Worker count need not equal media count and better matches future decoder cost | Show serialized reader use, task ownership, cancellation, teardown, queue fairness, and that active decoder contexts/caches still have explicit bounds. |

Selection criteria are callback safety, exact seek behavior, reader/decoder constraints, cancellation, page locality, simultaneous far-range service, destruction/reconstruction, deterministic tests, and explicit resource formulas. Neither a shared queue nor a worker pool is evidence by name alone.

## Reader and decoder ownership question

The gate must explicitly investigate the public JUCE `AudioFormatReader` constraints used by the fixture. Do not assume a reader is safe for concurrent calls, safely movable between workers, or compatible with more than one in-flight task merely because requests are queued.

The simplest bounded WAV model is one MediaSource-owned reader context used serially by whichever worker has been assigned its task. Record whether the reader remains permanently worker-affine, whether dispatch requires serialization, and whether creation/destruction must happen at a control boundary. A future FFmpeg decoder may require an even stricter per-media context/serial execution model. This gate does not solve that future integration; it records constraints that make a later design honest.

## Required planned gates

### M0: selected-B baseline reproduction

Reproduce the established source handoff through Option B with one MediaSource/multiple Clip views, then with three MediaSources. Retain the real-WAV, fixed page, fixed scratch, callback-zero, zero-on-miss, generation, B-graph and reconstruction boundaries. This is a control, not a reclassification of the handoff gate.

### M1: demand taxonomy

Establish deterministic cases for one media and across media:

```text
same page / deduplicated demand
overlapping ranges
adjacent ranges that may merge
far-apart ranges (critical)
cross-media concurrent demand (critical)
```

For every case record emitted request identity, dedup/merge result, priority/order, capacity consumption, pages made ready, misses/underruns, and exact marker output. A far request that is intentionally dropped by bounded overflow must render exact zero and be republished; it cannot become invisible.

### M2: fixed-capacity request representation

Use a fixed-capacity request representation with no callback allocation, container growth, or unbounded queue. Requests must carry integer Source range and MediaGeneration. Define deterministic overflow behavior, deduplication/merge behavior, priority policy, and capacity reclamation on completion/cancellation.

Overflow may produce a recoverable exact-zero miss followed by a later republish, but the fixture must measure whether that policy creates starvation. A callback must publish only through a bounded nonblocking mechanism; any callback mutex is a hard stop unless a nonblocking bounded proof is separately justified.

### M3: fairness and stale-work progress

Use deterministic alternating far-range demands and require both ranges eventually to become ready. Under frequent requests, prove no range experiences permanent starvation. With multiple media, show cross-media progress rather than one hot source monopolising service. Queue old-generation work, increment generation, and require stale work neither dominate service nor publish audio after invalidation.

Record issued, merged, rejected/overwritten, cancelled, completed, discarded-stale, and eventually-served requests, plus maximum wait in fixture service cycles. This is deterministic progress evidence, not a realtime deadline claim.

### M4: shared worker service, if selected

If Model B or C is selected, start with two and four fixed workers serving more MediaSources than workers: at minimum 8 media / 2 workers, then optionally 16 and 32 media where earlier evidence remains safe. Its question is `worker threads != media count`, not production tuning or benchmark throughput.

Show that a worker never concurrently uses the same non-concurrent reader context, and that a delayed/cancelled task cannot retain a destroyed runtime. If Model A is retained instead, record the evidence and limitation that keep one worker per media acceptable for this bounded gate.

### M5: resource ownership measurement

Separately measure and report worker threads, reader/decoder contexts, MediaSource runtimes, cache instances, cache bytes, pending-request capacity, active tasks, and file handles when observable. Do not use process working set as a substitute for these counts; working set is only a representative OS observation.

### M6: density matrix

Exercise Clip densities 4, 8, 16, and 32 across:

```text
one media
a few media
all distinct media
```

Smaller representative cells may be chosen only if documented with the bound they establish and the cells they do not establish. At every chosen cell, capture resource counts, callback counters, request/underrun data, timing error, output sum, and any stale output.

### M7: exact output and summing

Verify marker timing error is 0 samples. At non-overlap regions output must equal the observed decoded PCM16 source sample (and known downstream processing where present); overlap must be the exact linear float sum, including amplitudes greater than 1.0 with no unintended clipping. Same-source views must remain distinct Timeline consumers despite shared source pages.

### M8: callback invariants

For all density and arbitration tests require:

```text
callback reader/file/decode calls = 0
callback waits/blocks/spins = 0
callback allocation/growth = 0
synchronous miss fallback = 0
```

The source node may copy only already-published fixed storage and produce zero plus underrun for uncovered frames. Worker-side synchronization and bounded control-boundary waits must be reported separately and must never migrate to callback execution.

### M9: stale generation and cancellation

Queue multiple ranges, retain some in flight, advance MediaGeneration through seek/edit/rebuild, and cancel/remove demand. Old completions must be discarded; their request capacity must be reclaimed; new-generation demand must make progress; and no stale page/audio may be published. Treat cancellation as both a correctness and a capacity-liveness condition.

### M10: lifecycle and destruction

Destroy a MediaSource with pending/active tasks while workers continue to serve unrelated media. Verify cancellation/completion cannot use freed memory, no deleted media can reappear, and unrelated media continue exactly. Bounded control-boundary teardown waits are allowed only outside callback processing. Then destroy final views/services and account for worker, reader, cache and request-state teardown.

### M11: deterministic reconstruction

Create an equivalent fresh set of MediaSource runtimes, arbitration state, worker service, Clip views, SourceNodes, and Option B graph from the same framework-free description. Render twice and require identical defined output, marker locations, timing error, and intentional miss/underrun observations. Queue/cache/worker history cannot become hidden persistent authority.

## Mandatory resource-scaling report

For each feasible row, publish a table with these fields. Missing values must be marked unavailable, not inferred.

| Unique media | Active clips | Worker threads | Reader/decoder contexts | Cache instances | Cache bytes | Pending capacity | Active tasks | Callback counters | Working set | Exact output / timing |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- | --- |
| 1 | 4 | | | | | | | | | |
| 4 | 8 | | | | | | | | | |
| 8 | 16 | | | | | | | | | |
| 16 | 32 | | | | | | | | | |
| 32 | 32 | | | | | | | | | |

State the formulas separately. For example, a bounded shared worker pool may establish `workers = configured pool size`, while reader contexts, caches, and cache bytes may still scale with active MediaSources. The gate must answer each independently, not label all resources “shared.”

## Required conclusion on worker necessity

The results must answer, with evidence, one of the following for each resource:

| Question | Permitted conclusion |
| --- | --- |
| Is one worker per MediaSource necessary? | No: globally bounded worker service is viable; Yes: reader/decoder constraints require per-media worker ownership for the tested model; or Not determined. |
| Do reader/decoder contexts scale per media? | Measured formula and API constraint; no assumption of sharing. |
| Do caches scale per media? | Measured formula, page bound, and sharing policy. |
| Does pending work scale safely? | Fixed global/per-media bounds, overflow and fairness evidence. |

“Workers are bounded” is not enough when readers, caches, handles, or active tasks remain unbounded.

## Hard stops

Stop the gate and classify it Reject or Inconclusive before widening scope if any of the following occur:

* nonzero sample timing error, incorrect linear sum, stale audio, or unintended clip;
* callback reader/file/decode, wait/block/spin, allocation/growth, synchronous fallback, or a callback mutex without a bounded nonblocking justification;
* unbounded request/cache/task growth, undefined overflow, or repeated loss/starvation of a demanded far range;
* concurrent use of a reader/decoder without public support, unsafe cancellation, use-after-free, leaked destroyed media, or lifecycle ownership ambiguity;
* an implementation that needs Tracktion source/header/type/linkage, changes Domain authority, or introduces a production scheduler disguised as fixture plumbing; or
* resource scaling that cannot be described with explicit configured bounds and per-media terms.

## Future evidence and classification

A later authorised implementation must create, without overwriting A2 evidence:

```text
benchmark-results/option-b/multisource-arbitration-api-investigation.md
benchmark-results/option-b/multisource-arbitration.md
benchmark-results/option-b/multisource-arbitration-complexity.md
docs/design/prototypes/option-b-multisource-arbitration-results.md
```

The result document must include scope/pins, model comparison and selected model, reader constraints, M0-M11 evidence, resource table/formulas, exact-output and callback counters, invalidation/cancellation/lifetime, reconstruction, Tracktion exclusion, JUCE dependency boundary, remaining risks, and one recommended next gate.

Classify the gate as `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`. Independently classify arbitration architecture as one of:

```text
Bounded shared worker model viable
Per-MediaSource worker model retained
Source scheduling complexity materially increased
Inconclusive
```

Classify implementation burden as `Bounded source-service extension`, `Substantial source-service subsystem`, or `Custom-engine-scale warning`. These are evidence conclusions, not labels selected in advance.

## Completion criteria and next gate

This planning task is complete when it supplies an unambiguous M0-M11 sequence, separate arbitration and worker-ownership decisions, deterministic hard stops, resource formulas/table, callback rules, and evidence outputs. It changes no source, test, target, dependency, ADR, or execution-plan state.

If bounded arbitration and worker ownership pass at timing error 0 with callback invariants intact, the likely single next gate is real mixed-rate Source-to-Timeline SRC policy and implementation. If this gate fails, the next action must resolve the specific arbitration, reader-ownership, or lifecycle failure rather than adding FFmpeg or device scope.
