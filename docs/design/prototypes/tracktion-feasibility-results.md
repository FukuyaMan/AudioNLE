# Tracktion Engine Feasibility Prototype — Results

## Recommendation

**Inconclusive overall; Phase 1 — Build bootstrap: Pass; Phase A — Framework boundary and reconstruction: Pass; Phase B — Integer Timeline Sample: Pass; Phase C — Basic Signal Flow: Pass; Phase D — Processing Stack order: Pass.**

The original Phase 1 environment block has been resolved. The pinned Tracktion/JUCE sources configure, build, and launch in a prototype-only headless executable on this Windows environment. Phase A established the bounded framework boundary and reconstruction path without making Tracktion runtime state authoritative. Phase B demonstrated zero-error integer-sample observation. Phase C demonstrated the stated basic signal-flow and Track Mix fixtures in a headless float-buffer observation point. Phase D then mapped framework-free deterministic Processor descriptions, in order, to Clip, Track, and Master runtime Stacks; it observed the expected non-commutative results and a stopped-state full rebuild for reorder. The overall feasibility conclusion remains Inconclusive because this task is explicitly limited to Phase D; Phases E–I have not been run. No ADR is created.

The execution plan requires each phase to pass before the next begins. This result does not infer any Phase E–I behaviour from the bootstrap or Phases A–D targets.

## 1. Environment

| Item | Value |
| --- | --- |
| Execution date | 2026-09-06 (Asia/Tokyo) |
| OS | Windows 11 Home 10.0.26200, x64 |
| CPU | AMD Ryzen 9 7900X, 12 cores / 24 logical processors |
| RAM | 31.12 GiB |
| Git | 2.43.0.windows.1 |
| Audio configuration | No runtime, GUI, plugin editor, or physical device opened |
| Visual Studio environment | Visual Studio 2026 Build Tools 18.9.2; MSVC 19.51.36256.0 / toolset 14.51.36231 |
| CMake / generator | CMake 4.3.1-msvc1; Ninja 1.13.2 |
| Windows SDK | 10.0.26100.0 |
| Build tooling | CMake, Ninja, and C++20-capable MSVC are discoverable after `VsDevCmd.bat` initialization |

The raw Phase 0/1 command results and observations are in [phase-0-1.md](../../../benchmark-results/tracktion-feasibility/phase-0-1.md).

## 1.1 Phase 1 retry result

| Criterion | Result |
| --- | --- |
| CMake discoverable | Pass — 4.3.1-msvc1 |
| C++20 compiler discoverable | Pass — MSVC 19.51.36256.0 |
| Build generator discoverable | Pass — Ninja 1.13.2 |
| Windows SDK available | Pass — 10.0.26100.0 |
| Exact Tracktion / JUCE revisions | Pass — both checkout SHAs match the Phase 0 record |
| Clean configure | Pass — from an empty `build/` directory; configure completed in 17.5 s |
| Prototype-only headless build | Pass — `tracktion_feasibility_headless.exe` built |
| Device-free startup/shutdown | Pass — CTest launched it and passed in 0.40 s; it does not select or open an audio device |
| Production dependency leakage | Pass — dependencies and includes are contained in `prototype/tracktion-feasibility/`; root CMake only adds that directory |

## 1.2 Phase A reconstruction result

The raw fixture, command result, and observation record are in [phase-a.md](../../../benchmark-results/tracktion-feasibility/phase-a.md).

| Criterion | Result |
| --- | --- |
| Framework-free Minimal Domain State | Pass — standard C++ value data contains IDs, Timeline/Source integer samples, processor descriptions, gain, and pan; it contains no Tracktion/JUCE type or runtime identity. |
| Separate transport state | Pass — stopped/playhead 24000 samples is independent of Project state. |
| Domain → Adapter → Runtime direction | Pass — adapter reads fixture state to construct runtime; no reverse synchronization occurs. |
| Source Media mapping | Pass — adapter-local source registry resolves the runtime reference to the generated fixture WAV; it is not persisted in Domain State. |
| Destroy / reconstruct | Pass — domain is unchanged after destroy; reconstructed observation equals the original. |
| Runtime observation | Pass — counts, Timeline/Source sample ranges, gain/pan, empty processor order, and transport setup match the fixture. |
| Phase A hard stop | Not reached — no Tracktion object graph, serialization, runtime identity, or reverse synchronization is required as authoritative state. |

## 1.3 Phase B integer Timeline result

The raw fixture definitions, runtime observations, and command result are in [phase-b.md](../../../benchmark-results/tracktion-feasibility/phase-b.md).

