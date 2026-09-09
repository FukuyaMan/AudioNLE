# 0004: Source Runtime Multichannel PCM Contract

## Status

Accepted

## Context

The production SourceRuntime page and node contracts were implicitly mono,
preventing a selected stereo MediaSource from reaching the graph safely.

## Decision

V1 source PCM is float32, frame-interleaved (`frame * channels + channel`),
with a fixed channel count and layout identity per `RuntimeIdentity`. V1
supports mono and stereo; capacity is bounded by two channels. Channel count
and layout identity participate in source/runtime/cache identity. Native pages,
SRC staging, prepared artifacts, SourceNode buffers, and graph inputs use this
same contract.

`SourceNode` accepts an explicit buffer `{data, frames, channels, interleaved}`
and rejects a channel mismatch with zero output and `Unsupported`. The V1 graph
only sums matching channel configurations; it performs no implicit remix,
upmix, downmix, truncation, or channel swap. libsamplerate uses one mutable
BEST `SRC_STATE` per view configured for the source channel count.

## Consequences

Page bytes are `pages * pageFrames * channels * sizeof(float)` and remain
duration-independent. Mono/stereo require 16 KiB/32 KiB for the two native
pages. 5.1/7.1 require a future capacity/graph policy; they are rejected rather
than silently altered. Integer source/timeline authority and ADR 0002 remain
unchanged.
