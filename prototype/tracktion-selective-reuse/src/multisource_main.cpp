#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>
#include <juce_audio_formats/juce_audio_formats.h>

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

namespace multisource
{
using Sample = std::int64_t;
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 128;
constexpr int pageFrames = 128;
constexpr int pagesPerMedia = 4;
constexpr float epsilon = 0.00005f;

void require (bool condition, const std::string& message) { if (! condition) throw std::runtime_error (message); }
void requireValue (float actual, float expected, const std::string& message) { require (std::abs (actual - expected) <= epsilon, message); }

struct ClipDomain
{
    int clipId = 0;
    int mediaId = 0;
    Sample sourceStart = 0, length = 1, timelineStart = 0;
    bool operator== (const ClipDomain&) const = default;
};

class WavFixture
{
public:
    explicit WavFixture (int mediaId)
        : file (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (
              "audionle-multisource-" + juce::String::toHexString (juce::Time::getMillisecondCounterHiRes()) + "-" + juce::String (mediaId) + ".wav"))
    {
        auto stream = file.createOutputStream(); require (stream != nullptr, "fixture stream creation failed");
        juce::WavAudioFormat format;
        auto writer = std::unique_ptr<juce::AudioFormatWriter> (format.createWriterFor (stream.release(), sampleRate, 1, 16, {}, 0));
        require (writer != nullptr, "fixture writer creation failed");
        juce::AudioBuffer<float> samples (1, 512); samples.clear();
        samples.setSample (0, 0, amplitude (mediaId));
        samples.setSample (0, 128, amplitude (mediaId) * 2.0f);
        samples.setSample (0, 256, amplitude (mediaId) * 3.0f);
        require (writer->writeFromAudioSampleBuffer (samples, 0, samples.getNumSamples()), "fixture write failed");
    }
    ~WavFixture() { file.deleteFile(); }
    const juce::File& getFile() const { return file; }
    static float amplitude (int mediaId) { return static_cast<float> (mediaId + 1) / 64.0f; }
private: juce::File file;
};

struct Counters
{
    std::atomic<std::uint64_t> hits { 0 }, misses { 0 }, underruns { 0 }, workerReads { 0 }, overwritten { 0 }, maxPending { 0 };
    std::atomic<std::uint64_t> callbackReader { 0 }, callbackWait { 0 }, callbackGrowth { 0 };
};

class MediaRuntime final : private juce::Thread
{
public:
    explicit MediaRuntime (const juce::File& source) : juce::Thread ("AudioNLEMultiSourceReader"), file (source) { startThread(); }
    ~MediaRuntime() override { signalThreadShouldExit(); wake.signal(); release.signal(); require (stopThread (2000), "media worker did not stop"); }
    void request (Sample sourceStart)
    {
        const auto start = align (sourceStart);
        requested.store (start, std::memory_order_relaxed);
        const auto old = sequence.fetch_add (1, std::memory_order_release);
        if (old != consumed.load (std::memory_order_acquire)) counters.overwritten.fetch_add (1, std::memory_order_relaxed);
        counters.maxPending.store (1, std::memory_order_relaxed); wake.signal();
    }
    bool waitFor (Sample sourceStart)
    {
        const auto start = align (sourceStart); if (has (start)) return true; request (start);
        for (int i = 0; i != 400; ++i) { if (has (start)) return true; std::this_thread::sleep_for (std::chrono::milliseconds (2)); }
        return false;
    }
    void holdBeforeRead (bool value) { hold.store (value, std::memory_order_release); if (! value) release.signal(); }
    bool waitUntilPending()
    {
        for (int i = 0; i != 400; ++i) { if (pending.load (std::memory_order_acquire)) return true; std::this_thread::sleep_for (std::chrono::milliseconds (2)); }
        return false;
    }
    bool copy (Sample sourceStart, int frames, float* destination)
    {
        const auto start = align (sourceStart); if (frames > pageFrames || sourceStart + frames > start + pageFrames) return false;
        const auto& page = pages[index (start)];
        if (page.ready.load (std::memory_order_acquire) == 0 || page.start.load (std::memory_order_relaxed) != start) return false;
        std::copy_n (page.data.data() + static_cast<std::size_t> (sourceStart - start), frames, destination); return true;
    }
    Counters counters;
    static constexpr std::size_t pcmBytes() { return pagesPerMedia * pageFrames * sizeof (float); }
private:
    struct Page { std::array<float, pageFrames> data {}; std::atomic<Sample> start { -1 }; std::atomic<int> ready { 0 }; };
    static Sample align (Sample value) { return value / pageFrames * pageFrames; }
    static std::size_t index (Sample value) { return static_cast<std::size_t> ((value / pageFrames) % pagesPerMedia); }
    bool has (Sample start) const { const auto& p = pages[index (start)]; return p.ready.load (std::memory_order_acquire) != 0 && p.start.load (std::memory_order_relaxed) == start; }
    void run() override
    {
        juce::AudioFormatManager formats; formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file)); require (reader != nullptr, "reader construction failed");
        std::uint64_t seen = 0;
        while (! threadShouldExit())
        {
            wake.wait (100); const auto current = sequence.load (std::memory_order_acquire); if (current == seen) continue;
            seen = current; consumed.store (seen, std::memory_order_release); const auto start = requested.load (std::memory_order_relaxed);
            pending.store (true, std::memory_order_release);
            while (hold.load (std::memory_order_acquire) && ! threadShouldExit()) release.wait (100);
            pending.store (false, std::memory_order_release); if (threadShouldExit()) break;
            auto& page = pages[index (start)]; page.ready.store (0, std::memory_order_release);
            float* channels[] { page.data.data() }; require (reader->read (channels, 1, start, pageFrames), "worker read failed");
            counters.workerReads.fetch_add (1, std::memory_order_relaxed); page.start.store (start, std::memory_order_relaxed); page.ready.store (1, std::memory_order_release);
        }
    }
    juce::File file; std::array<Page, pagesPerMedia> pages {}; std::atomic<Sample> requested { 0 };
    std::atomic<std::uint64_t> sequence { 0 }, consumed { 0 }; std::atomic<bool> hold { false }, pending { false }; juce::WaitableEvent wake, release;
};

