# Execution Plan: MediaSource FFmpeg Decoder Boundary

## Objective

Decode an explicitly selected audio stream from MP4, MKV, MOV, and standalone
audio through a backend-neutral production `NativeSourceProvider`, then prove
it reaches `NativeSourceService`, `SourceRuntime`, and the low-level graph
without decoder work in callbacks.

## Requirements

Preserve integer native-sample authority, exact project/source P/Q mapping,
bounded source pages, non-destructive media, explicit stream identity, and the
ADR 0001/0002 callback contract. FFmpeg types and decoder buffers remain
engine-private.

## Planned Components

* Engine-private media and selected-stream identity contract.
* FFmpeg decoder/provider implementation and worker-side seek/preroll/discard.
* Provider injection into shared native runtime ownership.
* Controlled MP4/MKV/MOV/WAV mono/stereo/multi-stream fixtures.
* SourceRuntime/graph and Windows IAT validation.

## Invariants and Edge Cases

Callbacks only copy published pages. Decode, demux, seek, file I/O, allocation,
and lifecycle stay worker-side. Native positions are integer sample coordinates;
PTS/start-time discontinuity handling is decoder-local. Stream selection,
generation, format/layout, and decoder configuration are part of source-page
identity. Forward/backward/repeat/large seek, codec preroll, EOF, and stale
generation publication require explicit tests.

## Multichannel prerequisite completed

The environment has only WinGet Gyan FFmpeg 7.1.1 GPL CLI executables. The
available Gyan 7.1.1 shared archive is a reproducible candidate, but its
development imports are MinGW `.dll.a`, not MSVC-compatible `.lib`; a CLI
subprocess adapter is rejected.

More fundamentally, the validated source runtime is mono-only:
`NativeSourceService` pages and copies carry one float per frame,
`SourceRuntime` configures one libsamplerate channel, and `SourceNode` has no
channel-aware buffer contract. Adding an FFmpeg stereo provider now would drop
or corrupt channel data and would violate this plan's fixture requirements.
ADR 0004 now defines bounded float32 interleaved mono/stereo pages, channel
identity, per-view multichannel BEST SRC, and explicit SourceNode buffers. The
production library validates native/realtime/prepared stereo routes and rejects
channel mismatch without truncation. FFmpeg dependency selection remains the
only active blocker. See ADR 0003 and ADR 0004.

## Completion Criteria

After a reproducible FFmpeg development dependency is available: real fixture
stream enumeration/selection, integer-coordinate decode and reconstruction,
bounded long-form memory, real graph integration, callback IAT evidence, and
focused CTest success.
