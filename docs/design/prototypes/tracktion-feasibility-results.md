# Tracktion Engine Feasibility Prototype — Results

## Recommendation

**Inconclusive overall; Phase 1 — Build bootstrap: Pass; Phase A — Framework boundary and reconstruction: Pass; Phase B — Integer Timeline Sample: Pass; Phase C — Basic Signal Flow: Pass; Phase D — Processing Stack order: Pass; Phase E — PDC: Inconclusive.**

The original Phase 1 environment block has been resolved. The pinned Tracktion/JUCE sources configure, build, and launch in a prototype-only headless executable on this Windows environment. Phase A established the bounded framework boundary and reconstruction path without making Tracktion runtime state authoritative. Phase B demonstrated zero-error integer-sample observation. Phase C demonstrated the stated basic signal-flow and Track Mix fixtures in a headless float-buffer observation point. Phase D then mapped framework-free deterministic Processor descriptions, in order, to Clip, Track, and Master runtime Stacks. Phase E added a deterministic latency-processor fixture, but its no-latency baseline offline render did not complete within the bounded 10-second range guard, so no PDC alignment measurement was produced. The overall feasibility conclusion remains Inconclusive. No ADR is created.

The execution plan requires each phase to pass before the next begins. This result does not infer any Phase F–I behaviour from the bootstrap or Phases A–D targets.

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

## 1.6 Phase E PDC result

The raw fixture and bounded-render evidence are in [phase-e.md](../../../benchmark-results/tracktion-feasibility/phase-e.md).

| Criterion | Result |
| --- | --- |
| Deterministic latency processor | Inconclusive — framework-free description and adapter-local 1024/2048 sample implementation exist, but runtime processing was not reached. |
| Baseline offline render | Inconclusive — no-latency 4096-sample requested range exceeded the 10-second guard. |
| Clip / Track PDC | Unmeasured — no completed baseline buffer exists for integer comparison. |
| Master latency | Unmeasured — no completed render buffer exists. |
| Mixed layer, bypass, latency change, reorder | Unmeasured — these require the same blocked render path. |
| Reconstruction | Unmeasured for PDC — the Phase A reconstruction result remains valid, but no Phase E signal/timing result exists. |
| Maximum alignment error | Unmeasured; it is not represented as zero or an approximate result. |
| VST3 latency-reporting path | Not attempted — E1 did not pass; no scanner, GUI, or dependency was added. |
| Phase E disposition | Inconclusive — no workaround is accepted, and Phase F does not start. |

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
| T-005 PDC | E | Inconclusive | Deterministic processor fixture exists, but the required baseline offline render timed out before an alignment sample could be observed; see `phase-e.md`. |
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
| PDC | Phase E Inconclusive — deterministic 1024/2048 sample processor fixture was added, but baseline offline render did not complete within 10 seconds. `alignment error = 0 timeline samples` was **not** established. |
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
| H6 PDC | Inconclusive | Deterministic processor/runtime mapping exists, but no completed offline baseline buffer permits a PDC alignment measurement. |
| H7 Effect Tail | Inconclusive | No deterministic test processor/runtime exists. |

No H6 hard fail was established: no completed deterministic path produced a non-zero alignment error. However, Phase E did not pass because the headless offline baseline did not complete; H6 and H7 remain unresolved, and Phase F must not start.

## 6. Domain-boundary and scope review

No production source exists or was changed. The Phase A–D code is under `prototype/tracktion-feasibility/` only. No Tracktion, JUCE, or VST3 type, runtime identity, runtime serialization, pointer, or binary artifact was introduced into an AudioNLE Domain model.

No Project persistence, GUI, media import, synchronization-group/member behaviour, Clip Group behaviour, or Ripple semantics was delegated to Tracktion or implemented. The prototype's `Domain → Adapter → Runtime` direction was exercised for the Phase A–D fixtures. The adapter-owned source resolver is an input mapping table only; it is not Project persistence or authoritative state. The Phase D custom processor factory and temporary runtime state trees are adapter/runtime construction details. Runtime seconds and float render buffers are observations only and are never synchronized back to Domain State.

