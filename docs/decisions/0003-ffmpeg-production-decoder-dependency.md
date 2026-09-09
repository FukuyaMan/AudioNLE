# 0003: FFmpeg Production Decoder Dependency

## Status

Accepted

## Context

AudioNLE requires a backend-neutral `MediaSource` decoder boundary that can
enumerate and explicitly select audio streams from MP4, MKV, MOV, and
standalone audio, then supply integer native-sample ranges to
`NativeSourceProvider`. The existing SourceRuntime callback boundary must not
receive FFmpeg types, decoder buffers, I/O, allocation, or decode work.

The current development environment contains only the WinGet Gyan
`ffmpeg.exe`/`ffprobe.exe` 7.1.1 full build. Its configuration enables GPL and
version3, and it provides neither FFmpeg C/C++ headers nor MSVC-compatible
`avformat`/`avcodec`/`avutil`/`swresample` link libraries. Gyan's 7.1.1 shared
archive is reproducibly downloadable, but its development import libraries are
MinGW `.dll.a`, not MSVC `.lib`; it is therefore not a valid MSVC dependency.

ADR 0004 subsequently established the required bounded mono/stereo
frame-interleaved SourceRuntime contract.

## Decision

Do not implement the production decoder by spawning `ffmpeg.exe` or parsing
`ffprobe` output. That would make the installed CLI an undeclared runtime
dependency, would not establish the requested library build provenance, and
would introduce an unbounded process/I/O lifecycle outside the intended worker
ownership model.

Use the vcpkg `ffmpeg` port at `9.0.1#1`, selected from the vcpkg baseline
`f781d9387e4684783e69e136e2e124ff4660bffc`. The repository manifest requests
only `avcodec`, `avformat`, and `swresample`; `avutil` is the port's core
library. `build.ps1` clones that exact vcpkg revision, bootstraps it if absent,
and invokes the vcpkg CMake toolchain with `x64-windows`, so no global FFmpeg
installation participates.

The port builds upstream FFmpeg 9.0.1 using MSVC x64 and emits shared DLLs
with MSVC import `.lib` files. Its pinned configuration disables FFmpeg CLI
programs, GPL/nonfree switches, and external GPL/nonfree codec libraries;
it enables only the required format/codec/resample libraries and Windows
system facilities. The resulting dependency is LGPL 2.1-or-later; the exact
upstream and port notices are installed as `share/ffmpeg/copyright` by vcpkg.
Consumers use `find_package(FFMPEG REQUIRED)` and `${FFMPEG_LIBRARIES}`.

## Alternatives

* Use the installed GPL CLI as a subprocess: rejected; it is not a linkable
  production decoder boundary and cannot meet the packaging/lifecycle contract.
* Use Gyan 7.1.1 shared development files: rejected for the MSVC target; its
  `.dll.a` imports are MinGW ABI/toolchain artifacts.
* Continue using JUCE WAV-only reader: insufficient for selected streams in
  MP4/MKV/MOV.
* Build FFmpeg from source ad hoc in this repository: rejected in favour of
  the versioned vcpkg port and manifest acquisition path.

## Consequences

No FFmpeg symbols enter Domain or SourceRuntime contracts. The engine-private
decoder exposes only media/stream identity, integer native coordinates,
channel metadata, and float PCM through `NativeSourceProvider`.

## Implementation validation

The decision is implemented and validated by the linked production source
runtime. `FfmpegNativeSourceProvider` uses FFmpeg seek plus a bounded
8192-native-frame preroll, then discards decoded samples until the requested
integer native coordinate. PTS/timebase selects a seek entry point only; it is
not exposed as the source-coordinate contract.

The test-only fixture helper uses those same linked FFmpeg libraries to encode
and mux controlled MP4, MKV, and MOV inputs; it has no `ffmpeg.exe` subprocess
or PATH dependency. Fixtures cover WAV, mono/stereo, two selected audio streams,
nonzero stream start, 44.1/48 kHz, forward/backward/repeat/EOF-near seeks,
selected-stream generation replacement, and a 30-second large-offset container
sequence. The provider's PCM conversion scratch is fixed at at most
8192 interleaved frames and retains no whole-file PCM. Native pages remain
bounded at two 2048-frame pages.

The focused graph and Windows IAT tests execute the linked native, realtime
SRC, and prepared routes. Callback intervals perform no provider read, FFmpeg
work, file I/O, allocation, lock/wait, or lifecycle mutation. This ADR remains
**Accepted**; CI is configured for hosted full-suite execution, with first-run
evidence pending push.
