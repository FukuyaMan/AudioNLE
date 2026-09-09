# Prepared Conversion real-media artifact evidence

`option_b_prepared_src_real_media` uses the existing JUCE `AudioFormatReader` as an engine-private, worker-only bounded native-frame provider for controlled PCM16 WAV fixtures. Domain does not see JUCE reader types. Native read coordinates, artifact page starts, and project output frames are integers.

The rebuildable artifact format is `PreparedCacheFormatVersionV1`: fixed magic `ANLEPRP1`, format version, source/generation/stream/config fingerprint, source/project rates, reduced P/Q, channel/layout identity, page frame size, logical project frame count, then indexed 256-frame interleaved float32 page records with a per-page FNV checksum. Artifacts are written under a separate temporary/session cache root as `.tmp`, closed, then atomically renamed. `SRC_STATE`, filter state, pointers, and worker state are never serialized.

| real decoded fixture | output frames | pages | restart/reopen | max abs |
| --- | ---: | ---: | --- | ---: |
| mono PCM16 WAV 44.1 -> 48 | 16300 | 64 | PASS | 0 |
| stereo PCM16 WAV 48 -> 44.1 | 14699 | 58 | PASS | 0 |
| stereo PCM16 WAV 44.1 -> 96 | 17520 | 69 | PASS | 0 |
| stereo PCM16 WAV 96 -> 48 | 8000 | 32 | PASS | 0 |

Header/key mismatch (generation) and short corrupt artifact are rejected before payload use. The canonical key includes selected stream identity and channel/layout fields, so streams/layouts cannot share an artifact accidentally. The current controlled fixtures are WAV only: no FFmpeg-backed compressed or multi-stream container decoder boundary exists in this repository, so those paths are not claimed.

Derived float32 disk storage is approximately 691.2 MB/hour at 48 kHz mono, 1.3824 GB/hour at 48 kHz stereo, and 2.7648 GB/hour at 96 kHz stereo. Disk is permitted to scale with duration. The existing 16×256-frame RAM page window remains the callback-facing hot cache; this real-media artifact fixture validates persistence/reopen but does not yet connect disk page loading, pinning, or eviction into a production runtime. Those are remaining implementation gates, not passing claims.
