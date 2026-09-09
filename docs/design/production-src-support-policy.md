# Production SRC V1 Support Policy

## Direction

AudioNLE V1 uses libsamplerate 0.2.2 (`b9c20b93660c3683fda12e3c2a01f0021bf96c56`), `SRC_SINC_BEST_QUALITY`, and `PrerollPolicyV1`. It preserves integer Timeline/native samples, exact reduced P/Q mapping, a 2048-native/512-project callback boundary, and the existing callback prohibitions. No automatic MEDIUM/FASTEST quality fallback is permitted.

## Terms

`GuaranteedRealtime` is a measured V1 reference-hardware tier: at most four simultaneous active mixed-rate SRC views, 256 project-frame callback blocks, and one of the validated common directions. It is not a guarantee for every Windows computer or arbitrary block size.

`BestEffortRealtime` may be attempted live but has no deadline promise. It must retain BEST quality and the normal callback contract.

`PreparedRequired` means reliable playback must wait for prepared/proxy/offline conversion; it never authorizes synchronous source reading, decoding, allocation, blocking, lifecycle, or quality degradation in the callback.

An active SRC view is a simultaneously processed `ClipRuntimeView` whose source rate differs from project rate. Native-rate clips bypass SRC and consume no SRC slot. Overlap, crossfade, and multiple active views count separately; an Effect Tail is separate from SRC physical settling.

## V1 table

| Source -> Project | GuaranteedRealtime | BestEffortRealtime | PreparedRequired | Evidence |
| --- | ---: | --- | --- | --- |
| 44.1 -> 48 | <=4 at 256 frames | 5--7 | >=8 | 8-view variation; low-tier StablePass |
| 48 -> 44.1 | <=4 at 256 frames | 5--15 | >=16 | 8 StablePass; 16 Fail |
| 44.1 -> 96 | <=4 at 256 frames | 5--7 | >=8 | 8 Fail; low-tier StablePass |
| 96 -> 44.1 | <=4 at 256 frames | 5--7 | >=8 | 8 Fail; low-tier StablePass |
| 48 -> 96 | <=4 at 256 frames | 5--15 | >=16 | 8 variable pass; 16 HardFail |
| 96 -> 48 | <=4 at 256 frames | 5--15 | >=16 | 8 StablePass; 16 Fail |

Other project rates, block sizes, and mixed direction loads are `BestEffortRealtime` unless preflight classifies them conservatively as within the common four-view reference tier. V1 uses a static rate/view admission table, not a dynamic DSP scheduler. Exact mixed-load cost composition is deferred: a mixed-direction project is admitted only if its active SRC total is <=4 and every direction is in the table; otherwise it is BestEffort or prepared according to the most restrictive active direction.

## Fallback and failure semantics

Outside the guarantee, V1 prefers background prepared conversion/cache, then a non-destructive proxy/resampled cache, and offline render. Original media is never modified. If required preparation is unavailable, the runtime produces deterministic silence/underrun and a diagnostic counter plus user-visible Preparing/Unavailable state; it does not synchronously decode or resample on the callback. Recovery occurs off callback for the current generation, then resumes deterministically.

Offline export supports every validated common 44.1/48/96 source-to-project direction with BEST because export has no audio callback deadline. 96 kHz media remains importable/editable/exportable; this policy only constrains simultaneous realtime mixed-rate conversion.

## Revisit triggers

Reconsider the policy or backend only when: a libsamplerate release materially changes API/performance; a candidate demonstrates timing/edit, quality, realtime and 8/16-view performance passes; minimum hardware is defined; telemetry shows prepared fallback is excessive; 96 kHz high-concurrency realtime becomes a product requirement; the public capacity contract is deliberately redesigned; or a vectorized/custom SRC becomes strategically justified. Random library shopping is not a trigger.