| Criterion | Result |
| --- | --- |
| Authoritative Timeline state | Pass — Timeline position and duration remain `int64_t` samples at the Project Timeline Sample Rate. |
| Authoritative Source state | Pass — Source position and duration remain separate `int64_t` samples at Media native sample rate. |
| Framework leakage | Pass — no Tracktion/JUCE time type enters Domain State. |
| 48 kHz arbitrary positions | Pass — 14 positions, including 0, 1, 47999/48000/48001, and arbitrary values, observe at 0 sample difference. |
| Long Timeline positions | Pass — 1/3/6/12 hour anchors and each ±1 sample observe at 0 sample difference. |
| 44.1 kHz Source / 48 kHz Project | Pass — six distinct Source offsets and one-second 44100/48000 durations preserve their separate domains at 0 sample difference. |
| Repeated reconstruction | Pass — 10 reconstructions per fixture, 30 total; Domain State is unchanged and observations are equal per fixture. |
| Maximum observed mapping error | Pass — `0` samples. |
| Phase B hard fail | Not reached — runtime seconds remain adapter details and do not replace the Domain's integer state. |

## 1.4 Phase C basic signal-flow result

The raw fixtures, float-buffer observations, and command result are in [phase-c.md](../../../benchmark-results/tracktion-feasibility/phase-c.md).

| Criterion | Result |
| --- | --- |
| Headless offline render | Pass — `Renderer::RenderTask` completed without a physical audio device. |
| Single Clip placement | Pass — source `0.25` is active only at Timeline [100, 164). |
| Two / three Clip Track Mix | Pass — `0.25 + 0.5 = 0.75`; three `0.25` clips also sum to `0.75`. |
| Internal > 0 dBFS sum | Pass — two `0.75` clip outputs observe as `1.5` in the float Master Output, not 1.0. |
| Track Gain | Pass — `0.5` at -6.020599913 dB observes as `0.25`. |
| Pan | Pass for mapping/observation — Domain pan -1 maps to runtime and observes left 1.0/right 0.0; production pan law remains undecided. |
| Processing Stacks | Pass for container/path presence with permitted empty stacks; non-empty processor mapping/order is deferred to Phase D. |
| Domain boundary | Pass — renderer observations remain test evidence and do not synchronize into Domain State. |

## 1.5 Phase D Processing Stack order result

The raw fixtures, runtime Stack observations, and command result are in [phase-d.md](../../../benchmark-results/tracktion-feasibility/phase-d.md).

| Criterion | Result |
| --- | --- |
| Framework-free Processor description | Pass — Domain processor ID, kind, parameter, and enabled state contain no Tracktion/JUCE type or runtime state. |
| Clip order and parameter mapping | Pass — Add `0.25` → Multiply `2.0` observes `1.0`; runtime order, IDs, and parameters match Domain. |
| Stopped-state reorder | Pass — Domain Multiply → Add reconstructs a new runtime and observes `0.75`; no reverse synchronization is used. |
| Track Processing Stack | Pass — after two-Clip Track Mix `0.5`, Add → Multiply observes `1.5`. |
| Master Processing Stack | Pass — Add → Multiply observes `1.0`. |
| Layer distinction | Pass — Clip Add → Track Multiply → Master Add observes `1.25`; each layer's runtime Stack is independently observed. |
| Destroy / reconstruct | Pass — re-creating the original Clip fixture retains order, parameters, and `1.0` output. |
| Runtime authority | Pass — runtime is discarded/rebuilt from unchanged Domain state; it never writes Domain state. |
| Update strategy | Pass for stopped scope — full runtime rebuild. Paused/playing update is intentionally unmeasured until Phase G. |

## 2. Dependency revisions and licence/NOTICE record

| Component | Exact revision | Status |
| --- | --- | --- |
| Tracktion Engine | `b88a6ee51913668cb53e911e030ab736b13342cf` | Prototype-only git submodule, built by the Phase 1 target. License: GPLv3-or-later or commercial. |
| Tracktion's JUCE gitlink | `37c894f83d379179b2070d437ccd0f1cd9af9576` | Nested Tracktion submodule, built by the Phase 1 target. License: AGPLv3 or commercial JUCE 8 license. |
| VST3 test plugin / SDK | Not selected | Deferred until deterministic PDC and Tail tests pass. |

The source notices at the pinned revisions were inspected and their bundled-dependency lists were recorded in the raw log. This is not legal advice and does not resolve redistribution obligations.

## 3. Test matrix

| Test | Phase | Status | Evidence / reason |
| --- | ---: | --- | --- |
| T-001 reconstruction | A | Pass | Framework-free fixture constructs, destroys, and reconstructs with equal observation; see `phase-a.md`. |
| T-002 integer sample round-trip | B | Pass | Three fixtures, 32 Clip observations, and 30 reconstructions report maximum error 0 samples; see `phase-b.md`. |
| T-003 overlap / Track Mix | C | Pass | Six headless fixtures observe basic signal flow, Track Mix, gain/pan, and internal 1.5 float sum; see `phase-c.md`. |
| T-004 Processing Stack order | D | Pass | Deterministic Add/Multiply runtime Stack mapping observes Clip `1.0`/`0.75` reorder, Track `1.5`, Master `1.0`, and layered `1.25`; see `phase-d.md`. |
| T-005 PDC | E | Inconclusive | No deterministic latency processor or VST3 plugin was implemented. |
| T-006 Effect Tail | F | Inconclusive | No deterministic Tail processor was implemented. |
| T-007 runtime editing | G | Inconclusive | Phase D verifies only stopped-state full rebuild for Stack reorder; paused/playing updates remain Phase G work. |
| T-008 long source | H | Inconclusive | Not started: outside this Phase B-only task. |
| T-009 dense edit | H | Inconclusive | Not started: outside this Phase B-only task. |
| T-010 headless / failure injection | I | Inconclusive | Bootstrap executable exists, but Phase I automation and failure injection are outside this task. |

