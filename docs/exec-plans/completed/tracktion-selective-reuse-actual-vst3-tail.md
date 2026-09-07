# Execution Plan: Option A2 Actual VST3 Effect Tail

## Objective

Measure a bounded actual hosted VST3 finite-tail path on the AudioNLE-controlled custom-source/low-level Tracktion graph. This is prototype evidence only, not a production Tail design.

## Scope and approach

Using the pinned Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf` and nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`, a repository-local headless VST3 fixture was added. It reports and emits exactly 1024 samples at 48 kHz. The framework-free Domain fixture uses Clip Start 1000, Source Duration 480, Source End 1480, and Processing End 2504.

The implementation uses public JUCE `VST3PluginFormatHeadless`, `AudioPluginFormatManager`, `prepareToPlay`, `getTailLengthSeconds`, and `processBlock`, alongside public Tracktion `Node`, `SimpleNodePlayer`, and `SummingNode`. Domain state keeps only integer clip/time policy values; hosted plugin instances and buffers are runtime-only.

## Invariants and results

* The public host report converts deterministically to 1024 Timeline samples; actual non-zero Tail is exactly `[1480,2504)`.
* Post-source input is silence (1208 observed zero samples in the bounded render); a downstream Multiply(2) sees the Tail, and `SummingNode` overlap is 0.75.
* Move shifts all Tail positions by +48000, delete yields no output, `CutAtSourceEnd` exposes no Tail, and destroy/rebuild is byte-identical.
* No high-level source scheduler, private API, patch/fork, hard-coded adapter tail, manual post-mix, or Domain mutation was used.

Combined PDC-plus-Tail, arbitrary plugins, unknown/infinite policies, realtime work, render planning, persistence, scanner/crash isolation, GUI, and ADRs remain excluded.

## Verification

`build.ps1` configured and built the new fixture and executable. The targeted CTest and direct executable passed:

```text
VST3-TAIL reported-seconds=0.0213333 reported-samples=1024 source-end=1480 processing-end=2504 first=1480 last=2503 actual-samples=1024 zero-input=1208 overlap=0.75 move=48000 cut=pass rebuild=equal
VST3-TAIL PASS
```

The repository-wide CTest run continues to expose the pre-existing unrelated high-level Phase E PDC baseline failure (`expected=1024`, `observed=1026`). It is unchanged and does not run through the new Option A2 VST3-Tail target.

## Changed files

* `prototype/tracktion-selective-reuse/CMakeLists.txt`
* `prototype/tracktion-selective-reuse/src/vst3_tail_fixture_plugin.cpp`
* `prototype/tracktion-selective-reuse/src/vst3_actual_tail_main.cpp`
* `benchmark-results/tracktion-selective-reuse/vst3-tail.md`
* `benchmark-results/tracktion-selective-reuse/complexity.md`
* `docs/design/prototypes/tracktion-selective-reuse-results.md`