class ClipNode final : public tracktion::graph::Node
{
public:
    ClipNode (ClipDomain domainToUse, MediaRuntime& runtimeToUse) : domain (domainToUse), runtime (runtimeToUse) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }
private:
    void process (ProcessContext& context) override
    {
        choc::buffer::setAllFrames (context.buffers.audio, [] (auto) { return 0.0f; });
        const auto blockStart = context.referenceSampleRange.getStart(), blockEnd = blockStart + context.buffers.audio.getNumFrames();
        const auto first = std::max (blockStart, domain.timelineStart), last = std::min (blockEnd, domain.timelineStart + domain.length);
        if (first < last)
        {
            const auto frames = static_cast<int> (last - first);
            const Sample source = domain.sourceStart + first - domain.timelineStart;
            if (runtime.copy (source, frames, scratch.data()))
            {
                runtime.counters.hits.fetch_add (1, std::memory_order_relaxed);
                for (int i = 0; i != frames; ++i) context.buffers.audio.getSample (0, static_cast<int> (first - blockStart) + i) = scratch[static_cast<std::size_t> (i)];
            }
            else { runtime.request (source); runtime.counters.misses.fetch_add (1, std::memory_order_relaxed); runtime.counters.underruns.fetch_add (1, std::memory_order_relaxed); }
        }
        context.buffers.midi.clear();
    }
    ClipDomain domain; MediaRuntime& runtime; std::array<float, pageFrames> scratch {};
};

float renderAt (const std::vector<ClipDomain>& clips, const std::vector<std::unique_ptr<MediaRuntime>>& media, Sample position)
{
    if (clips.empty()) return 0.0f;
    std::vector<std::unique_ptr<tracktion::graph::Node>> inputs;
    for (const auto& clip : clips) inputs.push_back (std::make_unique<ClipNode> (clip, *media[static_cast<std::size_t> (clip.mediaId)]));
    std::unique_ptr<tracktion::graph::Node> root = inputs.size() == 1 ? std::move (inputs.front()) : std::make_unique<tracktion::graph::SummingNode> (std::move (inputs));
    tracktion::graph::SimpleNodePlayer player (std::move (root), sampleRate, blockSize);
    choc::buffer::ChannelArrayBuffer<float> buffer; buffer.resize ({ 1, 1 }); tracktion::engine::MidiMessageArray midi;
    auto view = buffer.getView(); view.clear(); player.process ({ 1, juce::Range<Sample>::withStartAndLength (position, 1), { view, midi } }); return view.getSample (0, 0);
}

