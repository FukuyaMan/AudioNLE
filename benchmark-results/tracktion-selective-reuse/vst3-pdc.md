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

## Result

**Reject for the actual-VST3 PDC path at this pin, subject only to a separately authorized root-cause investigation.** This is not a rejection of the deterministic low-level PDC result. It is a hard failure of the actual VST3 latency-report contract, so V3–V9, Tail, Phase F, realtime work, scanner UX, production plugin hosting, and ADR work were not run.
