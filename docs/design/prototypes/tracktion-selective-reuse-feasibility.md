# Tracktion Selective Reuse Feasibility (Option A2)

## Status

Planned prototype only. This document does not choose an AudioNLE production architecture, adopt Tracktion Engine, create an ADR, or authorize Phase F.

## Context

The Tracktion feasibility prototype established the following bounded results:

| Area | Result | Meaning |
| --- | --- | --- |
| Phase 1 | Pass | The pinned sources build and run headlessly on the target Windows environment. |
| Phase A | Pass | Framework-free authoritative Domain State can construct and reconstruct transient runtime state. |
| Phase B | Pass | Integer Timeline and Source state map through the tested state boundary. It did not test rendered sample-edge placement. |
| Phase C | Pass | Basic signal flow and Track Mix work for constant-signal fixtures. |
| Phase D | Pass | Ordered deterministic Clip/Track/Master processor stacks work in the stopped/rebuild scope. |
| Phase E | Inconclusive | The high-level `WaveAudioClip` / source-reader path cannot establish a zero-error no-plugin impulse baseline. |

The Phase E source/resampler diagnostic observes a high-level source-path offset that varies with Timeline event position, render-start boundary, source rate, and tested resampling quality. For example, 48 kHz source / 48 kHz Project with event 1024 and render start 0 observes 1026. The result is not safely correctable by a constant adapter offset.

The affected path is:

```text
WaveAudioClip
  -> WaveNodeRealTime
  -> source reader / resampler reader
  -> graph
  -> renderer
```

This is a rejection signal for using Tracktion's high-level Clip/source scheduling as AudioNLE's authoritative sample scheduler. It is not yet evidence that every Tracktion low-level graph or processing primitive is unusable.

Related evidence:

- [Tracktion feasibility results](tracktion-feasibility-results.md)
- [Phase E PDC record](../../../benchmark-results/tracktion-feasibility/phase-e.md)
- [Phase E renderer diagnostic](../../../benchmark-results/tracktion-feasibility/phase-e-renderer-diagnostic.md)
- [Phase E source/resampler diagnostic](../../../benchmark-results/tracktion-feasibility/phase-e-source-latency-diagnostic.md)

## Question

Can AudioNLE retain authoritative integer-sample scheduling and supply its own sample-controlled source node/reader, while selectively reusing public, low-level Tracktion graph and processing infrastructure?

This candidate is called **Selective Tracktion Reuse** or **Option A2**. It must be evaluated independently of the high-level Tracktion Option A path.

## Proposed boundary under test

```text
AudioNLE Domain
  -> AudioNLE sample-based scheduling
  -> AudioNLE-controlled source node / source reader
  -> Tracktion low-level graph / processing infrastructure
  -> Track / Master processing
  -> renderer / output
```

The following remain outside Tracktion's authoritative scheduling role:

- Edit timeline semantics
- `WaveAudioClip` source scheduling
- `WaveNodeRealTime` source/resampler timing
- Tracktion Clip placement as authoritative timing state
- Project persistence and runtime graph persistence
- Synchronization Group/Member, Clip Group, and Ripple semantics

`Edit`, if needed as a transient runtime container, is not AudioNLE Project state and must not become persistence or a reverse-synchronization source.

## Hypotheses

| ID | Hypothesis | Required observation |
| --- | --- | --- |
| S1 | AudioNLE-controlled source scheduling preserves an integer Domain Timeline event exactly. | `Domain event = N` produces Master Output event `N`, error 0 samples. |
| S2 | The high-level source path can be bypassed. | The primary fixture does not instantiate `WaveAudioClip`, `WaveNodeRealTime`, or its source/resampler reader. |
| S3 | Low-level Tracktion processing remains reusable. | An ordered Clip-equivalent / Track / Master processing path can consume the custom source output. |
| S4 | PDC is testable after an exact source baseline exists. | A no-plugin zero-error reference can be established before any PDC test. |
| S5 | Selective reuse has worthwhile complexity. | It uses public/supported APIs, no fork, and materially less custom code/maintenance than a custom engine. |

## Minimum prototype scope

The first experiment is intentionally smaller than a production audio engine:

- 48 kHz Project
- generated mono single-sample impulse
- integer Timeline sample scheduling
- one custom/sample-controlled source node or reader
- one headless offline render path
- no VST3
- no Effect Tail
- no runtime editing
- no long-source test
- no GUI, persistence, waveform, FFmpeg, or production SRC decision

It must not add Tracktion/JUCE types to the Domain model. Source/Timeline conversion must be explicit and owned by the AudioNLE-side test adapter. No magic `-2` sample offset, Domain coordinate change, or output-buffer shift is permitted.