## 4. Measurements

Only the bounded Phase A runtime-construction/reconstruction measurements exist. The following required measurements are explicitly unmeasured rather than assigned invented values:

| Measurement | Result |
| --- | --- |
| Runtime construction | Phase A Pass — Domain fixture constructed one runtime, then after destruction reconstructed to an equal observation. |
| Integer sample mapping | Phase B Pass — 32 observations across 48 kHz, long Timeline, and 44.1/48 kHz fixtures; maximum observed error = 0 samples. |
| Overlap / Track Mix | Phase C Pass — 2- and 3-Clip overlap sum correctly; 1.5 internal float sum is not hard-clipped. |
| Processing Stack order | Phase D Pass — deterministic Add/Multiply maps in Domain order to Clip/Track/Master Stacks: Clip Add→Multiply `1.0`, reordered Multiply→Add `0.75`, Track `1.5`, Master `1.0`, and three-layer path `1.25`. |
| PDC | Not executed — no deterministic latency processor; `alignment error = 0 timeline samples` was **not** established. |
| Effect Tail | Not executed — Source End, first Tail sample, last non-zero Tail sample, Processing End, and offline render end are unmeasured. |
| Tail Move | Not executed — the required `+48000` sample movement case is unmeasured. |
| Runtime editing | Not executed — no stopped/paused/playing update measurements. |
| Long source | Not executed — no construction, memory, streaming, seek, or throughput data. |
| Dense Clip | Not executed — no 5,000-Clip construction/update/scheduling data. |
| Headless execution | Phase 1 Pass — a device-free bootstrap executable was built and passed CTest; Phase I failure-injection coverage remains unmeasured. |
| Missing / failure state | Not executed — no runtime failure can be injected. |

## 5. Hard-criterion status

| Hypothesis | Status | Basis |
| --- | --- | --- |
| H1 Domain Independence | Pass (Phase A scope) | `MinimalDomainState` is framework-free and unchanged across runtime destroy/rebuild; runtime types are confined to the adapter/runtime. |
| H2 Runtime Reconstruction | Pass (Phase A scope) | Same domain fixture produces an equal runtime observation after destroy/reconstruct. |
| H3 Integer Timeline Sample | Pass (Phase B scope) | Integer Domain state remains authoritative through 30 reconstructions; all requested Timeline and Source fields observe at 0 sample difference. |
| H5 Basic Signal Flow | Pass (Phase C scope) | Headless float render verifies stated Source/Clip/Track/Master path, overlap summation, gain/pan mapping, and no immediate hard clipping. |
| H8 Runtime Editing | Pass (Phase D stopped scope only) | Domain-first stopped-state Stack reorder is represented by full runtime reconstruction; paused/playing updates remain unmeasured. |
| H6 PDC | Inconclusive | No deterministic test processor/runtime exists. |
| H7 Effect Tail | Inconclusive | No deterministic test processor/runtime exists. |

No hard stop was reached. Phases A/B/C/D passed their bounded criteria; H6 and H7 remain unmeasured.

## 6. Domain-boundary and scope review

No production source exists or was changed. The Phase A–D code is under `prototype/tracktion-feasibility/` only. No Tracktion, JUCE, or VST3 type, runtime identity, runtime serialization, pointer, or binary artifact was introduced into an AudioNLE Domain model.

No Project persistence, GUI, media import, synchronization-group/member behaviour, Clip Group behaviour, or Ripple semantics was delegated to Tracktion or implemented. The prototype's `Domain → Adapter → Runtime` direction was exercised for the Phase A–D fixtures. The adapter-owned source resolver is an input mapping table only; it is not Project persistence or authoritative state. The Phase D custom processor factory and temporary runtime state trees are adapter/runtime construction details. Runtime seconds and float render buffers are observations only and are never synchronized back to Domain State.

## 7. Required next limited action

Phase D is complete. The next execution-plan step is Phase E — PDC, beginning with the deterministic latency processor and requiring `alignment error = 0 timeline samples`. It was intentionally not implemented or executed in this task. This results document does not prescribe a production architecture or Tracktion adoption.

## 8. ADR readiness

Not ready. The decision rule permits no Tracktion ADR from an Inconclusive result, and this task intentionally creates no ADR.
