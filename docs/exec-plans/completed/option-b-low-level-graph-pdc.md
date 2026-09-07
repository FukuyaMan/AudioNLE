# Execution Plan: Option B Minimal Low-level Graph / PDC

## Completion

B0–B7 completed on 2026-09-07. The isolated standard-C++ target passed B1–B6 at 0 sample error with graph-derived PDC and no process-time growth. Gate: Proceed with Constraints; subsystem: Small bounded graph core. This plan is complete.

## Objective and decision boundary

Implement one isolated, generated-source-only feasibility fixture answering whether replacing Tracktion low-level graph traversal, summing, and PDC produces a small understandable AudioNLE graph core or an already substantial engine subsystem. It must classify the gate as `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`, and complexity as `Small bounded graph core`, `Substantial graph subsystem`, or `Custom-engine-scale warning`.

This is not Option B production validation, an ADR, or an architecture selection. It excludes Tracktion graph, VST3, FFmpeg, actual media, realtime cache, device, GUI, Tail, live mutation, persistence, worker pools, and production scheduling.

## Scope and retained invariants

The prototype belongs in a standalone directory such as `prototype/option-b-low-level-graph/`; it must not share a target with Tracktion selective-reuse. It uses generated deterministic sources, integer absolute sample ranges, fixed 128-frame fixture blocks, headless caller-owned output, deterministic Add/Multiply, N-input summing, latency, prepare-time PDC, reconstruction, and complexity accounting.

The framework-free Domain/runtime descriptions retain source IDs, processor IDs/kind/parameter/enabled state, edges, and declared latency only. They contain no Node, buffer, pointer, graph, cache, or JUCE runtime identity. All Timeline/Source positions remain signed integer samples. No source-coordinate shift, negative preroll, test-specific output shift, or Domain latency correction may be used.

## Fixed minimal design decisions

### Node and graph ownership

Use a Node interface conceptually equivalent to `process(absoluteSampleRange, outputBuffer)`, explicit input dependencies, and declared `latencySamples()`. A runtime graph owns Nodes and edge records with `unique_ptr`/value ownership; the root/output is explicit. The fixture accepts DAGs only. Preparation must reject missing inputs, duplicate IDs, invalid root, and cycles with clear failure; cycle support is not added.

### Preparation and buffers

At prepare time: validate the DAG, create topological order, calculate input/output path latency, calculate summing-edge compensation, then allocate fixed runtime state. Use **per-node fixed scratch buffers**, allocated/prepared once for the 128-frame block size. Input buffers remain owned by their producing Node for the duration of a process call; summing reads inputs and writes only its own buffer, so overwrite/aliasing is prohibited. Process performs no unbounded allocation or growth.

### PDC representation

Choose edge metadata containing precomputed compensation samples rather than dynamically inserting general compensation Nodes. At each summing point, calculate `maxInputLatency`; each input edge gets `maxInputLatency - inputPathLatency`. A fixed delay state owned by that edge applies the metadata at runtime. This is smaller for the bounded fixture and keeps graph analysis distinct from audio processing. Dynamic latency changes are stopped/rebuild-only and excluded.

## B0 — design/API investigation

Inspect A2 graph/PDC fixtures and complexity records, public JUCE buffer/data APIs only where useful, standard C++ ownership/container primitives, and repository deterministic-fixture conventions. Tracktion may be read as comparison evidence but cannot become an implementation contract.

Write `benchmark-results/option-b/low-level-graph-api-investigation.md` before B1 with:

```text
Responsibility | A2 provider | Option B candidate | public dependency | ownership | allocation model | runtime cost | reason
```

Record Tracktion responsibilities replaced here: Node abstraction, dependency ownership/order, buffer flow, summing, latency propagation/PDC, and headless execution. Pass only when the minimal interfaces and allocation/ownership boundaries above are explicit and Tracktion is unnecessary.

## B1 — source to output

Create a sparse generated source: absolute event 1024, amplitude 0.25. Render at least `[0, …)`, `[512, …)`, and `[1000, …)` ranges through `Source -> headless output`. Pass when every observation retains absolute sample 1024, amplitude 0.25, and timing error exactly zero. This explicitly guards against render-start/reset dependence.

## B2 — ordered processing

Build graph descriptions, not construction-order chains, for:

```text
Source -> Add(0.25) -> Multiply(2) = 1.0
Source -> Multiply(2) -> Add(0.25) = 0.75
```

