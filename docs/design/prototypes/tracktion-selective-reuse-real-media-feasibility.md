# Option A2 Real-media / Long-source Streaming Feasibility

## Status and decision boundary

**Status: planned feasibility gate; no implementation or architectural decision.**

Option A2 has bounded evidence for generated same-rate source scheduling, ordered low-level processing, deterministic mixed-rate mapping, deterministic and actual-VST3 PDC, and deterministic and actual-VST3 finite tails. Those fixtures do not establish real-file reading, seek/reset, or bounded-memory behavior for long sources.

This gate asks one narrow question:

> Can an AudioNLE-controlled source node read explicitly requested ranges from a long, local, same-rate WAV file with exact integer-sample placement, deterministic seek/reset, and bounded working memory, while retaining the public low-level Tracktion graph for downstream processing?

The result classification is one of `Proceed`, `Proceed with Constraints`, `Reject`, or `Inconclusive`. `Proceed` does not authorize a production source subsystem; it only permits a separately scoped follow-up. A failed exactness or bounded-memory invariant is a rejection signal for this Option A2 source path, not a justification for silently delegating authoritative scheduling to Tracktion's high-level Clip path.

## Scope

### Included

* Local mono WAV only, containing PCM integer or floating-point samples.
* 48 kHz Source domain and 48 kHz Project Timeline domain.
* Generated-on-test temporary WAV fixtures with known, independently checkable sample markers.
* Framework-free authoritative Domain SourceReference and integer Source/Timeline scheduling.
* A bounded source-reader/cache experiment feeding the existing Option A2 custom source-node / public low-level Tracktion graph path.
* Headless, offline reads; exact-position, block-boundary, render-start, seek/reset, destroy/rebuild, EOF/error, memory, long-source, and bounded processing-compatibility observations.
* Source-level investigation of pinned JUCE and Tracktion public file-reading/cache primitives before a reader is selected.

### Explicitly excluded

* FFmpeg, MP4/MKV/MOV, compressed codecs, network media, and any production format-coverage claim.
* Mixed-rate real media, resampling quality, and a production SRC selection.
* Actual realtime playback, runtime editing, waveform/proxy generation, production cache management, export, persistence, plugin/PDC/Tail follow-up, GUI, ADR, and a final architecture selection.
* `WaveAudioClip`, `WaveNodeRealTime`, Tracktion Clip placement, or Tracktion project persistence as authoritative source scheduling.
* Whole-file PCM preload as the normal long-source implementation.

No source code, CMake target, test target, dependency pin, submodule, benchmark result, ADR, or production source is changed by this document.

## Invariants and architecture boundary

The authoritative direction remains one-way:

```text
AudioNLE Domain SourceReference
  -> AudioNLE integer Timeline <-> Source mapping and read scheduling
  -> bounded source reader / cache
  -> DomainControlled real-media source Node
  -> public low-level Tracktion graph
  -> processing / output observation
```

The Domain owns SourceReference identity, requested Source sample ranges, integer Timeline positions, seek/reset intent, cache policy, and source lifetime policy. It must not contain a reader handle, Tracktion/JUCE object, decoded PCM cache, file position, or runtime identity.

The reader/cache owns file I/O, WAV decode/sample extraction, and any bounded read buffering. Tracktion owns public graph preparation, traversal, block processing, downstream effects, summing, and PDC. Tracktion high-level Clip/source scheduling is not a fallback authority.

The following remain mandatory throughout every phase:

1. Source and Timeline positions are signed integer sample positions; at this same-rate gate their mapping is identity, but the domains remain distinct concepts.
2. A requested marker must emerge at its requested Timeline sample with zero error. No source shift, output shift, post-observation correction, or tolerance window is permitted.
3. Requested graph ranges must cause range-local reads. A far seek must not decode/load the entire source or replay it from the beginning.
4. Working memory must be bounded by reader/cache policy rather than source duration. The temporary fixture generator is not evidence of production reader behavior and must be kept separate from the read path.
5. Reader/runtime state is disposable. Destroy/rebuild from the same Domain must reproduce requested ranges and output exactly.
6. Offline synchronous read evidence must never be labelled realtime safe.

## R0 — source-reader API investigation

Before implementing a fixture reader, record source-level evidence against the pinned sources and existing repository code in `benchmark-results/tracktion-selective-reuse/real-media-api-investigation.md`.

Investigate and compare these candidates:

