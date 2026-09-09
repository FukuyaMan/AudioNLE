# Production SRC Density Performance

Release `steady_clock` callback-only measurements, microseconds per block. Representative 44.1->48 shared-source results: 1 view/128 worst 0.73 ms (0.27 block); 4/128 1.25 ms (0.47); 8/128 2.69 ms (1.01); 32/128 11.32 ms (3.90); 32/256 12.45 ms (2.15); 32/512 13.96 ms (1.20). The 32-view distinct-source 256-frame case worst was 14.13 ms (2.43 blocks). Smaller 48->44.1 1/8/32-view sanity workload showed the same linear-cost direction.

Timing is variable and machine-specific; no product deadline is invented. It nevertheless demonstrates insufficient worst-case 32-view headroom in this fixture. Classification: **Density performance requires optimisation before production direction**.
