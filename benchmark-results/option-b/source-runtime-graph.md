# SourceRuntime through the Option B low-level graph

`option_b_low_level_graph` now compiles the same engine-private source-runtime route used by the routing and IAT targets. `runtimeSource` calls `SourceNode::process` into the graph node's fixed caller-owned block buffer. It has no provider, artifact file, worker, decoder, or libsamplerate ownership.

The mixed fixture connects native-rate, 44.1-to-48 kHz realtime BEST, and PreparedRequired sources to the existing normal `sum` node. On the first graph render the prepared source is cold, so it writes exact zeros while the native and realtime graph inputs remain non-zero. After `SharedSourceWorker::drain`, the subsequent graph render differs because the prepared resident page is included; graph topology is unchanged. The sum is floating-domain addition and has no hard clipping stage.

PASS: `option_b_low_level_graph`.

This does not yet prove a common native/prepared worker fairness policy or transition/long-form coverage.
