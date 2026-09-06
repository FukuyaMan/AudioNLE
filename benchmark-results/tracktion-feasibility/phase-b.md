# Tracktion Feasibility — Phase B Record

Execution date: 2026-09-06 (Asia/Tokyo)

## T-002 integer sample round-trip

Test ID: T-002 integer sample round-trip

Hypothesis: H3 Integer Timeline Sample.

Environment: Windows 11 Home 10.0.26200 x64; AMD Ryzen 9 7900X; 31.12 GiB RAM; Visual Studio 2026 Build Tools 18.9.2; MSVC 19.51.36256.0 (toolset 14.51.36231); CMake 4.3.1-msvc1; Ninja 1.13.2; Windows SDK 10.0.26100.0.

Dependency: Tracktion Engine `b88a6ee51913668cb53e911e030ab736b13342cf`; nested JUCE `37c894f83d379179b2070d437ccd0f1cd9af9576`. The Phase 0 pins were retained without revision change.

Method: `MinimalDomainState` stores Project Timeline positions/durations as `int64_t` samples at the Project Timeline Sample Rate and Source positions/durations as separate `int64_t` samples at each Media's native sample rate. Only `TracktionAdapter` converts requested samples to Tracktion time values. Observation converts runtime Timeline values at the Project rate and runtime Source offset/duration values at the Media rate, then requires zero sample difference. Runtime values are evidence only; they are never written back into Domain State. Each fixture is constructed, observed, destroyed, and reconstructed ten times; the immutable Domain State must compare bit-for-bit equal after every iteration.

Expected: Runtime observation converted with the appropriate time-domain rate equals every requested integer field; maximum observed error is 0 samples; the 44.1 kHz Source / 48 kHz Project fixture preserves both domains independently; no runtime-derived state becomes authoritative.

### Fixture A — 48 kHz Project / 48 kHz Source

Requested Timeline and Source start sample values: `0`, `1`, `2`, `47999`, `48000`, `48001`, `96000`, `7`, `13`, `101`, `1009`, `12347`, `65537`, `99991`. Every Clip duration is one sample in both domains.

Runtime representation: Tracktion reports seconds. The captured observations include, for example, 0 / `2.08333e-05` / `4.16667e-05` seconds for starts 0 / 1 / 2, `0.999979` / `1` / `1.00002` seconds for 47999 / 48000 / 48001, and `2` seconds for 96000. Converted Timeline and Source observations equal every requested value; difference is 0 samples for all 14 clips.

### Fixture B — long Timeline positions

Project and Source are 48 kHz. For each 1, 3, 6, and 12 hour anchor, the test requests the exact anchor, anchor - 1 sample, and anchor + 1 sample. Source range is [0, 48000) for each Clip.

| Anchor | Timeline samples | Runtime seconds reported | Converted observation | Difference |
| --- | ---: | ---: | ---: | ---: |
| 1 hour | 172800000 ± 1 | printed as 3600 (default display precision) | exact requested anchor ± 1 | 0 |
| 3 hours | 518400000 ± 1 | printed as 10800 (default display precision) | exact requested anchor ± 1 | 0 |
| 6 hours | 1036800000 ± 1 | printed as 21600 (default display precision) | exact requested anchor ± 1 | 0 |
| 12 hours | 2073600000 ± 1 | printed as 43200 (default display precision) | exact requested anchor ± 1 | 0 |

The display rounding above is only `std::cout` formatting. The test converts the in-memory runtime value to Timeline samples and asserts the exact requested integer; all 12 cases passed.

### Fixture C — 44.1 kHz Source / 48 kHz Project

Project Timeline Sample Rate is 48000 Hz; Source native sample rate is 44100 Hz. Each Clip has Timeline Duration 48000 and Source Duration 44100, representing one second without conflating the two sample domains.

| Source start requested / observed | Timeline start requested / observed | Runtime Source seconds (printed) | Runtime Timeline seconds (printed) | Difference |
| ---: | ---: | ---: | ---: | ---: |
| 1 / 1 | 7 / 7 | `2.26757e-05` | `0.000145833` | 0 / 0 |
| 17 / 17 | 1016 / 1016 | `0.000385488` | `0.0211667` | 0 / 0 |
| 101 / 101 | 2025 / 2025 | `0.00229025` | `0.0421875` | 0 / 0 |
| 1000 / 1000 | 3034 / 3034 | `0.0226757` | `0.0632083` | 0 / 0 |
| 22051 / 22051 | 4043 / 4043 | `0.500023` | `0.0842292` | 0 / 0 |
| 44101 / 44101 | 5052 / 5052 | `1.00002` | `0.10525` | 0 / 0 |

All six runtime Timeline durations observed as 48000 samples and all Source durations as 44100 samples. This verifies the fixture's distinct integer domains; it does not choose a production SRC algorithm or rounding policy.

Observed: `PHASE_B_PASS fixtures=3 reconstructions=30 maximum_observed_error_samples=0` and process exit 0. Direct headless output recorded all 32 initial runtime observations. `./build.ps1` rebuilt and ran CTest: Phase A passed in 0.72 s, Phase B passed in 10.12 s, 2/2 passed, total 11.08 s. No physical audio device was opened.

Measurement: 32 Clip observation cases across three fixtures; 30 construct/observe/destroy/reconstruct iterations; maximum observed error = 0 Timeline or Source samples; Domain mutation = none.

Status: Pass.

Workaround: None. The Phase A adapter-owned source resolver remains a local input mapping table; it does not perform rounding, persistence, or reverse synchronization. The only conversion policy exercised is exact integer sample ↔ seconds conversion at the explicitly selected domain rate for the test fixture.

Requirement impact: Phase B passes TIME-001's integer Timeline position criterion and TIME-002's Source/Timeline separation criterion for this prototype scope. H3 passes. No Phase B hard fail was reached.

Notes: This does not select a production rounding API, SRC algorithm/library, project sample-rate-change semantics, playback quality, or offline SRC quality. It also does not test signal rendering, overlap/Track Mix, Processing Stack order, PDC, Effect Tail, runtime editing, long-source streaming performance, dense Clips, or failure injection. The long Timeline fixture tests addressing/mapping only; it does not allocate a long PCM source.
