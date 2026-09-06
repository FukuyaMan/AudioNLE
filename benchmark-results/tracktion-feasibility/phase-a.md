# Tracktion Feasibility — Phase A Record

Execution date: 2026-09-06 (Asia/Tokyo)

## T-001 reconstruction

Test ID: T-001 reconstruction

Hypothesis: H1 Domain Independence; H2 Runtime Reconstruction.

Environment: Windows 11 Home 10.0.26200 x64; AMD Ryzen 9 7900X; 31.12 GiB RAM; Visual Studio 2026 Build Tools 18.9.2; MSVC 19.51.36256.0 (toolset 14.51.36231); CMake 4.3.1-msvc1; Ninja 1.13.2; Windows SDK 10.0.26100.0.

Dependency: Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`. The Phase 0 pins were retained without revision change.

Fixture:

| Field | Value |
| --- | --- |
| Project Timeline Sample Rate | 48000 Hz |
| Media | `media-phase-a`, generated mono 16-bit WAV, 96000 samples |
| Track | `track-phase-a`; gain -3.0 dB; pan -0.25 |
| Clip 1 | `clip-phase-a-1`; Timeline [48000, 72000); Source [12000, 36000) |
| Clip 2 | `clip-phase-a-2`; Timeline [96000, 108000); Source [36000, 48000) |
| Processing Stack | Empty by design; non-empty mapping is deferred to Phase D |
| Transport | stopped; playhead 24000 Timeline samples |

Method: `MinimalDomainState` and `TransportState` are C++ standard-library value types. `TracktionAdapter` reads them once to construct an adapter-owned transient `tracktion::engine::Edit`; no runtime identity, Tracktion serialization, or Tracktion/JUCE type is stored in the Domain State. A local adapter `SourceRegistry` resolves the framework-independent media ID to the generated WAV only while constructing/observing runtime state. The test constructs, observes, destroys, verifies the Domain State equals its pre-construction copy, reconstructs from the same fixture, observes again, and requires equality of both observations.

Expected: One audio track and two clips; exact Timeline and Source sample ranges; fixture source file; gain/pan; empty processor order; stopped transport at 24000; unchanged Domain State; identical reconstruction observation.

Observed:

| Measurement | Result |
| --- | --- |
| Runtime construction | Pass; one Tracktion Edit was created from the fixture, destroyed, then rebuilt. |
| Track / Clip count | 1 / 2 on both constructions. |
| Timeline mapping | Clip 1 [48000, 72000), Clip 2 [96000, 108000) on both constructions. |
| Source mapping | Clip 1 [12000, 36000), Clip 2 [36000, 48000); both runtime source references resolve to the fixture WAV and `std::filesystem::equivalent` succeeds. |
| Gain / pan | -3.0 dB / -0.25 on both constructions. |
| Processing Stack order | Empty on both constructions, as required by this Phase A fixture. |
| Transport | stopped at 24000 Timeline samples on both constructions. |
| Domain mutation | None; pre- and post-construction `MinimalDomainState` values compare equal. |
| Reconstruction observation | Equal to the first observation. |

Measurement: `./build.ps1` completed configure, build, and CTest with `1/1` tests passed in 0.95 s (total 1.19 s). Direct headless invocation printed `PHASE_A_PASS tracks=1 clips=2 playhead_samples=24000 gain_db=-3 pan=-0.25` and exited 0. No physical audio device was opened.

Status: Pass.

Workaround: None. The adapter-owned source resolver is an intentional boundary mapping from a framework-free media ID to a supplied file path; it is neither Project persistence nor Domain State.

Requirement impact: H1 and H2 pass for this bounded fixture. No Phase A hard stop was reached: reconstruction did not require a Tracktion object graph, Tracktion serialization, runtime identity, or reverse synchronization to become authoritative.

Notes: This is not evidence for Phase B integer round-trip across sample rates. The adapter's fixed-fixture observation converts Tracktion time values back to samples solely to check this Phase A fixture; integer Timeline samples remain authoritative in the Domain State. No rendering, overlap/mix, non-empty Processing Stack, PDC, Effect Tail, runtime edit, long-form, dense-edit, or failure injection test was run.
