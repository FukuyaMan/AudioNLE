# Production SRC Release / Threshold Policy

## Licensing and reproducibility

The pinned `COPYING` file is BSD-2-Clause: copyright Erik de Castro Lopo (2012--2016), source redistribution retains copyright/conditions/disclaimer, and binary redistribution reproduces them in documentation or other distributed materials. Static linkage does not introduce copyleft/source-offer obligations under this license; it does require the notice/disclaimer in release materials. Current FetchContent build statically links only libsamplerate for this SRC target; no additional libsamplerate runtime dependency is shipped. CMake/Visual Studio tools are build-only; fixture-only dependencies are not SRC release artifacts.

Release must include `THIRD_PARTY_NOTICES` or equivalent `LICENSES/libsamplerate-BSD-2-Clause.txt`, copyright/disclaimer, upstream URL, version 0.2.2 and exact revision, static-link statement, build options, toolchain/build type and patch list (`none`). This is a SRC-component obligation; it does not replace audit of unrelated repository dependencies.

## Project-local quality acceptance

These are AudioNLE release criteria, not claims of industry standards: for every supported rate pair and fixed configuration, steady-state usable passband (up to 0.80 of the lower Nyquist) gain error <=0.10 dB and ripple <=0.05 dB; sampled downsample alias and upsample image rejection >=60 dB; DC/low-frequency gain error <=0.10 dB with no drift; no NaN/Inf; same-build deterministic frames, boundaries and samples; fresh reconstruction/edit max/RMS <=2e-5. Broadband, multitone, marker/impulse, spoken-like, music-like, Source-start/end and representative real-media fixtures are release coverage. Current generated fixtures satisfy the numerical/edit/determinism criteria; the sampled -65.12 dB upsample image passes with only 5.12 dB margin, so it requires production remeasurement rather than being a broad quality claim.

## Project-local performance acceptance

Release benchmarks require Release build, declared CPU class/power mode/background policy/timer/affinity, >=10,000 measured callback blocks after warm-up, three runs, and mean/p50/p95/p99/worst reporting. Hard gates are forbidden callback work = 0 and worst ratio <1.0. Minimum supported density is 8 active SRC views with worst ratio <0.50; recommended operating density is 16 views with worst ratio <0.70; 32 views is a stress tier with worst ratio <0.85, not a universal guarantee. Per-source cache is fixed 8224 bytes; per-view prepared/staging are fixed 8192/2048 bytes; duration-scaled PCM must remain zero. Release regression alert is >=15% p95 or worst regression on the same declared environment, subject to repeated-run confirmation.

Product requirements contain no Must/Should requirement for 32 simultaneous mixed-rate SRC views. Therefore 32 views is classified **Stress tier unsupported for production guarantee**, not a production prerequisite. No threshold was changed after the recorded 32-view / 128-frame hard-deadline failure (1.01063).

## Classification

**Deep realtime classification: Backend callback path supported with bounded residual blind spots.** Mandatory caller-IAT categories and first/steady libsamplerate smoke are verified, but the result is confined to the recorded executable/import routes and does not prove dynamically resolved or future-module routes.

**Production performance classification: Production performance requires optimisation.** Primary 44.1->48 kHz previously completed all 8- and 16-view rows, but the recorded 8/16 sanity run found threshold failures: 48->44.1 16-view run 1 = 0.823068; 44.1->96 8-view = 0.638888/0.592800/0.566250 and 16-view = 1.416710/1.018730/1.145550; 96->44.1 8-view = 0.611457/0.524187/0.588477 and 16-view = 1.056660/1.055920/1.216280; 48->96 16-view = 1.680750/0.678300/0.942863; 96->48 16-view run 1 = 0.729675. The 48->96 8-view rows and 96->48 8-view rows passed. No optimization is selected by this classification; a new optimization decision needs product evidence and a controlled-environment diagnosis.
