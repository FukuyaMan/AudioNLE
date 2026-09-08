# Option B Mixed-Rate SRC S4--S12 Results

Classification: **Proceed with Constraints**.

| Gate | Result |
| --- | --- |
| S4 real streaming | PCM16 44.1 kHz WAV -> worker -> fixed 257-frame native cache -> ClipRuntimeView -> integer ZOH -> SourceNode passed zero-error step boundaries at Source 147, 148, 1000, 255, 256, and 257. |
| S5 start / partitions | Continuous-reference slices at Timeline 0, 512, and 1000 match exactly. 128, 256, and irregular logical partitions are identical. |
| S6 callback / cache | reader/file/decode, wait/block/spin, allocation/growth, and fallback counters are all 0. Cache is 8 x 257 native float frames (8224 bytes); no decoded duration-scaled buffer exists. |
| S7 seek/reset | Forward, backward, and return-to-prior render slices match continuous output. ZOH has no physical history or preroll; reset derives integer phase from the requested Timeline sample. |
| S8 trim/re-expand | A nonzero Source-origin clip retains its original phase anchor. Left/right trim and re-expand reproduce the original reference with no phase accumulation. |
| S9 split / seam | Exact (160), fractional (161), and interior (513) Timeline splits reconstruct identically: no duplicate, gap, shift, or reset discontinuity. Independent views derive their phase from the common original Timeline placement. |
| S10 long / memory | 1, 3, 6, and 12-hour 44.1-to-48 arithmetic markers are exact. State remains the fixed cache plus fixed integer view state; no duration-scaled decoded PCM. |
| S11 views / sources | Overlapping/disjoint views with independent nonzero Source origins have independent integer phase and exact deterministic sum. This fixture exercises 44.1-to-48 ZOH; same-rate 48 kHz is already an exact native path, not a second converted source. |
| S12 reconstruction | Fresh WAV reader/cache/worker/views rebuilt from Source origin, Timeline placement, and P/Q produce identical output. |

Constraints: this is a timing/ownership feasibility result for ZOH only, not a production-quality bandlimited SRC result. It does not add FFmpeg, device I/O, GUI, live rate changes, or a general DSP resampler.

The former `mixed_rate_src.cpp` and all floating-JUCE-ZOH timing observations are retained only as historical root-cause evidence and are superseded for S4--S12 timing conclusions. JUCE floating ZOH is not authoritative for Source-to-Timeline sample identity in this prototype.

## Final evidence

`IntegerRationalZoh` is the authoritative Source-to-Timeline timing path. Floating JUCE ZOH timing evidence is superseded and retained only as root-cause history. S4 through S12 all pass, and `option_b_mixed_rate_src` CTest passes.

Callback forbidden operations are 0: reader/file/decode, wait/block/spin, allocation/growth, and synchronous fallback. The native cache remains fixed at 8224 bytes (8 x 257 float frames). Implementation complexity is a bounded source/SRC extension.

Final classification: **Proceed with Constraints**. Remaining constraints: ZOH timing feasibility only; production-quality bandlimited SRC, FFmpeg, device I/O, GUI, and live rate changes remain deferred.
