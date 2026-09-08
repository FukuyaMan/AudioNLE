# Option B Real Mixed-Rate Source-to-Timeline SRC Feasibility

## Status and objective

This is a planned bounded feasibility gate only. It changes no source, CMake target, dependency, submodule, benchmark result, execution plan, ADR, or selected backend.

It follows ADR 0001, the Option B realtime source handoff (`Proceed with Constraints`, `Thin backend handoff`), and multi-source arbitration/worker scaling (`Proceed with Constraints`, `Bounded shared worker model viable`, `Substantial source-service subsystem`). Those gates established same-rate source handoff and bounded worker ownership; they do not establish real mixed-rate media behavior.

The question is:

> Can Option B render real mixed-rate media through an AudioNLE-owned Source-to-Timeline mapping and explicit SRC boundary while preserving integer Timeline authority, bounded realtime source behavior, exact edit boundaries, deterministic reconstruction, and no hidden resampler timing offset?

## Authority and terminology

The Project Timeline remains authoritative in integer project-rate samples. A MediaSource remains authoritative in integer native-source-rate samples. Float seconds must not become persistent edit state, cache identity, split authority, or reconstruction input.

```text
integer Source sample domain
  -> explicit integer/rational mapping policy
  -> SRC input planning and filter state
  -> integer Timeline sample domain
```

The primary fixture is Source 44,100 Hz to Project 48,000 Hz:

```text
TimelineRate / SourceRate = 48000 / 44100 = 160 / 147
```

Use reduced integer factors `P = 160` and `Q = 147` for all authoritative mapping calculations. A useful secondary exact-ratio observation is 96,000 Hz Source to 48,000 Hz Project, but it must not replace the 44.1-to-48 non-integer-ratio work.

Three independent concepts must remain visible in every result:

| Concept | Owner | Meaning |
| --- | --- | --- |
| Edit-domain mapping | AudioNLE | Which integer Timeline boundary corresponds to an integer Source boundary |
| SRC filter behavior | chosen primitive/wrapper | Interpolation/filter response and input/output consumption |
| Algorithmic latency | wrapper metadata | Output-sample delay introduced by the implementation, if any |

An observation may not use an unexplained peak shift to silently redefine the edit-domain mapping. Record authoritative edit position, requested resampler input position, declared/observed latency, compensation or trim, and final Timeline observation position separately.

## Mapping policy to evaluate and select

The implementation must select a policy before real-WAV results are interpreted. The preferred candidate for same-rate-compatible half-open ranges is:

```text
forward boundary:  timelineBoundary(S) = floor(S * P / Q)
inverse read start: sourceBoundary(T) = floor(T * Q / P)
```

For a Source half-open range `[S0, S1)`, the corresponding authoritative Timeline range is:

```text
[ floor(S0 * P / Q), floor(S1 * P / Q) )
```

This is a candidate, not a pre-approved policy. S0/S1 must compare it with any nearest or alternative half-open policy and explain how it handles source boundaries that map to the same Timeline boundary. The chosen policy must provide deterministic rules for:

* forward Source-boundary to Timeline-boundary mapping;
* inverse Timeline position/range planning;
* a Timeline split that falls between nontrivial Source boundaries;
* Source trim/re-expand and source-end treatment; and
* intentionally non-bijective forward/inverse cases.

Forward and inverse functions need not be mathematical inverses at every individual sample. Their documented half-open semantics, integer overflow checks, and render behavior must be deterministic. Do not accumulate ratios in floating point.

## Ownership and state boundary

```text
MediaSourceRuntime
  owns source-rate page identity, reader/decoder context, cache, generation,
  and bounded worker request service.

ClipRuntimeView / SourceNode
  owns Timeline placement, Source range, rational mapping state, independent SRC phase/filter
  history for one active Timeline stream, fixed input/output buffers, and reset/seek control.

Option B graph
  receives project-rate audio only; downstream Clip processing, PDC and Tail stay project-rate.
```

One shared decoder/cache does not imply one shared resampler state. Far-apart Clip views of one MediaSource may have different Timeline placement, phase/history and reset behavior, so the gate must explicitly determine whether each active Clip view or SourceNode needs an independent state instance. SRC state must not be stored in Domain descriptions.

