# Option B Mixed-Rate SRC Step Observation Corrective Gate

## Canonical observation

For a Source boundary `S`, native samples before `S` are PCM16-decoded zero and samples at/after `S` are PCM16-decoded positive step values. The observation is the first project-rate output with the post-edge held value. Isolated-impulse first-nonzero timing is excluded from edit-boundary evidence.

## Results

| Path | S | Authoritative T | Observed step T | Error |
| --- | --: | --: | --: | --: |
| Direct generated ZOH phase probe | 147 | 160 | 160 | 0 |
| PCM16 WAV worker/cache/ClipRuntimeView/SourceNode | 147 | 160 | 161 | +1 |

The real PCM16 fixture is written with samples `[0, 147)` equal to zero and `[147, end)` equal to 0.5; it is decoded through the existing reader and fixed native page cache. Its old isolated-impulse observation is retained only as historical diagnostics and is not used above.

## Classification

**Observation defect fixed but streaming alignment defect remains.** The valid step observation still disagrees with the authoritative mapping in the established streaming path. No mapping bias, trim, preroll, primitive change, or cache correction was applied.

Native buffer differential, other source boundaries, render-start, page/block, continuous-state and seek/reset checks are deliberately not treated as passing: the first valid real streaming case already fails. Callback reader/wait/allocation/fallback counters remain zero for the worker/cache path.

## Next gate

**streaming SRC input/output anchor diagnostic**: trace the cache-copied native absolute indices, `srcInput[0]`, ZOH input consumption, output origin and absolute observation index for this exact step case before making any correction.
