# 96 kHz Capacity / Runtime Sanity

2048 native / 512 project fixed capacities cover 128/256/512 and irregular prepared partitions in the comparator. The density request rule remains `ceil(projectFrames/ratio)+4`, with exact project output request. Prepared cache callback counters remain zero; stale generation/configuration prepared state is rejected. Performance/quality sanity is finite and deterministic; this is not a replacement for production threshold/performance gates.
