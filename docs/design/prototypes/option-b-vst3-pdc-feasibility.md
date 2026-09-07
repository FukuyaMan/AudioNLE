# Option B Actual VST3 Hosting / Latency / PDC Feasibility

## Status and question

This is a planning-only bounded feasibility gate. It does not select a production architecture or create an ADR. It asks:

> Can Option B host an actual VST3 through public JUCE APIs, obtain plugin-reported latency only after valid preparation, propagate it through the AudioNLE graph/PDC layer, and retain exact alignment without Tracktion?

The retained boundary is framework-free Domain/runtime description -> AudioNLE integer scheduling -> AudioNLE low-level graph -> JUCE-hosted VST3 processor Node -> AudioNLE latency/PDC -> headless output. Tracktion is prohibited.

Included: repository-local deterministic VST3 fixture, public JUCE hosting, headless lifecycle, fixed rate/block, actual delay/latency verification, graph PDC, optional deterministic bypass, reconstruction, and complexity comparison. Excluded: arbitrary scan/database, UI, state persistence, crash isolation/sandboxing, live dynamic latency, Tail, FFmpeg/media, realtime cache/device, GUI, live mutation, production plugin manager, ADR, and final selection.

## Invariants and lifecycle

Domain contains processor kind, fixture identity, and enabled state only; no plugin pointer, JUCE type, graph runtime identity, or cached latency is authoritative. Timeline positions remain integer samples. No source shift, negative preroll, manual compensation override, fixture-specific correction, private API, patch, or fork is allowed.

Latency is invalid until the following ordering completes:

```text
create instance -> configure sample rate/block/layout -> prepareToPlay
-> query reported latency -> prepare graph path metadata/PDC -> render
```

Use two-stage runtime preparation if graph construction must precede processor preparation: Stage 1 instantiates/configures/prepares processors and extracts latency; Stage 2 computes graph paths, compensation, and fixed delay state. Do not query latency before preparation then repair it later.

## V0 — public API and lifecycle investigation

Inspect pinned public JUCE hosting APIs and existing repository-local VST3 fixtures. Determine format manager/VST3 format use, headless instance creation, sample-rate/block and bus/layout setup, `prepareToPlay`, process-block invocation, destruction, latency query timing, and public latency-change notification. Write `benchmark-results/option-b/vst3-api-investigation.md` before implementation.

Record which responsibilities A2 obtains through the Tracktion/JUCE path and which Option B must now own: fixture discovery/load, instance lifetime, configuration, buffer/channel adaptation, lifecycle timing, latency extraction, graph reprepare boundary, and destruction. Tracktion wrappers/classes must not be copied or linked.

## Deterministic repository-local fixture

Build a local, build-only, headless mono fixture VST3 (or document the smallest required mono-to-stereo adaptation). It must delay audio by a declared fixed 256, 1024, or 2048 samples; report exactly that latency through the public host path; apply no gain; use deterministic output; and perform no process-time allocation. Do not vendor a binary.

Before graph PDC, independently prove:

```text
fixture declared latency = host-observed post-prepare latency = actual signal delay
```

Any mismatch is a hosting/fixture hard stop, not a graph-PDC issue.

## V1 — single hosted path

Render generated event 1024, amplitude 0.25 through `Source -> hosted VST3 latency(1024) -> output`. Require reported latency 1024, observed event 2048, actual delay 1024, amplitude 0.25, and timing error zero.

## V2 — parallel graph PDC

Use `Source -> direct` in path A and `Source -> hosted VST3(1024)` in path B, then an AudioNLE Summing Node. Stage-2 graph preparation must derive A edge compensation 1024 and B edge compensation 0. Both contributions must sum at 2048 with zero error. No manual correction may enter the test or runtime.

## V3 — multiple latency values

Exercise fixture variants 256, 1024, and 2048 in parallel paths. Require graph-derived compensation and common event `1024 + 2048`. Record declared, host-observed and actual latency, path latency, compensation, expected/observed sample/amplitude, and timing error in `benchmark-results/option-b/vst3-pdc.md`.

## V4 — mixed custom and hosted latency

Verify accumulation across `deterministic Latency(256) -> hosted VST3(768)` and `hosted VST3(1024)`. Both paths must report/derive total latency 1024 and align without special handling.

## V5 — processing order

Compare `VST3 latency -> Multiply(2)` with `Multiply(2) -> VST3 latency`. Require equal timing and expected final amplitude, proving hosted processing order is separate from latency/PDC bookkeeping.

## V6 — optional bypass fixture

Only if the public host lifecycle permits an unambiguous deterministic policy, test enabled fixture latency versus prototype-disabled/bypassed fixture policy with graph latency zero. Do not claim or define production bypass semantics, and do not fake compensation.

## V7 — reconstruction and dynamic-latency boundary

From descriptions containing processor kind, fixture identity, and enabled state:

```text
description -> host instance -> configure/prepare -> query latency -> graph prepare/PDC
-> render A -> destroy
same description -> fresh instance/runtime -> render B
```

Compare output, marker positions, amplitudes, extracted/path latency, compensation metadata, and processing order. Domain must remain unchanged. Document public JUCE latency-change notification availability and the conceptual future response (`latency changed -> invalidate graph metadata -> stopped/control-boundary reprepare`); do not implement live changes.

## Process-time and ownership rules

The Option B wrapper must use fixed prepared buffers and no process-time growth. Report wrapper behavior separately from unmeasured JUCE/plugin-internal allocation. The VST3 Node owns its instance, prepared buffers, process invocation, post-prepare latency query, fixture enable policy, and destruction. General scanning, channel-layout support, plugin lifecycle manager, and crash policy are excluded.

## Complexity, A2 comparison, and result

Write `benchmark-results/option-b/vst3-complexity.md` with separate non-additive estimates for fixture discovery/load, format setup, instance ownership, configure/prepare lifecycle, channel/buffer adaptation, Node wrapper, latency extraction, two-stage preparation, graph integration, reconstruction, and destruction; separate fixture/assertion code.

Compare measured ownership only:

| Responsibility | A2 | Option B |
| --- | --- | --- |
| instance creation / prepare lifecycle | Tracktion/JUCE path | measured fixture result |
| latency extraction | Tracktion node/runtime | measured fixture result |
| graph latency propagation / PDC | Tracktion | AudioNLE measured result |
| process invocation / destruction | Tracktion plugin graph/runtime | measured fixture result |
| source timing | AudioNLE | AudioNLE |

Publish `docs/design/prototypes/option-b-vst3-pdc-results.md` with gate and hosting-subsystem classifications, scope/dependencies/exclusion proof, fixture/lifecycle/latency evidence, V1–V7, timing/allocation, LOC, A2 comparison, untested plugin responsibilities, architecture implication, and exactly one next step.

## Hard stops and classifications

Stop on fixture declared/host/actual latency mismatch; nonzero timing error; lifecycle-invalid latency use; manual correction; Domain runtime leakage; unsafe/process-time wrapper growth; Tracktion dependency; or scope growth into excluded systems. Classify gate as `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`; classify hosting work as `Small hosting extension`, `Substantial hosting subsystem`, or `Custom-engine-scale warning`.

Even on success, retain untested arbitrary plugins, scanning/database, state, crash isolation, GUI/editor, dynamic playback latency, bus-layout diversity, device callback, and production scheduling. The final question is whether actual hosting remains a bounded extension of Option B's small graph core or makes A2's Tracktion reuse materially more attractive.
