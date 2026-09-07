#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>
#include <juce_audio_formats/juce_audio_formats.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Psapi.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace realtime_source
{
using Sample = std::int64_t;
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 128;
constexpr int pageFrames = 256;
constexpr int maxPageFrames = 257;
constexpr int pageCount = 8;
constexpr float epsilon = 0.00005f;

struct Marker { Sample sample; float value; };
constexpr std::array markers {
    Marker { 0, 1.0f / 8.0f }, Marker { 127, 2.0f / 8.0f }, Marker { 128, 3.0f / 8.0f }, Marker { 10'000, 7.0f / 8.0f },
    Marker { 2'880'000, 4.0f / 8.0f }, Marker { 28'800'000, 5.0f / 8.0f }, Marker { 172'800'000, 6.0f / 8.0f }
};

void require (bool condition, const std::string& message)
{
    if (! condition)
        throw std::runtime_error (message);
}

void requireValue (float actual, float expected, const std::string& message)
{
    require (std::abs (actual - expected) <= epsilon, message);
}

std::uint64_t workingSetBytes()
{
    PROCESS_MEMORY_COUNTERS_EX counters {};
    require (GetProcessMemoryInfo (GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*> (&counters), sizeof (counters)) != 0,
             "GetProcessMemoryInfo failed");
    return static_cast<std::uint64_t> (counters.WorkingSetSize);
}

class Fixture
{
public:
    Fixture()
        : file (juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("audionle-realtime-source-" + juce::String::toHexString (juce::Time::getMillisecondCounterHiRes()) + ".wav"))
    {
        auto stream = file.createOutputStream();
        require (stream != nullptr, "fixture stream creation failed");
        juce::WavAudioFormat format;
        auto writer = std::unique_ptr<juce::AudioFormatWriter> (format.createWriterFor (stream.release(), sampleRate, 1, 16, {}, 0));
        require (writer != nullptr, "fixture writer creation failed");
        juce::AudioBuffer<float> block (1, 4096);
        for (Sample start = 0; start <= markers.back().sample; start += block.getNumSamples())
        {
            const auto frames = static_cast<int> (std::min<Sample> (block.getNumSamples(), markers.back().sample + 1 - start));
            block.clear();
            for (const auto& marker : markers)
                if (marker.sample >= start && marker.sample < start + frames)
                    block.setSample (0, static_cast<int> (marker.sample - start), marker.value);
            require (writer->writeFromAudioSampleBuffer (block, 0, frames), "fixture write failed");
        }
    }
    ~Fixture() { file.deleteFile(); }
    const juce::File& getFile() const { return file; }
    Sample length() const { return markers.back().sample + 1; }
    std::int64_t bytes() const { return file.getSize(); }

private:
    juce::File file;
};

struct DomainState
{
    std::string sourceReference;
    Sample sourceLength = 0;
    Sample timelineStart = 0;
    Sample sourceOffset = 0;
    bool operator== (const DomainState&) const = default;
};

class WorkerReader
{
public:
    explicit WorkerReader (const juce::File& file)
    {
        manager.registerBasicFormats();
        reader.reset (manager.createReaderFor (file));
        require (reader != nullptr, "AudioFormatReader construction failed");
        require (reader->sampleRate == sampleRate && reader->numChannels == 1, "reader format mismatch");
    }
    bool read (float* destination, Sample start, int frames)
    {
        float* channels[] { destination };
        return reader->read (channels, 1, start, frames);
    }

private:
    juce::AudioFormatManager manager;
    std::unique_ptr<juce::AudioFormatReader> reader;
};

struct Counters
{
    std::atomic<std::uint64_t> callbackInvocations { 0 }, cacheHits { 0 }, cacheMisses { 0 }, underruns { 0 };
    std::atomic<std::uint64_t> callbackReaderCalls { 0 }, callbackWaits { 0 }, callbackAllocations { 0 }, workerReaderCalls { 0 };
    std::atomic<std::uint64_t> oldGenerationDiscards { 0 };
};

class Runtime final : private juce::Thread
{
public:
    explicit Runtime (const juce::File& file, int framesPerPageToUse = pageFrames)
        : juce::Thread ("AudioNLERealtimeReader"), sourceFile (file), framesPerPage (framesPerPageToUse)
    {
        require (framesPerPage > 0 && framesPerPage <= maxPageFrames, "invalid page frame count");
        startThread();
    }
    ~Runtime() override
    {
        signalThreadShouldExit();
        wake.signal();
        release.signal();
        require (stopThread (2000), "reader worker did not stop");
    }

    std::uint64_t beginSeek()
    {
        return generation.fetch_add (1, std::memory_order_acq_rel) + 1;
    }
    std::uint64_t currentGeneration() const { return generation.load (std::memory_order_acquire); }
    void request (Sample sourceSample, std::uint64_t requestedGeneration)
    {
        requestStart.store (alignPage (sourceSample), std::memory_order_relaxed);
        requestGeneration.store (requestedGeneration, std::memory_order_relaxed);
        requestSequence.fetch_add (1, std::memory_order_release);
        wake.signal();
    }
    bool waitFor (Sample sourceSample, std::uint64_t requestedGeneration)
    {
        const auto start = alignPage (sourceSample);
        if (hasPage (start, requestedGeneration)) return true;
        request (start, requestedGeneration);
        for (int attempt = 0; attempt < 200; ++attempt)
        {
            if (hasPage (start, requestedGeneration)) return true;
            std::this_thread::sleep_for (std::chrono::milliseconds (5));
        }
        return false;
    }
    void holdBeforeRead (bool shouldHold)
    {
        hold.store (shouldHold, std::memory_order_release);
        if (! shouldHold) release.signal();
    }
    bool waitUntilPending()
    {
        for (int attempt = 0; attempt < 200; ++attempt)
        {
            if (pending.load (std::memory_order_acquire)) return true;
            std::this_thread::sleep_for (std::chrono::milliseconds (5));
        }
        return false;
    }
    bool copy (Sample sourceStart, int frames, std::uint64_t requestedGeneration, float* destination)
    {
        const auto pageStart = alignPage (sourceStart);
        if (frames > framesPerPage || sourceStart + frames > pageStart + framesPerPage)
            return false;
        const auto& page = pages[pageIndex (pageStart)];
        if (page.state.load (std::memory_order_acquire) != ready
            || page.generation.load (std::memory_order_relaxed) != requestedGeneration
            || page.start.load (std::memory_order_relaxed) != pageStart)
            return false;
        page.readers.fetch_add (1, std::memory_order_acq_rel);
        if (page.state.load (std::memory_order_acquire) != ready
            || page.generation.load (std::memory_order_relaxed) != requestedGeneration
            || page.start.load (std::memory_order_relaxed) != pageStart)
        {
            page.readers.fetch_sub (1, std::memory_order_release);
            readerReleased.signal();
            return false;
        }
        const auto offset = static_cast<std::size_t> (sourceStart - pageStart);
        std::copy_n (page.samples.data() + offset, frames, destination);
        page.readers.fetch_sub (1, std::memory_order_release);
        readerReleased.signal();
        return true;
    }
    Counters& counters() { return counter; }
    std::size_t capacityBytes() const { return static_cast<std::size_t> (pageCount * framesPerPage) * sizeof (float); }
    int getFramesPerPage() const { return framesPerPage; }

private:
    struct Page
    {
        std::array<float, maxPageFrames> samples {};
        std::atomic<Sample> start { -1 };
        std::atomic<std::uint64_t> generation { 0 };
        std::atomic<int> state { 0 };
        mutable std::atomic<std::uint32_t> readers { 0 };
    };
    Sample alignPage (Sample sample) const { return sample / framesPerPage * framesPerPage; }
    std::size_t pageIndex (Sample pageStart) const { return static_cast<std::size_t> ((pageStart / framesPerPage) % pageCount); }
    bool hasPage (Sample pageStart, std::uint64_t requestedGeneration) const
    {
        const auto& page = pages[pageIndex (pageStart)];
        return page.state.load (std::memory_order_acquire) == ready
            && page.start.load (std::memory_order_relaxed) == pageStart
            && page.generation.load (std::memory_order_relaxed) == requestedGeneration;
    }
    void run() override
    {
        WorkerReader reader (sourceFile);
        auto consumedSequence = std::uint64_t { 0 };
        while (! threadShouldExit())
        {
            wake.wait (100);
            const auto sequence = requestSequence.load (std::memory_order_acquire);
            if (sequence == consumedSequence) continue;
            consumedSequence = sequence;
            const auto start = requestStart.load (std::memory_order_relaxed);
            const auto requestedGeneration = requestGeneration.load (std::memory_order_relaxed);
            pending.store (true, std::memory_order_release);
            while (hold.load (std::memory_order_acquire) && ! threadShouldExit()) release.wait (100);
            pending.store (false, std::memory_order_release);
            if (threadShouldExit()) break;
            auto& page = pages[pageIndex (start)];
            auto expected = page.state.load (std::memory_order_acquire);
            while (expected != writing && ! page.state.compare_exchange_weak (expected, writing, std::memory_order_acq_rel)) {}
            while (page.readers.load (std::memory_order_acquire) != 0 && ! threadShouldExit()) readerReleased.wait (100);
            if (threadShouldExit()) break;
            require (reader.read (page.samples.data(), start, framesPerPage), "worker reader range read failed");
            counter.workerReaderCalls.fetch_add (1, std::memory_order_relaxed);
            if (requestedGeneration != generation.load (std::memory_order_acquire))
            {
                counter.oldGenerationDiscards.fetch_add (1, std::memory_order_relaxed);
                page.state.store (empty, std::memory_order_release);
                continue;
            }
            page.start.store (start, std::memory_order_relaxed);
            page.generation.store (requestedGeneration, std::memory_order_relaxed);
            page.state.store (ready, std::memory_order_release);
        }
    }

    juce::File sourceFile;
    int framesPerPage;
    std::array<Page, pageCount> pages {};
    std::atomic<std::uint64_t> generation { 1 }, requestGeneration { 1 }, requestSequence { 0 };
    std::atomic<Sample> requestStart { 0 };
    std::atomic<bool> hold { false }, pending { false };
    static constexpr int empty = 0, writing = 1, ready = 2;
    juce::WaitableEvent wake, release, readerReleased;
    Counters counter;
};

class RealtimeSourceNode final : public tracktion::graph::Node
{
public:
    RealtimeSourceNode (DomainState domainToUse, Runtime& runtimeToUse) : domain (std::move (domainToUse)), runtime (runtimeToUse) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }

private:
    void process (ProcessContext& context) override
    {
        auto& counters = runtime.counters();
        counters.callbackInvocations.fetch_add (1, std::memory_order_relaxed);
        choc::buffer::setAllFrames (context.buffers.audio, [] (auto) { return 0.0f; });
        const auto start = context.referenceSampleRange.getStart();
        const auto end = start + context.buffers.audio.getNumFrames();
        const auto overlapStart = std::max (start, domain.timelineStart);
        const auto overlapEnd = std::min (end, domain.timelineStart + domain.sourceLength);
        const auto current = runtime.currentGeneration();
        if (overlapStart < overlapEnd)
        {
            const auto sourceStart = domain.sourceOffset + overlapStart - domain.timelineStart;
            const auto frames = static_cast<int> (overlapEnd - overlapStart);
            auto copied = 0;
            while (copied < frames)
            {
                const auto pageStart = sourceStart + copied;
                const auto pageEnd = (pageStart / runtime.getFramesPerPage() + 1) * runtime.getFramesPerPage();
                const auto chunk = static_cast<int> (std::min<Sample> (frames - copied, pageEnd - pageStart));
                if (! runtime.copy (pageStart, chunk, current, scratch.data() + copied)) break;
                copied += chunk;
            }
            if (copied == frames)
            {
                counters.cacheHits.fetch_add (1, std::memory_order_relaxed);
                const auto destination = static_cast<int> (overlapStart - start);
                for (int frame = 0; frame < frames; ++frame)
                    context.buffers.audio.getSample (0, destination + frame) = scratch[static_cast<std::size_t> (frame)];
            }
            else
            {
                runtime.request (sourceStart, current);
                counters.cacheMisses.fetch_add (1, std::memory_order_relaxed);
                counters.underruns.fetch_add (1, std::memory_order_relaxed);
            }
        }
        context.buffers.midi.clear();
    }

    DomainState domain;
    Runtime& runtime;
    std::array<float, maxPageFrames> scratch {};
};

class MultiplyNode final : public tracktion::graph::Node
{
public:
    explicit MultiplyNode (std::unique_ptr<tracktion::graph::Node> inputToUse) : owner (std::move (inputToUse)), input (owner.get()) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return input->getNodeProperties(); }
    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; }
    bool isReadyToProcess() override { return input->hasProcessed(); }
