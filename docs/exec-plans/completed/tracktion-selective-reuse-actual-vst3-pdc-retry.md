# Execution Plan: Corrected Actual-VST3 PDC Retry

## Objective

Use the repaired public VST3 host lifecycle to test actual-VST3 latency propagation and Tracktion graph-owned PDC for the bounded Option A2 prototype. This follows the recorded initial V2 hard stop and its separately authorised diagnostic; it does not alter that chronology.

## Scope and invariants

- Keep the pinned Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf` and nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576` unchanged.
- Use repository-local, generated, headless mono VST3 latency fixtures only, at 48 kHz and 128-frame blocks.
- Create, configure, and `prepareToPlay` each hosted instance before publishing its `NodeProperties::latencyNumSamples`.
- Require declared fixture latency, post-prepare hosted latency, measured audio delay, adapter latency, and root graph latency to agree exactly.
- Keep authoritative processor/path descriptions framework-free; runtime VST3 instances and Tracktion nodes are transient and never mutate Domain state.
- Let public `SummingNode` own graph-wide balancing. The adapter must not compute or inject `maxLatency - pathLatency`, shift source coordinates/output, use a private API, patch, or fork.

## Test matrix

1. Baseline and two-path PDC for actual VST3 fixtures at 256, 1024, and 2048 samples.
2. Clip- and Track-equivalent VST3 placement; mixed deterministic Clip 256 plus actual-VST3 Track 768, alongside actual-VST3 1024.
3. Actual VST3 combined with deterministic multiply in both orders, checking timing and amplitude.
4. Fixture adapter policy for enabled and bypassed states.
5. Stopped destroy/rebuild latency change from 1024 to 2048 and reconstruction from unchanged framework-free descriptions.

## Edge cases and stop conditions

Any non-zero alignment error, declared/report/actual mismatch, adapter-owned balancing, high-level source scheduling, private API, patch/fork, Domain mutation, or failed rebuild is a hard stop. Playback-time latency changes, arbitrary third-party compatibility, production bypass semantics, scanning, state serialization, crash isolation, Tail, Phase F, realtime work, and GUI are excluded.

## Files and verification

Modify only prototype CMake/source and the associated raw/result/complexity records. Preserve the latency-contract diagnostic record and its historical chronology. Build and test using `build.ps1`, then run the targeted actual-VST3 CTest/direct executable and review the diff.

## Completion criteria

Every in-scope case records exact reported and observed values with maximum alignment error of zero Timeline samples, or the first hard stop is recorded without proceeding to later cases. Results explicitly distinguish this bounded Conditional Pass from production plugin hosting readiness.
