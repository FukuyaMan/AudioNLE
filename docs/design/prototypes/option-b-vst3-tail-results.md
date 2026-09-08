# Option B Actual VST3 Finite Tail Results

## Outcome

Finite actual VST3 Tail is **Proceed with Constraints** and its measured complexity is **Small Tail extension**. This is bounded prototype evidence, not an architecture selection or production guarantee.

The local mono VST3 uses public JUCE headless hosting at 48 kHz/128 frames. Its post-prepare Tail report converts by `round(seconds * 48000)` to 1024 samples. With `SourceEnd=1480`, AudioNLE runtime derives `ProcessingEnd=2504` and Tail range `[1480,2504)`. Fixture latency is zero.

## T0--T8 evidence

| Step | Result |
| --- | --- |
| T0/T1 API/direct host | Pass: public `AudioPluginInstance::getTailLengthSeconds()`, intended/converted/actual Tail all 1024; first Tail 1480, last 2503, first zero 2504. |
| T2 SourceEnd scheduling | Pass: source is exact zero from 1480 within its process block; Tail and derived finite end are exact. |
| T3 downstream | Pass: raw Tail `0.125` at 1497 becomes `0.25` through unconditional Multiply(2). |
| T4 overlap | Pass: ordinary summing gives `0.125` Tail-only, `0.625` overlap, and `0.5` B-only. |
| T5 Move | Pass: stopped rebuild by delta 4000 clears old output and yields moved Tail `[5480,6504)` with correct overlap. |
| T6 Delete | Pass: fresh runtime without A has no A source/Tail; B remains `0.5`. |
| T7 policy | Pass: `Reported` retains `[1480,2504)`; `CutAtSourceEnd` derives duration 0/end 1480 and emits zero thereafter. |
| T8 reconstruction | Pass: identical final descriptions produce equal complete output and equal derived extents after fresh VST3 construction. |

Maximum observed boundary/timing error is **0 samples**. Process-path growth, wait, and source I/O counters are **0**. The framework-free description contains ID, source range, placement, source value, enabled state, and policy; it has no JUCE object, plugin/Node pointer, cached host seconds/samples, ProcessingEnd cache, or Tail state.

## Ownership and comparison

| Responsibility | A2 | Option B measured |
| --- | --- | --- |
| Plugin Tail query | Tracktion/JUCE path | public JUCE host |
| Tail duration / SourceEnd policy | Tracktion + AudioNLE boundary | AudioNLE runtime |
| Tail propagation, downstream, summing | Tracktion graph | Option B graph fixture |
| Processing extent / Move/Delete | AudioNLE + Tracktion execution | AudioNLE stopped rebuild |

Option B Tail source/CMake/target has Tracktion source references, headers/types, and linkage all **0**; public JUCE headless VST3 hosting is the only hosting dependency. No Tail-specific mixer, PDC change, Tracktion wrapper, or general scheduler was required.

Untested: unknown/infinite/dynamic Tail, multiple arbitrary Tail processors, arbitrary third-party VST3s, scanning/crash isolation, device/realtime scheduling, mixed-rate media, FFmpeg/container media, worker pools, and persistent live mutation.

## Implication and next step

The finite actual VST3 Tail gate remained a bounded scheduling extension over the established Option B graph/VST3 core. The next recommended step is **A2 vs B ADR-level comparison preparation**; it is not an architecture selection.
