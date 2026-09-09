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

## Implemented evidence

The repository-local vcpkg manifest and pinned baseline acquire
`ffmpeg[avcodec,avformat,swresample]:x64-windows@9.0.1#1`; `build.ps1` verifies
that baseline before configuring the MSVC x64 build. ADR 0003 is accepted and
the decoder uses the resulting library, not the WinGet CLI.

`option_b_ffmpeg_native_source` creates controlled MP4/MKV/MOV fixtures through
the linked pinned FFmpeg encode/mux API (and creates WAV directly); it has no
`ffmpeg.exe` subprocess or PATH dependency. The fixtures have 44.1/48 kHz
mono/stereo and two audio streams. The test proves enumeration,
explicit stream selection, repeated range reconstruction, a nonzero-offset
native-coordinate read, stereo preservation, and selected provider injection
through `NativeSourceService`, `SourceRuntime`, and `SourceNode`. The linked
SourceRuntime, graph, stereo, production validation, and Windows IAT focused
tests cover the native, realtime SRC, prepared, graph, generation, and callback
contracts listed in this plan.

Windows CI is configured for repository-local pinned vcpkg and a full CTest run
on `windows-2022`. CI execution evidence is pending the first push/run.

## Full-suite failure classification

The latest local full CTest result is 48/51. None of the three failures is
caused by the FFmpeg decoder integration.

* `tracktion_feasibility_phase_e_pdc` is the documented historical Tracktion
  high-level PDC failure (`expected=1024`, `observed=1026`). It predates the
  FFmpeg commit and has no link to `audionle_engine_source_runtime`; see
  `benchmark-results/tracktion-feasibility/phase-e.md` and the completed source
  runtime plan.
* `option_b_libsamplerate_timing_contract` is a standalone synthetic
  libsamplerate fixture. CMake links it only to `samplerate`, not to
  `audionle_engine_source_runtime` or FFmpeg. Its `prepared cache` failure is
  reproducible from the fixture's expanded 320/147 invalidation case: an
  eight-page modulo cache is primed over more than eight physical pages, so the
  final page overwrites the first required page. The expanded ratio matrix was
  introduced in `a76f93a`, before FFmpeg integration commit `3697bce`. This is
  an independent fixture defect that must be repaired in the SRC feasibility
  work; it is not MediaSource evidence and is not masked by this plan.
* `option_b_mixed_rate_src_identity_trace` is a standalone JUCE audio-basics
  trace target, with no link to `audionle_engine_source_runtime` or FFmpeg. Its
  current 160-frame case reports `expected=147`, `actual=146`, and two exact
  misses. It needs separate SRC-trace ownership and investigation; it does not
  alter the completed MediaSource focused evidence.

## Completion evidence

`FfmpegNativeSourceProvider` now seeks to an integer native-sample position
bounded by an 8192-frame decoder preroll. FFmpeg PTS/timebase selects that
entry point only; decoded samples are accepted or discarded in the provider's
integer native-sample coordinate. The provider has no whole-file PCM retention:
its only PCM conversion workspace is a fixed 8192-frame × 2-channel float
array (64 KiB), in addition to FFmpeg's bounded decoder/demux state. The two
2048-frame native pages consume 16 KiB mono or 32 KiB stereo.

`option_b_ffmpeg_native_source` uses a 30-second real MP4 fixture and proves
beginning, middle, end, backward, repeat, and EOF-near large-offset reads. It
asserts that decoded work stays below 100000 frames across that sequence and
that the fixed conversion scratch never exceeds 8192 frames. It also exercises
forward/backward/repeat/nonzero stream start seeks across the controlled
MP4/MKV/MOV/WAV fixture set.

The same test executes actual FFmpeg selected-stream replacement in the native
service. Queued old-stream demand, a published old-stream page, and an
old-provider result that causes a control-side identity transition are each
generation-rejected; the newly selected mono stream is the only current data.
No callback invokes replacement, decode, demux, seek, file I/O, allocation, or
waiting.

## Completion Criteria

Met: real fixture stream enumeration/selection, integer-coordinate decode and
reconstruction, bounded long-form memory, real graph integration, callback IAT
evidence, FFmpeg seek/preroll, selected-stream invalidation, and focused CTest
success.

## Known limitations

V1 accepts only mono and stereo source streams. It rejects more than two
channels, does no implicit channel remix, and does not claim arbitrary codec
preroll, arbitrary timestamp discontinuity, or live callback-side source
replacement support. The local full-suite failures recorded above remain
independent defects; the Codex command runner's 30-second limit is an execution
environment constraint. CI is configured, but hosted execution evidence awaits
its first push/run.
