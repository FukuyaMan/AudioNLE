# Realtime Source Q9 Graph-path Diagnostic

The original Q9 failure was a fixture expected-value bug. PCM16 encoding of marker `0.875` decoded as `0.874969`; the Multiply output was `1.74994`, exactly twice the observed decoded source, but differed from mathematical `1.75` by more than the source tolerance.

| Observation | Value |
| --- | ---: |
| Marker Source/Timeline sample | 10,000 |
| Requested fixture value | 0.875 |
| Source-node decoded output | 0.874969 |
| Multiply output | 1.74994 |
| Source output × 2 | 1.74994 |
| Timing error | 0 |
| Cache hit expected case | yes |
| Callback reader/wait/growth | 0 / 0 / 0 |

The source-alone control and Multiply output establish that cache/source output and graph wrapper are correct. Q9 now compares processor output against the source-alone decoded PCM value times two; it does not shift timing or manually correct output. The near and one-hour cases pass.

```text
REALTIME-SOURCE Q9-DIAG source=0.874969 expected=0.875 multiplied=1.74994 expected-multiplied=1.75
REALTIME-SOURCE ... Q9 multiply=pass
```

Classification: fixture expected-value bug; corrected bounded Q9 retry Pass. Q10 may now be considered under separate authorisation.
