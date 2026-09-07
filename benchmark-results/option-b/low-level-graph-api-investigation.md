# Option B Low-level Graph API Investigation

The fixture uses only standard C++ (`unique_ptr`, vectors, fixed arrays). Tracktion/JUCE graph APIs are absent. AudioNLE owns Nodes, DAG edges, topological order, fixed per-node/edge buffers, latency propagation, PDC, and headless execution; A2 delegates those graph responsibilities to Tracktion. All allocations occur while constructing/preparing the fixture graph; processing uses fixed 128-frame arrays.
