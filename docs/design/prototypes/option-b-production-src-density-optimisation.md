# Production SRC Density Optimisation Evidence

The dominant cost is one `SRC_SINC_BEST_QUALITY` process call per active view, not scheduling, cache requests, lifecycle, or full-capacity clearing. The previous fixture incorrectly presented 512 native frames to every view regardless of graph block size. The bounded optimisation now presents only `ceil(projectFrames / ratio) + 4` prepared frames and requests exactly the graph block's output frames, while retaining 2048/512 fixed capacities and one call per active view.

This removes avoidable DSP/input work without changing converter mode, timing authority, ownership, allocation, or output staging lifetime. It is equivalent to bounded call coalescing/used-range utilisation; no dynamic buffer, zero-copy lifetime change, or scheduler redesign was introduced.

At 32 shared views, worst headroom improved from 3.90 to 0.69 (128), 2.15 to 0.64 (256), and 1.20 to 0.74 (512). Lower densities have clear headroom; 32 views are now below block duration in the measured fixture but remain a constrained upper-density result pending wider stress. Classification: **Density performance improved but 32-view constraint remains**. Gate: **Production SRC density optimisation Proceed with Constraints**.
