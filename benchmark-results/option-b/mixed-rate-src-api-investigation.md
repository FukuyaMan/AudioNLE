# Option B Mixed-Rate SRC API Investigation

The pinned public JUCE `GenericInterpolator` declares `process(speedRatio, input, output, outputCount)` where `speedRatio` is input samples per output sample, returns input consumed, is stateful, and requires `reset()` at discontinuities. It exposes `getBaseLatency()`; total process latency is documented as base latency divided by speed ratio.

`ZeroOrderHoldInterpolator` is a public alias with base latency 0. It has fixed internal state and no allocation API. The public documentation does not make a general thread-safety guarantee, so each fixture Clip view owns its own instance and invokes it only on its process path.

`ResamplingAudioSource` is public but owns dynamic buffers and uses callback locks internally, so it is rejected for this strict source-node boundary. A custom DSP implementation was not considered. This is timing/ownership evidence, not a quality endorsement for zero-order hold conversion.

The fixture selects `TimelineBoundary(S) = ceil(S * 160 / 147)` with inverse coverage `SourceBoundary(T) = floor(T * 147 / 160)`. This is a fixture mapping policy, not a product-wide policy decision.

## Streaming observation

The actual fixed-page, worker-fed streaming fixture rejects this candidate at the current wrapper boundary: Source marker 147 has edit boundary 160, but after the documented interpolator reset and bounded anchor preroll it is observed at Timeline 161--162. The primitive reports base latency 0, yet that does not establish a zero, render-start-independent wrapper alignment. No tolerance is accepted for this discrepancy. Therefore S4 and the dependent S5--S12 work remain unexecuted, and the gate remains **Inconclusive**. The active execution plan is retained; no results/complexity document or completed-plan move is warranted.

## Alignment corrective investigation (A0)

The pinned `GenericInterpolator` implementation takes `speedRatio` as input samples per output sample. For 44.1-to-48 kHz this is `147.0 / 160.0`. `process()` copies its persisted `subSamplePos` to a local `pos`; before each output it pushes input while `pos >= 1.0`, writes the trait value, then advances `pos += speedRatio`. It persists the resulting `pos` after the final output and returns the number of pushed input samples.

`reset()` sets `subSamplePos = 1.0`, zeroes `lastInputSamples`, and resets the history index. For the one-sample `ZeroOrderHoldInterpolator`, the first output after reset therefore first pushes `input[0]` and emits that history sample. There is no public operation to set the fractional phase or history independently. `getBaseLatency() == 0` describes only the trait's algorithmic latency; it does not promise first-output edit-boundary alignment after a reset.

The current source-node anchor uses an integer Timeline multiple of 160, supplies `SourceBoundary(anchor)` as `input[0]`, resets the primitive, processes bounded output including the pre-anchor discard, and exposes the requested output range. The generated and WAV observations show this does not establish the selected `ceil` mapping at Source 147. It is a deterministic candidate-wrapper mismatch, not evidence for changing the authoritative mapping.
