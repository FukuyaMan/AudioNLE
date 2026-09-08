# Execution Plan: Option B Multi-source Arbitration / Worker Scaling

## Completion

Completed. M0--M11 passed with fixture-local Model C: four fixed request slots and eight fixed pages per MediaSource, plus two shared worker threads. Maximum timing error was 0; callback forbidden-operation counters were 0; 32 MediaSources/32 Clips retained two workers; stale cancellation, held in-flight destruction, and fresh reconstruction passed. Final classifications: `Proceed with Constraints`, `Bounded shared worker model viable`, and `Substantial source-service subsystem`. See [results](../../design/prototypes/option-b-multisource-arbitration-results.md).

## Objective

Execute the bounded M0--M11 feasibility gate in [Option B multi-source arbitration feasibility](../../design/prototypes/option-b-multisource-arbitration-feasibility.md). Establish, reject, or constrain whether the selected Option B backend can serve concurrent multi-Clip and multi-MediaSource demand with bounded request arbitration and bounded worker ownership while preserving integer timing, callback safety, stale-work invalidation, lifecycle safety, and deterministic reconstruction.

This gate must independently classify:

```text
Gate: Proceed | Proceed with Constraints | Reject | Inconclusive

Arbitration architecture:
  Bounded shared worker model viable
  Per-MediaSource worker model retained
  Source scheduling complexity materially increased
  Inconclusive

Implementation burden:
  Bounded source-service extension
  Substantial source-service subsystem
  Custom-engine-scale warning
```

It is not an ADR, a production scheduler, FFmpeg work, device validation, or a change to the accepted Candidate B architecture.

## Established baseline and evidence discipline

Retain the completed Option B realtime-source handoff result unchanged:

```text
Gate = Proceed with Constraints
Integration = Thin backend handoff
Maximum timing error = 0 samples
callback reader/file/decode = 0
callback wait/block/spin = 0
callback allocation/growth = 0
synchronous fallback = 0
fixed cache = 8224 bytes
SourceNode scratch = 512 bytes
Tracktion linkage = 0
```

The handoff fixture proved a single bounded latest-request mechanism only. It did not prove multi-range arbitration or worker scaling.

The A2 Model A results are risk and design reference, not Option B evidence: one worker/reader/cache per unique MediaSource reached 32 workers/caches at 32 distinct media, and its one-slot policy can overwrite an unserved far range. Do not copy an A2 measurement into an Option B result. Preserve the accepted ADR and the source-handoff results.

## Scope and exclusions

Use repository-local temporary mono PCM16 WAV fixtures at 48 kHz, 128-frame B graph blocks, fixed source pages including the existing 257-frame geometry, Option B's source-node/graph handoff, and headless deterministic observations. A stopped/control-boundary edit is allowed only to test MediaGeneration, cancellation, and lifetime.

In scope: fixed-capacity request/task representation, request deduplication/merge/overflow/retry, deterministic fairness, candidate worker ownership, reader serialization, cache/resource accounting, source-node callback counters, cancellation/destruction, and reconstruction.

Out of scope: source-format expansion, FFmpeg or compressed/container media, mixed-rate SRC, devices, GUI, persistence, plugin state/scanning, live graph mutation, production priority tuning/throughput claims, Tail changes, dependencies/submodules, Tracktion, and ADR changes.

## Required ownership and invariants

```text
ClipRuntimeView
  owns Timeline placement, integer Timeline-to-Source mapping, Clip revision,
  and range demand.

MediaSourceRuntime
  owns media identity, Source pages/cache, MediaGeneration, request-arbitration state,
  and the reader/decoder context policy.

WorkerService
  owns bounded task dispatch, serialized reader/decoder execution, and completion publication.

Option B SourceNode
  consumes ready pages through fixed storage; on a miss emits exact zero and records
  underrun. It neither decodes, waits, allocates, nor arbitrates under a lock.
```

The cache key is exactly the MediaSource identity plus integer Source range/page identity plus MediaGeneration. It never contains Timeline placement, graph Node/buffer identity, worker identity, or a Domain runtime pointer. Clip revision and MediaGeneration remain independent.

Hard invariants:

1. Maximum marker timing error is 0 samples; expected amplitudes derive from observed PCM16-decoded samples.
2. Same-source Clip views are separate Timeline consumers even when pages/cache are shared; overlaps use the exact linear float sum and must not hard-clip values above 1.0.
3. Callback reader/file/decode, wait/block/spin, allocation/growth, and synchronous miss fallback remain 0 for every case.
4. Request, task, cache, scratch, reader and worker ownership have declared fixed bounds/formulas. No callback may grow a vector/list/queue or synchronously decode on overflow.
5. A stale completion cannot publish audio; cancellation reclaims capacity; a destroyed MediaSource cannot be retained by a task; unrelated MediaSources remain live.
6. Framework-free descriptions have no workers, readers, pages, queues, graph objects, JUCE types, or runtime identities. Fresh reconstruction has no hidden queue/cache history.
7. Tracktion source/header/type/linkage remains 0 in the Option B target.

