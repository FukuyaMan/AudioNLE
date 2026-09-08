# Option B Finite VST3 Tail Evidence

All figures use the deterministic local mono VST3 at 48 kHz and 128-frame blocks. The fixture reports a finite Tail of 1024 samples and zero latency.

| Gate | Observation | Result |
| --- | --- | --- |
| T0/T1 | Public JUCE report converts to 1024; actual range is `[1480,2504)` | Pass |
| T2 | SourceEnd 1480 is inside a block; first zero-fed input/Tail 1480, last Tail 2503, first zero 2504 | Pass |
| T3 | sample 1497 raw `0.125`, downstream Multiply(2) `0.25` | Pass |
| T4 | Tail-only `0.125`, Tail+B `0.625`, B-only `0.5` | Pass |
| T5 | stopped rebuild Move delta 4000: old output zero, moved Tail `[5480,6504)`, overlap `0.625` | Pass |
| T6 | stopped rebuild Delete: A source/Tail zero, B unchanged at `0.5` | Pass |
| T7 | Reported `[1480,2504)`; CutAtSourceEnd stops at 1480 and emits zero after it | Pass |
| T8 | same final description, fresh VST3 instances: output and derived extents equal | Pass |

Maximum observed boundary/timing error is **0 samples**. The fixed-buffer process path records growth, wait, and source I/O as **0**. This is fixture evidence only; it makes no allocation claim for arbitrary plugins.
