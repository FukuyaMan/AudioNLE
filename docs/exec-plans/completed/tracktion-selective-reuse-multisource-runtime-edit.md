# Execution Plan: Option A2 Multi-source / Runtime-edit Source Scheduling

## Status and decision boundary

Active planning document. It authorises a bounded feasibility prototype only; it does not select a production architecture, add an ADR, or alter existing pins. The final gate must classify both:

* gate: `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`;
* subsystem: `Bounded adapter extension`, `Substantial adapter extension`, or `Custom-engine-scale warning`.

The architectural question is whether multi-source and runtime-edit responsibilities remain a constrained AudioNLE source adaptation while public Tracktion low-level graph traversal, buffer flow, mix, PDC, VST3 integration, Tail flow, and headless processing remain materially valuable.

## Progress

M0–M11 were executed on 2026-09-07. Model A was selected for the bounded fixture. M9 bounded latest-slot arbitration and M10 fresh-runtime reconstruction passed; M11 classified the work as a substantial adapter extension and the gate as Proceed with Constraints. Persistent partial graph mutation remains unresolved. This plan is complete.

## Objective

Implement and measure, in hard-stop order, a same-rate local-WAV multi-Clip fixture that distinguishes MediaSource runtime from Clip runtime view, compares bounded ownership models, proves exact Source/Timeline mapping through dense and edited cases, and reports scaling and invalidation complexity without rebuilding Tracktion's high-level source scheduler.

Already established evidence is not a deliverable of this plan: framework-free Domain; integer authority; real-WAV reads; one-hour bounded reads; fixed realtime pages; callback reader/wait/allocation counters of zero; zero-plus-underrun misses; generation discard; non-divisible pages; destruction/reconstruction; downstream processing; `SummingNode`; and deterministic/actual VST3 PDC/Tail.

## Fixed scope and pins

* Tracktion Engine stays `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE stays `37c894f83d379179b2070d437ccd0f1cd9af9576`.
* Fixtures are local mono PCM16 WAV, Source and Timeline both 48 kHz, with 128-frame baseline process blocks.
* Reuse the existing JUCE reader-worker boundary, custom realtime Node path, and public Tracktion low-level graph only.
* Runtime edits are controlled/stopped-state until M8; no GUI or device callback is introduced.

Out of scope: FFmpeg/new decoder/concurrency dependency, compressed media, SRC, GUI, waveform, export, persistence, full ripple, synchronization groups, production cache sizing, CMake/dependency/submodule changes, ADR, and final architecture decision.

## Retained boundary and invariants

```text
AudioNLE Domain
  -> integer Timeline / Source scheduling
  -> source request planning
  -> realtime MediaSource runtime / cache / worker
  -> Clip runtime view(s)
  -> custom realtime source Node(s)
  -> public Tracktion low-level graph
  -> mix / processing / PDC / VST3 / Tail
```

The Domain must retain distinct `MediaSource` and `Clip` identities. The following runtime-only identities must not be saved into it: MediaSource runtime, Clip runtime view, Timeline placement, Source range request, cache page, worker, Clip revision, reader generation, queue, file handle, or Tracktion/JUCE object.

Across every phase:

* Source and Timeline use signed integer samples; required marker error is exactly zero Timeline samples.
* Same source sample at different Timeline placements must remain distinct output events.
* Callback filesystem/reader calls, waits/blocks/spins, allocation/growth, and synchronous miss fallback remain zero.
* A cache miss yields only exact zero plus underrun; stale/uninitialised output is forbidden.
* Pages, requests, pending work, caches, workers, and file handles use explicit bounds.
* No `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, private API, patch/fork, or Tracktion high-level source timing authority is permitted.

## Planned components and evidence

Later implementation may change only the repository-local prototype and its test integration as needed. It will write:

```text
benchmark-results/tracktion-selective-reuse/
  multisource-api-investigation.md
  multisource.md
  runtime-edit.md
  multisource-stress.md
  multisource-runtime-edit-complexity.md

docs/design/prototypes/
  tracktion-selective-reuse-multisource-runtime-edit-results.md
```

The active plan will move to `docs/exec-plans/completed/` only after all applicable phases, targeted build/test evidence, documentation, and self-review complete.

