#include <tracktion_engine/tracktion_engine.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace diagnostic
{
using Samples = std::int64_t;
constexpr Samples sampleRate = 48000;

struct Scenario
{
    const char* name;
    Samples sourceLength;
    bool impulse;
    Samples clipStart;
    Samples clipLength;
    Samples renderLength;
    Samples renderStart;
    Samples sourceRate;
    tracktion::engine::ResamplingQuality quality;
    const char* qualityName;
    bool resolver;
};

struct Capture final : juce::AudioFormatWriter::ThreadedWriter::IncomingDataReceiver
{
    void reset (int channels, double, std::int64_t count) override
    {
        ++resetCalls;
        buffer.setSize (channels, static_cast<int> (count));
        buffer.clear();
    }

    void addBlock (std::int64_t number, const juce::AudioBuffer<float>& data, int offset, int count) override
    {
        ++addBlockCalls;
        receivedSamples += count;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.copyFrom (channel, static_cast<int> (number), data, channel, offset, count);
    }

    juce::AudioBuffer<float> buffer;
    int resetCalls = 0, addBlockCalls = 0;
    Samples receivedSamples = 0;
};

tracktion::core::TimePosition pos (Samples samples)
{
    return tracktion::core::TimePosition::fromSeconds (static_cast<double> (samples) / sampleRate);
}

tracktion::core::TimeDuration dur (Samples samples)
{
    return tracktion::core::TimeDuration::fromSeconds (static_cast<double> (samples) / sampleRate);
}

juce::File writeSource (const Scenario& scenario)
{
    const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory)
                          .getChildFile (juce::String ("phase-e-diagnostic-") + scenario.name + ".wav");
    file.deleteFile();
    juce::WavAudioFormat format;
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        throw std::runtime_error ("Could not create diagnostic source");
    std::unique_ptr<juce::AudioFormatWriter> writer (
        format.createWriterFor (stream.release(), scenario.sourceRate, 1, 32, {}, 0));
    if (writer == nullptr)
        throw std::runtime_error ("Could not create diagnostic source writer");
    juce::AudioBuffer<float> samples (1, static_cast<int> (scenario.sourceLength));
    samples.clear();
    if (scenario.impulse)
        samples.setSample (0, 0, 0.25f);
    else
        for (int sample = 0; sample < samples.getNumSamples(); ++sample)
            samples.setSample (0, sample, 0.25f);
    if (! writer->writeFromAudioSampleBuffer (samples, 0, samples.getNumSamples()))
        throw std::runtime_error ("Could not write diagnostic source");
    return file;
}

void run (const Scenario& scenario)
{
    const auto source = writeSource (scenario);
    juce::ScopedJuceInitialiser_GUI initialiser;
    tracktion::engine::Engine engine ("AudioNLE Phase E renderer diagnostic");
    const auto editFile = juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile (juce::String ("phase-e-diagnostic-") + scenario.name + ".tracktionedit");
    auto edit = tracktion::engine::createEmptyEdit (engine, editFile);
    if (scenario.resolver)
        edit->filePathResolver = [&source] (const juce::String&)
        {
            return source;
        };
    edit->ensureNumberOfAudioTracks (1);
    edit->getMasterVolumePlugin()->setVolumeDb (0.0f);
    auto* track = tracktion::engine::getAudioTracks (*edit)[0];
    track->getVolumePlugin()->setVolumeDb (0.0f);
    const tracktion::engine::ClipPosition position {{ pos (scenario.clipStart), dur (scenario.clipLength) }, dur (0)};
    auto runtimeClip = track->insertWaveClip ("diagnostic", source, position, false);
    if (runtimeClip == nullptr)
        throw std::runtime_error ("Could not insert diagnostic source Clip");
    runtimeClip->setResamplingQuality (scenario.quality);

    juce::TemporaryFile output (".wav");
    tracktion::engine::Renderer::Parameters parameters (*edit);
    parameters.destFile = output.getFile();
    parameters.audioFormat = engine.getAudioFileFormatManager().getWavFormat();
    parameters.sampleRateForAudio = sampleRate;
    parameters.blockSizeForAudio = 128;
    parameters.bitDepth = 32;
    parameters.time = tracktion::core::TimeRange (pos (scenario.renderStart), pos (scenario.renderStart + scenario.renderLength));
    parameters.usePlugins = true;
    parameters.useMasterPlugins = true;
    parameters.canRenderInMono = false;
    parameters.mustRenderInMono = false;
    Capture capture;
    tracktion::engine::Renderer::RenderTask task ("Phase E renderer diagnostic", parameters, nullptr, &capture);
    const auto started = juce::Time::getMillisecondCounter();
    const auto deadline = started + 10000u;
    std::uint64_t runCalls = 0;
    while (task.runJob() == juce::ThreadPoolJob::jobNeedsRunningAgain)
    {
        ++runCalls;
        if (juce::Time::getMillisecondCounter() > deadline)
        {
            std::cout << "RENDERER_DIAGNOSTIC_TIMEOUT scenario=" << scenario.name
                      << " run_calls=" << runCalls << " elapsed_ms=10000 reset_calls=" << capture.resetCalls
                      << " add_block_calls=" << capture.addBlockCalls << " received_samples=" << capture.receivedSamples
                      << " error=" << task.errorMessage << '\n';
            return;
        }
    }
    Samples observed = -1;
    if (scenario.impulse)
        for (int index = 0; index < capture.buffer.getNumSamples(); ++index)
            if (std::abs (capture.buffer.getSample (0, index)) > 0.0001f) { observed = scenario.renderStart + index; break; }
    std::cout << "RENDERER_DIAGNOSTIC_COMPLETE scenario=" << scenario.name
              << " run_calls=" << runCalls << " elapsed_ms=" << (juce::Time::getMillisecondCounter() - started)
              << " reset_calls=" << capture.resetCalls << " add_block_calls=" << capture.addBlockCalls
              << " received_samples=" << capture.receivedSamples << " output_samples=" << capture.buffer.getNumSamples()
              << " source_rate=" << scenario.sourceRate << " render_start=" << scenario.renderStart
              << " quality=" << scenario.qualityName << " observed_event=" << observed
              << " offset=" << (observed < 0 ? -1 : observed - scenario.clipStart)
              << " error=" << task.errorMessage << '\n';
}
} // namespace diagnostic

