# Option B Production-Oriented SRC Component Planning

## Scope and baseline

This is a design/evidence plan, not production code or a production-backend decision. The selected prototype is static libsamplerate 0.2.2, `SRC_SINC_BEST_QUALITY`, caller-buffer streaming, validated only for 44.1 <-> 48 kHz. The parent selection is **Proceed with Constraints**.

AudioNLE authority remains integer Project Timeline coordinates, integer native Source coordinates, and exact rational Project/Source relation. Filter state, delay, history and output production are physical runtime details only.

## Intended component boundary

```text
Domain / ClipRuntimeView
  -> authoritative integer/rational mapping
  -> PhysicalSrcRangePlanner
  -> NativeSourceService (worker + bounded pages)
  -> BandlimitedSrcRuntime
  -> fixed output staging
  -> SourceNode / Option B graph
```

`ClipRuntimeView` contains Domain-derived identity, logical range and generation, never backend state. `PhysicalSrcRangePlanner` derives bounded native input from integer coordinates. `NativeSourceService` satisfies prepared page requests away from callback. `BandlimitedSrcRuntime` is an engine-private adapter around a prepared backend state. Fixed staging bridges runtime output to the SourceNode graph.

## Responsibility ownership

| Concern | Owner |
| --- | --- |
| Logical Source coordinates and Clip ownership | Domain / `ClipRuntimeView` |
| Project Timeline coordinates and exact rational relation | Domain mapping service |
| Rational phase and physical start calculation | `PhysicalSrcRangePlanner` |
| Empirical preroll and output discard | Planner + runtime wrapper, versioned by backend configuration |
| Backend state create/reset/delete | `BandlimitedSrcRuntime`, stopped/control thread only |
| Cache request/range planning and generation stamps | `NativeSourceService` + planner |
| Source-start padding policy | Planner/wrapper; never Domain coordinate mutation |
| End-of-source flush and logical clipping | Runtime wrapper/staging |
| Generation invalidation/reconstruction | `ClipRuntimeView` lifecycle orchestrator; stale state discarded |
| Callback input/output buffers | Runtime/staging, fixed and preallocated |
| Metrics and instrumentation | Engine diagnostics boundary; callback-safe counters only on callback |

## Carried prototype constraints

* Preroll is `1024 native frames + phase rounding`, bounded but empirically chosen.
* Evidence is specific to `SRC_SINC_BEST_QUALITY`.
* Converted-rate evidence is limited to 44.1 <-> 48 kHz.
* AudioNLE forbidden callback operations were zero in the prototype.
* libsamplerate internal allocation/lock safety is partially unproven.

None is a production guarantee. A converter mode or rate change invalidates the relevant empirical configuration record until re-tested.

## Future evidence gates

1. **Configuration matrix gate** — sample rates, ratio reduction, converter-mode policy, configuration-versioned preroll and numerical/spectral thresholds.
2. **Production realtime audit gate** — source audit plus allocator, lock/wait interception, long callback stress and deterministic repeat runs.
3. **Performance and density gate** — CPU, working set, cache pressure and prepare/process separation at realistic multi-view densities.
4. **Cache-pressure and long-form gate** — bounded eviction/prefetch, page misses, multi-hour mixed-rate reconstruction and failure recovery.
5. **Boundary-policy gate** — source-start padding, source end, flush/clipping and explicit no-Effect-Tail semantics.
6. **Reliability/reconstruction gate** — error propagation, corrupt/unavailable source behavior, generation races, project save/load versioning and state recreation.
7. **Release gate** — BSD-2-Clause notice/package audit, third-party source pin/reproducibility, production quality/performance thresholds.

## Planned sample-rate matrix

| Tier | Rates and intent |
| --- | --- |
| Common production paths | 44.1, 48, 96 kHz; both directions, including 44.1 <-> 48 |
| Common import/archive paths | 32, 88.2, 192 kHz against 44.1/48/96 as applicable |
| Unusual but supported candidates | 32 <-> 192, 44.1 <-> 96, 48 <-> 88.2/192, subject to cost limits |
| Deliberately unsupported until evidence exists | live dynamic ratio change, arbitrary mode switching on callback, and rates/configurations without a recorded bounded planner policy |

Every accepted pair needs phase, seek/split/trim/rebuild, source boundary, quality and cache-pressure coverage; no result is inferred merely from a similar ratio.

## Stronger realtime-audit plan

Before any stronger claim, audit pinned source/transitive runtime paths; interpose or otherwise record allocation/free/realloc and Windows heap activity around prepared `src_process`; intercept mutex/critical section, waits, sleeps and blocking OS calls; run long callback-like stress across block sizes and view density; and repeat runs for deterministic frames/results. Results must distinguish observed absent, observed present, and unproven. State lifecycle remains outside callback regardless of outcome.

## Performance plan

Measure CPU per active view at 1/4/8/16/32 mixed-rate views; 128/256/irregular process blocks; cache hit and pressure/eviction behavior; resident working set; worst nonzero-start range preparation; and stopped/control-thread state/cache preparation cost. Report p50/p95/worst bounded process time separately from prepare time. Test overlapping/disjoint view mixes and long-form seeks; do not amortise control work into callback results.

## Proposed internal API shape

```text
BandlimitedSrcRuntime::configure(BackendConfig, ViewFormat)          // control thread
BandlimitedSrcRuntime::plan(IntegerTimelineRange, ViewCoordinates)  // yields PhysicalInputPlan
BandlimitedSrcRuntime::prepare(PhysicalInputPlan, Generation)       // control thread
BandlimitedSrcRuntime::process(PreparedNativeInput, FixedOutput)    // callback, bounded
BandlimitedSrcRuntime::requiredCapacity(ViewFormat)                 // control thread
BandlimitedSrcRuntime::invalidate(Generation)                       // discard/reconstruct lifecycle
BandlimitedSrcRuntime::reconstruct(ViewCoordinates, Generation)     // control thread
```

`BackendConfig`, `ViewCoordinates`, `PhysicalInputPlan`, `PreparedNativeInput` and `FixedOutput` are engine types. Domain APIs expose logical coordinates and output only; they do not expose `SRC_STATE`, libsamplerate ratios, filter delay or backend buffers.

## Dedicated ADR entry criteria

Create a production SRC-backend ADR only after the configuration matrix, versioned preroll policy, production realtime audit, density/performance evidence, boundary/error/reconstruction policy, release licensing and production thresholds are accepted. It must decide whether libsamplerate remains the backend, whether the empirical preroll risk is acceptable, whether realtime evidence is sufficient, and whether libsoxr/r8brain comparison is required.

## Revisit rules

Revisit on new ratio/mode invalidating preroll; requirement for formal delay/filter-support guarantees; stronger realtime requirements; unacceptable deeper audit findings; production quality/performance failure; disproportionate integration cost; viable libsoxr integration; or a backend that materially simplifies deterministic reconstruction.

## Recommended next implementation gate

Run a narrowly scoped **production SRC component skeleton and configuration-matrix gate**: introduce no Domain timing change, first establish engine-private boundary types and control-thread lifecycle seams, then exercise the common 44.1/48/96 kHz matrix with configuration-versioned planning. It must stop before callback integration if the deeper realtime audit and fixed-capacity design are not demonstrably supportable.
