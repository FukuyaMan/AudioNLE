# Tracktion Selective Reuse — Realtime Source Q4–Q8 Stress Evidence

Execution date: 2026-09-07 (Asia/Tokyo)

## Page ownership/publication protocol

Each fixed page has PCM, Source start, generation, `Empty`/`Writing`/`Ready` state, and an atomic callback-reader count. The worker claims `Writing`, waits outside the callback until the reader count is zero, writes PCM and metadata, then release-publishes `Ready`. The callback increments its reader count, rechecks `Ready`/Source/generation, copies only then decrements and signals the worker. Seek changes generation eligibility without overwriting a page.

This establishes the prototype invariant that the worker writes a page only when no callback can concurrently copy its PCM. No callback mutex, wait, spin, reader access, or allocation/growth is used.

## Results

| Phase | Observation | Result |
| --- | --- | --- |
| Q4 | 50 held-worker rapid seek sequences (`A -> B -> C -> A -> near-end`) | final current marker exact; pending output exact zero; no stale output or old publication |
| Q5 (partial) | 128-frame process boundaries and 256-frame page edges at 127/128, 255/256/257, and 383/384/385 | exact expected output; maximum error 0 |
| Q6 | 345,600,106-byte one-hour WAV; fixed 8192-byte PCM cache | working set 8,605,696 before, 9,035,776 after sequential/rapid seek, 9,039,872 after boundaries; no duration-scaled cache allocation |
| Q7 | 20 idle and held-pending worker destruction cycles | bounded join completed; no observed hang or runtime/Domain mutation |
| Q8 | same Domain -> runtime -> observation -> destroy -> new runtime -> same observation | equal output at error 0 |

```text
REALTIME-SOURCE Q4 iterations=50 final=pass Q5 boundaries=8 max-error=0 Q6 cache-bytes=8192 file-bytes=345600106 ws-before=8605696 ws-sequential=9035776 ws-rapid=9035776 ws-boundaries=9039872 Q7 cycles=20 Q8 reconstruction=equal
```

Callback counters across the run were reader/file calls 0, waits/blocks 0, and allocation/growth 0. The requested non-divisible page-size fixture (192 or 257 frames) remains unexecuted, so Q5 is partial. Q9 downstream processing and Q10 complexity/final classification remain unexecuted.
