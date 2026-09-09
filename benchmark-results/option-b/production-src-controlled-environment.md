# Production SRC Controlled Environment

Release diagnostic host: Windows 11 Home 10.0.26200 build 26200; AMD Ryzen 9 7900X (24 logical processors); 31.12 GiB RAM; MSVC 14.51.36231 `/O2`; `steady_clock`; High Performance power scheme; normal process priority; initial process affinity mask `0xFFFFFF`. The diagnostic controlled mode temporarily constrained only the benchmark process to the lowest set affinity bit. It did not raise realtime priority and is not release evidence. Background-workload and CPU-frequency telemetry were not available; frequency correlation is unproven.

The 1,024-block short matrix showed migrations in product-like mode (3--24 changes in the sampled configurations). Fixed affinity recorded zero migrations but did not improve worst-case performance: 44.1->96 at 8 views changed from 0.490500 to 0.520463; at 16 views from 0.794400 to 1.344980. For 96->44.1 it changed from 0.463257 to 0.584497 at 8 views and from 0.924980 to 1.314940 at 16 views. Classification: product-like placement has migration, but migration/environment noise is not the primary measured cause because fixed placement was worse.

The High Performance scheme was already active for both modes, so this is not a normal-versus-high-performance comparison. No conclusion about frequency transitions is claimed.
