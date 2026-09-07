# Option A2 Realtime Prefetch / Cache / Thread Ownership Feasibility

## Status

Planned feasibility gate only. This document authorises no implementation and does not choose a production audio engine, source architecture, decoder, cache policy, synchronization primitive, or ADR.

## Context

Option A2 has bounded, offline evidence for AudioNLE-authoritative integer Timeline/Source scheduling, public low-level Tracktion processing and `SummingNode` mixing, deterministic PDC and finite Tail fixtures, actual fixed-fixture VST3 PDC/Tail, and same-rate real WAV exact read/seek/rebuild. The real-media gate selected public JUCE `AudioFormatReader` as a small offline range-read boundary and showed one-hour-class PCM WAV random access without a duration-scaled decoded PCM allocation.

That reader result is explicitly not realtime evidence. The existing `RealMediaSourceNode::process` may cause filesystem I/O, decoder work, allocation, or blocking. It cannot run unchanged on a realtime audio/process callback.

The next question is deliberately narrower than a production audio engine:

> Can AudioNLE preserve authoritative integer scheduling while moving local WAV reader work off the process callback, using a bounded prefetch/cache and explicit worker ownership, without stale audio, callback blocking, duration-scaled PCM retention, or high-level Tracktion source scheduling?

## Existing boundary retained

```text
AudioNLE Domain / Playback State
  -> AudioNLE integer Timeline / Source requests
  -> AudioNLE source scheduler
  -> prefetch request and cache-ownership layer
  -> background file-reader worker
  -> bounded PCM cache
  -> realtime-safe custom source Node
  -> Tracktion low-level graph
  -> processing / output observation
```

AudioNLE remains authoritative for Source identity, integer Source/Timeline mapping, request order, seek/reset intent, generation, cache policy, and lifetime policy. Tracktion may own only its public low-level graph traversal and downstream processing. `WaveAudioClip`, `WaveNodeRealTime`, `Edit`, Tracktion media/cache managers, and high-level Tracktion source scheduling must not become timing authorities.

## Scope

### Included

* Mono local PCM16 WAV at 48 kHz Source and 48 kHz Timeline rate.
* Existing public JUCE `AudioFormatReader`, Option A2 custom source Node, and public low-level Tracktion graph.
* Fixed 128-frame process-block baseline and temporary one-hour-class WAV fixture with distinct marker samples.
* A bounded AudioNLE-owned prefetch/cache, one background reader worker, explicit request publication, seek/reset generation invalidation, cancellation, destruction, and offline callback-path instrumentation.
* Exact marker observation, cache hit/miss/underrun behavior, cache/page and process-block boundaries, worker lifetime, reconstruction, and complexity evidence.

### Excluded

* Mixed-rate media, SRC, FFmpeg, compressed/container media, stream selection, production decoder coverage, and network media.
* Audio-device integration, GUI transport, UI editing, production scheduler, waveform/proxy, export, persistence, arbitrary plugins, PDC/Tail/VST3 revalidation, ADR, and final architecture selection.
* A production latency, MB, throughput, or dropout guarantee.

## Realtime definition for this gate

“Realtime-safe” here is an architectural callback-path claim, not an operating-system scheduling guarantee. During `Node::process` and the directly invoked audio/process callback path, the implementation must not:

* open, seek, read, map, or otherwise call the filesystem;
* invoke decoder work or synchronous fallback reads;
* take a blocking mutex, wait on a condition variable, spin, or retry-lock;
* allocate, free, resize, or grow a dynamically sized buffer on the callback path;
* depend on UI, worker progress, logger, or any duration-unbounded operation.

A lock-free primitive is not automatically acceptable: its memory ownership, publication rules, overwrite behavior, and callback operations must be inspected and measured. Passing this gate does not claim that Windows scheduling can never cause a dropout.

## Core invariants

1. Timeline and Source positions remain distinct signed integer sample domains; expected marker alignment error is exactly 0 Timeline samples.
2. Domain contains no worker, reader, file handle, cache page, queue, mutex, generation runtime object, or Tracktion/JUCE type.
3. Cache PCM capacity is explicit and independent of source duration: `pageCount * framesPerPage * channels * sizeof(float)`.
4. The callback reads only already-published fixed storage and writes its supplied output; it never performs forbidden work.
5. A cache miss produces the declared bounded underrun result: exact zero output plus a counter/event. It never returns stale or uninitialised PCM, blocks, or falls back to a synchronous reader call.
6. A seek/reset increments or replaces a runtime-only generation. Pages and reader completions from an earlier generation must not be observable as current data.
7. Worker/cache destruction is cancellable and complete: no use-after-free, hang, thread leak, file-handle leak, or Domain mutation.
8. Same Domain -> destroy runtime -> recreate runtime reproduces the defined observation; cache contents are never persisted as Domain state.
9. Fixture-only instrumentation cannot silently become proof of production realtime performance.

