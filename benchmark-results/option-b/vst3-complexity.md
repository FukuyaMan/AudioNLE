# Option B VST3 Complexity

The bounded host uses local fixture discovery, instance ownership, prepare lifecycle, fixed mono process buffers, post-prepare latency extraction, and the existing graph PDC metadata. It is a **Small hosting extension** for this fixture, but dynamic latency invalidation/reprepare and non-mono layouts remain future host responsibilities. Fixture and assertions are not production LOC estimates.