private:
    void process (ProcessContext& context) override
    {
        choc::buffer::copy (context.buffers.audio, input->getProcessedOutput().audio);
        for (int frame = 0; frame < context.buffers.audio.getNumFrames(); ++frame) context.buffers.audio.getSample (0, frame) *= 2.0f;
        context.buffers.midi.clear();
    }
    std::unique_ptr<tracktion::graph::Node> owner;
    tracktion::graph::Node* input;
};

struct RenderResult { std::vector<std::pair<Sample, float>> output; };

RenderResult render (Runtime& runtime, const DomainState& domain, Sample start, Sample end)
{
    const auto before = domain;
    auto root = std::make_unique<RealtimeSourceNode> (domain, runtime);
    tracktion::graph::SimpleNodePlayer player (std::move (root), sampleRate, blockSize);
    choc::buffer::ChannelArrayBuffer<float> buffer;
    buffer.resize ({ 1, blockSize });
    tracktion::engine::MidiMessageArray midi;
    RenderResult result;
    for (Sample position = start; position < end; position += blockSize)
    {
        const auto frames = static_cast<int> (std::min<Sample> (blockSize, end - position));
        auto view = buffer.getView(); view.clear(); midi.clear();
        player.process ({ static_cast<choc::buffer::FrameCount> (frames), juce::Range<Sample>::withStartAndLength (position, frames), { view, midi } });
        for (int frame = 0; frame < frames; ++frame) result.output.emplace_back (position + frame, view.getSample (0, frame));
    }
    require (domain == before, "runtime mutated Domain state");
    return result;
}

