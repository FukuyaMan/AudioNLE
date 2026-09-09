# Production SRC Error Semantics

Prepared unavailable/stale/capacity/backend/process/unsupported/provider failures map to engine-private categories. Callback returns deterministic silence/underrun for unavailable prepared input and increments callback-safe counters; detailed logs are formatted off callback. Repeated preparation failure storms discard stale state, then recover only after valid current-generation preparation; no lifecycle or Domain mutation occurs on callback.
