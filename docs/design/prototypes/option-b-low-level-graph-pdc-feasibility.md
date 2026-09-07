# Option B Minimal Low-level Graph / PDC Feasibility

## Status, purpose, and boundary

This is a planning-only feasibility gate. It authorises neither implementation nor an ADR or production architecture selection. It answers one narrow question:

> When AudioNLE replaces Tracktion low-level graph traversal, summing, and PDC with a deliberately small graph layer, what engine-critical responsibility and complexity appears?

The proposed boundary is:

```text
AudioNLE Domain
  -> integer Timeline / Source scheduling
  -> minimal AudioNLE graph description and runtime
  -> custom source Nodes
  -> mix / deterministic processors / PDC
  -> headless caller-owned output
```

Only generated deterministic sources are permitted. Tracktion graph classes must not be used. Public JUCE primitives may be used only for buffers or audio data representations when they reduce incidental code without supplying graph/PDC authority.

Excluded: VST3, FFmpeg, media decoding, realtime pages/cache, device I/O, GUI, Tail, persistence, live mutation, thread pools, waveform, export, full editor, ADR, and final architecture selection.

## Retained product invariants

Domain state remains framework-free and non-destructive. Timeline and Source positions are signed integer samples; source identity and Timeline placement remain separate. Domain contains no Node, graph, buffer, runtime pointer, latency cache, or JUCE object. The prototype must not change source Timeline coordinates, use negative preroll, or install fixture-specific output shifts to obtain PDC.

## B0 — design/API investigation

Before implementation, inspect the A2 low-level graph/PDC fixtures and their complexity evidence; relevant public JUCE buffer/processor APIs; standard C++ ownership facilities; and existing deterministic fixture conventions. Record the result in `benchmark-results/option-b/low-level-graph-api-investigation.md`.

The investigation must identify the responsibilities Tracktion currently supplies automatically: Node dependency ownership, preparation/topological order, buffer propagation, summing, latency propagation/PDC, and headless execution. It must not copy Tracktion internals or use private implementation knowledge as a dependency.

## Minimal graph model

The smallest admissible prototype supplies only:

1. a Node interface accepting an absolute integer process range and caller-provided output buffer;
2. explicit ownership of input dependencies;
3. preparation-time topological ordering and graph latency analysis;
4. fixed/caller-owned buffer flow for the fixture block size;
5. source, deterministic processor, summing, and latency Nodes;
6. precomputed per-input compensation consumed at runtime;
7. headless graph execution; and
8. reconstruction from a framework-free runtime description of sources, processors, edges, and declared latencies.

No generic engine abstraction, dynamic graph mutation, plugin lifecycle, media layer, or device scheduler may be added merely for future use.

## B1 — source to output

Use one sparse generated source: event sample 1024, amplitude 0.25. Render `Source -> headless output`. Pass only when sample 1024 has amplitude 0.25 and timing error is exactly zero Timeline samples.

## B2 — ordered processing

Implement deterministic `Add(0.25)` and `Multiply(2)` Nodes. Verify order:

```text
0.25 -> Add(0.25) -> Multiply(2) = 1.0
0.25 -> Multiply(2) -> Add(0.25) = 0.75
```

Pass requires explicit dependency order, not accidental construction or evaluation order.

## B3 — two-source summing

Render source A=0.25 and source B=0.50 at the same integer Timeline sample through a Summing Node. Require 0.75, then optionally verify downstream Multiply produces 1.50 without hard clipping. Summing must derive from explicit inputs and process order.

## B4 — deterministic latency primitive

Define a latency Node that delays audio by its declared latency and reports that integer latency to graph preparation. Exercise 256, 1024, and 2048 samples. It must contain no hidden correction.

## B5 — PDC

PDC belongs to **preparation-time graph analysis**: calculate each input path latency, determine the maximum latency at each summing point, and attach precomputed compensation to the shorter input paths. Runtime Nodes consume that metadata; dynamic latency changes are excluded.

Test parallel paths with latency 0 and 1024 for event 1024, requiring alignment at 2048. Then test path latencies 256, 1024, and 2048, requiring each event at `event + max(path latency)`. The graph must derive all compensation from declared path latency. Manually changing source placement, using preroll, or hard-coding test-case delay is a hard failure.

## B6 — reconstruction

Build from a small framework-free description (`sources`, `processors`, `edges`, `latencies`), observe, destroy all runtime state, rebuild, and compare output. Pass requires equal observations, zero timing error, and unchanged Domain/runtime description values. No runtime Node identity may become authoritative.

## B7 — complexity and comparison

Write these later evidence files:

```text
benchmark-results/option-b/
  low-level-graph-api-investigation.md
  low-level-graph.md
  low-level-graph-pdc.md
  low-level-graph-complexity.md
```

The complexity record must separately estimate architecture-relevant LOC for Node interface, graph ownership/topology, traversal/order, buffer management, summing, processor wrapper, latency analysis, compensation/PDC, headless runner, and reconstruction. Fixture generation, assertions, and instrumentation must be separate; prototype LOC must not be projected as production LOC.

Compare measured ownership only:

| Responsibility | A2 | Option B prototype |
| --- | --- | --- |
| Graph Node abstraction | Tracktion | AudioNLE, to measure |
| Traversal/order | Tracktion | AudioNLE, to measure |
| Buffer flow | Tracktion | AudioNLE, to measure |
| Summing | Tracktion | AudioNLE, to measure |
| Deterministic processing | Tracktion graph | AudioNLE, to measure |
| Latency propagation / PDC | Tracktion | AudioNLE, to measure |
| Headless execution | Tracktion | AudioNLE, to measure |
| Source scheduling | AudioNLE | AudioNLE |

VST3/Tail, realtime source cache, FFmpeg, device callback, and live graph mutation remain outside this comparison. A favourable result does not by itself beat A2.

## Hard stops and classifications

Stop if exact integer timing fails; graph order is ambiguous; summing depends on hidden/manual ordering; PDC needs fixture-specific correction; Domain gains runtime identity; buffer ownership becomes unsafe/unbounded; scope expands into plugin/device/media subsystems; or complexity is already custom-engine-scale.

Classify the gate as `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`; classify the subsystem as `Small bounded graph core`, `Substantial graph subsystem`, or `Custom-engine-scale warning`.

The results document, `docs/design/prototypes/option-b-low-level-graph-pdc-results.md`, must cover recommendation, scope, graph model/ownership, B1–B6, timing, complexity, measured A2 comparison, untested responsibilities, architecture implication, and exactly one next step.

## Verification and decision relevance

Later implementation must use the repository build entry, a targeted CTest and direct executable, `git diff --check`, and review that Tracktion graph types never enter the target. The critical final question is whether this slice is a small understandable graph core worth broadening, or whether it already demonstrates that Tracktion saves substantial engine-critical work.