## Candidate-model investigation and selection rule

Before implementation, document the actual API/lifetime constraints and compare all models. Do not select a winner in this plan.

| Model | Shape | Evidence required before accepting it |
| --- | --- | --- |
| A | Fixed per-MediaSource request slots/ring, cache and reader; per-media or assigned worker | Fixed capacity, far-range behavior, dedup/merge, deterministic overflow, cancellation/fairness, per-media resource formula. |
| B | Shared bounded request service across runtimes | Global capacity, per-media fairness, dedup/cancel/generation, no callback lock, lifecycle safety, no starvation. |
| C | Per-media bounded arbitration/cache/reader context with a fixed shared worker pool | All Model-A media bounds plus worker-pool task ownership, reader serialization, cancellation and teardown. |

Audit public JUCE `AudioFormatReader` use before attempting B or C. Record MediaSource ownership, whether tasks only reference a stable reader rather than move it, creation/destruction boundary, serialization requirement, and why concurrent calls are prohibited or supported. Unless public API evidence establishes otherwise, one MediaSource reader must never be processed concurrently by more than one worker task. Future FFmpeg implications may be documented as architectural inference only, never as FFmpeg evidence.

Choose the smallest candidate that satisfies all gates. If Model A is retained, document why a bounded shared worker model was not demonstrated; do not call per-media worker count “bounded” without stating its media-count term.

## M0: baseline control

Reproduce the handoff fixture through the selected B source-node contract for:

```text
1 MediaSource / multiple Clip views
3 MediaSources
```

Check real-WAV markers, 257/128 boundary geometry, fixed cache and scratch, exact-zero miss/recovery, generation behavior, reconstruction, and Tracktion exclusion. This is regression control only and does not reclassify the handoff gate.

## M1--M3: request arbitration

### M1: deterministic demand taxonomy

Create deterministic cases for same-page demand, overlapping ranges, adjacent ranges, far-apart same-media ranges, and cross-media concurrent ranges. Far-apart demand is mandatory. For each record request ID/range/generation, dedup/merge result, capacity used, service order, misses/underruns, pages made ready, and exact output.

### M2: fixed-capacity representation

Implement only fixed-capacity request and, where required, task storage. Each request includes at minimum MediaSource identity, integer Source range/page identity, MediaGeneration, and deterministic priority/order metadata. Define and test deduplication, overlap merge, adjacent merge policy, deterministic overflow, republish behavior, cancellation, completion, and capacity reclamation.

Overflow may yield `zero + underrun + later republish`; it may not cause synchronous decode or a permanently invisible range.

### M3: fairness and progress

Use service-cycle observations, not wall-clock deadline claims:

```text
alternate two far ranges from one media
hot frequently requested range plus cold range
concurrent demand across multiple media
old-generation work followed by current-generation work
```

Both far ranges and every participating media must eventually become ready. Record issued, deduplicated, merged, rejected/overflowed, republished, cancelled, completed, discarded stale, eventually served, and maximum fixture service cycles until ready. Stale backlog must neither publish audio nor permanently consume service capacity.

## M4--M6: worker ownership and scaling

### M4: worker-model test

For Model B or C, first run 8 MediaSources with 2 workers, then at least 16 MediaSources with 2 or 4 workers; run 32 with the same configured worker count when practical and when earlier gates are safe. The assertion is `worker threads != MediaSource count`, not throughput tuning.

Show serial access to every non-concurrent reader, stable task lifetime, cancellation before unsafe completion publication, and continued progress after a delayed task. For Model A, quantify workers/readers/caches per media and explain why no shared-worker evidence exists.

### M5: resource accounting

For every executed configuration record, separately count:

```text
MediaSource runtimes; workers; reader/decoder contexts; observable file handles;
cache instances and bytes; pending-request capacity; active-task capacity;
SourceNode scratch; callback counters; underruns; representative working set.
```

Publish formulas based only on measured/configured ownership. Working set is an OS observation, never a substitute for resource counts.

### M6: density matrix

Prefer these rows and document every skipped row plus the unmeasured bound:

| Unique media | Active clips |
| ---: | ---: |
| 1 | 4 |
| 4 | 8 |
| 8 | 16 |
| 16 | 32 |
| 32 | 32 |

At every executed row, retain M7/M8 output and callback assertions and record pending/active bounds, fairness data, resource counts, cache bytes, working set and underruns.

