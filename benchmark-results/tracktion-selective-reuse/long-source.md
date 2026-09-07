# Tracktion Selective Reuse — Long-source WAV Memory and Range-locality Evidence

Execution date: 2026-09-06 (Asia/Tokyo)

## Fixture generation

The direct executable generates then deletes a temporary local mono 48 kHz PCM16 WAV. It writes a fixed 4096-frame `AudioBuffer` per chunk and never builds a whole-file PCM vector. The fixture is 172,800,001 samples (one hour plus an end marker sample), 345,600,106 bytes, with markers at 0, 2,880,000, 28,800,000, and 172,800,000.

## R4 observations

| Observation | Bytes |
| --- | ---: |
| Working set before long fixture open | 9,101,312 |
| After fixture generation | 9,117,696 |
| After public reader open | 9,117,696 |
| After far-forward one-hour marker read | 9,117,696 |
| After far-backward one-minute read | 9,117,696 |
| After graph reads at all long markers | 9,121,792 |

The prototype owns no duration-scaled decoded cache. Its source-node scratch capacity is fixed at 128 float frames (512 bytes); reader-buffer policy is therefore `0` explicit retained PCM frames. OS/filesystem buffering is outside this prototype's Domain and cannot be treated as a production cache policy.

All direct and graph reads at beginning, one minute, ten minutes, and one hour observed their expected values at error 0. The reader uses absolute `int64` source offsets; a far backward read after the one-hour read returned the one-minute marker correctly.

```text
REAL-MEDIA R4 duration-samples=172800001 file-bytes=345600106 reader-buffer=0 ws-before-open=9101312 ws-after-write=9117696 ws-after-open=9117696 ws-after-forward=9117696 ws-after-backward=9117696 ws-after-graph=9121792 long-markers=4 max-error=0
```

## Bounded-memory and range-locality assessment

**Pass for this bounded PCM16 WAV fixture.** Source evidence shows JUCE reads requested absolute ranges through the file-backed `AudioFormatReader`; the measured working set does not scale to the 345 MB decoded-file size, reader construction does not allocate whole PCM, and far forward/backward reads do not exhibit a duration-scaled allocation or an output offset.

Actual byte-read counters are not public through this reader API, so range locality is supported by the public absolute-range contract, direct far-seek observations, fixed prototype scratch capacity, and memory observation—not by per-request OS I/O byte accounting. This is sufficient for the bounded offline feasibility gate, not a realtime cache/prefetch result.
