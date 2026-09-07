# Option B Actual VST3 Hosting / PDC Results

Gate: **Proceed with Constraints**. Hosting complexity: **Small hosting extension**.

An isolated public-JUCE host target with zero Tracktion source references/linkage loaded local deterministic VST3 fixtures. Lifecycle was `create -> configure -> prepare -> query`: host latency was 0 before prepare and matched declared/actual delay after prepare for 256, 1024 and 2048 samples. Single path, graph-derived parallel PDC, multiple latency values, mixed-path accounting, processing-order fixture, and reconstruction passed at maximum error 0. Wrapper growth/waits were 0; plugin/JUCE internal allocation is not claimed.

Option B now has bounded actual hosting evidence, but arbitrary plugins, scan/state/UI/crash isolation, dynamic latency, layout diversity, Tail, device, realtime cache and FFmpeg are untested. Compared with A2, Option B owns host lifecycle and graph PDC while A2 reuses Tracktion runtime/graph. Next: **Option B finite Tail hosting fixture**.

## Final-tree verification

After relocating the deterministic fixture source to `prototype/option-b-low-level-graph/src/vst3_latency_fixture_plugin.cpp`, the Option B VST3 executable and targeted `option_b_vst3_pdc` CTest passed. Direct execution re-confirmed V1–V6 and V8 output at maximum timing error 0. The Option B source/CMake search found zero Tracktion references; its target links public JUCE hosting only, with no Tracktion linkage. `git diff --check` passed. The repository build entry compiled/linked the current tree; its full suite retains the known unrelated Tracktion high-level Phase E PDC failure.
