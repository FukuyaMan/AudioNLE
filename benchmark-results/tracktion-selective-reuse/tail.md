# Tracktion Selective Reuse — Phase F Deterministic Effect Tail

Execution date: 2026-09-06 (Asia/Tokyo)

## Scope

Bounded Option A2 low-level graph test only: 48 kHz, 128 frames, public `Node`, `SummingNode`, and `SimpleNodePlayer`; no VST3 Tail, realtime editing, high-level Clip scheduling, private API, patch, or fork.

The framework-free Domain fixture has Clip start 1000, Source Duration 480, Source End 1480, and a reported/actual Tail of 1024 samples. `Reported` schedules `[1480,2504)`; `CutAtSourceEnd` creates no Tail. Runtime nodes and buffers are transient and Domain state remains unchanged.

| Test | Expected | Observed | Status |
| --- | --- | --- | --- |
| F1/F2 Source End, silence feed, reported Tail | first/last `1480/2503`; Processing End 2504 | `1480/2503`; 2504 | Pass |
| F3 order | Tail -> Multiply(2) is 0.50; Multiply -> Tail remains 0.25 | exact | Pass |
| F4 Track Mix overlap | Tail 0.25 + later Clip 0.50 at 1800 | 0.75 | Pass |
| F5 move | all Tail coordinates move +48000 | `49480..50503` | Pass |
| F6 delete | deleted Domain has no output/Tail | empty output | Pass |
| F7/F8 policy | CutAtSourceEnd no Tail; Reported uses 1024 | exact | Pass |
| F10 reconstruction | same Domain -> rebuild | equal output | Pass |

```text
TAIL source-end=1480 processing-end=2504 first=1480 last=2503 overlap=0.75 move=48000 cut=pass rebuild=equal
TAIL PASS
```

**Conditional Pass.** AudioNLE-owned planning keeps Source End distinct from Processing End and schedules post-source silence into the graph. Tracktion owns graph traversal, downstream buffer propagation, and `SummingNode` mixing. Actual VST3/unknown/infinite tails, realtime updates, and production render planning remain unmeasured.