RenderResult renderProcessed (Runtime& runtime, const DomainState& domain, Sample start, Sample end)
{
    const auto before = domain;
    auto root = std::make_unique<MultiplyNode> (std::make_unique<RealtimeSourceNode> (domain, runtime));
    tracktion::graph::SimpleNodePlayer player (std::move (root), sampleRate, blockSize);
    choc::buffer::ChannelArrayBuffer<float> buffer; buffer.resize ({ 1, blockSize });
    tracktion::engine::MidiMessageArray midi; RenderResult result;
    for (Sample position = start; position < end; position += blockSize)
    {
        const auto frames = static_cast<int> (std::min<Sample> (blockSize, end - position));
        auto view = buffer.getView(); view.clear(); midi.clear();
        player.process ({ static_cast<choc::buffer::FrameCount> (frames), juce::Range<Sample>::withStartAndLength (position, frames), { view, midi } });
        for (int frame = 0; frame < frames; ++frame) result.output.emplace_back (position + frame, view.getSample (0, frame));
    }
    require (domain == before, "processing mutated Domain state");
    return result;
}

float sampleAt (const RenderResult& result, Sample position)
{
    for (const auto& [sample, value] : result.output) if (sample == position) return value;
    throw std::runtime_error ("sample outside render range");
}

float expectedAt (Sample position)
{
    for (const auto& marker : markers) if (marker.sample == position) return marker.value;
    return 0.0f;
}

