# Tracktion Selective Reuse — Realtime Source API / Architecture Investigation

Execution date: 2026-09-07 (Asia/Tokyo)

## Pinned source evidence

| Area | Source evidence | Result for this gate |
| --- | --- | --- |
| Tracktion `Node` process lifecycle | `tracktion_graph/tracktion_Node.h:223-246,523-574` defines public `Node`, invokes `preProcess`, then virtual `process(ProcessContext&)`; `SimpleNodePlayer.h:22-42` invokes `node->process` for the supplied reference range | The custom source Node can consume only fixed, already-published cache pages. The process call is not a reader-worker API. |
| JUCE worker lifecycle | `juce_Thread.h:232,286,321,474` exposes public construction, start, bounded `stopThread`, and worker-side wait | One dedicated JUCE `Thread` owns the JUCE reader. Shutdown signals then boundedly joins it. |
| JUCE worker wakeup | `juce_WaitableEvent.h:43,64,76,93` documents signal/wait semantics | The worker waits; callback publication only calls public `signal()`. No callback wait is used. This is source-inspection evidence for the fixture, not a universal realtime certification of every platform implementation. |
| JUCE fixed FIFO | `juce_AbstractFifo.h:199,204,292,327-339` exposes `noexcept` read bookkeeping over caller-owned storage | Candidate B is viable in principle, but does not itself define per-page Source range, generation, publication, or page ownership. It was not selected. |
| Tracktion audio FIFO/cache | `tracktion_engine/audio_files/tracktion_AudioFifo.h:24,139-183` is public but is an audio FIFO rather than an AudioNLE source-page/generation contract | Candidate C is not selected: it adds Tracktion-specific buffering semantics without replacing AudioNLE scheduling, seek generation, or decoder boundary responsibilities. |
| `AudioFormatReader` | Existing real-media investigation records public `AudioFormatReader::read` range semantics and synchronous file-backed ownership | A worker-local reader is allowed. Calling it from `Node::process` remains prohibited. |

No private API, patch, fork, high-level `WaveAudioClip`/`WaveNodeRealTime`, Tracktion project persistence, or Tracktion media-cache manager is used.

## Candidate records

### Candidate A — AudioNLE fixed PCM pages + dedicated JUCE reader thread

```text
Candidate: A
APIs: public JUCE Thread, WaitableEvent, AudioFormatReader; C++ atomics; public Tracktion Node/SimpleNodePlayer
Public/supported: yes
Callback operations: atomic request publication, WaitableEvent::signal, ready-page validation/copy, bounded counters only
Producer ownership: one reader worker owns AudioFormatReader and fills one fixed page
Consumer ownership: source Node reads only a ready page matching current generation
Publication model: page data -> SourceStart/Generation metadata -> release-store ReadyState
Generation invalidation: runtime-only monotonic generation; old completion is discarded
Cancellation: signalThreadShouldExit + wake/release signal + bounded stopThread
Memory bound: 8 pages * 256 frames * 1 channel * sizeof(float) = 8192 bytes PCM
Blocking behavior: worker waits; callback does not wait, lock, spin, or synchronously read
Allocation behavior: all page PCM and node scratch storage are fixed before processing
Tracktion coupling: public low-level Node/SimpleNodePlayer only
Decoder coupling: JUCE WAV reader is confined to worker
Future FFmpeg suitability: page contract uses integer decoded Source ranges/generation, not WAV byte offsets
Selected: yes
Reason: smallest explicit ownership and generation boundary that preserves AudioNLE scheduling authority
```

### Candidate B — AudioNLE fixed cache + `AbstractFifo`

```text
Candidate: B
APIs: public JUCE AbstractFifo plus caller-owned fixed storage
Public/supported: yes
Callback operations: potentially bounded, subject to an AudioNLE page metadata protocol
Producer ownership: would be AudioNLE worker
Consumer ownership: would be AudioNLE source Node
Publication model: FIFO indices alone are insufficient for SourceRange/Generation/ready-page validity
Generation invalidation: additional AudioNLE protocol required
Cancellation: additional worker lifecycle design required
Memory bound: caller-defined fixed storage
Blocking behavior: FIFO bookkeeping is nonblocking; wake/lifetime remain separate
Allocation behavior: caller-defined
Tracktion coupling: none
Decoder coupling: none
Future FFmpeg suitability: possible
Selected: no
Reason: no smaller than A once required page metadata and seek invalidation are added
```

### Candidate C — public Tracktion low-level buffering/cache primitive

```text
Candidate: C
APIs: public Tracktion AudioFifo and audio-file/cache facilities inspected
Public/supported: headers are public, but source/cache options add Tracktion-specific ownership/coupling
Callback operations: not sufficient evidence for the required source-page/generation contract
Producer ownership: Tracktion-oriented buffer/cache semantics
Consumer ownership: Tracktion-oriented buffer/cache semantics
Publication model: does not make AudioNLE SourceRange/Generation authority explicit
Generation invalidation: not established independently of Tracktion media/cache lifecycle
Cancellation: not established for this custom source boundary
Memory bound: not the explicit fixture contract
Blocking behavior: not accepted without separate evidence
Allocation behavior: not accepted without separate evidence
Tracktion coupling: greater than A/B
Decoder coupling: Tracktion media/cache ownership
Future FFmpeg suitability: weaker boundary
Selected: no
Reason: reuse adds coupling without eliminating AudioNLE scheduling/cache responsibilities
```

## Selected fixed-page representation

```text
Page {
  fixed float[256] PCM;
  atomic SourceStartSample;
  atomic Generation;
  atomic ReadyState;
}
```

There are eight mono pages. The worker writes PCM first, writes metadata, and release-publishes `ReadyState`; the callback acquire-checks ready/start/generation before bounded copying. A page is invalidated only at a controlled seek boundary before the next callback observation in this prototype. Concurrent page replacement while an earlier callback is copying is not claimed as Q0–Q3 evidence; Q4–Q7 remain deferred.
