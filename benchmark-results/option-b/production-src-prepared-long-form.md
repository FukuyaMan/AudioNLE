# Prepared SRC Long-Form Evidence

The deterministic fixture writes a sparse/generated artifact representing three hours at 48 kHz, then reads page 0, midpoint, and final page. It keeps no page-offset index: `index-bytes=0`; residency is at most 16 pages. Stereo slot PCM capacity is 256 × 2 × 4 = 2048 bytes; request table is eight fixed entries. Disk artifact extent scales with duration, while runtime cache and metadata do not.
