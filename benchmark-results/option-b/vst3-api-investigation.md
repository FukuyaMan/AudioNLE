# Option B VST3 API Investigation

Public JUCE `VST3PluginFormatHeadless`, `AudioPluginFormatManager`, `createPluginInstance`, `setRateAndBufferSizeDetails`, `prepareToPlay`, `getLatencySamples`, and `processBlock` suffice for the fixture. Latency was 0 before prepare and valid only after prepare. Option B owns the two-stage processor/graph preparation boundary. No Tracktion API is used.
