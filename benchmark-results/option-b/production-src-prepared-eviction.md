# Prepared SRC Eviction Evidence

The residency fixture loads 24 pages through a fixed 16-slot window. Worker-only least-age selection evicts only Ready, unpinned slots; Loading and pinned slots are never candidates. Re-requesting an evicted page first returns `Unavailable`, then reloads bit-identical artifact PCM. A held pin remains readable while other pages are admitted.

Demand requests are selected before one-page speculative prefetch. Prefetch does not recursively prefetch, preventing sequential work from monopolising the fixed queue.
