# Tracktion Feasibility — Phase 0 and Phase 1 Record

Execution date: 2026-09-06 (Asia/Tokyo)

## Environment

| Item | Observed value |
| --- | --- |
| OS | Windows 11 Home 10.0.26200, x64 |
| CPU | AMD Ryzen 9 7900X 12-Core Processor (12 cores / 24 logical processors) |
| Physical memory | 31.12 GiB |
| Git | 2.43.0.windows.1 |
| CMake | Not installed / not discoverable on `PATH` |
| Ninja | Not installed / not discoverable on `PATH` |
| MSVC (`cl`, `msbuild`) | Not installed / not discoverable on `PATH` |
| Clang / GCC | Not installed / not discoverable on `PATH` |
| Audio configuration | Not applicable: no runtime was built and no physical audio device was opened. |

## Phase 0 — Discovery and pinning

### Dependency revision and retrieval record

| Dependency | Exact revision | Retrieval method evaluated | License / NOTICE record |
| --- | --- | --- | --- |
| Tracktion Engine | `b88a6ee51913668cb53e911e030ab736b13342cf` (the observed `develop` head on 2026-09-06) | Git clone by explicit commit; future prototype retrieval must fetch this SHA, with submodules recursively initialized at their recorded SHAs. No source or binary dependency was added to this repository. | `LICENSE.md` at this revision: GPLv3-or-later or commercial license. README lists bundled third-party notices: rpmalloc (public domain), ConcurrentQueue (Simplified BSD/BSL), choc (ISC), crill (BSL-1.0), expected (CC0-1.0), libsamplerate (BSD-2-Clause), MPMCQueue (MIT), magic_enum (MIT), farbot (MIT), doctest (MIT), signalsmith-stretch (MIT). Redistribution obligations still require legal review. |
| JUCE submodule required by that Tracktion revision | `37c894f83d379179b2070d437ccd0f1cd9af9576` | Tracktion's `.gitmodules` identifies `modules/juce`; a clean probe checkout resolved and checked out this exact gitlink. | `LICENSE.md` at this revision: AGPLv3 or JUCE 8 commercial licence. The notice enumerates bundled dependencies, including VST3 SDK (MIT) and ASIO (proprietary Steinberg ASIO licence/GPLv3). Redistribution and optional-module selection require legal review. |
| VST3 test plugin | Not selected | Not retrieved. The plan requires deterministic processors first; no plugin scan or binary artifact was used. | Inconclusive; only needed after deterministic PDC and Tail tests pass. |

Tracktion's README at the pinned revision states that it requires C++20 and supports CMake builds. Its root CMake configuration uses the pinned JUCE submodule unless a parent project supplies JUCE. `TE_ADD_EXAMPLES=OFF` would be required for a prototype-only configuration to avoid example targets.

No floating revision was used in a build. The temporary inspection clone was outside this repository and is not a prototype artifact.

## Phase 1 — Build bootstrap

Test ID: Phase 1 bootstrap / H10 early investigation
Hypothesis: A prototype-only headless executable can be built and launched without a physical audio device.
Environment: As listed above.
Fixture: None; this phase intentionally has no GUI, audio device, plugin scan, or edit fixture.
Expected: Clean-checkout CMake configuration and C++20 build, followed by a headless process that starts and exits.
Observed: No supported C++ compiler, CMake, Ninja, MSBuild, or `vswhere` was installed or discoverable. Configuration could not begin; consequently no executable exists and no runtime was started.
Measurement: Construction, integer mapping, signal, PDC, Tail, runtime-edit, long-form, dense-edit, headless-runtime, and failure-state measurements are **not executed**. Reporting a numeric value for them would be fabricated.
Status: Inconclusive
Workaround: Install a pinned/reproducible CMake + C++20 Windows toolchain, then repeat only Phase 1 before proceeding to Phase A. No Tracktion-specific workaround has been identified or attempted.
Requirement impact: H10 cannot be evaluated. Per the execution plan, later phases require the preceding Phase to pass and therefore were not started.
Notes: This is an environment bootstrap block, not evidence that Tracktion requires a GUI or physical audio device. It is also not a Phase A/B/E/F hard stop, because no Tracktion runtime was built or observed.

## Phase 1 retry — Pass

Execution date: 2026-09-06 (Asia/Tokyo)
Test ID: Phase 1 bootstrap / H10 early investigation (retry)
Hypothesis: A prototype-only headless executable can be built from the pinned sources and launched without a physical audio device.
Environment: Windows 11 Home 10.0.26200 x64; AMD Ryzen 9 7900X (12 cores / 24 logical processors); 31.12 GiB RAM; Visual Studio 2026 Build Tools 18.9.2; MSVC 19.51.36256.0 (`14.51.36231` toolset); CMake 4.3.1-msvc1; Ninja 1.13.2; Windows SDK 10.0.26100.0; Git 2.43.0.windows.1.
Fixture: No audio fixture. `tracktion_feasibility_headless` only constructs and destroys `juce::ScopedJuceInitialiser_GUI` and `tracktion::engine::Engine`; it neither selects nor opens an audio device.
Dependency: Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`. Both actual checkout SHAs were compared with the Phase 0 record and matched exactly.
Method: `./build.ps1` initializes `VsDevCmd.bat -arch=x64 -host_arch=x64`, configures `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`, builds, then runs `ctest --test-dir build --output-on-failure`. The first corrected invocation started from an empty `build/` directory and completed configure/generate successfully (17.5 s configure). A prior bootstrap-only configure failed because the root project enabled CXX but not C, while Tracktion's CMake project enables C; the prototype root now explicitly enables both C and CXX. This was a local build-definition correction, not a dependency revision change.
Observed: `tracktion_feasibility_headless.exe` was built successfully. The standard build entry point's final CTest invocation reported `1/1` test passed in 0.40 s (total 0.41 s), with no warnings or errors in its final build/test log.
Measurement: Clean configure: Pass; build: Pass; executable startup/shutdown: Pass; CTest exit: 0; physical audio device: not configured or opened. No Phase A signal, timing, PDC, Tail, edit, long-form, dense-edit, or failure-state measurement was performed.
Status: Pass
Workaround: None.
Requirement impact: The Phase 1 headless-bootstrap prerequisite passes. H10 as a complete failure-injection / CI integration hypothesis remains untested until Phase I.
Notes: The root `CMakeLists.txt` contains only `add_subdirectory(prototype/tracktion-feasibility)`. All Tracktion/JUCE includes, source, and targets remain under `prototype/tracktion-feasibility/`; no AudioNLE Domain or production source exists or depends on these libraries.
