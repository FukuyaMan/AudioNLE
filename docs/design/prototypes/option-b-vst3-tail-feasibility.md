# Option B Actual VST3 Finite Effect Tail Feasibility

## Status and question

This is a planning-only bounded gate. It does not choose a production architecture or implement Tail. It asks whether Option B can query a finite actual VST3 Tail through public JUCE hosting, schedule SourceEnd silence in the AudioNLE graph, preserve exact finite ProcessingEnd, downstream processing, overlap, Move/Delete, and reconstruction without Tracktion.

```text
framework-free description -> integer source/timeline scheduling -> AudioNLE graph
-> JUCE-hosted VST3 -> AudioNLE SourceEnd/Tail scheduling
-> downstream processing/summing -> headless output
```

Tracktion is forbidden. Include only a deterministic local finite-tail VST3, public JUCE Tail query, finite Tail, silence feeding, exact boundaries, Multiply, overlap, stopped rebuild Move/Delete, `Reported`/`CutAtSourceEnd`, reconstruction, and complexity comparison. Exclude unknown/infinite or changing Tail, UI, automatic policies, arbitrary plugins, media/FFmpeg, cache/device, live mutation, export, persistence, ADR, and final selection.

## Product semantics and ownership

Effect Tail is Processing Output after SourceEnd, not a Source range extension:

```text
SourceReference -> Clip Processing Stack -> Clip Processing Output -> Track Mix
-> Track Processing Stack -> Master Processing Stack -> Master Output
```

AudioNLE owns SourceEnd, TailDuration policy, ProcessingEnd, silence-feed scheduling, and render/process extent. The plugin reports processor metadata; the plugin host must not silently choose Timeline extent. For a one-Clip fixture:

```text
TailRange = [SourceEnd, SourceEnd + TailDuration)
ProcessingEnd = SourceEnd + TailDuration
```

All values are integer samples. If JUCE reports seconds, convert using an explicit documented rule; choose 48 kHz and a fixture Tail of 1024 samples so `1024 / 48000` converts exactly back to 1024. Floating seconds are not authoritative Timeline state.

## T0 — Tail API/lifecycle investigation

Inspect pinned public JUCE APIs and existing local finite-tail fixture evidence. Record in `benchmark-results/option-b/vst3-tail-api-investigation.md`: host Tail API/unit, valid lifecycle point, preparation effect, zero/no-Tail behavior, release/destruction, and public/private status. Confirm Tracktion wrappers are unnecessary.

Record reported value, sample rate, derived tail samples, and rounding rule. Determine whether fixture latency is zero; if not, separately model output timing latency versus Tail processing-duration extension.

## Fixture

Use or create a build-only deterministic mono local VST3 fixture: finite reported Tail 1024 samples at 48 kHz, no GUI/gain/process allocation, zero latency preferred, known Tail under zero input after nonzero source, and exact finite stop. Do not vendor binaries.

## T1 — fixture validation hard gate

Before graph scheduling, direct-host validate:

```text
intended Tail = post-prepare host report converted to samples = actual emitted Tail duration
```

Record SourceEnd, report, derived Tail samples, first Tail sample, last nonzero Tail sample, first zero after Tail, actual duration, and boundary error. Use half-open ranges exactly. Stop on disagreement before T2.

## T2 — SourceEnd silence scheduling

Render generated source through hosted Tail fixture. At/after SourceEnd, the AudioNLE source Node supplies exact zero while graph processing continues through ProcessingEnd. Require Tail start/end, no invented source after SourceEnd, exact finite stop, and derived render extent. No plugin/runtime pointer or cached Tail enters Domain.

## T3 — downstream order

Verify `Source -> Tail VST3 -> Multiply(2)` multiplies Tail samples. Optionally compare `Multiply -> Tail VST3`, retaining that stack order changes Tail semantics. Tail is ordinary downstream audio once emitted.

## T4 — overlap/summing

Place Clip B during Clip A's Tail and use the AudioNLE Summing Node. Require expected linear sum; no Tail-specific mixer or hard clipping.

## T5/T6 — stopped rebuild Move/Delete

Move Clip A from T1 to T2 by mutating framework-free description then rebuilding. Source and Tail ranges must move by the same integer delta; old source/Tail is zero and new Tail exact. Delete A, rebuild, and require its source/Tail absent while unrelated B remains exact. No live mutation or stale cached Tail is allowed.

## T7 — policy fixture

Implement only `Reported` (host finite Tail metadata) and `CutAtSourceEnd` (TailDuration 0, ProcessingEnd SourceEnd). Manual/BoundedAutomatic remain conceptual and no production UI is defined.

## T8 — reconstruction

```text
description (source range, placement, fixture identity, enabled, Tail policy)
-> instantiate/prepare/query -> derive ProcessingEnd -> graph/runtime -> render A -> destroy
same description -> fresh runtime -> render B
```

Compare complete output, SourceEnd, TailDuration, ProcessingEnd, Tail samples, overlap, downstream amplitude, and policy. Runtime/plugin pointers and cached Tail cannot become authoritative.

## Process-time rules and complexity

Wrapper/graph uses fixed buffers with no growth, wait, or source I/O; fixture allocation is zero. Do not claim arbitrary plugin internals allocation-free.

Write `benchmark-results/option-b/vst3-tail-complexity.md`, separating non-additive responsibility/LOC for Tail query/conversion/metadata, SourceEnd tracking, ProcessingEnd derivation, silence feed, extended render scheduling, downstream processing, overlap, Move/Delete rebuild, policy, and reconstruction from fixture/assertion/instrumentation code.

Compare actual ownership:

| Responsibility | A2 | Option B |
| --- | --- | --- |
| plugin Tail query | Tracktion/JUCE path | measured fixture result |
| Tail metadata / SourceEnd silence | Tracktion graph + AudioNLE boundary | AudioNLE |
| Tail propagation / overlap | Tracktion | AudioNLE graph |
| processing extent / Move/Delete | AudioNLE policy + Tracktion execution | AudioNLE |

## Result, hard stops, and next step

Write `docs/design/prototypes/option-b-vst3-tail-results.md` with T0–T8, classifications, max boundary error, process behavior, LOC, A2 comparison, untested responsibilities, and exactly one next step. Classify gate as `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`; Tail complexity as `Small Tail extension`, `Substantial Tail subsystem`, or `Custom-engine-scale warning`.

Stop on report/actual disagreement, ambiguous integer boundaries, source after SourceEnd, early/overrunning Tail, incorrect overlap, old Tail after Move/Delete, manual ProcessingEnd correction, Domain runtime leakage, process growth, Tracktion dependency, private API, general scheduler requirement, or excluded scope expansion.

Even on success, retain untested unknown/infinite/dynamic Tail, Tail stacking, arbitrary plugins, device and realtime scheduling. If finite Tail is a small bounded extension, the likely next step is A2-vs-B ADR-level comparison because graph, PDC, actual hosting, latency, Tail, and headless execution will have comparable bounded evidence.