| Candidate | Questions to answer | Selection condition |
| --- | --- | --- |
| A. JUCE low-level reader | `WavAudioFormat`, `AudioFormatManager`, `AudioFormatReader`, stream ownership, `read` range semantics, sample format conversion, random access, EOF behavior, buffering, memory-mapped availability, and thread/blocking characteristics | Preferred if public API gives exact random-access samples without hidden whole-file preload and keeps the future decoder boundary clean. |
| B. Independently reusable Tracktion low-level primitive | Public file/cache primitive, ownership, cache lifetime, range semantics, whether it drags high-level scheduling/persistence into the source boundary | Use only if it is public, independently reusable, exact, bounded, and does not make Tracktion authoritative for source timing. |
| C. Minimal prototype-owned WAV reader | RIFF/WAV parsing surface, PCM conversion, random access, EOF/error behavior, buffering responsibility, future replacement cost | Use only if A/B cannot meet the boundary with a smaller and auditable prototype surface. It is not authorization for a production reader. |

R0 must distinguish documented/public API from inferred implementation behavior and state the pinned file paths/lines inspected. It must also identify relevant tests/examples. The comparison must cover exact sample addressing, random access, memory behavior, ownership, blocking/thread implications, coupling, and a future FFmpeg adapter boundary.

**R0 stop conditions:** a candidate requires a private API, a Tracktion/JUCE patch or fork, authoritative high-level scheduling, or normal whole-file PCM preload. Such a candidate is rejected rather than worked around.

## Fixture strategy

Fixture WAV files are generated as temporary local files during a test or direct executable run. They are not committed binaries and are not downloaded media. Each marker has an unambiguous value and is independently checked after reading; ordinary source samples are zero so accidental offsets are visible.

The small exact-read fixture must include markers at these Source samples:

```text
1, 100, 127, 128, 129,
1023, 1024, 1025,
4095, 4096, 4097
```

The long fixture must additionally include markers at exact integer positions for 1 minute, 10 minutes, and 1 hour at 48 kHz:

```text
2,880,000
28,800,000
172,800,000
```

The fixture format, sample encoding, marker generation formula, duration, byte size, temporary-file lifecycle, and cleanup behavior must be reported so a later run can reproduce the observation. The fixture writer must not share its decoded PCM vector or cache with the reader under test.

## Measurement protocol

All results are recorded with requested Timeline range, requested Source range, actual reader range/position where observable, output sample position/value, alignment error, reader/cache allocation policy, and error result. A `Pass` for an exactness case requires zero Timeline-sample error and the expected marker value.

### R1 — small same-rate exact-read baseline

Hypothesis: a real 48 kHz WAV can be read by an AudioNLE-controlled source node at explicitly requested sample ranges without an offset.

Run the complete small marker set individually and through a contiguous range. Verify both direct reader observations and graph output observations. The first successful result must state which R0 reader candidate was selected and why the rejected candidates were not used.

### R2 — block-boundary and render-start matrix

Hypothesis: graph block segmentation and render start do not alter absolute output placement.

For the boundary markers 127/128/129, 1023/1024/1025, and 4095/4096/4097, render with at least:

| Render start | Purpose |
| --- | --- |
| 0 | baseline absolute coordinate |
| immediately before the marker | source start inside a requested graph block |
| at/near the marker | render-start independence |
| immediately after the marker where applicable | no stale reader/cache output |

Use the existing 128-frame prototype block size initially; record any additional block sizes only as supplementary evidence. The source node must consume the requested range, not retain a hidden sequential position as its authority.

### R3 — seek, reset, and reconstruction matrix

Hypothesis: random access is deterministic and runtime lifecycle does not enter Domain state.

Run each transition and record requested/observed Source sample, output position, and error:

```text
start -> near end
near end -> beginning
middle -> later
later -> earlier
same position repeated
```

Then run:

```text
same Domain -> open reader/runtime -> seek/read/render -> destroy
same Domain -> new reader/runtime -> same seek/read/render
```

The sequences must produce equal output and equal requested-range observations. Reopening the file may be part of reconstruction; a file handle, decoder position, or cache instance must not be persisted in Domain state.

### R4 — long-source bounded-memory test

Hypothesis: reader working memory is not proportional to full decoded PCM duration.

Use the generated 1-hour, mono, 48 kHz fixture and probe beginning, 1-minute, 10-minute, and 1-hour-class markers, including a far forward and a far backward seek. Measure and report:

* fixture file byte size and logical duration;
* process working set/RSS before opening, after opening, during sequential reads, after far forward seek, and after far backward seek;
* maximum observed working set where available;
* reader/cache configured capacity, bytes, and number of source frames;
* bytes read/decoded per requested range where observable;
* source position width, seek result/cost, and reader lifecycle behavior.

