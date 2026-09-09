# Execution Plan: Tracktion Selective Reuse Prototype (Option A2)

> Historical plan. ADR 0001 selected Option B and rejected Tracktion as the production backend. The executable prototype was removed from current `main`; this plan and its evidence are retained, and the implementation is recoverable from Git history.

## 1. Objective

Determine whether AudioNLE can retain authoritative integer-sample source scheduling while selectively reusing public low-level Tracktion graph, processing, plugin, renderer, and thread infrastructure. This is a feasibility decision between `Proceed`, `Proceed with Constraints`, `Reject`, and `Inconclusive`; it does not adopt Tracktion or define production architecture.

## 2. Inputs and evidence

This plan is governed by [selective reuse feasibility](../../design/prototypes/tracktion-selective-reuse-feasibility.md), [Tracktion feasibility results](../../design/prototypes/tracktion-feasibility-results.md), and Phase E raw records. Existing evidence rejects high-level `WaveAudioClip` / `WaveNodeRealTime` scheduling as an authoritative sample scheduler: its no-plugin impulse offset changes by Timeline event, render start, source rate, and quality. It does not reject low-level graph reuse without a dedicated test.

Product invariants remain authoritative: non-destructive media, integer Timeline samples, distinct Source and Timeline domains, Domain -> Adapter -> Runtime direction, no Tracktion persistence, and AudioNLE-owned Synchronization Group/Member, Clip Group, and Ripple semantics.

## 3. Scope

In scope:

- Phase 0 public low-level API investigation.
- A2-1, a generated 48 kHz mono impulse driven by an AudioNLE-controlled source node/reader.
- Exact event-position and render-start matrix in headless offline render.
- Domain/runtime reconstruction observation after A2-1 only if it passes.
- A2-2 ordered deterministic processing compatibility after A2-1 only if it passes.
- A2-3 44.1/48 kHz conditional fixture after A2-1 only if it passes.
- Complexity, public-API, leakage, and dependency-pin review.

## 4. Non-goals

No Phase F Tail, original Phase E PDC retry, VST3, scanner, GUI, persistence, waveform, FFmpeg, runtime editing, long-source/dense-Clip benchmark, production SRC choice, production architecture, ADR, Tracktion patch, or fork.

## 5. Assumptions and dependency policy

Use only the existing pinned sources:

| Component | Revision |
| --- | --- |
| Tracktion Engine | `b88a6ee51913668cb53e911e030ab736b13342cf` |
| nested JUCE | `37c894f83d379179b2070d437ccd0f1cd9af9576` |

No floating revision, dependency update, binary artifact, submodule change, or VST3 dependency is permitted. Any necessary revision change stops this plan pending a documented decision.

## 6. Architecture boundary

```text
AudioNLE framework-free Domain samples
  -> AudioNLE sample scheduler and source mapping
  -> AudioNLE-controlled source node/reader
  -> Tracktion public low-level graph/processing primitives
  -> transient renderer/output observation
```

Forbidden as authoritative state or scheduling: `Edit` timeline semantics, `WaveAudioClip`, `WaveNodeRealTime`, Tracktion Clip placement, runtime graph persistence, Tracktion serialization, plugin pointers/state trees, and reverse synchronization. A transient `Edit` is permitted only if source investigation proves it is required as a low-level runtime container and it remains adapter-owned.

## 7. Prototype isolation and file plan

Keep Option A2 separate from the high-level feasibility prototype:

```text
prototype/tracktion-selective-reuse/
  CMakeLists.txt
  src/api_investigation_main.cpp
  src/a2_1_main.cpp
  src/a2_2_main.cpp              # only after A2-1 Pass
  src/a2_3_main.cpp              # conditional
benchmark-results/tracktion-selective-reuse/
  api-investigation.md
  a2-1.md
  a2-2.md
  a2-3.md
  complexity.md
docs/design/prototypes/tracktion-selective-reuse-results.md
```

Root build integration must preserve the existing `build.ps1` entry point. No code, CMake target, or dependency is added under production directories. Existing high-level prototype results are retained unchanged.

## 8. Phase 0: public API investigation

Read the pinned source and record exact public declarations, call paths, ownership, and test/example evidence for:

- `tracktion_graph` `Node`, ProcessContext, requested sample ranges, PlayHead, graph ownership, and latency propagation.
- Public custom-node/source-node construction and renderer-compatible graph inputs.
- Plugin/processor-node connection after a custom source.
- Headless/offline renderer lifecycle and thread/scheduler requirements.

Pass: a public/supported path can create an AudioNLE-controlled source node/reader and connect it to a renderer-compatible low-level graph without high-level Clip scheduling.

Hard stop: private/internal API, undocumented graph construction, mandatory `WaveAudioClip` path, or Tracktion patch/fork. Record `Reject` or `Inconclusive` and do not create A2-1 if source evidence cannot distinguish them.

## 9. Phase A2-1: zero-error custom-source baseline

Implement a framework-free fixture with Project rate 48 kHz, generated mono impulse, Domain event sample 1024, and render range `[0,4096)`. The custom source receives the requested graph Timeline sample range, maps it from integer Domain state, and writes the impulse directly; it must not instantiate the high-level source path.