## Source-level investigation required before implementation

At the pinned revisions below, inspect and cite only public/supported APIs and source evidence needed for the experiment:

- Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`
- nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`

Required investigation areas:

1. `tracktion_graph` Node, ProcessContext, PlayHead, node graph construction, plugin node, and latency propagation APIs.
2. Whether a custom Node or source reader can be connected to a renderer-compatible low-level graph through public APIs.
3. Whether requested graph sample ranges can be interpreted directly as AudioNLE integer Timeline samples without a seconds round-trip.
4. Renderer graph-input and headless/offline lifecycle requirements.
5. Existing Tracktion tests, examples, and comments concerning custom nodes, source nodes, latency, and rendering.

Private API use, a Tracktion source patch/fork, or reliance on undocumented internal graph construction is a Reject signal for Option A2.

## Test plan

### A2-1: zero-error same-rate baseline

```text
Project rate: 48000 Hz
Source: generated mono impulse
Requested Timeline impulse: 1024
Render range: [0, 4096)
Expected Master Output impulse: 1024
Expected error: 0 Timeline samples
```

Repeat at Timeline positions `1`, `100`, `1024`, `2048`, and `3000`. Repeat with multiple render starts that include the event. Record requested range, source range, absolute and render-relative output indices, first/last delivered sample, and every alignment error.

Pass requires error 0 for every stated position and render-start case. One sample of error is not a pass.

### A2-2: processing-graph compatibility

Only after A2-1 passes, feed the source through a minimal ordered processor path. Reuse only public low-level Tracktion processing primitives that can be placed after the custom source. Verify deterministic output and confirm that Tracktion high-level Clip/source scheduling is not activated.

This is not a Phase E PDC retry. It only establishes whether the selective boundary can carry a source signal into processing infrastructure.

### A2-3: mixed source rate (conditional)

Only after A2-1 passes, test 44.1 kHz source into a 48 kHz Project. Define and test an explicit AudioNLE-controlled rational mapping or a small deterministic SRC fixture. This does not select production SRC quality or policy.

## PDC handoff gate

No deterministic PDC processor is added under this plan. A later, separate Phase E retry is allowed only when all of the following are demonstrated by A2-1:

```text
Domain Timeline event == Master Output event
alignment error = 0 Timeline samples
for all required same-rate positions and render starts
```

The baseline then becomes the PDC reference. PDC, VST3, Tail, and Phase F do not begin merely because a custom source node compiles.

## Complexity measurement

Record the following before judging Option A2 worthwhile:

| Area | Measurement / evidence |
| --- | --- |
| Custom code | Lines/components required for source scheduling, source read, graph glue, and headless render. |
| API stability | Public API references versus internal/private dependencies. |
| Tracktion knowledge | Required knowledge of node lifecycle, graph ownership, thread/scheduler behavior, and latency semantics. |
| Reuse benefit | Processing graph, processor order, PDC feasibility, renderer, thread/scheduler reuse. |
| Escape cost | Additional custom code needed before the result is effectively a custom engine. |
| Maintenance risk | Fork/patch requirement, high-level object dependency, and pin sensitivity. |

The experiment is a Reject if the boundary needs a fork/private API, cannot preserve exact samples, cannot reuse enough processing infrastructure, or requires custom-engine-scale scheduling/graph ownership.

## Decision rules

| Result | Meaning |
| --- | --- |
| Proceed | All criteria pass with public APIs and no material adapter burden. This does not itself adopt Tracktion. |
| Proceed with Constraints | Source scheduling is AudioNLE-owned; low-level Tracktion processing reuse is viable with explicit, bounded adapter glue. |
| Reject | Exact scheduling fails, the high-level source path cannot be bypassed, public APIs are insufficient, a fork is needed, or complexity approaches a custom engine. |
| Inconclusive | The minimal public-API experiment cannot yet determine one of the criteria. |

The existing high-level Tracktion Option A remains a rejection signal for authoritative source scheduling regardless of the Option A2 result. Option B (AudioNLE Domain plus custom scheduling and selected low-level audio/plugin primitives) remains the comparison path; this document does not design it.

## Non-goals

- Phase F Effect Tail
- VST3 testing
- production GUI, persistence, waveform, autosave, or FFmpeg
- long-source optimization or runtime editing
- production SRC/resampler selection
- final production architecture
- ADR creation

## Expected result document

The implementation task, if separately authorized, must create `docs/design/prototypes/tracktion-selective-reuse-results.md` and conclude `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`. It must state the high-level Option A result, the Option A2 result, the Option B comparison implication, source-code evidence, exact dependency pins, measurements, boundary/leakage review, complexity findings, and whether a PDC retry is authorized.
