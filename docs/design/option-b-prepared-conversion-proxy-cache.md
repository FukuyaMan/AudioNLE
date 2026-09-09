# Option B Prepared Conversion / Proxy Cache

## Boundary

`ClipRuntimeView` consults `SrcAdmission`. Guaranteed and BestEffort routes may use the existing realtime SRC path; PreparedRequired routes use a prepared project-rate page source. Both feed the same SourceNode/Clip-processing downstream path. Prepared PCM is runtime/cache material, not a Clip type, Domain authority, or a mutation of original media.

```text
SourceReference -> NativeSourceService worker -> libsamplerate BEST
                -> private project-rate page -> immutable publish
ClipRuntimeView callback -> prepared page lookup -> FixedOutput -> SourceNode
```

Preparation lifecycle is `NotRequested -> Queued -> Preparing -> Ready`, with `Failed` and `Stale` terminal observations. Duplicate requests share key/page ownership, not Clip ownership. Clip delete/move does not destroy shared source cache data; source generation/config changes prevent old work from publishing as current.

## Coordinates and boundaries

Pages use integer `PreparedProjectStart` and 256-frame `PreparedPageIndex`. Artifact frame zero is logical project-rate source frame zero. Worker conversion owns physical history/flush; callback exposure is clipped at the authoritative logical source end. Source-start clamping never reads negative samples. SRC settling is not an Effect Tail; plugin/Clip tails remain downstream.

## Callback contract

Only fully published immutable pages are readable. Missing, stale, or failed pages produce deterministic silence/underrun and diagnostic counters (`hits`, `misses`, `unavailable`, `stale`, `unexpectedRealtimeSrcFallback`). Callback code performs no decode, I/O, SRC, allocation, growth, lock/wait, lifecycle, or synchronous preparation. The V1 vertical slice validates this under the existing Windows IAT harness; dynamic module/import routes remain the existing documented residual blind spot.

## Persistence and export

V1 is an ephemeral rebuildable session cache. Project persistence contains source/config intent and logical coordinates only. A future disk cache must validate cache format version, full artifact key, page length/checksum, and truncation/corruption before publication. Offline export never depends on cache availability: it can stream BEST off callback, optionally reusing a compatible artifact only after deterministic validation.

## Classification

**Moderate prepared conversion subsystem.** The fixed page/cache boundary is small, but durable disk artifacts, real decode worker integration, cache eviction/pinning, multi-channel/layouts, and UI progress are intentionally deferred. This is the appropriate boundary: it keeps source authority and downstream processing independent of whether PCM arrived through live or prepared SRC.
