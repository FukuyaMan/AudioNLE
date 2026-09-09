# libsoxr realtime evidence

Not run.  The Windows IAT harness is available and validates CRT, heap, locks, waits/sleep, and file-I/O routes for the existing production SRC comparator, but RT0/RT1 are gated behind a passing libsoxr quality candidate.  libsoxr VHQ fails the mandatory six-direction quality gate, so an interception result would not make it a production candidate.

Source inspection shows creation/initialisation allocates opaque state and filter/buffer storage; that lifecycle is incompatible with callback creation and would have to occur in preparation.  No claim is made here about steady `soxr_process` allocation/lock behaviour until an IAT run is performed.
