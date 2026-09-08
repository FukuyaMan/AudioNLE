# Option B VST3 Tail T0/T1 Investigation

Public JUCE `AudioPluginInstance::getTailLengthSeconds()` returns seconds. The local mono fixture reports `1024 / 48000`; after `setRateAndBufferSizeDetails` and `prepareToPlay`, `round(seconds * 48000)` yields exactly 1024. Direct host execution observed Tail `[1480,2504)`: first Tail 1480, last nonzero 2503, and first zero 2504. Fixture latency is zero. The pre-prepare report is diagnostic only and is not Domain or graph metadata.

T2--T4 use the same post-prepare report only to instantiate framework-free runtime extent metadata: `SourceEnd=1480`, `TailDurationSamples=1024`, and `ProcessingEnd=2504`. The source scheduler emits `0.25` through sample 1479 and exact zero at 1480, within the `[1408,1536)` process block. The hosted fixture emits Tail `[1480,2504)`, followed by exact zero. The measured renderer has process growth, wait, and source-I/O counters all zero.

At sample 1497, raw Tail is `0.125`; downstream unconditional Multiply(2) observes `0.25`. Ordinary two-input summing observes `0.125` for A Tail only (1600), `0.625` for A Tail plus B active (2000), and `0.5` for B only (2520). The maximum observed boundary and timing error is zero samples. This evidence covers only T2--T4; Move, Delete, policy, reconstruction, and classification remain unexecuted.