Domain may persist MediaSource identity/native sample rate, integer Source range, Timeline placement and edit description. It must not persist a resampler object, fractional phase accumulator, filter history, preroll buffer, latency-compensation cache, worker, page, or graph identity.

## Candidate SRC approaches

| Candidate | Shape | Required investigation |
| --- | --- | --- |
| A: public JUCE primitive | Use an available public JUCE resampler directly | Streaming contract, ratio units/direction, block consumption, reset, latency, allocation, thread/realtime assumptions, and public API status |
| B: AudioNLE mapping wrapper around a public primitive | AudioNLE owns rational mapping, planning, state/reset, latency metadata and alignment; primitive performs conversion | Whether it can yield exact, render-start-independent Timeline observations with fixed callback storage |
| C: custom SRC | AudioNLE owns conversion DSP and all state | Consider only if A/B cannot satisfy timing ownership; do not implement merely for preference |

No candidate is selected by this document. Candidate C is fallback-only. The later gate must record only guarantees documented by the pinned public API; undocumented thread safety, latency or allocation behavior is not assumed.

## S0: public API and latency investigation

Before implementation inspect the repository-pinned public JUCE resampling APIs actually available. Create `benchmark-results/option-b/mixed-rate-src-api-investigation.md` and, for each candidate, record public/private status, input/output contract, streaming and block behavior, ratio control, state lifetime, reset/seek semantics, latency reporting or absence, allocation behavior, realtime suitability assumptions, and thread-safety assumptions.

The API investigation must answer whether the primitive exposes algorithmic latency. If not, the wrapper must use an independently reproducible observation to establish its effective alignment or classify the candidate inconclusive. A helper must not silently define a Timeline offset.

## S1: generated-source integer mapping baseline

Before a real WAV, use a generated Source-domain marker/step fixture. Include Source markers at least:

```text
0, 1, 100, 147, 148, 1000, a large position, about one minute, about one hour
```

Derive each expected Timeline boundary entirely using the selected integer policy and `P/Q`, then record expected and observed mapping positions and maximum mapping error. Use robust observations: a deterministic step edge, known reference output, or declared impulse-response center/alignment. Do not define correctness solely as the largest impulse sample unless the selected filter gives that rule a documented meaning.

## S2: real 44.1 kHz WAV through Option B

Use a repository-local PCM16 44.1 kHz WAV with known markers:

```text
real WAV -> source worker/cache -> mixed-rate request planner -> SRC
-> Option B SourceNode/graph -> headless project-rate observation
```

Keep the established fixed-page, zero-on-miss, generation and callback boundary. Verify marker/edge timing against the authoritative mapping. Amplitude checks may use a tolerance/reference appropriate to filter spreading; they must not treat a filtered impulse's peak height as a sample-identity rule.

## S3: latency hard gate

Determine algorithmic latency in output Timeline samples, or demonstrate effective zero alignment under the wrapper. The result must state one of:

```text
latency = 0 under the documented observation/alignment policy
latency = L Timeline samples, explicitly held as runtime metadata and compensated/trimmed
```

`expected 1024, observed 1026` is a failure, not a tolerance. Unknown, path-dependent, or uncompensated latency is a hard stop.

## S4--S6: timing stability and state reset

### S4: render-start independence

Render equivalent source events with Timeline render starts 0, 512 and 1000. After documented initialization, preroll and latency compensation, observed Timeline placement must be exact and independent of render start.

### S5: block and page independence

Exercise Source/event positions around 127, 128, 129, 255, 256 and 257 or their relevant mapped boundaries. Test both process-block and 257-frame page crossing. Neither block geometry nor SRC streaming partition may redefine the mapped edit position.

### S6: seek/reset

Run forward render, backward seek/render, far-forward seek/render, and the same seek again. Define whether reset needs an AudioNLE-planned source preroll. Require no stale filter history, deterministic output, exact mapped boundaries, bounded reset buffers, and no hidden float phase accumulation.

## S7--S9: editing and long-form authority

### S7: trim and re-expand

With one real mixed-rate Clip, trim Source start/end, rebuild at the permitted control boundary, render, then re-expand. Require Source integers remain authoritative, Timeline placement remains stable according to the selected policy, and repeated edits do not accumulate fractional drift.

### S8: split

