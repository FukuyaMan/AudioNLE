# Option B Generated Worker/Cache Identity Trace

Fixture-local provider emits `float(S)` exactly for Source `S=0..511` (all integers in this range are exactly representable in `float`). A worker fills two fixed 257-frame native pages; the callback copies only published cache values and processes the AudioNLE-owned integer-rational ZOH across 128-frame graph blocks.

```text
T0..400: 401 matched, 0 mismatched
mismatch positions: none
exact-boundary mismatches: 0
fractional mismatches: 0
worker/cache sequence == corrected direct sequence: yes
callback generator/wait/allocation/fallback: 0
resets at graph block boundaries: 0
```

Cache tokens are exact at Source 0..5, 143..151, 255..258, and 290..298, including the Source 257 page transition.

| T | Expected | Direct | Worker/cache |
| --: | --: | --: | --: |
| 159 | 146 | 146 | 146 |
| 160 | 147 | 147 | 147 |
| 161 | 147 | 147 | 147 |
| 319 | 293 | 293 | 293 |
| 320 | 294 | 294 | 294 |
| 321 | 294 | 294 | 294 |

Classification: **Worker/cache corrected identity trace passes**. The primitive has fixed rational state; callback counters remain generator/wait/allocation/fallback = 0.
