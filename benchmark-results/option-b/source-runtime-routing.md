# Source Runtime Routing Evidence

`option_b_source_runtime_routes` passes a mixed fixture with one native-rate, one 44.1-to-48 kHz GuaranteedRealtime BEST, and one PreparedRequired artifact source. All feed the same caller-owned project-rate output contract. Native has zero SRC/prepared calls; realtime has SRC calls; prepared has zero realtime SRC calls. A cold prepared page is silence/`Unavailable`; fixed round-robin worker service reloads it.

The fixture also passes move (timeline-only), split (shared source), and fresh prepared-runtime reconstruction. The worker registry is fixed at eight sources and creates no worker thread per source/artifact/clip.

`option_b_windows_iat_crt_harness` additionally executes this actual runtime route under its existing callback marker. Native Ready, realtime Ready, prepared cold/reload, and mixed native/realtime/prepared rendering pass with zero CRT, heap, lock, wait/sleep, and file-I/O observations. Route counts establish that only realtime invokes `src_process`.