## Hard-stop protocol

Run M0 through M11 in order. A failed required invariant is a hard stop: record the raw result and classification, update the later result document, and do not implement unrelated later phases. A slow 32-Clip fixture alone is not a rejection; only architectural evidence such as invariant breach, unbounded growth, stale output, exact-timing failure, worker/file/cache explosion, or loss of Tracktion reuse is.

Immediate stop/reject signals are callback file/decode work; callback blocking/spin; callback allocation/growth; unbounded request storage; stale post-edit output; wrong Source-to-Timeline mapping; nonzero timing error; same-source/different-placement confusion; deleted output reappearing; shared-media lifetime corruption; clearly impractical worker/cache scaling; private API/patch/fork; required high-level Tracktion scheduler; Domain/runtime leakage; or AudioNLE needing a general graph scheduler/dependency mutator/plugin lifecycle/PDC updater.

## M0 — architecture and ownership investigation

1. Inspect pinned Tracktion/JUCE source and `realtime_source_main.cpp`; record public API and source/line evidence for `Node`, `SimpleNodePlayer`, graph ownership, reader lifetime/concurrency, and any independently reusable low-level cache primitive.
2. Evaluate Model A (one worker/cache per MediaSource, many Clip views), Model B (one worker/cache per active Clip), and Model C (bounded shared-service arbitration) against worker/reader/cache ownership, duplicate/overlapping reads, seek independence, request bound, invalidation, callback work, lifecycle, custom responsibility/LOC, Tracktion coupling, and future FFmpeg extension.
3. Select the smallest viable fixture model from evidence. Do not select Model C merely because it resembles production scaling.
4. Write `multisource-api-investigation.md` before M1.

Pass: public APIs suffice and a bounded ownership model is explicit. Hard stop: high-level scheduler/private API is necessary or callback-safe reader/cache ownership cannot be defined.

## M1 — same-file multiple Clip views

1. Create one marker WAV and a framework-free Domain fixture with at least `A: Source X -> Timeline T1`, `B: Source Y -> Timeline T2`, and `C: Source X -> Timeline T3`.
2. Render disjoint Source ranges, overlapping ranges, and identical Source ranges at distinct Timeline placements through separate Clip runtime views.
3. For each observation record Clip ID, MediaSource ID, Source/Timeline ranges, selected cache/runtime identity, requested/observed marker and amplitude, timing error, hit/miss/underrun, and callback counters.

Pass: all markers and amplitudes are exact at zero timing error, with no cross-Clip stale PCM; identical-source/different-placement is the mandatory hard identity case.

## M2 — different-file multiple sources

1. Add at least three distinct marker WAV media identities and overlapping Timeline Clip views.
2. Feed their Nodes into public `SummingNode`; calculate expected linear sums in integer-scheduled fixture positions.
3. Record workers, caches, PCM bytes, observable open readers/files, request/pending counts, underruns, callback counters, timing error, and summed amplitude.

Pass: expected sums and all markers are exact; resource ownership follows the selected bounded formula.

## M3 — dense workload scaling

For 4, 8, 16, and 32 active Clips, independently run `many Clips / one media`, `many Clips / several media`, and `many Clips / all distinct media`.

At each point record active Clips, unique media, graph Node count, workers, caches, cache PCM bytes, configured request slots, maximum pending work, coalesced/overwritten/dropped requests, underruns, callback counters, maximum timing error, stale-output count, and working set. This measures responsibility/scaling, not throughput or dropout.

The results must state formulas. For example, Model A:

```text
workers = unique active MediaSource runtimes
cache PCM bytes = unique media runtimes * pagesPerMedia * framesPerPage * channels * sizeof(float)
```

Model B must state equivalent Clip-count scaling. Model C must define fixed worker, page-pool, and request-slot bounds without hiding them in dynamic containers.

## M4 — stopped-state Move rebuild control

Before partial mutation, establish a control:

```text
initial Domain -> build runtime -> observe at T1
stop/control boundary -> Domain Move T1 to T2 -> destroy/rebuild affected or full runtime -> observe
```

Record old/new ranges, source-range invariance, requested/observed markers, error, stale output, cache/worker teardown/recreation, and callback counters.

Pass: T1 is silent, T2 is exact, Source range is unchanged, error is zero, and Domain remains authoritative.

