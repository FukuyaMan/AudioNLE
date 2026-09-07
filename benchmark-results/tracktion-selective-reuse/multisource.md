# Tracktion Selective Reuse — Multi-source M1–M3 Evidence

Execution date: 2026-09-07 (Asia/Tokyo)

## M1 — same file, multiple Clip views

One MediaSource supplied three independent Clip views: Source 0 at Timeline 1000, Source 128 at 2000, and the identical Source 0 at Timeline 3000. The observed amplitudes were exact at all placements (maximum timing error: 0 Timeline samples). Two same-file views at Timeline 4000 summed linearly through public `SummingNode`.

The hard identity case passed: Source sample 0 emitted at both Timeline 1000 and 3000 without placement confusion or stale output.

## M2 — different files

Three distinct WAV MediaSources at one Timeline position fed public `SummingNode`. The observed value equalled the exact linear sum of their distinct marker amplitudes. This used three workers, three caches, 6,144 PCM bytes, and one fixed latest-request slot per media.

## Callback contract

All M1–M3 observations recorded zero callback reader/file calls, waits/blocks/spins, allocation/growth, stale outputs, and timing error. Miss policy remains zero plus underrun; fixture observations were prefetched hits.

## Formula

```text
workers = unique MediaSource runtimes
caches = unique MediaSource runtimes
cache PCM bytes = unique media * 4 pages * 128 frames * 1 channel * sizeof(float)
request capacity = unique media (one latest-request slot each)
```

M4 and later runtime-edit/invalidation work were intentionally not executed.
