# Prepared Conversion / Proxy Cache V1 evidence

`option_b_prepared_src_policy` implements the accepted ADR 0002 static table: native-rate bypass, GuaranteedRealtime <=4 at 256 frames, the exact per-direction BestEffort/PreparedRequired thresholds, and UnsupportedConfiguration for unvalidated block/rate pairs.

`option_b_prepared_src_cache` is the first engine-private vertical slice. Its `Key` contains stable source identity, source generation, backend/config fingerprint, source/project rate, reduced P/Q, and channel identity. The test configuration fingerprint represents pinned libsamplerate BEST + `PrerollPolicyV1`; production identity must be a canonical serialization of those fields, never a pointer or opaque `SRC_STATE`.

The storage is an ephemeral, rebuildable 16-page fixed in-memory window of 256 project-rate float frames (16 KiB total PCM). A worker produces project-rate float PCM with libsamplerate BEST, fills a private page, then release-publishes a complete immutable page. Callback lookup performs acquire reads only. It returns `Ready`, `Unavailable`, `Failed`, or `Stale`; a range spanning an unpublished page is entirely unavailable, never partial audio. Callback fallback is deterministic silence/underrun plus diagnostics, with no realtime BEST fallback.

| fixture | result |
| --- | --- |
| 44.1 -> 48, 44.1 -> 96, 96 -> 44.1 direct BEST comparison | PASS, page seam max abs 0 |
| repeated/forward-backward lookup and shared-page reads | PASS |
| partially available spanning read | `Unavailable`, no partial publication |
| injected worker failure | `Failed`, no publication |
| source-generation and project-rate key mismatch | `Stale`, rejected |
| retry on current generation | PASS |
| 3-hour project coordinate (518,400,000 samples) | PASS; 16 resident pages, 1024-byte page, 1024 native/output worker blocks |
| Windows IAT prepared playback | PASS; CRT/heap/lock/wait/file events 0, `src_process` callback calls 0 |

The synthetic reference vector is test-only. The production representation remains bounded streaming worker input/output and a bounded cache window; it does not retain duration-scaled PCM in RAM. Persistent disk artifacts are intentionally not implemented in this V1 slice. They would be rebuildable derived cache data with a versioned header and the same key validation, never serialized backend state or source edits.