## Q0: API and lifecycle investigation

Before implementation, inspect the pinned Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf` and nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`, plus existing repository code. Record public/private status, file/line references, thread and allocation assumptions, and relevant tests/examples in `benchmark-results/tracktion-selective-reuse/realtime-source-api-investigation.md`.

Investigate:

* Tracktion `Node`/player process lifecycle and any graph worker/prefetch hook, including whether it can be used without high-level source scheduling.
* JUCE `AbstractFifo`, applicable fixed-storage FIFO/ring primitives, worker/thread-pool primitives, and cancellation/lifetime facilities.
* Existing Tracktion audio FIFO/cache primitives only as candidates; do not assume suitability merely because a public header exists.
* The current JUCE reader’s ownership, concurrent access, seek/read, and destruction constraints.
* Existing repository instrumentation and tests relevant to allocation, worker ownership, and process callbacks.

Private APIs, a Tracktion/JUCE patch/fork, required high-level scheduling, or a callback reader call are hard Reject signals. Q0 must compare at least these architectures before Q1:

| Candidate | Required acceptance evidence |
| --- | --- |
| A. AudioNLE fixed PCM pages/ring plus dedicated reader thread | fixed callback storage; clear producer/consumer ownership; explicit generation/cancellation; bounded memory |
| B. AudioNLE cache using a public JUCE concurrency primitive | public semantics support the required producer/consumer ownership without callback blocking or dynamic growth |
| C. Independently reusable public Tracktion low-level cache/buffer | no high-level scheduling ownership, bounded memory, callback-safe consumption, and no unwanted `Engine`/media-cache coupling |

Select the smallest viable candidate. Do not force reuse of C if it weakens AudioNLE timing authority.

## Cache representation decision

Q0 must choose and document exactly one fixture representation: fixed sample blocks, fixed pages, or a contiguous fixed window around the playhead. The choice must define page/block state, valid Source range, generation, ready/publication state, producer ownership, callback consumer ownership, eviction/overwrite rule, and miss behavior.

The representation must support exact sample addressability, sequential playback, forward/backward seek, page boundaries, process-block boundaries, wrap behavior if applicable, and a future decoder with non-WAV read granularity. It must not imply a production cache size or policy.

## Q1: sequential prefetched playback

Use the one-hour-class WAV and a background reader to prefetch a known sequential region before callback processing begins. The realtime source Node may consume only cache data. Render known markers through the custom source Node and the public low-level graph.

For every marker record requested Timeline and Source positions, output position/value, alignment error, cache hit, callback file-read count, callback allocation count where measurable, callback wait/block count, and process duration. Pass only if all required marker errors are 0 and all forbidden callback-operation counters are 0.

## Q2: deliberate cache miss / underrun policy

Deliberately request a range the worker has not yet published. The fixture policy is:

```text
cache miss -> exact zero output + underrun counter/event
```

Record request, cache state, generation, output, underrun count, callback counters, and later recovery. Reject immediately for stale prior audio, uninitialised PCM, callback blocking, or synchronous reader fallback. Recovery must occur only after a matching current-generation page is published.

## Q3: seek and generation invalidation

Issue these seek transitions through AudioNLE-owned runtime request state:

```text
beginning -> near end
near end -> beginning
middle -> later
later -> earlier
```

Define a monotonic runtime generation/epoch or equivalent request version. Verify that pending old-generation reads cannot publish as current pages, that old cached audio is never emitted after a new seek, and that each available new-generation marker appears at its exact Timeline position. Domain must not persist the generation.

## Q4: rapid seek/reset stress

Exercise a bounded rapid request sequence such as:

```text
A -> B -> C -> A -> near-end
```

The worker may abandon obsolete work, but it must not publish it as current. Record every issued generation, published generation, cancellation/abandon observation, underrun, callback forbidden-work counters, and final exact marker result. This is a correctness/lifetime test, not a UI responsiveness benchmark.

## Q5: cache/page and process-block boundaries

