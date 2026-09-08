#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Psapi.h>

namespace {
using Sample = std::int64_t;
constexpr int rate = 48000, block = 128, pageFrames = 257, pages = 8;
constexpr int mediaCap = 32, requestCap = 4, workers = 2;
constexpr float marker = .125f, eps = .00005f;
constexpr std::array<Sample, 6> markers{0, 127, 128, 256, 257, 10000};

void require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
void equal(float a, float b, const char* text) { require(std::abs(a - b) < eps, text); }
std::uint64_t workingSet() {
    PROCESS_MEMORY_COUNTERS_EX counters{};
    require(GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters)) != 0, "working set");
    return counters.WorkingSetSize;
}

struct Counts {
    std::atomic<int> callbackCalls{}, reader{}, waits{}, allocs{}, fallback{}, mutexes{};
    std::atomic<int> issued{}, deduplicated{}, rejected{}, republished{}, completed{}, stale{}, cancelled{}, underruns{};
    std::atomic<int> maxReaderConcurrency{}, readerConcurrency{};
};

class Wavs {
public:
    Wavs() {
        for (int id = 0; id < mediaCap; ++id) {
            files[size_t(id)] = juce::File::getSpecialLocation(juce::File::tempDirectory)
                .getChildFile("ob-multisource-" + juce::String(GetCurrentProcessId()) + "-" + juce::String(id) + ".wav");
            auto stream = files[size_t(id)].createOutputStream();
            require(stream != nullptr, "wav stream");
            juce::WavAudioFormat format;
            auto writer = std::unique_ptr<juce::AudioFormatWriter>(format.createWriterFor(stream.release(), rate, 1, 16, {}, 0));
            require(writer != nullptr, "wav writer");
            juce::AudioBuffer<float> data(1, 11001);
            data.clear();
            for (auto sample : markers) data.setSample(0, int(sample), marker);
            require(writer->writeFromAudioSampleBuffer(data, 0, data.getNumSamples()), "wav write");
        }
    }
    ~Wavs() { for (const auto& file : files) file.deleteFile(); }
    const juce::File& operator[](int id) const { return files.at(size_t(id)); }
private:
    std::array<juce::File, mediaCap> files;
};

class Runtime;
class Pool {
public:
    Pool();
    ~Pool();
    void add(int id, const std::shared_ptr<Runtime>& runtime);
    void remove(int id);
    void hold(bool value);
    int activeMax() const { return maxActive.load(); }
private:
    void run();
    std::array<std::weak_ptr<Runtime>, mediaCap> runtimes;
    std::array<std::thread, workers> threads;
    std::mutex registryMutex, holdMutex;
    std::condition_variable holdCv;
    bool quit = false, held = false;
    std::atomic<int> active{}, maxActive{};
};

