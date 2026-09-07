# Tracktion Selective Reuse — Realtime Source Q1–Q3 Evidence

Execution date: 2026-09-07 (Asia/Tokyo)

## Scope

This bounded prototype uses a temporary mono PCM16 48 kHz WAV with markers at 0, 127, 128, 10,000, 1 minute, 10 minutes, and 1 hour. A public JUCE `AudioFormatReader` exists only inside one background `juce::Thread`. The custom public Tracktion `Node` publishes an atomic page request and reads only a matching ready fixed page through the public low-level `SimpleNodePlayer` path.

Cache capacity is exactly `8 * 256 * 1 * sizeof(float) = 8192` bytes PCM. It is a fixture capacity, not a production cache decision. Domain remains path/source length/Timeline start/Source offset only; it contains no reader, page, worker, generation, queue, file handle, or Tracktion/JUCE type.

## Callback invariant instrumentation

| Counter | Observed | Status |
| --- | ---: | --- |
| Callback reader/file calls | 0 | Pass |
| Callback waits/blocks | 0 | Pass |
| Callback allocation/growth | 0 | Pass |
| Worker reader calls | 9 in representative direct runs | Pass; confirms reader work occurred outside callback |
| Callback cache hits/misses/underruns | recorded by fixed atomics | Pass for Q1–Q3 policy checks |

The node has no reader member and no dynamic container. Counters are fixed atomics; no callback logging is performed. Source inspection and counters jointly support the fixture claim; neither is a Windows realtime scheduling guarantee.

## Q1 sequential prefetched playback

Each marker page was published by the worker before its one-sample graph observation. All seven marker values were observed at the requested absolute Timeline sample.

| Markers | Expected / observed timing | Maximum error | Status |
| ---: | --- | ---: | --- |
| 0, 127, 128, 10,000, 2,880,000, 28,800,000, 172,800,000 | exact expected PCM16 marker value | 0 Timeline samples | Pass |

## Q2 deliberate miss and recovery

The worker was held before its read of the 10,000-sample marker page. The callback received a cache miss and emitted exact zero with an underrun increment. It made no reader call, wait, allocation, synchronous fallback, stale-audio output, or uninitialised output. After release and current-generation page publication, the same marker was observed with its expected value and alignment error 0.

## Q3 seek generation invalidation

The test holds a generation-N read pending, increments runtime-only generation on seek, requests generation N+1, then releases the old read. The old completion is discarded and never eligible as current output. Before the new page is ready, output follows Q2 policy (`zero + underrun`); after publication, the one-hour marker is exact at its requested Timeline sample.

Five consecutive direct runs each observed one obsolete completion discard and no old-generation audio. The exact count remains diagnostic rather than a production scheduling contract.

```text
REALTIME-SOURCE Q1 markers=7 max-error=0 worker-reader=9 callback-reader=0 callback-waits=0 callback-growth=0
REALTIME-SOURCE Q2 zero=pass underruns=3 recovery=pass
REALTIME-SOURCE Q3 old-generation=3 new-generation=4 stale-discard=1 max-error=0
REALTIME-SOURCE PASS
```

## Hard-stop review

No callback reader/file call, callback wait/block, callback allocation/growth, synchronous miss fallback, stale/uninitialised miss output, nonzero alignment error, Domain leakage, private API, patch/fork, or high-level Tracktion source scheduler was observed in Q0–Q3.

Q4 rapid seek stress, Q5 boundary stress, Q6 long-source memory, Q7 destruction stress, Q8 reconstruction, Q9 downstream processing, and Q10 complexity/final decision are deliberately not executed.