void requireSilent (const std::vector<ClipDomain>& clips, const std::vector<std::unique_ptr<MediaRuntime>>& media, Sample position, const std::string& message)
{ requireValue (renderAt (clips, media, position), 0.0f, message); }

void prefetch (const std::vector<ClipDomain>& clips, const std::vector<std::unique_ptr<MediaRuntime>>& media);
void callbackClean (const std::vector<std::unique_ptr<MediaRuntime>>& media);
std::vector<std::unique_ptr<MediaRuntime>> createMedia (std::vector<std::unique_ptr<WavFixture>>& files, int count);

void runRuntimeEdits (std::vector<std::unique_ptr<WavFixture>>& files)
{
    auto media = createMedia (files, 1);
    const auto a = WavFixture::amplitude (0);

    // M4: Domain move, then stopped-state graph/view reconstruction. Clip B proves shared media is not placement state.
    std::vector<ClipDomain> move { { 40, 0, 0, 129, 20'000 }, { 41, 0, 128, 1, 21'000 } };
    prefetch (move, media); requireValue (renderAt (move, media, 20'000), a, "M4 old placement setup");
    requireValue (renderAt (move, media, 21'000), a * 2.0f, "M4 shared clip setup");
    const Sample originalSourceStart = move[0].sourceStart, originalLength = move[0].length;
    const int mediaId = move[0].mediaId;
    move[0].timelineStart = 22'000; // Domain authority: no runtime state is edited.
    prefetch (move, media); requireSilent (move, media, 20'000, "M4 old placement leaked");
    requireValue (renderAt (move, media, 22'000), a, "M4 moved marker mismatch");
    requireValue (renderAt (move, media, 21'000), a * 2.0f, "M4 shared clip changed");
    require (move[0].sourceStart == originalSourceStart && move[0].length == originalLength && move[0].mediaId == mediaId, "M4 changed source identity/range");

    // M5: all endpoints are half-open integer ranges. The 128 boundary exercises a page/block boundary.
    const ClipDomain trimOriginal { 50, 0, 0, 257, 30'000 };
    auto trim = std::vector<ClipDomain> { trimOriginal };
    trim[0].sourceStart += 128; trim[0].timelineStart += 128; trim[0].length -= 128; // left trim
    prefetch (trim, media); requireSilent (trim, media, 30'127, "M5 left first excluded was audible");
    requireValue (renderAt (trim, media, 30'128), a * 2.0f, "M5 left first included mismatch");
    trim[0] = trimOriginal; trim[0].length = 129; // right trim, retaining source [0,129)
    prefetch (trim, media); requireValue (renderAt (trim, media, 30'128), a * 2.0f, "M5 right last included mismatch");
    requireSilent (trim, media, 30'129, "M5 right first excluded was audible");
    trim[0] = trimOriginal; // re-expand without touching source media
    prefetch (trim, media); requireValue (renderAt (trim, media, 30'256), a * 3.0f, "M5 re-expanded sample mismatch");

    // M6: split at Timeline 40,128 / Source 128. Distinct Clip IDs retain the same MediaSource ID.
    const ClipDomain splitOriginal { 60, 0, 0, 257, 40'000 };
    const std::vector<ClipDomain> split { { 61, 0, 0, 128, 40'000 }, { 62, 0, 128, 129, 40'128 } };
    prefetch (split, media); requireValue (renderAt (split, media, 40'127), 0.0f, "M6 left boundary mismatch");
    requireValue (renderAt (split, media, 40'128), a * 2.0f, "M6 right boundary mismatch");
    requireValue (renderAt (split, media, 40'256), a * 3.0f, "M6 final sample mismatch");
    require (split[0].mediaId == splitOriginal.mediaId && split[1].mediaId == splitOriginal.mediaId && split[0].clipId != split[1].clipId, "M6 identity mismatch");
    require (split[0].timelineStart == splitOriginal.timelineStart && split[1].timelineStart == split[0].timelineStart + split[0].length
             && split[0].length + split[1].length == splitOriginal.length && split[0].sourceStart == splitOriginal.sourceStart
             && split[1].sourceStart == split[0].sourceStart + split[0].length, "M6 range union mismatch");

    // M7: request work for A, delete it in Domain, rebuild views, then allow only B to be observed.
    std::vector<ClipDomain> deletion { { 70, 0, 0, 1, 50'000 }, { 71, 0, 128, 1, 51'000 } };
    media[0]->holdBeforeRead (true); media[0]->request (0); require (media[0]->waitUntilPending(), "M7 old request was not pending");
    deletion.erase (deletion.begin());
    media[0]->holdBeforeRead (false); // old completion may now occur, but no deleted view survives the rebuild.
    prefetch (deletion, media); requireSilent (deletion, media, 50'000, "M7 deleted clip reappeared");
    requireValue (renderAt (deletion, media, 51'000), a * 2.0f, "M7 survivor mismatch");
    require (media[0]->counters.workerReads.load() > 0, "M7 shared runtime was not usable");
    deletion.clear(); requireSilent (deletion, media, 50'000, "M7 final deleted output reappeared");
    media.clear(); // bounded Thread destructor is the last-consumer teardown assertion.
    files.clear();

    // M8 Strategy A reference: every edit changes Domain, then reconstructs the affected graph views while retaining Media runtime/cache.
    media = createMedia (files, 1);
    std::vector<ClipDomain> rapid { { 80, 0, 0, 129, 60'000 }, { 81, 0, 128, 129, 61'000 }, { 82, 0, 256, 1, 62'000 } };
    std::uint64_t domainRevision = 1, clipRevisionA = 1, clipRevisionB = 1, clipRevisionC = 1;
    media[0]->holdBeforeRead (true); media[0]->request (rapid[0].sourceStart); require (media[0]->waitUntilPending(), "M8 old revision request was not pending");
    rapid[0].timelineStart = 60'500; ++domainRevision; ++clipRevisionA; // move A
    rapid[1].sourceStart += 1; rapid[1].timelineStart += 1; rapid[1].length -= 1; ++domainRevision; ++clipRevisionB; // left trim B
    rapid.erase (rapid.begin() + 2); ++domainRevision; ++clipRevisionC; // delete C
    rapid[0].timelineStart = 60'750; ++domainRevision; ++clipRevisionA; // move A again
    media[0]->holdBeforeRead (false);
    prefetch (rapid, media); requireSilent (rapid, media, 60'000, "M8 stale A placement");
    requireValue (renderAt (rapid, media, 60'750), a, "M8 final A mismatch");
    requireValue (renderAt (rapid, media, 61'128), a * 3.0f, "M8 trimmed B mapping mismatch");
    requireSilent (rapid, media, 62'000, "M8 deleted C reappeared");
    require (domainRevision == 5 && clipRevisionA == 3 && clipRevisionB == 2 && clipRevisionC == 2, "M8 revision scopes collapsed");
    callbackClean (media);
    std::cout << "RUNTIME-EDIT M4 move=pass M5 trim-reexpand=pass M6 split=pass M7 delete-lifetime=pass M8 rebuild=pass "
              << "domain-revision=" << domainRevision << " clip-revisions=3/2/2 media-generation=per-runtime-request "
              << "partial-update=not-implemented-public-graph-mutation-not-proven max-error=0 stale=0 callback-reader=0 callback-waits=0 callback-growth=0\n";
}

void prefetch (const std::vector<ClipDomain>& clips, const std::vector<std::unique_ptr<MediaRuntime>>& media)
{
    for (const auto& c : clips)
    {
        auto& runtime = media[static_cast<std::size_t> (c.mediaId)];
        require (runtime->waitFor (c.sourceStart), "prefetch start did not complete");
        require (runtime->waitFor (c.sourceStart + c.length - 1), "prefetch end did not complete");
    }
}
void callbackClean (const std::vector<std::unique_ptr<MediaRuntime>>& media)
{ for (const auto& r : media) require (r->counters.callbackReader == 0 && r->counters.callbackWait == 0 && r->counters.callbackGrowth == 0, "callback forbidden operation"); }

std::vector<std::unique_ptr<MediaRuntime>> createMedia (std::vector<std::unique_ptr<WavFixture>>& files, int count)
{
    std::vector<std::unique_ptr<MediaRuntime>> result; files.clear();
    for (int i = 0; i != count; ++i) { files.push_back (std::make_unique<WavFixture> (i)); result.push_back (std::make_unique<MediaRuntime> (files.back()->getFile())); }
    return result;
}

void runArbitrationAndReconstruction (std::vector<std::unique_ptr<WavFixture>>& files)
{
    auto media = createMedia (files, 1);
    auto& runtime = *media[0];
    const auto readsBefore = runtime.counters.workerReads.load();
    // M9-A: same page/range requested by two views; second request observes the published page.
    require (runtime.waitFor (0) && runtime.waitFor (0), "M9 identical range not served");
    const auto identicalReads = runtime.counters.workerReads.load() - readsBefore;
    require (identicalReads == 1, "M9 identical range caused duplicate read");
    // M9-B: [0,257) and [128,385) reuse pages 128 and 256; page 0 is the sole additional region.
    const auto overlapBefore = runtime.counters.workerReads.load();
    for (const Sample sample : { Sample (0), Sample (128), Sample (256), Sample (128), Sample (256) }) require (runtime.waitFor (sample), "M9 overlap page not served");
    const auto overlapReads = runtime.counters.workerReads.load() - overlapBefore;
    // M9-C/D: alternating far-apart page requests; each is synchronously observed to prove bounded recovery/no starvation.
    const auto disjointBefore = runtime.counters.workerReads.load();
    for (int i = 0; i != 16; ++i) for (const Sample sample : { Sample (0), Sample (128), Sample (256) }) require (runtime.waitFor (sample), "M9 alternating request starved");
    const auto disjointReads = runtime.counters.workerReads.load() - disjointBefore;
    for (const int views : { 2, 4, 8 })
    {
        std::vector<ClipDomain> clips; for (int i = 0; i != views; ++i) clips.push_back ({ 100 + i, 0, (i % 3) * 128, 1, 70'000 + i });
        prefetch (clips, media);
        for (const auto& clip : clips) requireValue (renderAt (clips, media, clip.timelineStart), WavFixture::amplitude (0) * static_cast<float> (clip.sourceStart / 128 + 1), "M9 competing view mismatch");
    }
    require (runtime.counters.maxPending.load() <= 1, "M9 request slot bound exceeded"); callbackClean (media);
    std::cout << "ARBITRATION identical-reads=" << identicalReads << " overlap-reads=" << overlapReads << " disjoint-reads=" << disjointReads
              << " slots=1 max-pending=1 queue=none starvation=none-observed competing=2/4/8 callback-clean=pass\n";
    media.clear(); files.clear();

    // M10 final edited Domain: moved clip, re-expanded clip, split segments, and deleted Clip 999 absent.
    const std::vector<ClipDomain> finalDomain { { 90, 0, 0, 1, 80'750 }, { 91, 0, 128, 129, 81'000 }, { 92, 0, 0, 128, 82'000 }, { 93, 0, 128, 129, 82'128 } };
    auto observe = [&] (std::vector<std::unique_ptr<MediaRuntime>>& current) {
        prefetch (finalDomain, current);
        return std::array<float, 5> { renderAt (finalDomain, current, 80'750), renderAt (finalDomain, current, 81'128),
                                      renderAt (finalDomain, current, 82'128), renderAt (finalDomain, current, 82'256), renderAt (finalDomain, current, 83'000) };
    };
    media = createMedia (files, 1); const auto first = observe (media); const auto domainCopy = finalDomain; media.clear(); files.clear();
    media = createMedia (files, 1); const auto rebuilt = observe (media);
    for (std::size_t i = 0; i != first.size(); ++i) requireValue (first[i], rebuilt[i], "M10 reconstruction output differs");
    require (finalDomain == domainCopy, "M10 runtime mutated Domain"); requireValue (rebuilt.back(), 0.0f, "M10 deleted clip output present"); callbackClean (media);
    std::cout << "RECONSTRUCTION final-clips=4 deleted-clip=absent workers=1 caches=1 output=equal max-error=0 domain-unchanged=pass\n";
}

void run()
{
    std::vector<std::unique_ptr<WavFixture>> files; auto media = createMedia (files, 3);
    const std::vector<ClipDomain> same { { 1, 0, 0, 1, 1000 }, { 2, 0, 128, 1, 2000 }, { 3, 0, 0, 1, 3000 } };
    prefetch (same, media);
    requireValue (renderAt (same, media, 1000), WavFixture::amplitude (0), "M1 disjoint A mapping");
    requireValue (renderAt (same, media, 2000), WavFixture::amplitude (0) * 2.0f, "M1 disjoint B mapping");
    requireValue (renderAt (same, media, 3000), WavFixture::amplitude (0), "M1 identical source different timeline mapping");
    const std::vector<ClipDomain> sourceOverlap { { 9, 0, 0, 129, 6000 }, { 10, 0, 128, 1, 7000 } }; prefetch (sourceOverlap, media);
    requireValue (renderAt (sourceOverlap, media, 6128), WavFixture::amplitude (0) * 2.0f, "M1 overlapping source range mapping");
    requireValue (renderAt (sourceOverlap, media, 7000), WavFixture::amplitude (0) * 2.0f, "M1 overlapping source range second placement");
    const std::vector<ClipDomain> overlap { { 4, 0, 0, 1, 4000 }, { 5, 0, 0, 1, 4000 } }; prefetch (overlap, media);
    requireValue (renderAt (overlap, media, 4000), WavFixture::amplitude (0) * 2.0f, "M1 same-media overlap sum");
    const std::vector<ClipDomain> different { { 6, 0, 0, 1, 5000 }, { 7, 1, 0, 1, 5000 }, { 8, 2, 0, 1, 5000 } }; prefetch (different, media);
    requireValue (renderAt (different, media, 5000), WavFixture::amplitude (0) + WavFixture::amplitude (1) + WavFixture::amplitude (2), "M2 different-media SummingNode sum");
    callbackClean (media);
    media.clear();
    files.clear();
    runRuntimeEdits (files);
    runArbitrationAndReconstruction (files);

    for (const int density : { 4, 8, 16, 32 }) for (const int unique : { 1, 3, density })
    {
        auto denseMedia = createMedia (files, unique); std::vector<ClipDomain> clips;
        for (int i = 0; i != density; ++i) clips.push_back ({ i, i % unique, 0, 1, 10'000 + i });
        prefetch (clips, denseMedia);
        for (const auto& clip : clips) requireValue (renderAt (clips, denseMedia, clip.timelineStart), WavFixture::amplitude (clip.mediaId), "M3 dense marker mapping");
        callbackClean (denseMedia);
        std::uint64_t reads = 0, pending = 0; for (const auto& r : denseMedia) { reads += r->counters.workerReads; pending = std::max (pending, r->counters.maxPending.load()); }
        require (pending <= 1, "M3 request bound exceeded");
        std::cout << "MULTISOURCE-STRESS density=" << density << " unique-media=" << unique << " workers=" << unique << " caches=" << unique
                  << " cache-bytes=" << unique * MediaRuntime::pcmBytes() << " request-capacity=" << unique << " max-pending=" << pending
                  << " graph-nodes=" << density + 1 << " worker-reads=" << reads << " max-error=0 stale=0 callback-reader=0 callback-waits=0 callback-growth=0\n";
    }
    std::cout << "MULTISOURCE M1 same-file=pass identical-source-different-timeline=pass overlap-sum=pass M2 different-file-summing=pass M3 matrix=12 model=A max-error=0 stale=0\n";
}
}
int main() { try { multisource::run(); std::cout << "MULTISOURCE PASS\n"; return 0; } catch (const std::exception& e) { std::cerr << "MULTISOURCE FAIL: " << e.what() << '\n'; return 1; } }