## 7. Required next limited action

Phase E is Inconclusive. Before any later Phase, only a bounded root-cause retry for the Phase E headless offline baseline/render path is appropriate; it must establish a completed deterministic buffer and then `alignment error = 0 timeline samples`. Phase F is not authorized by this result. This results document does not prescribe a production architecture or Tracktion adoption.

## 8. ADR readiness

Not ready. The decision rule permits no Tracktion ADR from an Inconclusive result, and this task intentionally creates no ADR.

## 9. Phase E limited renderer diagnostic

The raw diagnostic record is in [phase-e-renderer-diagnostic.md](../../../benchmark-results/tracktion-feasibility/phase-e-renderer-diagnostic.md). It did not rerun PDC or any later Phase.

The Phase E adapter omitted the adapter-owned `Edit::filePathResolver` that Phases C/D provide through their source registry. A minimal migration restored a Phase C-equivalent baseline, then the 4096-sample source/range, Timeline event 1024, and 4096-sample Clip baseline. Each completed within 471 ms. Removing only the resolver timed out at 10 seconds after 708110 `RenderTask::runJob()` calls, with one receiver reset but zero delivered blocks and no task error.

This establishes a prototype adapter/harness source-resolution defect, rather than a requirement for a physical device, worker pool, GUI, or Tracktion Engine change. The Phase E PDC fixture remains **Inconclusive** because it was deliberately not rerun and `alignment error = 0 timeline samples` remains unmeasured. A narrowly bounded Phase E retry may add the adapter-local resolver, retain the 10-second guard, and run deterministic PDC before Phase F is considered. Phase F is not authorized by this diagnostic.

## 10. Phase E retry result

The retry installed the Phase C/D-style adapter-owned `Edit::filePathResolver` and reran only the required no-latency baseline. The render completed and delivered the full requested 4096-sample receiver range without a physical audio device. However, its impulse was observed at Timeline sample `1026`, rather than the Domain event sample `1024`: baseline timing error = `2` Timeline samples.

Pinned-source inspection associates this with Tracktion's default Lagrange reader path, which uses an interpolation base latency during reset. No adapter compensation, Domain offset, render-range shift, or altered source/Timeline mapping was accepted. Such a policy would need independent specification and verification and cannot be silently introduced to make this feasibility fixture pass.

Phase E therefore remains **Inconclusive**. H6 is not yet a hard Fail because no deterministic processor with a correct latency report reached a PDC comparison; H9 offline-render timing is also Inconclusive. The baseline gate prevents Clip/Track/Master/mixed/bypass/latency-change/reorder/reconstruction PDC cases and E2 VST3 from running. Phase F remains unauthorized.

## 11. Phase E source/resampler baseline latency diagnostic

The raw evidence is in [phase-e-source-latency-diagnostic.md](../../../benchmark-results/tracktion-feasibility/phase-e-source-latency-diagnostic.md). The no-plugin offset is not a stable `+2`: 48 kHz source/Project observations vary between `0` and `+2` samples as event position and render start change. 44.1/48 kHz and 96/48 kHz paths observe `+1`. The public `lagrange`, `sincFast`, `sincMedium`, and `sincBest` Clip resampling qualities all observe `+2` for the Phase E event-1024, render-start-0 fixture.

Pinned-source inspection shows that the Clip source always enters a resampler reader and that the Lagrange path uses an interpolation base latency during reset. This latency is not exposed through the fixture's plugin or graph latency report. Tracktion renderer graph-latency handling therefore provides no observed compensation at the Master Output.

No Domain coordinate shift, magic adapter correction, output-buffer shift, source patch, or production SRC policy was added. Consequently a valid zero-error no-plugin reference is unavailable and PDC retry is not currently possible. H6 PDC and H9 offline-render timing remain **Inconclusive**; they are not promoted to Pass or Fail without a completed correct-latency PDC comparison. Phase F and actual VST3 remain unauthorized.