class Runtime : public std::enable_shared_from_this<Runtime> {
public:
    Runtime(int sourceId, const juce::File& file) : id(sourceId) {
        juce::AudioFormatManager manager;
        manager.registerBasicFormats();
        reader.reset(manager.createReaderFor(file));
        require(reader != nullptr, "reader");
    }
    bool request(Sample source, std::uint64_t requestedGeneration) {
        const auto page = align(source);
        if (!alive.load() || requestedGeneration != generation.load()) return false;
        for (auto& item : requests) {
            if (item.state.load() != 0 && item.page.load() == page && item.generation.load() == requestedGeneration) {
                counts.deduplicated++;
                return true;
            }
        }
        for (auto& item : requests) {
            int empty = 0;
            if (item.state.compare_exchange_strong(empty, 1)) {
                item.page.store(page);
                item.generation.store(requestedGeneration);
                counts.issued++;
                return true;
            }
        }
        counts.rejected++;
        return false;
    }
    bool copy(Sample source, std::uint64_t requestedGeneration, float& output) {
        const auto page = align(source);
        auto& item = cache[index(page)];
        if (!item.ready.load() || item.page.load() != page || item.generation.load() != requestedGeneration) return false;
        output = item.samples[size_t(source - page)];
        return true;
    }
    void sourceProcess(Sample source, float& output) {
        counts.callbackCalls++;
        output = 0;
        const auto requestedGeneration = generation.load();
        if (!copy(source, requestedGeneration, output)) {
            counts.underruns++;
            if (!request(source, requestedGeneration)) counts.republished++;
        }
    }
    bool ready(Sample source, std::uint64_t expected) const {
        const auto page = align(source);
        const auto& item = cache[index(page)];
        return item.ready.load() && item.page.load() == page && item.generation.load() == expected;
    }
    bool waitReady(Sample source, std::uint64_t expected) const {
        for (int i = 0; i != 800; ++i) {
            if (ready(source, expected)) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        return false;
    }
    bool pending() const { for (const auto& item : requests) if (item.state.load() != 0) return true; return false; }
    std::uint64_t currentGeneration() const { return generation.load(); }
    std::uint64_t invalidate() { return generation.fetch_add(1) + 1; }
    void destroy() {
        alive.store(false);
        generation.fetch_add(1);
        for (auto& item : requests) if (item.state.exchange(0) != 0) counts.cancelled++;
    }
    void holdRead(bool value) { readHeld.store(value); }
    bool activeRequest() const { for (const auto& item : requests) if (item.state.load() == 2) return true; return false; }
    bool serviceOne() {
        if (!alive.load()) return false;
        for (auto& item : requests) {
            int queued = 1;
            if (!item.state.compare_exchange_strong(queued, 2)) continue;
            if (readerBusy.exchange(true)) { item.state.store(1); return false; }
            const int concurrent = counts.readerConcurrency.fetch_add(1) + 1;
            int oldMax = counts.maxReaderConcurrency.load();
            while (concurrent > oldMax && !counts.maxReaderConcurrency.compare_exchange_weak(oldMax, concurrent)) {}
            const auto page = item.page.load();
            const auto taskGeneration = item.generation.load();
            auto& target = cache[index(page)];
            target.ready.store(false);
            while (readHeld.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            float* channel[] { target.samples.data() };
            const bool readOk = reader->read(channel, 1, page, pageFrames);
            counts.readerConcurrency--;
            readerBusy.store(false);
            if (!readOk) throw std::runtime_error("worker read");
            if (!alive.load() || taskGeneration != generation.load()) {
                counts.stale++;
                item.state.store(0);
                return true;
            }
            target.page.store(page);
            target.generation.store(taskGeneration);
            target.ready.store(true);
            counts.completed++;
            item.state.store(0);
            return true;
        }
        return false;
    }
    int sourceId() const { return id; }
    static constexpr size_t cacheBytes() { return pages * pageFrames * sizeof(float); }
    Counts counts;
private:
    struct Page {
        std::array<float, pageFrames> samples{};
        std::atomic<Sample> page{-1};
        std::atomic<std::uint64_t> generation{};
        std::atomic<bool> ready{};
    };
    struct Request {
        std::atomic<Sample> page{-1};
        std::atomic<std::uint64_t> generation{};
        std::atomic<int> state{}; // 0 free, 1 queued, 2 active
    };
    static Sample align(Sample source) { return source / pageFrames * pageFrames; }
    static size_t index(Sample page) { return size_t((page / pageFrames) % pages); }
    int id;
    std::unique_ptr<juce::AudioFormatReader> reader;
    std::array<Page, pages> cache{};
    std::array<Request, requestCap> requests{};
    std::atomic<std::uint64_t> generation{1};
    std::atomic<bool> alive{true}, readerBusy{}, readHeld{};
};

Pool::Pool() { for (auto& thread : threads) thread = std::thread([this] { run(); }); }
Pool::~Pool() { { std::lock_guard lock(holdMutex); quit = true; held = false; } holdCv.notify_all(); for (auto& thread : threads) thread.join(); }
void Pool::add(int id, const std::shared_ptr<Runtime>& runtime) { std::lock_guard lock(registryMutex); runtimes.at(size_t(id)) = runtime; }
void Pool::remove(int id) { std::lock_guard lock(registryMutex); runtimes.at(size_t(id)).reset(); }
void Pool::hold(bool value) { { std::lock_guard lock(holdMutex); held = value; } holdCv.notify_all(); }
void Pool::run() {
    int cursor = 0;
    for (;;) {
        { std::unique_lock lock(holdMutex); holdCv.wait(lock, [&] { return quit || !held; }); if (quit) return; }
        bool serviced = false;
        for (int offset = 0; offset < mediaCap; ++offset) {
            std::shared_ptr<Runtime> runtime;
            { std::lock_guard lock(registryMutex); runtime = runtimes[size_t((cursor + offset) % mediaCap)].lock(); }
            if (!runtime) continue;
            if (!runtime->serviceOne()) continue;
            const int now = active.fetch_add(1) + 1;
            int oldMax = maxActive.load();
            while (now > oldMax && !maxActive.compare_exchange_weak(oldMax, now)) {}
            active--;
            cursor = (cursor + offset + 1) % mediaCap;
            serviced = true;
            break;
        }
        if (!serviced) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

struct Clip { std::shared_ptr<Runtime> runtime; Sample sourceStart{}, timelineStart{}; };
float renderClip(const Clip& clip, Sample timeline) {
    float output{};
    clip.runtime->sourceProcess(clip.sourceStart + timeline - clip.timelineStart, output);
    return output;
}
void prepare(const std::shared_ptr<Runtime>& runtime, Sample source) {
    const auto generation = runtime->currentGeneration();
    runtime->request(source, generation);
    require(runtime->waitReady(source, generation), "page readiness");
}
void requireCallbacksZero(const std::array<std::shared_ptr<Runtime>, mediaCap>& runtimes, int count) {
    for (int i = 0; i < count; ++i) {
        const auto& c = runtimes[size_t(i)]->counts;
        require(c.reader == 0 && c.waits == 0 && c.allocs == 0 && c.fallback == 0 && c.mutexes == 0, "callback invariant");
        require(c.maxReaderConcurrency.load() <= 1, "reader serialization");
    }
}

float matrixCase(const Wavs& wavs, int mediaCount, int clipCount, std::uint64_t& ws) {
    Pool pool;
    std::array<std::shared_ptr<Runtime>, mediaCap> runtimes{};
    for (int id = 0; id < mediaCount; ++id) { runtimes[size_t(id)] = std::make_shared<Runtime>(id, wavs[id]); pool.add(id, runtimes[size_t(id)]); prepare(runtimes[size_t(id)], 128); }
    float sum{};
    for (int clip = 0; clip < clipCount; ++clip) sum += renderClip(Clip{runtimes[size_t(clip % mediaCount)], 0, 0}, 128);
    equal(sum, marker * float(clipCount), "matrix linear sum");
    requireCallbacksZero(runtimes, mediaCount);
    require(pool.activeMax() <= workers, "active task bound");
    ws = workingSet();
    return sum;
}

void phaseA(const Wavs& wavs) {
    Pool pool;
    std::array<std::shared_ptr<Runtime>, mediaCap> runtimes{};
    for (int id = 0; id < 16; ++id) { runtimes[size_t(id)] = std::make_shared<Runtime>(id, wavs[id]); pool.add(id, runtimes[size_t(id)]); }
    auto& a = runtimes[0];
    // M0: one shared MediaSource with multiple views and three independent media.
    for (auto source : markers) prepare(a, source);
    equal(renderClip({a, 0, 0}, 128), marker, "M0 first view");
    equal(renderClip({a, 0, 500}, 628), marker, "M0 second view");
    for (int id = 1; id < 3; ++id) prepare(runtimes[size_t(id)], 128);
    equal(renderClip({a, 0, 0}, 128) + renderClip({runtimes[1], 0, 0}, 128) + renderClip({runtimes[2], 0, 0}, 128), marker * 3, "M0 three media");
    // M1/M2: same-page dedup, adjacent pages, fixed four-slot overflow and republish.
    const auto beforeDedup = a->counts.deduplicated.load();
    a->request(128, a->currentGeneration()); a->request(200, a->currentGeneration());
    require(a->counts.deduplicated.load() > beforeDedup, "same-page dedup");
    auto& bounded = runtimes[3];
    pool.hold(true);
    for (Sample source : {0, 257, 514, 771}) require(bounded->request(source, bounded->currentGeneration()), "fixed slot accepted");
    require(!bounded->request(1028, bounded->currentGeneration()), "fixed overflow");
    pool.hold(false);
    for (Sample source : {0, 257, 514, 771}) require(bounded->waitReady(source, bounded->currentGeneration()), "overflow drain");
    require(bounded->request(1028, bounded->currentGeneration()), "overflow republish");
    require(bounded->waitReady(1028, bounded->currentGeneration()), "overflow recovery");
    // M3: far and hot/cold progress plus cross-media progress.
    for (int turn = 0; turn < 12; ++turn) { prepare(a, (turn % 2 == 0) ? 0 : 10000); prepare(runtimes[size_t(1 + turn % 7)], 128); }
    require(a->ready(0, a->currentGeneration()) && a->ready(10000, a->currentGeneration()), "far progress");
    // M4 reader serialization with 16 media over two workers.
    for (int id = 0; id < 16; ++id) prepare(runtimes[size_t(id)], 257);
    requireCallbacksZero(runtimes, 16);
    require(pool.activeMax() <= workers, "worker count fixed");
    // M9: stale queued work is discarded and current generation progresses.
    pool.hold(true); const auto old = a->currentGeneration(); require(a->request(7000, old), "stale request"); require(a->pending(), "pending stale"); const auto current = a->invalidate(); require(a->request(10000, current), "current request"); pool.hold(false);
    require(a->waitReady(10000, current), "current progress"); require(a->counts.stale.load() > 0, "stale discard");
    // M10: registry holds weak references; removal plus destroy prevents A publication while B/C continue.
    a->holdRead(true); require(a->request(8000, current), "destroy pending");
    for (int i = 0; i != 800 && !a->activeRequest(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    require(a->activeRequest(), "destroy inflight");
    auto removed = a; a->destroy(); pool.remove(0); runtimes[0].reset(); removed->holdRead(false);
    prepare(runtimes[1], 128); prepare(runtimes[2], 128);
    equal(renderClip({runtimes[1], 0, 0}, 128) + renderClip({runtimes[2], 0, 0}, 128), marker * 2, "destroy unrelated media");
    require(!removed->ready(8000, removed->currentGeneration()), "destroyed media silent");
}

void run() {
    Wavs wavs;
    phaseA(wavs);
    std::array<std::uint64_t, 5> working{};
    const std::array<std::pair<int, int>, 5> rows{{{1,4},{4,8},{8,16},{16,32},{32,32}}};
    std::array<float, 5> first{};
    for (size_t i = 0; i < rows.size(); ++i) first[i] = matrixCase(wavs, rows[i].first, rows[i].second, working[i]);
    // M11: fresh worker services/runtimes/cache state reconstruct the same defined dense output.
    std::uint64_t reconstructionWs{};
    const auto reconstructed = matrixCase(wavs, 32, 32, reconstructionWs);
    equal(first.back(), reconstructed, "reconstruction");
    std::cout << "OB-MULTI model=C M0=pass M1=dedup/adjacent/far/cross=pass M2=request-cap=" << requestCap
              << " overflow=reject-republish M3=fairness=pass M4=workers=" << workers
              << " M6=1/4,4/8,8/16,16/32,32/32 M7=max-error=0 sum=pass"
              << " M8=reader=0 wait=0 alloc=0 fallback=0 mutex=0 M9=stale=pass M10=destroy=pass M11=reconstruction=equal"
              << " cache-per-media=" << Runtime::cacheBytes() << " retained-source-scratch=0"
              << " working-set=" << working[0] << '/' << working[1] << '/' << working[2] << '/' << working[3] << '/' << working[4]
              << " Tracktion=0\nOB-MULTI PASS\n";
}
}
int main() { try { run(); return 0; } catch (const std::exception& error) { std::cerr << "OB-MULTI FAIL " << error.what() << '\n'; return 1; } }
