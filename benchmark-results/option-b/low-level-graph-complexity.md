# Option B Low-level Graph Complexity

`main.cpp` is approximately 66 dense prototype lines. Node/edge ownership, DFS topology/validation, fixed buffer flow, traversal, latency propagation and PDC are materially overlapping responsibilities; fixture assertions are interleaved and not a production LOC estimate. The bounded slice is a **Small bounded graph core**: it remains understandable and avoids plugin/media/device/lifecycle systems, but it demonstrates that Option B owns graph order, buffers and PDC that A2 reuses from Tracktion.