Place markers at page start - 1, page start, page start + 1, page end - 1, page end, page end + 1, process-block boundaries, and cache wrap boundaries if the selected representation wraps. Repeat with render starts immediately before, at, and after relevant markers. Every cache hit must preserve 0 sample error; every miss must follow Q2’s exact-zero policy.

## Q6: long-source bounded-memory evidence

Use the existing one-hour-class fixture and report source bytes, configured cache capacity, allocated cache bytes, worker-side buffers, process working set before/after sequential playback, far seek, and backward seek, plus retained page count. Pass only if cache allocation is explicit and does not scale with source duration. Do not invent a production MB limit or claim an OS page cache is AudioNLE’s cache.

## Q7: worker cancellation and destruction

Test both idle and pending-read destruction:

```text
Domain -> worker/cache/runtime -> process -> destroy while idle
Domain -> worker/cache/runtime -> issue pending read -> cancel/destroy
```

Verify bounded completion under the harness timeout, no use-after-free, no hang, no thread leak, no file-handle leak where observable, no callback forbidden work during teardown, and no Domain mutation. The cancellation protocol must establish whether the worker is joined, stopped, or otherwise made unable to access released cache storage before destruction returns.

## Q8: reconstruction, complexity, and next decision

Verify:

```text
same Domain -> runtime/cache/worker -> defined playback observation -> destroy
same Domain -> new runtime/cache/worker -> same observation
```

The comparison must include output, exact marker positions, requested ranges, miss/underrun behavior under the same intentional timing control, and no stale generation leak.

Create `benchmark-results/tracktion-selective-reuse/realtime-source-complexity.md` with custom cache, queue/control, worker, cancellation/lifetime, and realtime-node LOC; public JUCE/Tracktion APIs; OS-specific APIs; synchronization primitives; allocations; thread count; memory capacity; Tracktion coupling; decoder coupling; and future FFmpeg compatibility. Assess whether the boundary remains bounded adapter glue or has become custom-engine-scale work.

## Callback instrumentation requirements

Instrumentation must be bounded and preallocated, or be collected outside the callback through an independently reviewed mechanism. At minimum record:

* callback thread identity;
* callback filesystem/read invocation count;
* callback allocation/resizing count where measurable;
* callback blocking/wait count;
* cache hit/miss and underrun count;
* requested and published generation;
* process duration as diagnostic evidence only.

An inability to count a category is not proof that it is absent; combine measurement with source inspection. Do not log, allocate, or perform I/O in the callback merely to instrument it.

## Processing compatibility

After Q1-Q8 hard gates pass, run only one downstream case:

```text
prefetched real-media source -> Multiply(2) -> output observation
```

It must retain exact marker timing and callback invariants. PDC, VST3, Tail, arbitrary plugins, device playback, and production effect chains remain out of scope.

## Future compressed-media implication

This gate does not add FFmpeg. Its cache/worker boundary must nevertheless remain usable by a future decoder that has packet granularity, seek preroll, codec delay, frame cache, cancellation, and varying output block sizes. A WAV-specific memory map or reader behavior must not become a hidden requirement of the cache protocol.

## Rejection signals

Classify the gate as Reject if any required result shows callback filesystem/decoder access, callback allocation/growth, callback blocking/spin/retry, synchronous miss fallback, stale or uninitialised miss output, nonzero marker timing error, source-duration-scaled retained PCM, incomplete cancellation/destruction, private API/patch/fork dependence, high-level Tracktion source scheduling, or custom-engine-scale complexity without compensating reusable value.

## Expected evidence and result document

The later, separately authorised implementation must create:

```text
benchmark-results/tracktion-selective-reuse/
  realtime-source-api-investigation.md
  realtime-source.md
  realtime-source-stress.md
  realtime-source-complexity.md
docs/design/prototypes/
  tracktion-selective-reuse-realtime-source-results.md
```

The final results document must state one of `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`, and cover scope/pins, selected thread/cache architecture, callback invariants, memory capacity, Q1-Q8 results, instrumentation, processing compatibility, complexity, future FFmpeg implications, remaining realtime risks, and the next permitted gate.

## Completion criteria

The planning gate is complete when the later implementation can follow a hard-stop Q0 -> Q8 sequence with no ambiguity about callback prohibitions, cache miss behavior, generation invalidation, ownership/destruction, exact sample criteria, bounded-memory evidence, instrumentation limits, non-goals, and decision rules. This planning document itself makes no runtime-safety claim.