## M5 — stopped-state Trim and re-expand control

Using Domain mutation plus stopped-state rebuild, run left trim, right trim, and re-expand. Record original and edited integer Timeline/Source ranges, expected/observed first and last included samples, outside-range silence, errors, and reconstruction inputs. Do not use float seconds.

Pass: boundaries and retained mapping follow the editing model; excluded samples are silent without source-media mutation.

## M6 — stopped-state Split control

Split a Clip at an internal integer Timeline sample, rebuild, and render separate Clip runtime views. Record original and left/right identities/ranges/markers.

Pass: both views retain the same MediaSource; Timeline and Source unions equal the original; split boundary is exact; no sample duplicates or drops. Do not implement SynchronizationMember behavior beyond the fixture's need.

## M7 — Delete and shared-media lifetime

Use at least two Clip views of one MediaSource. Delete one while the other remains active, with deleted-view work pending where controllable; then delete the last consumer.

Record Clip revisions, requests/pages/completions/discards, survivor output, deleted output, reference/lifetime state, worker/cache teardown, and callback counters.

Pass: deleted output never returns, survivor remains exact, shared runtime survives while needed, and final teardown is bounded.

## M8 — rapid invalidation and partial-update comparison

Run only after M4–M7 pass. With a pending worker operation, execute:

```text
move A -> trim B -> delete C -> move A again
```

Keep and record separate Domain revision, Clip runtime-view revision, and Media reader generation. If a single counter is proposed, explicitly justify its scope/lifetime tradeoff. Record old/new revisions, pending requests, completions, discarded work, page publication, stale output, final marker, and callback counters.

Compare Strategy A (stopped-state full/affected runtime rebuild) with Strategy B (narrow runtime mutation/replacement) for correctness, complexity, public Tracktion API sufficiency, graph ownership, stale-state risk, cache/worker preservation, and callback safety. Full rebuild may remain the prototype recommendation if partial mutation only adds complexity.

## M9 — cache/request arbitration

Apply the selected model's bounded arbitration. Define fixed latest-request slots, fixed queue, or bounded per-media slots; measure configured capacity, maximum pending, duplicate/overlapping range requests, cache reuse versus duplicate reads, coalesced/overwritten obsolete work, drops, starvation, and optional diagnostic request latency. For Model A/C, directly measure identical, overlapping, and disjoint same-file range reuse.

Pass: bounded storage and callback safety remain intact; no starvation or unbounded queue is hidden by the fixture.

## M10 — reconstruction

```text
initial Domain -> build dense runtime -> observe -> edits -> final observation -> destroy
same final Domain -> fresh runtime -> final observation
```

Compare active Clip set, Timeline/Source ranges, final output, markers/errors, requests, deleted-output absence, and worker/cache ownership. Pass only if runtime history has no authority over the final Domain-derived observation.

## M11 — complexity, reuse, and final decision

Write `multisource-runtime-edit-complexity.md` with architecture-relevant LOC separated into MediaSource runtime, Clip runtime view, cache/arbitration, invalidation, lifecycle, graph integration, and test/instrumentation. Report worker/cache/file-handle formulas, request bound, synchronization primitives, Domain/runtime boundary, public JUCE and Tracktion reuse, and FFmpeg implications: shared media/stream selection, multiple views per decoder, seek/preroll, source numbering, arbitration, cancellation, and cache sharing.

Write the results document with scope/pins, selected model, all M1–M10 evidence, callback invariants, rebuild/partial comparison, classifications, remaining limitations, overall Option A2 implication, and next gate. Make no production decision or ADR.

## Verification and completion

For later implementation, use `build.ps1`, then the scoped CTest/direct executable required by the added fixture; run `git diff --check` and review no forbidden source-scheduling dependency or Domain leak entered the prototype. This planning-only change requires no build or test.

Before closing this plan, verify: only intended plan/result/evidence files changed; M0–M11 executed in order; Model A/B/C comparison is documented; same/different media and dense dimensions ran; stopped rebuild preceded partial update; Move/Trim/re-expand/Split/Delete/invalidation/reconstruction ran; bounded arbitration and formulas are explicit; and Tracktion high-level source scheduling remains excluded.
