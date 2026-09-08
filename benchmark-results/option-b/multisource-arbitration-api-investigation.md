# Option B Multi-source Arbitration API / Reader Investigation

## Scope

This records the public JUCE reader boundary used by the Option B fixture. It is not a claim about FFmpeg, arbitrary JUCE formats, or production thread safety.

## `AudioFormatReader` finding

The repository-pinned public declaration at `prototype/tracktion-feasibility/third_party/tracktion_engine/modules/juce/modules/juce_audio_formats/format/juce_AudioFormatReader.h` exposes `read` overloads that read a source stream into caller-provided buffers. The declaration documents ranges and zero padding, but does not state that concurrent calls on the same reader are safe, that a reader is movable between workers, or that one reader can service multiple in-flight reads.

The fixture therefore uses the conservative model:

```text
one MediaSourceRuntime owns one AudioFormatReader
worker tasks only reference that stable runtime-owned reader
at most one active read per MediaSourceRuntime
reader does not have permanent worker affinity
creation/destruction occurs at control boundaries
```

`readerBusy` is an atomic per-runtime serialization guard. `maxReaderConcurrency` is instrumented and required to be at most one. Worker-side registry access uses `weak_ptr`/`shared_ptr`; source callbacks neither lock the registry nor own task lifetime.

## Candidate comparison and selected fixture model

| Model | Finding |
| --- | --- |
| A: fixed per-media arbitration and workers | Bounded local state, but workers retain a per-media scaling term and cannot answer the ADR scaling risk. |
| B: globally shared request service | Could bound global requests, but would add a second global fixed queue and fairness policy not needed to test the selected reader/cache shape. |
| C: per-media fixed request slots/cache/reader plus fixed shared workers | Selected. Four fixed request slots per media, eight fixed pages per media, and two fixed worker threads. It keeps page locality/reader ownership local while proving workers do not scale with media count. |

This selection is fixture-local. It does not prove that every decoder context may migrate among workers; it proves only serialized task access to a stable WAV reader.
