# Shared native source-runtime ownership evidence

`SharedNativeSourceRuntime` owns `FixtureNativeProvider` and its `NativeSourceService`. Two `SourceRuntime` instances constructed with one source/generation identity receive the same shared object. Their first realtime callbacks use already-published shared native PCM and do not add provider reads; each owns a separate `SRC_STATE` and records its own realtime call.

The focused route fixture destroys one of those views and verifies that the remaining view continues to render. The shared runtime is held by `shared_ptr` only on the control/lifecycle side; no callback constructs, destroys, or reconfigures it.

PASS: `option_b_source_runtime_shared_ownership`.

Prepared artifact residency still has an independent runtime ownership path, so a fully shared native/prepared source runtime and common worker fairness are not claimed here.

PreparedRequired views now also accept one shared residency binding. The binding checks its artifact material key (source, generation, config, channels, project rate), so Timeline placement cannot affect it and an incompatible runtime cannot be reused. Two cold views for the same artifact coalesce their page-load demand; after worker drain, both read the resident page. Destroying one view leaves the other ready. The worker topology/fairness across native and prepared queues remains separate and is not claimed by this result.
