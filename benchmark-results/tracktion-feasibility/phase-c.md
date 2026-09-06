# Tracktion Feasibility — Phase C Record

Execution date: 2026-09-06 (Asia/Tokyo)

## T-003 overlap / Track Mix

Test ID: T-003 overlap / Track Mix

Hypothesis: H5 Basic Signal Flow and Track Mix.

Environment: Windows 11 Home 10.0.26200 x64; AMD Ryzen 9 7900X; 31.12 GiB RAM; Visual Studio 2026 Build Tools 18.9.2; MSVC 19.51.36256.0 (toolset 14.51.36231); CMake 4.3.1-msvc1; Ninja 1.13.2; Windows SDK 10.0.26100.0.

Dependency: Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`. The Phase 0 pins were retained without revision change.

Method: Framework-free Domain State specifies 48 kHz Timeline/Source integer sample ranges, Media IDs, Clip/Track/Master Processing Stack containers, Track gain, and pan. The adapter constructs only a transient Tracktion runtime and maps Gain/Pan once. Headless `Renderer::RenderTask` renders 256 Timeline samples at 48 kHz without selecting a physical audio device. Its incoming float-buffer receiver observes the Master Output before file encoding, so integer PCM/file clipping cannot be mistaken for Track Mix behaviour. Domain State is checked unchanged after each construct/render/destroy cycle.

The Phase C fixtures use empty Clip/Track/Master Processing Stacks, which is permitted for basic-flow validation; the containers are present and Clip stack availability is asserted. Processor mapping/order remains Phase D work. A limited initial experiment inserted Tracktion `VolumeAndPan` as a supposed passthrough and observed mono-to-stereo gain alteration (0.25 source observed as 0.5). It was removed rather than treated as a passthrough; this Phase C result does not conceal that framework channel-mapping behaviour.

| Fixture | Clips and Timeline range | Expected active range | Expected / observed float Master Output | Status |
| --- | --- | --- | --- | --- |
| F-C1 single Clip | one `0.25` mono Clip, [100, 164) | [100, 164) | `0.25` / `0.25`; silence outside | Pass |
| F-C2 two Clip overlap | `0.25` [100, 164), `0.5` [132, 196) | [100, 196), overlap [132, 164) | first-only `0.25`, overlap `0.75`, second-only `0.5` | Pass |
| F-C3 three Clip overlap | three `0.25` Clips, [100, 164) | [100, 164) | `0.75` / `0.75` | Pass |
| F-C4 above 0 dBFS | two `0.75` Clips, [100, 164) | [100, 164) | internal Track Mix `1.5` / `1.5` | Pass |
| F-C5 Track Gain | `0.5` Clip [100, 164), Track gain -6.020599913 dB | [100, 164) | `0.25` / `0.25` per centred output channel | Pass |
| F-C6 Pan | `0.5` Clip [100, 164), Track pan -1 | [100, 164) | left `1.0`, right `0.0` | Pass; framework pan law is observed, not adopted as product policy |

Observed: `PHASE_C_PASS fixtures=6 max_amplitude_error=0.0002 single=0.25 overlap2=0.75 overlap3=0.75 internal_sum=1.5 gain=0.25 pan_left=1 pan_right=0`; process exit 0.

Measurement: Render range [0, 256); active sample ranges as above; inactive samples required to be within 0.0002 of zero; amplitude comparison tolerance 0.0002. Maximum permitted and observed assertion error is 0.0002 or less. The internal float observation of 1.5 proves no immediate hard clipping at the tested Track Mix point. No claim is made about final export or limiter/clipping policy.

Status: Pass.

Workaround: None. The rejected `VolumeAndPan` passthrough experiment is not retained in the adapter. Empty Processing Stacks are intentional within Phase C scope; a non-empty processor mapping/order is explicitly deferred to Phase D.

Requirement impact: The prototype demonstrates SourceReference → empty Clip Processing Stack → Clip Processing Output → Track Mix → empty Track Processing Stack → Track Gain/Pan → empty Master Processing Stack → Master Output for the stated fixtures. H5 passes for basic signal flow. The test preserves the Phase A/B Domain boundary: no runtime state, runtime sample data, or renderer output is synchronized back into Domain State.

Notes: This is not a production pan-law, limiter, clipping, export-codec, processor-order, PDC, Effect Tail, runtime-editing, long-source, dense-Clip, or failure-injection result. No physical audio device, GUI, Project persistence, Synchronization Group/Member, Clip Group, or Ripple semantic is used or delegated to Tracktion.