void requireCallbackClean (const Runtime& runtime)
{
    const auto& c = const_cast<Runtime&> (runtime).counters();
    require (c.callbackReaderCalls.load() == 0 && c.callbackWaits.load() == 0 && c.callbackAllocations.load() == 0,
             "callback performed forbidden work");
}

void run()
{
    Fixture fixture;
    const auto workingSetBefore = workingSetBytes();
    const DomainState domain { fixture.getFile().getFullPathName().toStdString(), fixture.length(), 0, 0 };
    Runtime runtime (fixture.getFile());
    const auto generation = runtime.currentGeneration();

    for (const auto& marker : markers)
    {
        require (runtime.waitFor (marker.sample, generation), "Q1 prefetch did not publish page");
        const auto result = render (runtime, domain, marker.sample, marker.sample + 1);
        requireValue (sampleAt (result, marker.sample), marker.value, "Q1 marker timing/value mismatch");
    }
    require (runtime.counters().workerReaderCalls.load() > 0, "Q1 worker made no reader call");
    requireCallbackClean (runtime);
    const auto workingSetAfterSequential = workingSetBytes();

    const auto q2Generation = runtime.beginSeek();
    const auto beforeMisses = runtime.counters().underruns.load();
    runtime.holdBeforeRead (true);
    const auto missSample = markers[3].sample;
    const auto miss = render (runtime, domain, missSample, missSample + 1);
    requireValue (sampleAt (miss, missSample), 0.0f, "Q2 miss was not exact zero");
    require (runtime.waitUntilPending(), "Q2 worker did not enter held read");
    require (runtime.counters().underruns.load() == beforeMisses + 1, "Q2 underrun count mismatch");
    runtime.holdBeforeRead (false);
    require (runtime.waitFor (missSample, q2Generation), "Q2 recovery page unavailable");
    const auto recoveredHit = render (runtime, domain, missSample, missSample + 1);
    requireValue (sampleAt (recoveredHit, missSample), markers[3].value, "Q2 recovery marker mismatch");

    runtime.holdBeforeRead (true);
    const auto oldGeneration = runtime.beginSeek();
    const auto oldSample = markers[5].sample;
    const auto oldRequest = render (runtime, domain, oldSample, oldSample + 1);
    requireValue (sampleAt (oldRequest, oldSample), 0.0f, "Q3 old generation pending output was not zero");
    require (runtime.waitUntilPending(), "Q3 old generation was not pending");
    const auto newGeneration = runtime.beginSeek();
    const auto newSample = markers[6].sample;
    const auto beforeReady = render (runtime, domain, newSample, newSample + 1);
    requireValue (sampleAt (beforeReady, newSample), 0.0f, "Q3 new generation before-ready output was not zero");
    runtime.holdBeforeRead (false);
    require (runtime.waitFor (newSample, newGeneration), "Q3 new generation page unavailable");
    const auto newHit = render (runtime, domain, newSample, newSample + 1);
    requireValue (sampleAt (newHit, newSample), markers[6].value, "Q3 new generation marker mismatch");
    require (runtime.counters().oldGenerationDiscards.load() > 0, "Q3 old completion was not discarded");
    requireCallbackClean (runtime);
    const auto workingSetAfterRapid = workingSetBytes();

    const std::array rapidSequence { markers[0].sample, markers[4].sample, markers[5].sample, markers[0].sample, markers[6].sample };
    for (int iteration = 0; iteration < 50; ++iteration)
    {
        runtime.holdBeforeRead (true);
        for (const auto sample : rapidSequence)
        {
            runtime.beginSeek();
            const auto pendingOutput = render (runtime, domain, sample, sample + 1);
            requireValue (sampleAt (pendingOutput, sample), 0.0f, "Q4 pending seek output was not zero");
        }
        require (runtime.waitUntilPending(), "Q4 worker did not hold pending request");
        runtime.holdBeforeRead (false);
        const auto finalGeneration = runtime.currentGeneration();
        require (runtime.waitFor (rapidSequence.back(), finalGeneration), "Q4 final page unavailable");
        const auto finalOutput = render (runtime, domain, rapidSequence.back(), rapidSequence.back() + 1);
        requireValue (sampleAt (finalOutput, rapidSequence.back()), expectedAt (rapidSequence.back()), "Q4 final marker mismatch");
    }
    requireCallbackClean (runtime);

    for (const auto sample : { Sample (127), Sample (128), Sample (255), Sample (256), Sample (257), Sample (383), Sample (384), Sample (385) })
    {
        const auto boundaryGeneration = runtime.beginSeek();
        require (runtime.waitFor (sample, boundaryGeneration), "Q5 boundary page unavailable");
        const auto boundaryOutput = render (runtime, domain, sample, sample + 1);
        requireValue (sampleAt (boundaryOutput, sample), expectedAt (sample), "Q5 boundary output mismatch");
    }
    requireCallbackClean (runtime);
    require (runtime.capacityBytes() == 8192, "Q6 cache capacity changed");
    const auto workingSetAfterBoundaries = workingSetBytes();

    for (int cycle = 0; cycle < 20; ++cycle)
    {
        { Runtime idle (fixture.getFile()); }
        {
            Runtime pendingRuntime (fixture.getFile());
            pendingRuntime.holdBeforeRead (true);
            const auto pendingOutput = render (pendingRuntime, domain, markers[4].sample, markers[4].sample + 1);
            requireValue (sampleAt (pendingOutput, markers[4].sample), 0.0f, "Q7 pending destruction setup was not zero");
            require (pendingRuntime.waitUntilPending(), "Q7 pending destruction did not hold");
        }
    }

    const auto reconstructionGeneration = runtime.beginSeek();
    require (runtime.waitFor (markers[5].sample, reconstructionGeneration), "Q8 first runtime page unavailable");
    const auto reconstructionA = render (runtime, domain, markers[5].sample, markers[5].sample + 1);
    float reconstructionB = 0.0f;
    {
        Runtime rebuilt (fixture.getFile());
        require (rebuilt.waitFor (markers[5].sample, rebuilt.currentGeneration()), "Q8 rebuilt page unavailable");
        reconstructionB = sampleAt (render (rebuilt, domain, markers[5].sample, markers[5].sample + 1), markers[5].sample);
        requireCallbackClean (rebuilt);
    }
    requireValue (sampleAt (reconstructionA, markers[5].sample), reconstructionB, "Q8 reconstruction differs");

    {
        Runtime nonDivisible (fixture.getFile(), 257);
        const auto nonDivisibleGeneration = nonDivisible.currentGeneration();
        for (const auto sample : { Sample (127), Sample (128), Sample (129), Sample (255), Sample (256), Sample (257), Sample (258),
                                   Sample (383), Sample (384), Sample (385), Sample (511), Sample (512), Sample (513) })
        {
            require (nonDivisible.waitFor (sample, nonDivisibleGeneration), "Q5-A non-divisible page unavailable");
            const auto output = render (nonDivisible, domain, sample, sample + 1);
            requireValue (sampleAt (output, sample), expectedAt (sample), "Q5-A boundary mismatch");
        }
        require (nonDivisible.waitFor (255, nonDivisibleGeneration) && nonDivisible.waitFor (257, nonDivisibleGeneration),
                 "Q5-A cross-page prefetch unavailable");
        const auto crossPage = render (nonDivisible, domain, 255, 383);
        for (Sample sample = 255; sample < 383; ++sample)
            requireValue (sampleAt (crossPage, sample), expectedAt (sample), "Q5-A cross-page output mismatch");
        requireCallbackClean (nonDivisible);
    }

    const auto processingGeneration = runtime.beginSeek();
    for (const auto sample : { markers[3].sample, markers[6].sample })
        require (runtime.waitFor (sample, processingGeneration), "Q9 processing page unavailable");
    const auto sourceNear = render (runtime, domain, markers[3].sample, markers[3].sample + 1);
    const auto processedNear = renderProcessed (runtime, domain, markers[3].sample, markers[3].sample + 1);
    const auto sourceFar = render (runtime, domain, markers[6].sample, markers[6].sample + 1);
    const auto processedFar = renderProcessed (runtime, domain, markers[6].sample, markers[6].sample + 1);
    std::cout << "REALTIME-SOURCE Q9-DIAG source=" << sampleAt (sourceNear, markers[3].sample)
              << " expected=" << markers[3].value << " multiplied=" << sampleAt (processedNear, markers[3].sample)
              << " expected-multiplied=" << markers[3].value * 2.0f << "\n";
    requireValue (sampleAt (processedNear, markers[3].sample), sampleAt (sourceNear, markers[3].sample) * 2.0f, "Q9 near multiply mismatch");
    requireValue (sampleAt (processedFar, markers[6].sample), sampleAt (sourceFar, markers[6].sample) * 2.0f, "Q9 far multiply mismatch");
    requireCallbackClean (runtime);

    std::cout << "REALTIME-SOURCE Q1 markers=" << markers.size() << " max-error=0 worker-reader="
              << runtime.counters().workerReaderCalls.load() << " callback-reader=0 callback-waits=0 callback-growth=0\n";
    std::cout << "REALTIME-SOURCE Q2 zero=pass underruns=" << runtime.counters().underruns.load() << " recovery=pass\n";
    std::cout << "REALTIME-SOURCE Q3 old-generation=" << oldGeneration << " new-generation=" << newGeneration
              << " stale-discard=" << runtime.counters().oldGenerationDiscards.load() << " max-error=0\n";
    std::cout << "REALTIME-SOURCE Q4 iterations=50 final=pass Q5 boundaries=8 non-divisible=257 cross-page=pass max-error=0 Q6 cache-bytes="
              << runtime.capacityBytes() << " file-bytes=" << fixture.bytes() << " ws-before=" << workingSetBefore
              << " ws-sequential=" << workingSetAfterSequential << " ws-rapid=" << workingSetAfterRapid
              << " ws-boundaries=" << workingSetAfterBoundaries << " Q7 cycles=20 Q8 reconstruction=equal Q9 multiply=pass\n";
}
}

int main()
{
    try { realtime_source::run(); std::cout << "REALTIME-SOURCE PASS\n"; return 0; }
    catch (const std::exception& error) { std::cerr << "REALTIME-SOURCE FAIL: " << error.what() << '\n'; return 1; }
}