int main()
{
    try
    {
        using namespace diagnostic;
        // Each following scenario changes exactly one relevant dimension from its predecessor.
        using Q = tracktion::engine::ResamplingQuality;
        run ({ "c-baseline", 256, false, 100, 64, 256, 0, 48000, Q::lagrange, "lagrange", true });
        run ({ "impulse-content", 256, true, 100, 64, 256, 0, 48000, Q::lagrange, "lagrange", true });
        run ({ "long-source", 4096, true, 100, 64, 256, 0, 48000, Q::lagrange, "lagrange", true });
        run ({ "long-range", 4096, true, 100, 64, 4096, 0, 48000, Q::lagrange, "lagrange", true });
        run ({ "event-position", 4096, true, 1024, 64, 4096, 0, 48000, Q::lagrange, "lagrange", true });
        run ({ "long-clip", 4096, true, 1024, 4096, 4096, 0, 48000, Q::lagrange, "lagrange", true });
        // Source/resampler latency diagnostic. No Domain or output-buffer compensation is applied.
        run ({ "event-1", 4096, true, 1, 4096, 4096, 0, 48000, Q::lagrange, "lagrange", true });
        run ({ "event-100", 4096, true, 100, 4096, 4096, 0, 48000, Q::lagrange, "lagrange", true });
        run ({ "event-2048", 4096, true, 2048, 4096, 4096, 0, 48000, Q::lagrange, "lagrange", true });
        run ({ "event-3000", 4096, true, 3000, 4096, 4096, 0, 48000, Q::lagrange, "lagrange", true });
        run ({ "start-512", 4096, true, 1024, 4096, 4096, 512, 48000, Q::lagrange, "lagrange", true });
        run ({ "start-1000", 4096, true, 1024, 4096, 4096, 1000, 48000, Q::lagrange, "lagrange", true });
        run ({ "source-44100", 4096, true, 1024, 4096, 4096, 0, 44100, Q::lagrange, "lagrange", true });
        run ({ "source-96000", 8192, true, 1024, 4096, 4096, 0, 96000, Q::lagrange, "lagrange", true });
        run ({ "sinc-fast", 4096, true, 1024, 4096, 4096, 0, 48000, Q::sincFast, "sincFast", true });
        run ({ "sinc-medium", 4096, true, 1024, 4096, 4096, 0, 48000, Q::sincMedium, "sincMedium", true });
        run ({ "sinc-best", 4096, true, 1024, 4096, 4096, 0, 48000, Q::sincBest, "sincBest", true });
        run ({ "without-resolver", 4096, true, 1024, 4096, 4096, 0, 48000, Q::lagrange, "lagrange", false });
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "RENDERER_DIAGNOSTIC_FAIL " << error.what() << '\n';
        return 1;
    }
}
