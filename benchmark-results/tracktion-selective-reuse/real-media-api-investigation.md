# Tracktion Selective Reuse — Real-media Reader/API Investigation

Execution date: 2026-09-06 (Asia/Tokyo)

Pins: Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`.

## Candidate A — JUCE low-level WAV reader (selected)

| Field | Evidence |
| --- | --- |
| Candidate | `juce::AudioFormatManager` + `juce::AudioFormatReader`, registering `WavAudioFormat` through `registerBasicFormats()` |
| Source file/symbol | `juce_audio_formats/format/juce_AudioFormatManager.h:135-149`, `juce_AudioFormatManager.cpp:129-159`; `juce_audio_formats/format/juce_AudioFormatReader.h:80-151`; `juce_AudioFormatReader.cpp:70-128`; `juce_WavAudioFormat.cpp:1208+` |
| Public/supported | Yes: public JUCE audio-format API, directly used with no Tracktion type or high-level scheduling object |
| Exact random access | `AudioFormatReader::read` takes `int64 startSampleInSource`; documentation states the position is samples from the start of the stream. The implementation passes the requested start to `readSamples` after only explicit negative-range handling. |
| Whole-file preload | No source-level whole-file PCM vector is created by `AudioFormatManager::createReaderFor(File)` or the WAV reader constructor; the reader owns an input stream and parses RIFF chunks. This gate separately measures the actual long fixture. |
| Buffering | No prototype cache is added; the source node has a fixed 128-frame scratch array. Filesystem/OS buffering is implementation/environment behavior, not a Domain cache. |
| EOF behavior | The public `read` contract says out-of-range samples are returned as zeros and an over-length request is zero-padded; it is not an error solely for being out of range. |
| Sample conversion | `AudioFormatReader::read(float*...)` uses the reader's native `read` then converts fixed-point samples to float when needed (`juce_AudioFormatReader.cpp:70-83`). The fixture uses PCM16 and compares with conversion tolerance only for value quantisation; positions remain exact. |
| Stream/file ownership | `AudioFormatManager::createReaderFor(File)` creates an input stream and transfers it to the successful reader (`juce_AudioFormatManager.cpp:129-143`). The prototype stores its reader in `unique_ptr`; it is runtime-only. |
| Thread/blocking | File stream construction and synchronous `read` perform filesystem work. No real-time claim is made; this is headless/offline only. |
| High-level Tracktion dependency | None. The implementation creates no `Engine`, `Edit`, `WaveAudioClip`, `WaveNodeRealTime`, Tracktion cache manager, or persistence object. |
| Future FFmpeg boundary | The prototype reader is behind an AudioNLE-owned range-read boundary (`read(float*, Sample, int)`). A future decoder adapter can satisfy that boundary without granting it Timeline authority. |

**Selection:** Candidate A. It satisfies the R0 public API, explicit `int64` sample-offset, headless/offline, no-high-level-scheduling, and no-whole-file-preload criteria with less prototype code than a custom RIFF parser.

## Candidate B — Tracktion low-level file/cache primitives (not selected)

| Field | Evidence |
| --- | --- |
| Candidate | `tracktion::engine::AudioFileUtils::createReaderFor` / mapped-reader helpers and `AudioFileManager` / cache surface |
| Source file/symbol | `tracktion_AudioFileUtils.cpp:13-87`, `tracktion_AudioFileManager.h`, `tracktion_AudioFileCache.h`, `tracktion_BufferedAudioReader.h` |
| Public/supported | Public headers exist, but construction requires an `Engine` and inherits Tracktion audio-file/cache lifetime and format-manager coupling. |
| Exact random access | The simple helper delegates to the same JUCE reader; its mapping helper additionally creates a `MemoryMappedFile` over the file. |
| Whole-file preload / buffering | A mapped-file path maps the source file and cache ownership belongs to Tracktion. This is not the bounded explicit reader/cache policy being measured here. |
| High-level Tracktion dependency | It is not the high-level Clip scheduler, but it adds `Engine`/audio-file-manager ownership without adding needed source scheduling value. |
| Decision | Rejected for this gate: Candidate A supplies the required public reader with smaller coupling and leaves cache policy explicitly AudioNLE-owned. |

## Candidate C — minimal prototype WAV reader (not selected)

| Field | Evidence |
| --- | --- |
| Candidate | New RIFF parsing, PCM conversion, file ownership, range/EOF validation, and buffering implementation |
| Public/supported | Would be repository-owned code, not a reused public reader API |
| Exact random access | Possible, but unmeasured and duplicates Candidate A's range-read capability |
| Future FFmpeg boundary | Could be adapter-friendly, but adds an unnecessary format-specific implementation before evidence that JUCE is unsuitable |
| Decision | Not selected. Candidate A already meets R0 without a private API, patch/fork, or high-level Tracktion scheduling. |

## Hard-stop review

No candidate required a private API, Tracktion/JUCE patch/fork, high-level Tracktion source scheduling, or normal whole-file PCM preload. R0 passes and permits R1–R4 only.
