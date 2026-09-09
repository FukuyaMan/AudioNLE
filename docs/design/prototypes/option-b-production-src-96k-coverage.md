# Production SRC 96 kHz Coverage

`SRC_SINC_BEST_QUALITY` with PrerollPolicyV1 passes the bounded comparator for 44.1->96 (320/147), 96->44.1 (147/320), 48->96 (2/1) and 96->48 (1/2). Each uses continuous-reference fresh reconstruction, exhaustive output phase classes, nonzero/source starts, compact lifecycle/edit/cache/partition/reconstruction fixtures, fixed 2048/512 capacity and generation rejection. No opaque state is required.

All four rows are `ValidatedV1`. This covers configuration evidence only; deep realtime proof, release/licensing and production thresholds remain ADR blockers.
