# Execution Plan: Option B Actual VST3 Hosting / Latency / PDC

## Completion

V0–V9 completed. Local fixture declared/post-prepare host/actual delays agreed at 256/1024/2048; graph-derived PDC and reconstruction passed at 0 sample error. Gate: Proceed with Constraints. Hosting complexity: Small hosting extension. This plan is complete.

## Objective and decision boundary

Run a bounded feasibility fixture answering whether Option B can host a deterministic local VST3 through public JUCE APIs, query valid latency only after preparation, feed it into AudioNLE-owned graph latency/PDC, and retain exact alignment without Tracktion. Classify the gate as `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`; classify hosting complexity as `Small hosting extension`, `Substantial hosting subsystem`, or `Custom-engine-scale warning`.

This neither selects production architecture nor validates arbitrary plugins. Exclude scanner/database, UI, state persistence, crash isolation, Tail, FFmpeg/media, realtime cache, device, GUI, live mutation, plugin manager, ADR, and final selection.

## Boundary and invariants

```text
framework-free descriptions -> integer scheduling -> AudioNLE graph
-> JUCE-hosted VST3 Node -> AudioNLE path analysis/PDC -> headless output
```

Tracktion headers, types, linkage, and copied wrapper logic are forbidden. Domain descriptions contain processor ID, VST3 fixture kind/identity, and enabled state only; no plugin pointer, runtime identity, or latency cache. Source coordinates remain integer and must never be shifted for PDC. No preroll, manual compensation, fixture correction, private API, patch, or fork is allowed.

## V0 — public JUCE API/lifecycle investigation

Inspect pinned public JUCE hosting APIs and repository-local VST3 fixtures. Record plugin format setup, VST3 registration, headless instance creation, rate/block/layout setup, `prepareToPlay`, process invocation, post-prepare latency query, destruction, public latency-change notifications, and public/private status in `benchmark-results/option-b/vst3-api-investigation.md`.

Record responsibility, public API, lifecycle point, observed/prepared state, Option B ownership, A2 equivalent, and risk. Identify any mono/stereo requirement without presuming an A2 cause.

## Fixed two-stage preparation

Stage 1: instantiate fixture, configure sample rate/block/channel layout, call `prepareToPlay`, then query latency. Stage 2: build/prepare AudioNLE graph, propagate Stage-1 path latency, compute edge compensation, and allocate fixed compensation state. Graph metadata must never be built from pre-prepare latency; two-stage lifecycle is later complexity evidence.

## Fixture and V1 validation

Use a build-only repository-local deterministic VST3 fixture, reusing an existing one if public lifecycle and mono requirements fit. It reports and delays exactly 256, 1024, and 2048 samples (768 if useful for V5), applies no gain, needs no GUI, and has no fixture process-time allocation.

Before graph PDC, independently record fixture-declared latency, host latency before prepare (diagnostic only), host latency after prepare, actual delay, expected/observed event, and amplitude. Pass only when declared = post-prepare host = actual delay and timing error is zero. Any mismatch is a hard stop before V2.

## V2 — single hosted path

Render event 1024, amplitude 0.25 through hosted 1024-latency fixture. Require post-prepare report 1024, output event 2048, amplitude 0.25, and zero error.

## V3 — parallel PDC

Build direct source path A and hosted VST3(1024) path B into the existing Option B Summing Node. Stage-2 graph analysis must derive A compensation 1024 and B compensation 0; fixture/test code cannot provide these values. Require both contributions aligned/summed at 2048 without source shift or manual correction.

## V4 — multiple hosted latencies

Parallel hosted paths 256, 1024, and 2048 must derive compensation 1792, 1024, and 0 from graph paths, respectively, and align at 3072. Record declared/host/actual latency, computed path/compensation, expected/observed sample and amplitude, and timing error in `benchmark-results/option-b/vst3-pdc.md`.

## V5 — mixed deterministic and hosted latency

Compare `deterministic Latency(256) -> hosted VST3(768)` with hosted VST3(1024). Both must produce total path latency 1024 and align via ordinary graph analysis. This proves unified custom/hosted accumulation.

## V6 — processing order and layout audit

Compare `VST3 latency -> Multiply(2)` with `Multiply(2) -> VST3 latency`; require same timing and expected amplitude with no gain duplication. Record fixture layout, host input/output channel counts, Option B Node buffer channels, and any adaptation. Include required adaptation responsibility in complexity; do not generalise channel-layout support.

## V7 — optional bypass

Run only if public hosting permits a clean fixture-only policy. Enabled means processed with reported graph latency; disabled means not processed and graph latency zero. Do not define production bypass semantics or fake compensation. Otherwise record `Skipped` with reason.

## V8 — reconstruction and latency-change boundary

```text
description -> instantiate/configure/prepare/query -> graph prepare -> render A -> destroy
same description -> fresh plugin/runtime -> render B
```

Compare full output, event sample, amplitude, extracted/path latency, compensation metadata, and processing order. Description must not mutate. Document only the conceptual future response to public latency change (`invalidate metadata -> stopped/control-boundary reprepare`); do not implement dynamic changes.

The wrapper owns instance, fixed prepared buffers, process invocation, post-prepare query, fixture enable policy, and destruction. Verify wrapper process has no resize, unbounded allocation, or wait; do not claim JUCE/plugin internals allocation-free without evidence.

## V9 — complexity, comparison, and final evidence

Write `benchmark-results/option-b/vst3-complexity.md`, separating non-additive architecture LOC/responsibility for format setup, fixture discovery/load, instance ownership, configure/prepare, layout adaptation, Node wrapper/process, latency extraction, Stage-1/Stage-2 bridge, graph integration, reconstruction, destruction, and latency-change invalidation concept. Separately count fixture source, assertions, instrumentation, CLI. Record instance/buffer counts, prepare allocations, wrapper process allocation, metadata, compensation state, adaptation buffers, and runtime owners.

Write `docs/design/prototypes/option-b-vst3-pdc-results.md`, including scope/dependencies/Tracktion exclusion, fixture/lifecycle evidence, V1–V8, timing/allocation, LOC, A2 comparison, untested responsibilities, architecture implication, and exactly one next step.

| Responsibility | A2 | Option B measured |
| --- | --- | --- |
| plugin creation / configure / prepare | Tracktion/JUCE wrapper/runtime | fixture evidence |
| latency extraction | Tracktion node/runtime | fixture evidence |
| graph propagation / PDC | Tracktion | AudioNLE evidence |
| process invocation / destruction | Tracktion runtime | fixture evidence |
| source timing | AudioNLE | AudioNLE |

## Hard-stop protocol

Stop without unrelated later phases on declared/host/actual mismatch; use of invalid pre-prepare latency; nonzero error; manual PDC; source shift; Domain plugin/runtime leakage; wrapper process growth; Tracktion dependency; private API/patch/fork; excluded-scope expansion; or custom-engine-scale growth.

## Verification and completion

Later implementation must use `build.ps1`, targeted Option B VST3 CTest, direct executable, repeated deterministic runs, `git diff --check`, include/dependency review, and Tracktion reference/linkage search limited to Option B target/source/CMake. Report `Tracktion references = 0` and `Tracktion linkage = 0`.

Move this plan to `docs/exec-plans/completed/` only after V0–V9 and the final results are complete. Even a pass must retain untested arbitrary plugins, scanner/database, state, crash isolation, UI/editor, dynamic latency, bus diversity, device callback, and production scheduling.
