# Tracktion Feasibility - Phase E Source/Resampler Baseline Latency Diagnostic

Execution date: 2026-09-06 (Asia/Tokyo)

Scope: a bounded diagnostic of the no-plugin baseline offset only. No Domain Timeline value, output buffer, or source position was compensated; no PDC processor, VST3, Tail, Phase F, or production SRC policy was implemented.

Dependency pins are unchanged: Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`.

## Source evidence

At the pinned revision, `WaveNodeRealTime::buildAudioReaderGraph()` constructs either `LagrangeResamplerReader` or `HighQualityResamplerReader` for the Clip source. `AudioClipBase` defaults `ResamplingQuality` to `lagrange`. On reset, `LagrangeResamplerReader` obtains `juce::LagrangeInterpolator::getBaseLatency()` and sets `timeSourceIsAheadDueToLatency`; its reader position is consequently adjusted. `WaveNode::getNodeProperties()` reports audio/channel properties but no reader latency. The renderer obtains only graph-node latency for its preroll/drop handling. Thus the reader base latency is not exposed as a reported plugin or graph latency for this fixture.

Call path inspected: `WaveAudioClip` -> `WaveNodeRealTime` -> audio-file cache reader -> resampler reader -> graph node -> `NodeRenderContext` -> receiver.

## Measurements

All impulse tests use an adapter-owned resolver, a 48 kHz Project, 4096 requested samples, headless `Renderer::RenderTask`, and no physical device. `offset = observed Timeline sample - requested Domain Timeline event`.

| Diagnostic ID | Source rate | Event / render start | Quality | Observed | Offset |
| --- | ---: | ---: | --- | ---: | ---: |
| A same-rate baseline | 48000 | 1024 / 0 | Lagrange | 1026 | +2 |
| B event 1 | 48000 | 1 / 0 | Lagrange | 1 | 0 |
| B event 100 | 48000 | 100 / 0 | Lagrange | 100 | 0 |
| B event 2048 | 48000 | 2048 / 0 | Lagrange | 2050 | +2 |
| B event 3000 | 48000 | 3000 / 0 | Lagrange | 3000 | 0 |
| C render start | 48000 | 1024 / 512 | Lagrange | 1026 | +2 |
| C render start | 48000 | 1024 / 1000 | Lagrange | 1024 | 0 |
| D source-rate conversion | 44100 | 1024 / 0 | Lagrange | 1025 | +1 |
| D source-rate conversion | 96000 | 1024 / 0 | Lagrange | 1025 | +1 |
| E quality | 48000 | 1024 / 0 | sincFast | 1026 | +2 |
| E quality | 48000 | 1024 / 0 | sincMedium | 1026 | +2 |
| E quality | 48000 | 1024 / 0 | sincBest | 1026 | +2 |

Each completed in 452-479 ms and delivered 4096 receiver samples. The resolver-negative control remains unchanged: removing only the resolver timed out at 10 seconds with zero delivered blocks.

## Interpretation

The offset is not a constant adapter-correctable value: it changes with Timeline event position, render start/reset boundary, source rate, and supported resampling quality. The same-rate case still goes through a resampler reader; all publicly selectable tested qualities retain the +2 result for the Phase E event 1024 case. No public reader, graph, or plugin latency report was found for this source-reader offset, and renderer graph-latency handling cannot compensate an unreported latency.

Phase C did not expose the issue because it uses a constant source and verifies samples inside an active range, rather than a single-sample onset. That earlier result remains valid for its stated signal-flow checks but is not evidence of sample-edge placement.

Classification: **Tracktion source/resampler baseline limitation for this strict sample-accurate fixture**. It is not an adapter-manageable policy without adding a magic offset or silently changing authoritative Domain/receiver coordinates, both excluded by this diagnostic. Phase E PDC retry is not valid until an independently specified, supported zero-error source-to-Timeline reference can be demonstrated. H6 and H9 remain Inconclusive; Phase F and E2 do not start.
