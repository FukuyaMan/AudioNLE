# Prepared SRC Disk Residency Evidence

`option_b_prepared_src_disk_residency` uses persistent fixed-record float32 artifacts. Header/key validation occurs off callback; page address is arithmetic and its RAM page-index cost is zero bytes. A cold callback request zeros the full requested range and returns `Unavailable`; the worker later reads, validates checksum, privately fills, and publishes an immutable page.

The fixture verifies duplicate request coalescing, two-page stereo seam copies, all-or-unavailable behavior, shared-view reuse, corruption rejection, generation-stale rejection, reopen, and a callback/worker publication stress loop. It contains no realtime SRC fallback.
