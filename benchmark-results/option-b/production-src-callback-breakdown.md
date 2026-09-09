# Callback Breakdown

Prepared active-view iteration is bounded and negligible relative to DSP in this fixture. Each active view performs one `src_process` call; no cache request, backend lifecycle, allocation/growth, wait, or fallback occurs on callback. Input gathering uses the prepared fixed buffer; output staging is exactly the requested graph block range. Graph summing is not material in this standalone skeleton fixture and needs graph-level confirmation later.

Per-active-view cost remains approximately linear through 32 views after correcting input/output utilisation. Indirect locality evidence is the modest shared/distinct 32-view difference (3.74 vs 3.79 ms at 256), not a hardware-counter proof.
