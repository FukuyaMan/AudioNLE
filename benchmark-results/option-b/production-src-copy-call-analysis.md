# Copy / Clear / Call Analysis

Before optimisation, the wrapper offered 512 native frames and a 512-frame output target for every block, including 128-frame blocks. After optimisation it offers `ceil(projectFrames / ratio) + 4` prepared frames, requests exactly 128/256/512 output frames, and retains one bounded `src_process` call per active view. Fixed buffers are not dynamically cleared or allocated; only backend-written output range is consumed. Capacity remains 2048 native / 512 project frames.

Inactive configured views are not placed in the active processing set; callback cost scales with active view count. Output/frame determinism, generation rejection and forbidden-operation counters remain unchanged.
