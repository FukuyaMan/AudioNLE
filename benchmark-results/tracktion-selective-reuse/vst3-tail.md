# Tracktion Selective Reuse — Actual VST3 Effect Tail

Execution date: 2026-09-06 (Asia/Tokyo)

## Scope and pins

This bounded prototype uses the pinned Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf` and its nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`, unchanged. It hosts a repository-local, build-only VST3 fixture using a known generated path. The fixture is mono, headless, reports `1024 / 48000` seconds, passes source audio through, and after source silence emits exactly 1024 samples of `0.25`, then exact silence.

The authoritative framework-free Domain fixture uses 48 kHz integer Timeline samples: Clip Start 1000, Source Duration 480, Source End 1480, reported Tail 1024, and Processing End 2504. The test uses the custom Option A2 source node, public low-level `Node`, `SimpleNodePlayer`, and `SummingNode`. It does not instantiate `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, a high-level Tracktion source scheduler, private APIs, a Tracktion/JUCE patch, or a fork.

## VT0: tail-report API evidence

The fixture's `AudioProcessor::getTailLengthSeconds()` returns `1024 / 48000`. In pinned JUCE's VST3 client, `JuceVST3Component::getTailSamples()` calls that method and converts its finite seconds value using the active process sample rate (`juce_audio_plugin_client_VST3.cpp:3480-3488`). In pinned JUCE's headless VST3 host, `VST3PluginInstanceHeadless::getTailLengthSeconds()` reads `processor->getTailSamples()`, maps `Vst::kInfiniteTail` to infinity, and otherwise divides the bounded sample count by the host sample rate (`juce_VST3PluginFormatImpl.h:2730-2748`).

The adapter follows the public lifecycle `findAllTypesForFile` → `createPluginInstance` → `setRateAndBufferSizeDetails(48000, 128)` → `prepareToPlay(48000, 128)` → `AudioPluginInstance::getTailLengthSeconds`. It converts the resulting finite seconds with `llround(seconds * 48000)` before planning; floating-point seconds never enter Domain state.

The pinned host path represents finite and zero values in seconds and maps VST3's infinite sentinel to infinity. This fixture demonstrates only the finite case. Unknown/infinite semantics and any production `Reported` / `Manual` / `BoundedAutomatic` policy remain unmeasured.

## Measurements

| Test ID | Observation | Expected | Observed | Status |
| --- | --- | --- | --- | --- |
| VT1 | Source End / Tail boundaries | Source End 1480; Tail `[1480,2504)`; last 2503; Processing End 2504 | exact | Pass |
| VT2 | Input supplied after Source End | 480 source-active input samples; at least 1024 zero input samples after Source End | 480 / 1208 | Pass |
| VT3 | Downstream graph | VST3 tail `0.25` → public downstream Multiply(2) = `0.50` | exact | Pass |
| VT4 | Track Mix | VST3 tail `0.25` + independent source `0.50` at 1800 through `SummingNode` | `0.75` | Pass |
| VT5 | Move | Source End, first/last Tail, Processing End all shift by +48000 | exact | Pass |
| VT6 | Delete | empty Domain produces no source output or Tail | empty | Pass |
| VT7 | `CutAtSourceEnd` | Processing output stops at Source End; no Tail is exposed | exact | Pass |
| VT8 | `Reported` | Processing End = Source End + public host-reported finite Tail | `1480 + 1024 = 2504` | Pass |
| VT10 | Reconstruction | same Domain → new runtime/plugin → identical output and Tail boundary | equal | Pass |

```text
VST3-TAIL reported-seconds=0.0213333 reported-samples=1024 source-end=1480 processing-end=2504 first=1480 last=2503 actual-samples=1024 zero-input=1208 overlap=0.75 move=48000 cut=pass rebuild=equal
VST3-TAIL PASS
```

The host-visible report and actual non-zero Tail agree at exactly 1024 samples. The Source End / Processing End error is 0 Timeline samples.

`CutAtSourceEnd` clears output frames at and after Source End, including the trailing portion of a graph block which began before Source End. A VST3 `processBlock` call cannot be subdivided after it has begun, so the source-active block is processed once; AudioNLE's policy prevents any post-source output from escaping. This is bounded block-boundary evidence, not a realtime stopping contract.

## Deferred VT9 and exclusions

The requested combined latency-plus-tail fixture (`Latency = 256`, `Tail = 1024`) is deferred. The existing actual-VST3 PDC fixture and this Tail fixture independently establish their contracts, but neither is a combined production-tail/PDC result. No manual compensation was introduced.

Arbitrary third-party plugins, unknown/infinite tails, dynamic tail reports, realtime playback/editing, production render/export planning, scanner/state persistence, crash isolation, GUI, long-source/dense-Clip benchmarks, and ADR work remain out of scope.

## Result

**Conditional Pass for bounded actual-VST3 finite Effect Tail.** The public hosted VST3 report, explicit integer conversion, custom-source silence feed, exact Tail boundary, downstream low-level graph processing, public mix, move/delete, TailPolicy, and reconstruction contracts all pass. This is not a production tail architecture decision.
