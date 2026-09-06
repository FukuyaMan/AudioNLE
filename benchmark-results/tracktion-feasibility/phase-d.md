# Tracktion Feasibility — Phase D Record

Execution date: 2026-09-06 (Asia/Tokyo)

## T-004 Processing Stack order

Test ID: T-004 Processing Stack order

Hypothesis: H5 Basic Signal Flow; H8 runtime editing boundary (stopped-state reconstruction scope only).

Environment: Windows 11 Home 10.0.26200 x64; AMD Ryzen 9 7900X; 31.12 GiB RAM; Visual Studio 2026 Build Tools 18.9.2; MSVC 19.51.36256.0 (toolset 14.51.36231); CMake 4.3.1-msvc1; Ninja 1.13.2; Windows SDK 10.0.26100.0.

Dependency: Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`. The Phase 0 pins were retained without revision change.

Method: `MinimalDomainState` is framework-free C++ value data. Each `ProcessorState` has only a Domain ID, deterministic kind (`add` or `multiply`), float parameter, and enabled flag; no Tracktion/JUCE type, pointer, state tree, or runtime identity enters Domain State. The adapter creates transient Tracktion `Plugin` instances through its prototype-only `EngineBehaviour` factory and maps each Domain Stack, in vector order, into the Clip, Track, or Master runtime PluginList. The temporary runtime `ValueTree` carries only the adapter's construction parameters and is neither Project persistence nor authoritative state.

The deterministic processor does exactly one operation per float sample and output channel: Add is `output = input + A`; Multiply is `output = input * B`. It is not a Tracktion/JUCE built-in processor and no `VolumeAndPan` processor is used as a purported generic passthrough. All renders are headless `Renderer::RenderTask` float-buffer observations of Master Output, over Timeline [0, 256) at 48 kHz, without a physical audio device.

| Fixture | Domain Stack(s) | Expected / observed Master Output at Timeline sample 120 | Runtime order / parameter observation | Status |
| --- | --- | --- | --- | --- |
| Clip order | Clip: Add `A=0.25` → Multiply `B=2` | `(0.25 + 0.25) * 2 = 1.0` / `1.0` | `add`, `multiply`; `0.25`, `2.0` | Pass |
| Clip reorder | Clip: Multiply `B=2` → Add `A=0.25` | `0.25 * 2 + 0.25 = 0.75` / `0.75` | `multiply`, `add`; `2.0`, `0.25` | Pass |
| Track layer | Two `0.25` Clips → Track: Add → Multiply | `(0.25 + 0.25 + 0.25) * 2 = 1.5` / `1.5` | Track `add`, `multiply`; `0.25`, `2.0` | Pass |
| Master layer | Clip `0.25` → Master: Add → Multiply | `(0.25 + 0.25) * 2 = 1.0` / `1.0` | Master `add`, `multiply`; `0.25`, `2.0` | Pass |
| Layer order | Clip Add → Track Multiply → Master Add | `((0.25 + 0.25) * 2) + 0.25 = 1.25` / `1.25` | Separate Clip / Track / Master stacks each preserve ID and parameter | Pass |
| Reconstruction | destroy then rebuild the Clip-order fixture | `1.0` / `1.0` | same `add`, `multiply`; same parameters | Pass |

Stopped-state reorder strategy: the authoritative Domain vector is changed from Add → Multiply to Multiply → Add, then the previous runtime is discarded and a new runtime is constructed from the changed Domain State before render. This is a full stopped-runtime rebuild; it requires no reverse synchronization from Tracktion. Paused/playing updates are intentionally not exercised and remain Phase G work.

Observed direct invocation: `PHASE_D_PASS clip_forward=1 clip_reordered=0.75 track=1.5 master=1 layered=1.25 reconstruction=1`; exit 0.

Measurement: `./build.ps1` configured, built, and ran CTest. Phase A passed in 0.74 s, Phase B in 9.82 s, Phase C in 4.81 s, and Phase D in 5.03 s; all 4/4 tests passed in 20.63 s. No physical audio device was opened. Amplitude assertions use tolerance 0.0002; all stated values met the tolerance.

Status: Pass.

Workaround: None. The custom deterministic processor and its EngineBehaviour factory are test instrumentation confined to `prototype/tracktion-feasibility/`; they do not select a production processor hosting, serialization, parameter, or update architecture.

Requirement impact: The bounded prototype demonstrates the formal flow `Clip Processing Stack → Clip Processing Output → Track Mix → Track Processing Stack → Track Gain/Pan → Master Processing Stack → Master Output`, and proves the non-commutative expected output for the tested Stack orders. Runtime PluginLists are recreated solely from Domain state and never become authoritative.

Notes: This is not evidence for PDC, Effect Tail, VST3 hosting, persistence, live playback reorder, paused/playing update safety, production realtime safety, or a production AudioNLE processor model. No GUI, Project persistence, Synchronization Group/Member, Clip Group, or Ripple semantics was implemented or delegated to Tracktion.
