# Tracktion Selective Reuse — Real-media WAV Exactness, Seek, and Error Evidence

Execution date: 2026-09-06 (Asia/Tokyo)

## Fixture and path

The test generates a temporary mono, 48 kHz, PCM16 WAV in 4096-frame writes. Normal samples are zero. Markers have distinct quantisation-safe values (`n / 16`) at 1, 100, 127, 128, 129, 1023, 1024, 1025, 4095, 4096, and 4097. No PCM vector is shared with the reader.

The selected public JUCE reader receives absolute `int64` Source offsets. The framework-free Domain uses the WAV path, source length, Timeline start, and Source offset; the custom Option A2 `Node` derives each read from `ProcessContext::referenceSampleRange`. Downstream processing is limited to public low-level `Node`, `SimpleNodePlayer`, and `SummingNode` objects.

## R1 exact-read matrix

| Source marker | Timeline marker | Direct reader | Source Node / graph | Alignment error |
| ---: | ---: | --- | --- | ---: |
| 1 | 1 | expected value | expected value | 0 |
| 100 | 100 | expected value | expected value | 0 |
| 127 | 127 | expected value | expected value | 0 |
| 128 | 128 | expected value | expected value | 0 |
| 129 | 129 | expected value | expected value | 0 |
| 1023 | 1023 | expected value | expected value | 0 |
| 1024 | 1024 | expected value | expected value | 0 |
| 1025 | 1025 | expected value | expected value | 0 |
| 4095 | 4095 | expected value | expected value | 0 |
| 4096 | 4096 | expected value | expected value | 0 |
| 4097 | 4097 | expected value | expected value | 0 |

Maximum alignment error: **0 Timeline samples**. PCM16 conversion is compared with a value tolerance of 0.00005; no timing tolerance is applied.

## R2 block-boundary and render-start matrix

All boundary markers `127/128/129`, `1023/1024/1025`, and `4095/4096/4097` were rendered with 128-frame blocks and starts at 0, immediately before the marker, at the marker, and immediately after the marker. For starts at or before a marker, its absolute output sample and value matched exactly; a start after the marker did not retain the pre-start marker. Maximum alignment error: **0 Timeline samples**.

## R3 seek/reset and reconstruction

The same reader and source-node path read this sequence without stale data or offset:

```text
1 -> 4097 -> 100 -> 4096 -> 127 -> 127
```

This covers forward, backward, middle/later, later/earlier, and repeated-position reads. Each direct and graph observation had error 0. A Domain -> runtime -> render -> destroy -> same Domain -> new runtime -> render comparison had equal output and equal requested source-range observations. No reader, file handle, cache, or runtime identity entered Domain state.

## EOF/error behavior

| Case | Observed bounded behavior | Status |
| --- | --- | --- |
| Missing file | `AudioFormatManager::createReaderFor` cannot create a reader; prototype construction fails explicitly | Pass |
| Invalid WAV | reader construction fails explicitly | Pass |
| Truncated WAV | reader construction fails explicitly | Pass |
| Request past EOF | public reader returns exact zero | Pass |

Past-EOF access produced neither stale audio nor uninitialised data. These are offline test-harness semantics, not a production UI/recovery policy.

```text
REAL-MEDIA R1-R3 markers=11 max-error=0 reconstruction=equal eof=zero errors=pass
REAL-MEDIA PASS
```

## R5 minimum downstream processing compatibility

The test keeps the real WAV custom source node as the only source of fixture A audio. `MultiplyNode` is a deterministic public low-level wrapper; it copies its processed input buffer and multiplies it by 2. Track mixing is performed only by public `tracktion::graph::SummingNode`, never by adapter-side sample addition. A separate generated marker node supplies fixture B at Timeline sample 1024; it is independently scheduled and has no shared reader or Domain runtime state with the WAV source.

| Test | Graph and observation | Expected | Observed | Status |
| --- | --- | ---: | ---: | --- |
| R5-T1 | real WAV marker at 1024 -> Multiply(2) | `0.4375 -> 0.875`; zero at 1022/1026 | exact | Pass |
| R5-T2 | real WAV A + independent B `0.25` through `SummingNode` | `0.4375 + 0.25 = 0.6875`; zero at 1022/1026 | exact | Pass |
| R5-T3 | R5-T2 mix -> Multiply(2) | `1.375`; zero at 1022/1026 | exact | Pass |
| R5-T4 | 10-minute real WAV marker -> Multiply(2) | `0.375 -> 0.75`; zero adjacent samples | exact | Pass |
| R5-T5 | same Domain -> new source/processing graph -> render | output and source-range requests equal | equal | Pass |

All R5 timing observations have an alignment error of **0 Timeline samples**. This uses no high-level Tracktion source scheduler, manual output correction, manual mix, private API, patch/fork, or runtime state in Domain.

```text
REAL-MEDIA R5 source=1024 input=0.4375 processed=0.875 mix=0.6875 mix-then-multiply=1.375 long=0.75 reconstruction=equal max-error=0
REAL-MEDIA PASS
```
