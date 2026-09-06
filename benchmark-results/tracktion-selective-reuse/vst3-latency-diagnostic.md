# Tracktion Selective Reuse — VST3 latency-report contract diagnostic

Execution date: 2026-09-06 (Asia/Tokyo)

## Scope and pins

This separately authorised root-cause diagnostic uses Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf` and nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`, unchanged. It runs headless at 48 kHz / 128 samples only. No dependency update, patch, fork, scanner database, manual PDC, graph compensation, source/output shift, Tail, Phase F, or production code was used.

## Source evidence

`juce::AudioProcessor::setLatencySamples` stores the value and calls `updateHostDisplay` only when it changes (`juce_AudioProcessor.cpp`). The fixture sets 1024 in its constructor. The pinned JUCE VST3 client exposes it through `JuceVST3Component::getLatencySamples`, returning `getPluginInstance().getLatencySamples()` (`juce_audio_plugin_client_VST3.cpp:3445-3448`). A later change is communicated as `Vst::kLatencyChanged` by `JuceVST3EditController::audioProcessorChanged` (`:1553-1566`) and `IComponentHandler::restartComponent` (`:1621-1629`).

The pinned public headless host activates during `VST3PluginInstanceHeadless::prepareToPlay`: it sets rate/block/layout, reads the VST3 processor latency, stores it through `AudioPluginInstance::setLatencySamples`, then calls `setActive(true)` (`juce_VST3PluginFormatImpl.h:2295-2365`). On a later `kLatencyChanged`, `VST3HostContextHeadless::restartComponent` reads and stores it again (`:3493-3537`). A fixed-latency fixture therefore needs no artificial post-activation notification; prepare captures the initial value.

`tracktion::graph::SimpleNodePlayer` prepares graph nodes only after topology/latency properties are queried (`tracktion_NodePlayerUtilities.h:120-131`). The former adapter read its plugin latency before the required public prepare lifecycle.

## D1–D9 measurements

| Check | Original | Corrected diagnostic | Result |
| --- | --- | --- | --- |
| D1 fixture contract | constructor `setLatencySamples(1024)` | unchanged | 1024 declared |
| D2/D3 create only | adapter read immediately after create | `create=0` | expected pre-prepare state |
| D3/D4 prepare/activation | absent from former adapter | `prepare=1024`, 48 kHz / 128 | Pass |
| D2 after first process | not reached | `first-process=1024` | Pass |
| D5 fixture audio implementation | reads `(write + 1) % 1024`, actual delay 1023 | reads `write`, then writes and advances | corrected to 1024 |
| D6 latency change | fixed fixture emits no later change | source path verified; initial prepare is sufficient | Pass |
| D7 direct public host | unmeasured | `actual-delay=1024` | Pass |
| D8 direct host vs adapter | not compared | both report 1024 after public prepare | Pass |
| D9 lifecycle | create → Node properties | create → rate/block → `prepareToPlay` → Node properties | root cause |

Original V2 executable:

```text
VST3 PDC FAIL: plugin latency report=0 expected=1024
```

Corrected diagnostic:

```text
VST3 latency diagnostic create=0 prepare=1024 first-process=1024 actual-delay=1024 adapter-after-prepare=1024
```

`ctest --test-dir build -R tracktion_selective_reuse_vst3_latency_diagnostic --output-on-failure` passed 1/1. `build.ps1` configured and built the diagnostic; its full suite retains the known unrelated high-level Phase E baseline failure.

## Classification and stop

**Adapter lifecycle bug, plus fixture off-by-one delay bug.** The host reports 1024 through its public API after the required lifecycle. The fixture correction changes only its actual delay index; the adapter correction prepares before publishing graph latency. Neither forces latency or adds compensation.

An actual-VST3 PDC retry is technically ready for separate authorisation because host-visible and actual delay both equal 1024. It was not executed here.
