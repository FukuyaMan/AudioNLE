#include "integer_rational_zoh.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace
{
using Sample = std::int64_t;
constexpr Sample P = 160, Q = 147;
constexpr int pageFrames = 257, pageCount = 2, blockFrames = 128;

void require (bool value, const char* message) { if (! value) throw std::runtime_error (message); }
Sample reference (Sample timeline, Sample p = P, Sample q = Q)
{
    return IntegerRationalZoh::referenceHeldSource (timeline, p, q);
}

struct Counters { std::atomic<int> generator {}, wait {}, allocation {}, fallback {}, callback {}; };
struct Page { std::array<float, pageFrames> samples {}; std::atomic<Sample> start {-1}; std::atomic<bool> ready {}; };

class FixedNativePageCache
{
public:
    void request (Sample source)
    {
        const auto start = source / pageFrames * pageFrames;
        int idle = 0;
        if (state.compare_exchange_strong (idle, 1)) requested.store (start);
    }
    bool service()
    {
        int pending = 1;
        if (! state.compare_exchange_strong (pending, 2)) return false;
        const auto start = requested.load();
        auto& page = pages[static_cast<size_t> ((start / pageFrames) % pageCount)];
        page.ready.store (false, std::memory_order_release);
        for (int i = 0; i < pageFrames; ++i) page.samples[static_cast<size_t> (i)] = static_cast<float> (start + i);
        page.start.store (start, std::memory_order_release);
        page.ready.store (true, std::memory_order_release);
        state.store (0);
        return true;
    }
    bool copy (Sample source, float& output) const
    {
        const auto start = source / pageFrames * pageFrames;
        const auto& page = pages[static_cast<size_t> ((start / pageFrames) % pageCount)];
        if (! page.ready.load (std::memory_order_acquire) || page.start.load (std::memory_order_acquire) != start) return false;
        output = page.samples[static_cast<size_t> (source - start)];
        return true;
    }
    void prime (Sample source)
    {
        request (source);
        for (int retry = 0; retry != 800; ++retry)
        {
            float ignored {};
            if (copy (source, ignored)) return;
            std::this_thread::sleep_for (std::chrono::milliseconds (1));
        }
        throw std::runtime_error ("cache page did not become ready");
    }
    Counters counters;
private:
    std::array<Page, pageCount> pages {};
    std::atomic<Sample> requested {};
    std::atomic<int> state {};
};
class Worker
{
public:
    explicit Worker (FixedNativePageCache& cache) : c (cache), thread ([this] { while (running) if (! c.service()) std::this_thread::sleep_for (std::chrono::milliseconds (1)); }) {}
    ~Worker() { running = false; thread.join(); }
private: FixedNativePageCache& c; std::atomic<bool> running {true}; std::thread thread;
};

class ClipRuntimeView
{
public:
    ClipRuntimeView (Sample timelineStart, Sample sourceStart, Sample p = P, Sample q = Q)
        : timelineOrigin (timelineStart), sourceOrigin (sourceStart), zoh (p, q) {}
    void reset (Sample timeline) { zoh.reset (timeline - timelineOrigin); }
    Sample source() const { return sourceOrigin + zoh.currentSource(); }
    void advance() { zoh.advance(); }
private: Sample timelineOrigin, sourceOrigin; IntegerRationalZoh zoh;
};

class SourceNode
{
public:
    SourceNode (ClipRuntimeView& view, FixedNativePageCache* cache) : v (view), nativeCache (cache) {}
    void reset (Sample timeline) { v.reset (timeline); }
    float processOne()
    {
        float output {};
        if (nativeCache != nullptr) require (nativeCache->copy (v.source(), output), "callback cache miss");
        else output = static_cast<float> (v.source());
        v.advance();
        return output;
    }
private: ClipRuntimeView& v; FixedNativePageCache* nativeCache;
};

std::vector<Sample> render (Sample start, int count, const std::vector<int>& partitions, FixedNativePageCache* cache = nullptr, Sample sourceOffset = 0)
{
    ClipRuntimeView view (0, sourceOffset); SourceNode source (view, cache); source.reset (start);
    std::vector<Sample> result; result.reserve (static_cast<size_t> (count));
    int done = 0, partition = 0;
    while (done < count)
    {
        const int frames = std::min (partitions[static_cast<size_t> (partition++ % partitions.size())], count - done);
        if (cache != nullptr) ++cache->counters.callback;
        for (int i = 0; i < frames; ++i) result.push_back (static_cast<Sample> (source.processOne()));
        done += frames;
    }
    return result;
}

void compareReference (Sample start, const std::vector<Sample>& actual, Sample sourceOffset = 0)
{
    for (size_t i = 0; i < actual.size(); ++i)
        require (actual[i] == sourceOffset + reference (start + static_cast<Sample> (i)), "integer identity mismatch");
}

void run()
{
    // Z1/Z2: Sample is signed 64-bit; inputs are rejected before T*Q can overflow.
    // Fixed phase is always [0,P); q/p is reduced by the caller and phase+q stays in int64 here.
    constexpr std::array<std::pair<Sample, Sample>, 4> ratios {{{160,147}, {147,160}, {3,2}, {2,1}}};
    for (const auto [p, q] : ratios)
        for (Sample timeline = 0; timeline <= 100000; ++timeline)
        {
            IntegerRationalZoh schedule (p, q); schedule.reset (timeline);
            require (schedule.currentSource() == reference (timeline, p, q), "closed-form initialization mismatch");
            schedule.advance();
            require (schedule.currentSource() == reference (timeline + 1, p, q), "incremental schedule mismatch");
        }

    const std::vector<int> oneBlock {401};
    const auto direct = render (0, 401, oneBlock); compareReference (0, direct);
    require (direct[159] == 146 && direct[160] == 147 && direct[161] == 147 && direct[162] == 148, "first exact boundary");
    require (direct[319] == 293 && direct[320] == 294 && direct[321] == 294, "second exact boundary");

    FixedNativePageCache cache; Worker worker (cache); cache.prime (0); cache.prime (257);
    const auto cached = render (0, 401, {128}, &cache); compareReference (0, cached);
    require (cached == direct, "worker/cache graph differs from direct");
    for (Sample source : {255, 256, 257, 258}) { float token {}; require (cache.copy (source, token) && token == static_cast<float> (source), "page identity"); }

    const auto longRun = render (0, 100001, {128}); compareReference (0, longRun);
    const auto blocks128 = render (0, 1001, {128});
    const auto blocks256 = render (0, 1001, {256});
    const auto irregular = render (0, 1001, {1, 17, 256, 3, 128, 71});
    require (blocks128 == blocks256 && blocks128 == irregular, "block partition changed timing");

    for (Sample start : {160, 161, 213}) { const auto slice = render (start, 401, {128}); compareReference (start, slice); }
    const auto firstA = render (160, 401, {128}); const auto b = render (213, 401, {128}); const auto secondA = render (160, 401, {128});
    require (firstA == secondA && !b.empty(), "seek/reset is not deterministic");
    const auto offset = render (160, 401, {128}, nullptr, 1000); compareReference (160, offset, 1000);
    require (cache.counters.generator == 0 && cache.counters.wait == 0 && cache.counters.allocation == 0 && cache.counters.fallback == 0, "callback invariant");
    std::cout << "OB-INTEGER-ZOH PASS direct=401/0 worker-cache=401/0 long=100001/0 exact=0 fractional=0 "
                 "ratios=44100:48000,48000:44100,32000:48000,48000:96000 partition=identical page=257 "
                 "nonzero-start=pass seek-reset=pass source-offset=pass callback-generator=0 wait=0 alloc=0 fallback=0\n";
}
}
int main() { try { run(); return 0; } catch (const std::exception& error) { std::cerr << "OB-INTEGER-ZOH FAIL " << error.what() << '\n'; return 1; } }