The acceptance argument is qualitative and evidence-backed: no normal reader construction or source-node path may allocate a full decoded PCM representation proportional to the full one-hour duration. A fixed cache/buffer must be declared explicitly. Do not turn a specific MB threshold into a production requirement in this prototype.

**R4 hard failures:** whole-file PCM decode/load on normal construction, a duration-scaled PCM vector/cache, decoding from file start to satisfy a far seek, or inability to retain bounded working memory.

### R5 — bounded processing compatibility

Hypothesis: the selected real-media source node supplies its output to the established low-level processing graph without losing exact placement.

Use only the minimum graph:

```text
real WAV -> DomainControlled source Node -> Multiply(2) -> SummingNode -> observation
```

Verify an exact marker after the downstream processor and an overlap with a second independently scheduled source. This phase does not repeat PDC, Tail, VST3, or production effect-chain work.

### R6 — complexity and boundary review

Record custom reader/cache LOC and responsibilities, reused public APIs, exact scheduling/seek/reset/buffering code, filesystem and decoder ownership, Tracktion-specific coupling, memory management, offline-thread assumptions, and the future FFmpeg boundary. Compare this surface to the custom-engine alternative without selecting either architecture.

The review must explicitly decide whether the source-reader/cache glue remains a bounded adapter responsibility or is approaching a custom-engine-scale subsystem. It must also confirm that no Tracktion high-level source scheduler became authoritative.

### R7 — mixed-rate follow-up decision

Do not implement mixed-rate real media in this gate. Based on R0–R6, record only whether a separate 44.1 kHz source to 48 kHz Timeline feasibility phase is justified, and name the unresolved SRC-quality, mapping, cache, and dependency questions.

## Failure and EOF behavior

The prototype must have deterministic, bounded behavior for missing files, truncated files, invalid WAV headers, and requests past EOF. R0/R1 must specify the selected reader's observable failure mechanism and the source-node response. Past-EOF reads must neither read out of bounds nor produce stale cached audio. Whether they produce silence, an explicit failure result, or another bounded test-harness outcome must be chosen and asserted before R1 is judged; it is not silently deferred.

These are offline fixture behaviors only. They do not define runtime recovery, UI messaging, or production media relinking.

## Threading constraint

This feasibility gate is headless and offline. It may use synchronous file reading solely to establish sample, seek, reset, and memory behavior. The source investigation must record whether the selected reader blocks, performs allocation, touches the filesystem, or has thread-safety constraints. None of those observations may be interpreted as permission to call the reader on a realtime audio thread.

Any later realtime implementation must establish a separate prefetch/cache and ownership model that obeys the repository's realtime-audio restrictions.

## Evidence and result records

Planned output records:

```text
benchmark-results/tracktion-selective-reuse/
  real-media-api-investigation.md
  real-media.md
  long-source.md
  real-media-complexity.md
```

`real-media-api-investigation.md` records R0 source evidence and reader selection. `real-media.md` records R1–R3, R5, exact matrices, EOF/error behavior, and reconstruction. `long-source.md` records R4 fixture, memory, range-locality, and seek observations. `real-media-complexity.md` records R6/R7.

After the prototype phases complete, create `docs/design/prototypes/tracktion-selective-reuse-real-media-results.md` with: recommendation; scope; pins; reader/API selection; Domain/reader boundary; exact and render-start matrices; seek/reset; reconstruction; memory and long-source results; processing compatibility; threading limitations; EOF/error behavior; complexity; FFmpeg boundary; Option A2 implication; and next gate.

## Rejection signals

Stop and classify the result appropriately if any of the following occurs:

* a requested Source sample cannot be observed at its exact Timeline position;
* seek/reset produces an offset, drift, stale data, or nondeterministic output;
* a selected reader depends on Tracktion high-level source scheduling or source-time authority;
* bounded memory or range-local access cannot be demonstrated;
* a private API, patch/fork, or whole-file preload is needed;
* reader/cache complexity grows into an unjustifiably large custom I/O subsystem;
* future compressed/container support would require a disruptive second source architecture rather than an adapter boundary.

## Completion criteria for the future prototype

The gate is complete only when R0–R6 have evidence records, all exactness observations use zero sample error, long-source memory/range-locality behavior is measured, seek/reset/rebuild behavior is deterministic, EOF/error behavior is asserted, processing compatibility is demonstrated, and the result document states the remaining realtime and mixed-rate limitations. No implementation or test work is authorized by this planning document alone.