## M7--M8: exact graph output and callback safety

### M7: output

Exercise same-source overlap, cross-media sum, and dense sum. At every marker require timing error 0. Unique regions equal decoded PCM expectations; overlap regions equal exact linear float sums, including values above 1.0, with no unintended clipping. Confirm shared cache identity never makes two distinct Timeline placements collapse into one.

### M8: callback invariants

At the SourceNode boundary require in every M0--M7 density/arbitration case:

```text
reader/file/decode = 0
wait/block/spin = 0
allocation/growth = 0
synchronous fallback = 0
```

Callback request publication must be bounded and nonblocking. A callback mutex is a hard stop unless a separately documented proof establishes bounded nonblocking behavior. Worker-side synchronization or bounded control-thread teardown waits must be kept outside callback execution and reported.

## M9--M11: invalidation, lifetime, reconstruction

### M9: generation and cancellation

Queue multiple pending ranges, allow some worker tasks to be active, advance MediaGeneration via the permitted control-boundary seek/edit/rebuild, then request current ranges. Require stale tasks to be discarded, stale capacity reclaimed, current ranges to progress, and no stale audio/page publication. Report all relevant counters.

### M10: destruction

Run:

```text
MediaSources A/B/C active -> pending requests -> destroy A
-> A task cancellation/completion -> B/C continue
```

Require no use-after-free, no reappearance of A audio, reclaimed A capacity, exact continued B/C output, and no callback destruction wait. Record control/worker teardown bounds and final worker/reader/cache/request-state release.

### M11: reconstruction

From one unchanged framework-free final description, construct fresh MediaSource runtimes, arbitration state, worker service, Clip views, B SourceNodes and graph; render; destroy; reconstruct again; render. Require equal defined output, marker timing, maximum error 0 and equivalent deliberately induced miss/recovery observations. No persisted task/queue/cache history is permitted.

## Planned implementation surface

Keep all implementation fixture-local under `prototype/option-b-low-level-graph/`. The exact source/CMake/test target names follow the M0 contract audit. Do not modify A2 prototype sources, product/Domain code, dependencies, submodules, ADR 0001, or Tracktion configuration.

Expected evidence output after an authorised implementation:

```text
benchmark-results/option-b/multisource-arbitration-api-investigation.md
benchmark-results/option-b/multisource-arbitration.md
benchmark-results/option-b/multisource-arbitration-complexity.md
docs/design/prototypes/option-b-multisource-arbitration-results.md
```

The complexity record must separate existing/common-source responsibilities (MediaSource cache, Clip views, generation, SourceNode), new arbitration (fixed requests, dedup/merge, overflow, fairness, cancellation), new worker service (dispatch, reader serialization, lifetime, completion), and fixture-only controls/assertions/instrumentation. Do not produce a fake production LOC total.

## Hard stops

Stop and classify the gate `Reject` or `Inconclusive` before adding scope when any of the following occurs:

* timing error is nonzero, the sum is wrong/clipped, stale audio appears, or a deleted source reappears;
* callback does I/O/decode, waits/spins, allocates/grows, uses fallback, or locks without a proven nonblocking bound;
* request/task/cache storage is unbounded, overflow is undefined, or a demanded far range permanently starves;
* reader calls are concurrent without public support, cancellation/lifetime is unsafe, a task causes use-after-free, or destroyed-media state leaks;
* resource scaling lacks explicit configured bounds/formulas, Timeline enters source/cache identity, or graph/worker identity leaks into Domain; or
* Tracktion enters the target, or passing requires FFmpeg, devices, live mutation, or a disguised production scheduler.

## Verification and completion

After implementation, run:

```powershell
.\build.ps1
ctest --test-dir build -R '<actual-option-b-multisource-arbitration-regex>' --output-on-failure
```

Also directly run the generated fixture at least three deterministic times; perform `git diff --check`; review Tracktion source/header/linkage, JUCE dependency boundary, Domain/runtime leakage, resource formulas/counts, and the final diff. Report the known unrelated Tracktion high-level Phase E failure separately if it remains; any new compile/link failure is a hard failure.

Pass requires M0--M11, timing error 0, exact output/summing, callback invariants, bounded request/task/cache/worker evidence, fairness progress, reader serialization, safe cancellation/destruction, deterministic reconstruction, Tracktion linkage 0, and the four evidence documents above.

When complete, move this plan to `docs/exec-plans/completed/option-b-multisource-arbitration.md`. If bounded request arbitration and bounded worker ownership pass, recommend exactly **real mixed-rate Source-to-Timeline SRC**. Otherwise recommend one narrowly named corrective source-service gate.
