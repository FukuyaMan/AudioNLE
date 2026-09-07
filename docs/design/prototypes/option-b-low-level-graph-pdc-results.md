# Option B Low-level Graph / PDC Results

## Result

Gate: **Proceed with Constraints**. Complexity: **Small bounded graph core**.

The isolated standard-C++ target has no Tracktion link, include, type, or source reference. It owns framework-free descriptions, DAG Node/edge ownership, DFS preparation, fixed 128-frame buffers, summing, deterministic processing, latency and graph-derived edge PDC. B1 source placement, B2 ordered processing, B3 N-input summing, B4 declared/actual latency, B5 PDC, and B6 reconstruction passed with maximum timing error 0 and process-time allocation/growth 0.

Compared with A2, this proves the minimal graph/PDC slice can be small, but A2 still delegates traversal, buffer flow, summing, PDC and headless execution to Tracktion. This does not compare VST3 hosting/lifecycle, Tail, realtime cache, FFmpeg, device, live mutation, or production multi-source scaling.

Architecture implication: Option B deserves broader evidence; next step: **broaden Option B with VST3/PDC hosting fixture**. This is not a production architecture decision.