Split at an integer Timeline position whose inverse source boundary is nontrivial. Define the exact inverse-map boundary selection. Rebuilt left/right Clips must have a documented half-open Source/Timeline relationship and must not unintentionally duplicate or drop audio beyond that policy. This gate tests mapping semantics, not a new editor command implementation.

### S9: long-form drift

Test mapped positions at one minute, ten minutes and one hour. If a real file is impractical, use a generated/arithmetic fixture for optional 3/6/12-hour positions and label it accordingly. Record expected/observed integer positions and reject cumulative float drift or duration-scaled state.

## S10: realtime source/cache and SRC buffers

Retain all established callback constraints for every mixed-rate case:

```text
callback reader/file/decode = 0
callback wait/block/spin = 0
callback allocation/growth = 0
synchronous miss fallback = 0
```

SRC processing on the callback is allowed only with preallocated state and fixed caller/SourceNode buffers. Record source cache bytes, SRC input scratch bytes, output scratch bytes, filter/history state, and per-Clip/per-source scaling. Reject full-duration decoded or resampled storage and any duration-scaled buffering.

## S11: simultaneous mixed-rate sources

Run at least one 44.1 kHz source and one 48 kHz source into the Option B graph at the same time; optionally add 96-to-48. Verify independent mappings, project-rate Timeline authority, defined tolerance/reference for converted-source summing, unchanged bounded worker/cache ownership, and exact behavior for the native-rate source. Do not repeat the full 32-media matrix unless a concrete SRC interaction requires it.

## S12: reconstruction

From unchanged framework-free descriptions containing MediaSource identity, Source sample rate, integer Source range and Timeline placement, build fresh readers, caches, worker service, SRC state, Clip views, SourceNodes and Option B graph twice. Require equal defined output, mapping observations and intentional miss/recovery behavior. Do not require identical OS worker scheduling. Runtime SRC history and latency caches must not become persistent authority.

## Processing and existing graph boundaries

SRC occurs before Clip processing:

```text
Source read -> SRC to project-rate stream -> Clip processing -> Option B graph
```

VST3 processing does not run at source rate. Existing PDC and Tail evidence continues to operate in the project-rate graph; this gate does not expand into a combined PDC/Tail/SRC test except where a narrow boundary check is required.

## Hard stops

Stop and classify the gate `Reject` or `Inconclusive` before adding scope if any of the following occurs:

* float seconds becomes canonical edit/cache/reconstruction state, integer mapping drifts, or overflow is unhandled;
* forward/inverse, trim or split boundaries are ambiguous or render-start/block-dependent;
* resampler latency is unknown, path-dependent or uncompensated;
* seek leaks stale filter history, requires duration-scaled preroll/buffering, or fails deterministic reset;
* SRC introduces callback allocation/growth, wait/spin, reader/decode work or synchronous fallback;
* Timeline placement enters MediaSource cache identity, SRC state enters Domain, or far-apart Clip views incorrectly share history;
* Tracktion enters the target; or
* passing requires FFmpeg, device output, live mutation, or custom DSP without first rejecting public-primitive/wrapper options with evidence.

## Evidence, classification and completion

A later authorised implementation must produce:

```text
benchmark-results/option-b/mixed-rate-src-api-investigation.md
benchmark-results/option-b/mixed-rate-src.md
benchmark-results/option-b/mixed-rate-src-complexity.md
docs/design/prototypes/option-b-mixed-rate-src-results.md
```

The result document must cover final classifications; selected API/model; mapping policy and formulas; SRC ownership; API/latency result; S1--S12; memory/state formulas; complexity; Tracktion/JUCE boundary; remaining risks; and exactly one next gate.

Classify the gate as `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`; SRC architecture as `Public SRC primitive with AudioNLE mapping wrapper viable`, `Custom SRC layer required`, `Mixed-rate source architecture materially complex`, or `Inconclusive`; and burden as `Bounded SRC extension`, `Substantial SRC subsystem`, or `Custom-DSP warning`.

This planning task is complete when S0--S12, hard stops, ownership, deterministic integer mapping, latency criteria, resource accounting and evidence outputs are unambiguous. It makes no implementation change. If the later gate passes with exact deterministic mapping, known/compensated latency, no render-start offset, no long-form drift and callback invariants intact, recommend exactly **FFmpeg/container boundary**. Otherwise recommend one narrowly named SRC corrective gate.
