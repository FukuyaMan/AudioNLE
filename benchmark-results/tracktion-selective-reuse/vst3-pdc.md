# Tracktion Selective Reuse — Actual VST3 PDC

Execution date: 2026-09-06 (Asia/Tokyo)

## Scope and pins

This is the authorized actual-VST3 latency/PDC prototype only. Tracktion Engine remains `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE remains `37c894f83d379179b2070d437ccd0f1cd9af9576`. No external dependency was retrieved: the fixture uses JUCE's bundled VST3 SDK headers (MIT license, under the pinned JUCE source), and a repository-local prototype-only fixture plugin. Its artifact is generated under the ignored build directory and is neither vendored nor recorded as a repository binary.

## V0 investigation

Pinned public JUCE provides `VST3PluginFormatHeadless::findAllTypesForFile`, `AudioPluginFormatManager::createPluginInstance`, and `AudioPluginInstance::getLatencySamples`. The fixture uses an explicit known generated VST3 path only; it implements no scanner database or UI. Hosting is runtime-only in a custom low-level `Node` adapter and does not instantiate `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, or Tracktion plugin persistence.

## Measurements and hard stop

| Test | Expected | Observed | Status |
| --- | --- | --- | --- |
| V0 fixture build | generated 1024-sample-delay VST3 | Pass after two normal JUCE build-configuration corrections | Pass |
| V1 custom-source baseline | event 1024 | constructed but not accepted as final baseline because V2 hard-stop follows | Not decisive |
| V2 plugin latency report | `AudioPluginInstance::getLatencySamples() = 1024` | value was not 1024 | **Fail / hard stop** |
| V3–V9 PDC, layers, bypass, change, rebuild | not run | prohibited after V2 | Skipped |

Direct executable and targeted CTest both report:

```text
VST3 PDC FAIL: plugin latency report is not 1024
```

The actual fixture declares `setLatencySamples(1024)`. The public hosted instance did not expose 1024 through the adapter's early `getLatencySamples` check. Therefore actual/report equality and graph latency propagation had not been established at the original hard stop. No manual compensation, latency override, source shift, output shift, high-level scheduling, private API, patch, or fork was applied.

## Diagnostic chronology after the hard stop

A separately authorised diagnostic identified the original V2 read as pre-`AudioPluginInstance::prepareToPlay`: it observed 0. The adapter now performs public rate/block setup and prepare before it publishes graph latency. The fixture delay-ring read index was also corrected from `(write + 1) % 1024` (actual 1023) to `write` before storing/advancing (actual 1024). The corrected diagnostic observes `create=0`, `prepare=1024`, `first-process=1024`, `actual-delay=1024`, and `adapter-after-prepare=1024`.

This is not a PDC retry. The original hard-stop record remains valid chronologically; source evidence and the bounded correction are in [vst3-latency-diagnostic.md](vst3-latency-diagnostic.md). A PDC retry requires separate authorisation.

## Corrected formal retry

The separately authorised retry uses generated repository-local VST3 fixtures at 256, 768, 1024, and 2048 samples. Each host instance follows `create -> rate/block -> prepareToPlay -> read latency -> Node properties -> graph`; the adapter only adds the post-prepare hosted latency to its input `NodeProperties`.

| Test ID | Fixture / paths | Declared / hosted / actual / graph latency | Common output | Error | Status |
| --- | --- | --- | --- | ---: | --- |
| VST3-PDC-0 | custom source only | 0 / 0 / 0 / 0 | 1024 | 0 | Pass |
| VST3-PDC-1 | two paths, actual VST3 256 | 256 / 256 / 256 / 256 | 1280 | 0 | Pass |
| VST3-PDC-2 | two paths, actual VST3 1024 | 1024 / 1024 / 1024 / 1024 | 2048 | 0 | Pass |
| VST3-PDC-3 | two paths, actual VST3 2048 | 2048 / 2048 / 2048 / 2048 | 3072 | 0 | Pass |
| VST3-PDC-4 | Clip- and Track-equivalent 1024 | 1024 / 1024 / 1024 / 1024 | 2048 | 0 | Pass |
| VST3-PDC-5 | 0; deterministic Clip 256 + actual Track 768; actual Clip 1024 | path totals 0 / 1024 / 1024; graph 1024 | 2048 | 0 | Pass |
| VST3-PDC-6 | VST3 1024 before/after Multiply(2) | 1024 / 1024 / 1024 / 1024 | 2048, amp 0.50 | 0 | Pass |
| VST3-PDC-7 | adapter-policy bypass | 0 / 0 / 0 / 0 | 1024 | 0 | Pass |
| VST3-PDC-8 | stopped destroy/rebuild 1024 -> 2048 | agreement at 1024 -> 2048 | 2048 -> 3072 | 0 | Pass |
| VST3-PDC-9 | mixed Domain -> render -> destroy -> rebuild | graph 1024, equal outputs | 2048 | 0 | Pass |

```text
VST3-PDC value=256 declared=256 hosted=256 actual=256 graph=256 observed=1280 error=0
VST3-PDC value=1024 declared=1024 hosted=1024 actual=1024 graph=1024 observed=2048 error=0
VST3-PDC value=2048 declared=2048 hosted=2048 actual=2048 graph=2048 observed=3072 error=0
VST3-PDC layers=clip,track,mixed order=both observed=2048 error=0
VST3-PDC bypass=0 stopped-change=1024->2048 reconstruction=equal error=0
VST3-PDC PASS maximum-alignment-error=0
```

Public `SummingNode` owns `maxLatency - pathLatency` balancing. No manual compensation, source/output shift, high-level source scheduler, private API, patch, fork, or Domain mutation was used. Bypass is an adapter-only fixture policy and does not establish production plugin bypass semantics.

**Corrected result: Conditional Pass for bounded actual-VST3 PDC.** Maximum alignment error: **0 Timeline samples**. Third-party VST3 compatibility, playback-time latency changes, scanning, state serialization, crash isolation, production bypass semantics, Tail, Phase F, realtime editing, and production hosting readiness remain unmeasured.

## Original hard-stop result (superseded by corrected retry)

**Reject for the actual-VST3 PDC path at this pin, subject only to a separately authorized root-cause investigation.** This is not a rejection of the deterministic low-level PDC result. It is a hard failure of the actual VST3 latency-report contract, so V3–V9, Tail, Phase F, realtime work, scanner UX, production plugin hosting, and ADR work were not run.
