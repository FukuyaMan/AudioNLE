# NativeSourceService request-table evidence

The engine-private service uses two fixed 2048-frame mono PCM pages (16,384 bytes) and eight fixed request records. A callback miss does not call `NativeSourceProvider::read`; it scans the fixed page/request arrays, coalesces a same-source/same-generation/same-page request, or increments `queueFull` and returns unavailable/silence. `serviceOne()` is the exclusive provider-read path.

`option_b_native_source_service` passes with eight unique requests, duplicate coalescing, saturated-table diagnostics, worker drain, and a recovered subsequent request. The service publishes generation-tagged pages and rejects a prior-generation page after reconfiguration.

This evidence does not establish cross-source fairness, shared service ownership between `SourceRuntime` objects, or a formal starvation bound; those remain active integration work.