Pass only when explicit dependencies/topology determine both values. Record processor descriptions as framework-free values (`ID`, `kind`, `parameter`, `enabled`) and prove no runtime pointer enters them.

## B3 — summing

Render Source A=0.25 and Source B=0.50 at one Timeline sample through a generic N-input Summing Node; require 0.75. Then require `Sum -> Multiply(2) = 1.50` in internal float output without clipping. Add the three-input control `0.25 + 0.25 + 0.25 = 0.75` to prevent a two-input special case. Pass requires explicit inputs and no manual mix ordering.

## B4 — deterministic latency primitive

Implement a latency Node whose declared and actual delay match. Exercise 256, 1024, and 2048 samples; require event 1024 through latency 1024 at 2048 with timing error zero. Delay storage is fixed at prepare time; process-time growth is forbidden. Record declared versus observed delay for every value.

## B5 — prepare-time graph PDC

For every Node calculate input path latency, own latency, and output path latency during prepare. At summing Nodes derive compensation from graph metadata, never fixture constants.

Run in hard-stop order:

1. Two paths: event 1024, path latency 0 and 1024; require both contributions summed at 2048.
2. Three paths: latencies 256, 1024, and 2048; require all at `1024 + 2048` with zero error.
3. Layered path: `Latency(256) -> processor -> Latency(768)`; prove cumulative latency is 1024.
4. `Latency(1024) -> Multiply(2)` versus `Multiply(2) -> Latency(1024)`; require equal timing and expected amplitude, proving PDC and processor order are distinct.

Hard stop on fixture-specific correction, source shift, preroll, actual/reported mismatch, ambiguous topology, or nonzero timing error.

## B6 — reconstruction

From one framework-free description containing Source, Processor, Edge, and Latency values:

```text
description -> runtime -> output A -> destroy
same description -> fresh runtime -> output B
```

Compare complete rendered output, event positions, path latencies, compensation metadata, and processing values. Pass only if outputs are equal, Domain/description values never mutate, and runtime identities are absent from the description.

## B7 — complexity, comparison, and result

Write:

```text
benchmark-results/option-b/
  low-level-graph.md
  low-level-graph-pdc.md
  low-level-graph-complexity.md

docs/design/prototypes/option-b-low-level-graph-pdc-results.md
```

Separate architecture-relevant LOC estimates for Node interface, descriptions, graph/edge ownership, validation, topological sort, traversal, fixed buffers, source, processor, summing, latency, path analysis, PDC, headless runner, and reconstruction. Separately report fixture generation, assertions, instrumentation, and CLI/output; explain overlapping estimates and do not extrapolate production LOC.

Fill measured comparison:

| Area | A2 | Option B measured |
| --- | --- | --- |
| Node abstraction | Tracktion | fixture result |
| graph ownership / traversal / buffers | Tracktion | fixture result |
| summing / processing | Tracktion | fixture result |
| latency propagation / PDC | Tracktion | fixture result |
| headless execution | Tracktion | fixture result |
| source authority | AudioNLE | AudioNLE |

State explicitly that VST3, plugin latency lifecycle, Tail, realtime cache, FFmpeg, devices, and live mutation remain untested. Choose exactly one next step: broaden Option B with VST3/PDC hosting, prepare A2-vs-B ADR-level comparison, stop Option B, or another named bounded gate.

## Hard-stop protocol

Stop and record the current result; do not advance to unrelated phases on: nonzero timing error; ambiguous/construction-order graph behavior; unsafe or unbounded buffer ownership; manual/hard-coded summing; latency declaration/actual mismatch; fixture-specific PDC; Domain runtime leakage; Tracktion dependency/type in the target; scope expansion into plugin/media/device/live mutation; or clear custom-engine-scale growth.

## Verification

Later implementation must use `build.ps1`, targeted Option B CTest, direct executable, repeated deterministic runs, and `git diff --check`. Review target dependencies for no Tracktion/transitive Tracktion linkage; review includes; and search Option B source and CMake for `tracktion`, `Tracktion`, and Tracktion graph types. JUCE usage, if any, must be limited to public buffer/data representation and must not own traversal or PDC authority.

## Completion criteria

The result is complete only when B0→B7 ran in order; generated source, render-start control, ordered processing, generic N-input summing, declared/actual latency, graph-derived PDC, reconstruction, and complexity comparison passed or a hard stop was recorded. The final result must classify both the gate and subsystem without selecting a production architecture.