Run event positions `1`, `100`, `1024`, `2048`, and `3000`; for event 1024 run `[0,4096)`, `[512,4608)`, and `[1000,5096)`. Record requested Timeline range, source range, first/last receiver samples, absolute and render-relative observation, elapsed time, and alignment error.

Pass criteria:

1. Every required event equals the requested Domain event: `alignment error = 0 Timeline samples`.
2. Render starts do not change absolute alignment.
3. The source path remains low-level/custom; no `WaveAudioClip`, `WaveNodeRealTime`, source-resampler path, magic offset, Domain shift, or output-buffer shift is used.
4. Headless/offline execution uses no physical device and completes under the bounded guard.
5. Domain State remains framework-free and unchanged after runtime destruction.

Hard stop: any non-zero sample error, high-level source-path necessity, private API, patch/fork, reverse synchronization, or coordinate workaround. Do not run A2-2, A2-3, PDC, VST3, or Phase F.

## 10. A2-1 reconstruction

Only after A2-1 Pass, execute Domain -> runtime -> render -> destroy -> same Domain -> rebuild -> render. Require all event observations and output samples to remain equal at zero sample error. Runtime object graph remains disposable and never becomes persistence.

## 11. Phase A2-2: processing compatibility

Only after A2-1 and reconstruction Pass, attach the Phase D-style deterministic Add then Multiply processors after the custom-source boundary through public low-level Tracktion processing infrastructure. Use input `0.25`, Add `0.25`, Multiply `2.0`, expected `1.0`; also confirm reversed order differs as expected. Record ordering, processor ownership/lifecycle, graph latency exposure, source-path exclusion, and output.

Pass: Domain order maps one-way to low-level runtime processing and expected output is observed without high-level Clip scheduling. This is not PDC validation.

Stop: processing reuse requires a high-level Clip path, private API, reverse sync, or custom-engine-scale graph ownership.

## 12. Phase A2-3: conditional mixed-rate fixture

Only after A2-1 Pass, use 44.1 kHz source and 48 kHz Project. Define a small explicit AudioNLE-controlled rational/sample conversion fixture and measure its stated expected output. Do not select a production resampler or quality. If this work is not needed to decide A2 viability, record it as deferred rather than inferring a result.

## 13. Test matrix

| ID | Phase | Fixture | Pass criterion | Stop impact |
| --- | --- | --- | --- | --- |
| A2-T001 | 0 | public API/source evidence | supported custom-source graph path | Reject/Inconclusive before code |
| A2-T002 | A2-1 | impulse positions | every error 0 samples | hard stop |
| A2-T003 | A2-1 | render-start matrix | absolute event unchanged | hard stop |
| A2-T004 | A2-1 | reconstruction | equal zero-error outputs | hard stop |
| A2-T005 | A2-2 | Add/Multiply order | expected ordered signal | stop processing reuse |
| A2-T006 | A2-3 | 44.1/48 mapping | explicit conditional criterion | conditional/defer |
| A2-T007 | final | complexity review | reuse benefit exceeds glue burden | Reject/Inconclusive |

## 14. Measurement and recording

Use `build.ps1` for configure/build/test. Each raw record includes environment, exact pins, test ID, source/project rate, API/source references, requested and observed positions, error, render boundaries, receiver count, device status, runtime lifecycle, status, workaround, and stop impact. Record raw evidence in `benchmark-results/tracktion-selective-reuse/` and synthesize it in `tracktion-selective-reuse-results.md`.

## 15. Complexity evaluation

Measure custom files/components and approximate LOC; public versus private API count; required Tracktion concepts; patch/fork count; source scheduling, source read, seek/reset, graph ownership, latency, render-boundary, and thread responsibilities. Contrast those with actual reused graph, processing, plugin, renderer, and scheduler infrastructure. Reject if the adapter owns graph scheduling, plugin graph behavior, latency propagation, or lifecycle to a degree equivalent to Option B/custom engine work.

## 16. Stop conditions

Early stop and final result recording are mandatory for public-API insufficiency, high-level path necessity, non-zero error, reverse synchronization, Tracktion patch/fork, or clear custom-engine-scale complexity. A2-1 failure prevents all later phases. PDC, Tail, VST3, and Phase F are never fallback workarounds.

## 17. Result and decision handoff

Create `docs/design/prototypes/tracktion-selective-reuse-results.md` on completion or hard stop. It must compare high-level Option A rejection, Option A2 evidence, and Option B implication; state Domain-boundary/leakage review, reused components, custom responsibilities, complexity, PDC retry readiness, and exactly one conclusion: `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`.

No ADR is created. Only a later review of a `Proceed` or `Proceed with Constraints` result may consider an architecture decision.

## 18. Completion criteria

This plan is complete when Phase 0 and A2-1 have a recorded result; later phases are either passed, explicitly conditionally deferred, or correctly skipped after a stop; pins and raw measurements exist; no production leakage occurred; the results document contains a decision; and no unauthorized PDC/Tail/VST3/Phase F work was performed.
