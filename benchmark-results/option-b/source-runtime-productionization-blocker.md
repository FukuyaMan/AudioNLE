# SourceRuntime productionization blocker

Inspection on 2026-09-09 found that the root project is named `AudioNLEPrototypeBootstrap` and only adds `prototype/tracktion-feasibility`, `prototype/tracktion-selective-reuse`, and `prototype/option-b-low-level-graph`. No engine or application source-runtime library target exists.

The current runtime is defined inside `source_runtime_routes.cpp` with fixture-local types. `option_b_source_runtime_routes` compiles it as an executable, while `option_b_low_level_graph` and `option_b_windows_iat_crt_harness` include the `.cpp` directly to obtain those types. This makes their runtime states translation-unit-local and prevents one production worker/snapshot owner from owning the native and prepared request categories across graph/IAT/runtime use.

The fixed `SharedSourceWorker` is therefore test orchestration, not a production shared worker: it iterates fixture `SourceRuntime` pointers and calls separate service methods. It cannot select category-owned pending work, impose demand-over-prefetch policy, report cross-category service gaps, or coordinate atomic G1-to-G2 publication across all source resources.

The remaining gate cannot be validly completed by adding more fixture tests. It requires a real engine-private source-runtime target and extraction of `SourceRuntime`, prepared residency binding, shared worker, and `SourceNode`; graph and IAT targets must link that target. Until then the correct classification is `Production source runtime architecture requires redesign`.
