# Option B ZOH Raw Source-Identity Trace

Identity fixture: native Source sample `S` is represented by exactly representable float token `float(S)` for the tested range. A persistent ZOH stream runs from Source 0 with no reset near boundaries.

For Timeline 0--400, `HeldSource(T)=floor(T*147/160)` matches 399 samples. The only mismatches are the exact rational boundaries T160/Source147 and T320/Source294; fractional-region mismatches are zero.

```text
T157 expected 144 actual 144
T158 expected 145 actual 145
T159 expected 146 actual 146
T160 expected 147 actual 146
T161 expected 147 actual 147
T162 expected 148 actual 148
T163 expected 149 actual 149
```

This confirms a one-project-sample-late advance at an exact rational boundary in the direct persistent ZOH sequence, while adjacent non-boundary samples are correct. The pinned loop advances only under `while (pos >= 1.0)`; the exact crossing requires a later detailed phase trace.

Worker/cache identity, page geometry and block-partition comparisons remain unexecuted, so the root-cause classification remains **Inconclusive**. No correction is applied. The required next diagnostic is worker/cache reproduction of this identity trace; only after that may **derive bounded exact-boundary ZOH initialization/advance correction** proceed.
