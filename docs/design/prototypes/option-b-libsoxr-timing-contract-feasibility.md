# Option B libsoxr Timing-Contract Feasibility

## Result

**libsoxr Inconclusive; run libsamplerate comparator**.

The experiment pinned libsoxr `0.1.3` at revision `6e80f8b597c92afd3edf7e714a3b8d7b4534213d`, planned as static linkage with VHQ, linear phase, one runtime thread, OpenMP off, and no libsamplerate-compatibility bindings.

The current root CMake integration cannot configure that upstream revision as a `FetchContent` subproject. Its upstream `CMakeLists.txt` appends `${CMAKE_SOURCE_DIR}/cmake/Modules`, where `CMAKE_SOURCE_DIR` resolves to AudioNLE's root during subproject configuration; it then cannot include its required `SetSystemProcessor` module. CMake 4.3 additionally requires a compatibility-policy override for its old minimum version. No upstream/vendor patch, global CMake workaround, or manually copied source was added because those would expand this gate beyond backend timing compatibility.

Therefore no linked libsoxr target ran: L1--L18, including latency/preroll, fresh-state starts, worker/cache, callback instrumentation, quality, and lifecycle behavior, are unmeasured. The integer/rational timing contract and completed ZOH gate are unchanged.

The next authorised action is the preselected **libsamplerate timing-contract comparator**. A later libsoxr retry may first use a maintained package or an upstream CMake fix, then repeat this gate from L0; it must not infer timing compatibility from API documentation alone.
