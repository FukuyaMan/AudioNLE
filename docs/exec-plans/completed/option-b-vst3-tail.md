# Execution Plan: Option B Actual VST3 Finite Effect Tail

## Objective and boundary

## Progress

Completed. T0--T8 pass in the bounded local fixture at maximum boundary/timing error 0. T9 classifies finite deterministic Tail as `Small Tail extension` and the gate as `Proceed with Constraints`. The plan moved here after standard build, targeted/direct/repeated execution, dependency review, and diff check; full suite retains only the known unrelated Tracktion Phase E baseline failure.

Execute a bounded feasibility fixture answering whether Option B can query finite public-JUCE VST3 Tail metadata, convert it to exact integer samples, silence-feed past SourceEnd, and retain Tail propagation/overlap/edit semantics without Tracktion. Final classifications: gate `Proceed`/`Proceed with Constraints`/`Reject`/`Inconclusive`; complexity `Small Tail extension`/`Substantial Tail subsystem`/`Custom-engine-scale warning`.

```text
framework-free description -> integer scheduling -> AudioNLE graph -> JUCE VST3
-> AudioNLE SourceEnd/Tail/ProcessingEnd -> downstream/summing -> headless output
```

Tracktion is prohibited. Include only deterministic local finite Tail, public JUCE host, silence-feed, integer extent, Multiply, overlap, stopped rebuild Move/Delete, Reported/CutAtSourceEnd, reconstruction, and complexity. Exclude unknown/infinite/dynamic Tail, media/device/cache, GUI, live mutation, export/persistence, plugin management, ADR, and selection.

## Fixed model and ownership

Use 48 kHz, 128-frame process blocks, fixture Tail 1024 samples, SourceEnd 1480, and half-open `TailRange=[1480,2504)`, `ProcessingEnd=2504`. Convert host seconds to samples with `round(reportedSeconds * projectSampleRate)` only for this exact fixture after verifying `1024/48000` returns 1024; integer samples are authoritative and seconds are not stored in Domain.

AudioNLE owns SourceEnd, Tail policy, derived TailDurationSamples, ProcessingEnd, silence-feed scheduling, and render extent. Plugin owns finite metadata and Tail generation under zero input. The graph scheduler processes each block's active-source portion, zero-fed portion, and processing-stop boundary; SourceEnd 1480 deliberately falls inside a 128-frame block.

## T0 — public API/lifecycle investigation

Inspect pinned public JUCE Tail API, return unit, lifecycle validity, pre/post-prepare behavior, zero Tail, destruction, and fixture latency. Record `benchmark-results/option-b/vst3-tail-api-investigation.md` with API/unit/lifecycle/rate dependency/Option B ownership/A2 equivalent/risk. Confirm no Tracktion wrapper is required and distinguish zero-latency Tail from PDC.

## Fixture and T1 hard gate

Use/reuse a build-only local deterministic mono fixture: Tail 1024 at 48 kHz, zero latency preferred, no GUI/gain/process allocation, nonzero known Tail under zero input, then exact zero. Before graph work, direct-host record SourceEnd, intended/report/converted/actual Tail, first Tail, last nonzero, first zero, and boundary error. Pass only if intended = converted host report = actual and error 0; otherwise stop.

## T2 — SourceEnd silence scheduling

Render generated source through hosted Tail VST3. Before SourceEnd emit ordinary source; at/after it emit exact zero while processing through derived ProcessingEnd. Require no invented source post-end, exact Tail begin/end/first-zero, and render extent derived from metadata rather than test correction.

## T3/T4 — downstream and overlap

T3 requires `Source -> Tail VST3 -> Multiply(2)` with exact doubled Tail. Optionally record reverse order without implementing general stack-Tail policy. T4 places Clip B during A Tail, uses existing Option B N-input sum, and verifies Tail-only, overlap, and B-only samples as exact linear sum without a Tail mixer.

## T5/T6 — stopped rebuild Move/Delete

T5 mutates only framework-free placement T1→T2 then rebuilds. Source and Tail move by the same integer delta; old source/Tail are zero; source range is unchanged; new boundaries are exact. T6 deletes A from description then rebuilds: A source/Tail disappear, unrelated B remains exact, and no old Tail state appears.

## T7 — policy fixture

Implement only `Reported` (`ProcessingEnd=SourceEnd+convertedTail`) and `CutAtSourceEnd` (`TailDuration=0`, `ProcessingEnd=SourceEnd`). Require Reported Tail and immediate post-end zero for CutAtSourceEnd. Do not define production UI, Manual, or BoundedAutomatic.

## T8 — reconstruction

Descriptions contain Clip ID, source range, placement, fixture identity, enabled state, and Tail policy—never plugin/JUCE/Node pointers, cached seconds/samples, or Tail state.

```text
description -> instantiate/prepare/query/convert -> derive ProcessingEnd -> runtime -> render A -> destroy
same description -> fresh runtime -> render B
```

Compare full output, SourceEnd, TailDuration, ProcessingEnd, boundaries, overlap, downstream amplitude, and policy. Pass requires equality and no description mutation.

## T9 — complexity and result

Write `benchmark-results/option-b/vst3-tail-complexity.md`, separating non-additive architecture LOC/responsibility for Tail query/conversion/metadata, SourceEnd/policy/ProcessingEnd, block-edge zeroing, silence feed, extended loop, downstream/overlap, Move/Delete rebuild, and reconstruction from fixture/assertion/instrumentation/CLI code.

Write `docs/design/prototypes/option-b-vst3-tail-results.md` including T0–T8, maximum boundary error, process behavior, classification, A2 comparison, untested responsibilities, and exactly one next step.

| Responsibility | A2 | Option B measured |
| --- | --- | --- |
| plugin Tail query / metadata | Tracktion/JUCE + AudioNLE policy | fixture result |
| SourceEnd silence / propagation / overlap | Tracktion path | AudioNLE graph |
| processing extent / Move/Delete | AudioNLE + Tracktion execution | AudioNLE |

## Process, exclusion, and verification

Wrapper/graph and fixture have fixed buffers with growth/allocation/wait/source-I/O 0; do not generalise to arbitrary plugins. Later verify `build.ps1`, targeted CTest, direct executable, repeated runs, `git diff --check`, includes/dependencies, and Option B source/CMake search: `Tracktion references=0`, `Tracktion linkage=0`.

## Hard stops and completion

Stop on Tail report/actual mismatch, non-integer boundary, post-SourceEnd source, early/overrunning Tail, overlap failure, old Tail after Move/Delete, manual extent correction, Domain runtime leakage, growth, Tracktion/private API, general scheduler requirement, or excluded-scope expansion. Move this plan to completed only after T0–T9 and results. If T1–T8 pass at error 0 with small/manageable Tail complexity, recommend **A2 vs B ADR-level comparison** rather than adding more Option B functionality.
