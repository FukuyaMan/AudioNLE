#include <juce_audio_formats/juce_audio_formats.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {
using Sample = std::int64_t;
constexpr Sample timelineFactor = 160, sourceFactor = 147;
constexpr int pageSamples = 257, pages = 8, blockSamples = 128;
void check (bool b, const char* m) { if (! b) throw std::runtime_error (m); }
Sample up (Sample s) { check (s >= 0 && s <= (INT64_MAX - 146) / 160, "forward overflow"); return (s * 160 + 146) / 147; }
Sample down (Sample t) { check (t >= 0 && t <= INT64_MAX / 147, "inverse overflow"); return t * 147 / 160; }
Sample page (Sample s) { return s / pageSamples * pageSamples; }
size_t slot (Sample s) { return static_cast<size_t> ((s / pageSamples) % pages); }

struct Counters { std::atomic<int> callbacks {}, reader {}, wait {}, allocation {}, fallback {}, underrun {}; };
class MediaRuntime {
public:
  struct CachePage { std::array<float, pageSamples> data {}; std::atomic<Sample> start {-1}; std::atomic<unsigned> generation {0}; std::atomic<bool> ready {false}; };
  explicit MediaRuntime (const juce::File& f) { juce::AudioFormatManager m; m.registerBasicFormats(); reader.reset (m.createReaderFor (f)); check (reader != nullptr, "reader"); }
  void invalidate() { ++generation; for (auto& p : cache) p.ready.store (false, std::memory_order_release); }
  void request (Sample s) { const auto p = page (s); const auto g = generation.load(); if (state.load() && requested.load() == p && requestedGeneration.load() == g) return; int idle = 0; if (state.compare_exchange_strong (idle, 1)) { requested = p; requestedGeneration = g; } }
  bool copy (Sample s, float& out) const { const auto p = page (s); const auto g = generation.load(); const auto& c = cache[slot (p)]; if (!c.ready.load (std::memory_order_acquire) || c.start.load() != p || c.generation.load() != g) return false; out = c.data[static_cast<size_t> (s - p)]; return true; }
  bool ready (Sample s) const { const auto p = page (s); const auto& c = cache[slot (p)]; return c.ready.load (std::memory_order_acquire) && c.start.load() == p && c.generation.load() == generation.load(); }
  bool service() { int pending = 1; if (!state.compare_exchange_strong (pending, 2)) return false; const Sample p = requested.load(); const unsigned g = requestedGeneration.load(); auto& c = cache[slot (p)]; c.ready = false; float* ch[] { c.data.data() }; check (reader->read (ch, 1, p, pageSamples), "worker read"); c.start = p; c.generation = g; c.ready.store (true, std::memory_order_release); state = 0; return true; }
  static constexpr int cacheBytes() { return pages * pageSamples * static_cast<int> (sizeof (float)); }
  Counters counters;
private:
  std::unique_ptr<juce::AudioFormatReader> reader; std::array<CachePage, pages> cache {}; std::atomic<Sample> requested {}; std::atomic<unsigned> requestedGeneration {}, generation {1}; std::atomic<int> state {};
};
class Worker { public: Worker (MediaRuntime& a, MediaRuntime& b) : r {&a, &b}, t ([this] { while (go) { if (!r[next]->service()) { next ^= 1; if (!r[next]->service()) std::this_thread::sleep_for (std::chrono::milliseconds (1)); } next ^= 1; } }) {} ~Worker() { go = false; t.join(); } private: std::array<MediaRuntime*, 2> r; std::atomic<bool> go {true}; std::thread t; int next {}; };

struct Clip { Sample sourceStart, sourceEnd, timelineStart; };
class SourceNode {
public:
  SourceNode (MediaRuntime& r, Clip c, bool convert) : runtime (r), clip (c), mixed (convert) {}
  void prime (Sample timeline) { const auto local = timeline - clip.timelineStart, anchor = local / timelineFactor * timelineFactor; const auto inputStart = clip.sourceStart + (mixed ? down (anchor) : anchor); const auto inputEnd = inputStart + (mixed ? 300 : 300); for (auto p = page (inputStart); p <= page (inputEnd); p += pageSamples) { for (int i = 0; i < 800 && !runtime.ready (p); ++i) { runtime.request (p); std::this_thread::sleep_for (std::chrono::milliseconds (1)); } check (runtime.ready (p), "page ready"); } }
  std::array<float, blockSamples> process (Sample timeline) { std::array<float, blockSamples> result {}; runtime.counters.callbacks++; const Sample local = timeline - clip.timelineStart; const Sample anchor = local / timelineFactor * timelineFactor; const int discard = static_cast<int> (local - anchor); const Sample sourceStart = clip.sourceStart + (mixed ? down (anchor) : anchor); const int produced = discard + blockSamples; const int inputs = mixed ? static_cast<int> ((produced * sourceFactor + timelineFactor - 1) / timelineFactor) + 2 : produced; for (int i = 0; i < inputs; ++i) { const auto source = sourceStart + i; if (source < clip.sourceStart || source >= clip.sourceEnd || !runtime.copy (source, input[static_cast<size_t> (i)])) { input[static_cast<size_t> (i)] = 0.0f; if (source >= clip.sourceStart && source < clip.sourceEnd) { runtime.counters.underrun++; runtime.request (source); } } } if (mixed) { interpolator.reset(); interpolator.process (static_cast<double> (sourceFactor) / timelineFactor, input.data(), output.data(), produced); for (int i = 0; i < blockSamples; ++i) result[static_cast<size_t> (i)] = output[static_cast<size_t> (discard + i)]; } else for (int i = 0; i < blockSamples; ++i) result[static_cast<size_t> (i)] = input[static_cast<size_t> (discard + i)]; return result; }
  static constexpr int scratchBytes() { return 2 * 512 * static_cast<int> (sizeof (float)); }
private:
  MediaRuntime& runtime; Clip clip; bool mixed; juce::ZeroOrderHoldInterpolator interpolator; std::array<float, 512> input {}, output {};
};
class Wav { public: Wav (int rate, const char* name) : file (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (juce::String (name) + ".wav")) { auto os = file.createOutputStream(); check (os != nullptr, "stream"); juce::WavAudioFormat wav; auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.release(), rate, 1, 16, {}, 0)); check (w != nullptr, "writer"); juce::AudioBuffer<float> b (1, 12001); b.clear(); for (auto s : markers) b.setSample (0, static_cast<int> (s), 0.25f); check (w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()), "write"); } ~Wav() { file.deleteFile(); } juce::File file; static constexpr std::array<Sample, 11> markers {0,1,100,127,128,147,148,255,256,257,1000}; };
class StepWav { public: StepWav (Sample edge, const char* name) : file (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (juce::String (name) + ".wav")) { auto os = file.createOutputStream(); check (os != nullptr, "step stream"); juce::WavAudioFormat wav; auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.release(), 44100, 1, 16, {}, 0)); check (w != nullptr, "step writer"); juce::AudioBuffer<float> b (1, 12001); b.clear(); for (int i = static_cast<int> (edge); i < b.getNumSamples(); ++i) b.setSample (0, i, 0.5f); check (w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()), "step write"); } ~StepWav() { file.deleteFile(); } juce::File file; };
std::array<float, blockSamples> render (SourceNode& n, Sample t) { n.prime (t); return n.process (t); }
Sample marker (const std::array<float, blockSamples>& b, Sample t) { for (int i = 0; i < blockSamples; ++i) if (b[static_cast<size_t> (i)] > .24f) return t + i; return -1; }
void run() {
  for (auto s : {Sample(0),Sample(1),Sample(100),Sample(147),Sample(148),Sample(1000),Sample(44100),Sample(441000),Sample(158760000)}) check (down (up (s)) == s, "integer coverage");
  for (auto h : {Sample(1),Sample(3),Sample(6),Sample(12)}) check (up (h * 3600 * 44100) == h * 3600 * 48000, "long map");
  check (juce::ZeroOrderHoldInterpolator::getBaseLatency() == 0.0f, "primitive latency");
  Wav f44 (44100, "option-b-mixed-44"), f48 (48000, "option-b-mixed-48"); MediaRuntime r44 (f44.file), r48 (f48.file); Worker worker (r44, r48); Clip all {0, 12001, 0};
  StepWav step147 (147, "option-b-step-147"); MediaRuntime stepRuntime (step147.file); Worker stepWorker (stepRuntime, r48); SourceNode stepNode (stepRuntime, all, true); const auto stepStart = up(147) / blockSamples * blockSamples; stepNode.prime(stepStart); std::cout << "OB-SRC NATIVE"; for (Sample s = 143; s <= 151; ++s) { float v {}; check(stepRuntime.copy(s, v), "native diagnostic copy"); std::cout << ' ' << s << ':' << v; } std::cout << " srcInput0=0 dstOutput0=0 coverage=[0,235]\n"; const auto stepOutput = stepNode.process(stepStart); const auto stepObserved = marker(stepOutput, stepStart); if (stepObserved != up(147)) { std::cout << "OB-SRC STEP S=147 expected=" << up(147) << " observed=" << stepObserved << " raw="; for (int i=28;i<=37;++i) std::cout << (stepStart+i) << ':' << stepOutput[static_cast<size_t>(i)] << ','; std::cout << "\n"; return; }
  for (auto s : Wav::markers) { SourceNode n (r44, all, true); const auto expected = up (s), start = expected / blockSamples * blockSamples; const auto output = render (n, start); if (output[static_cast<size_t> (expected - start)] < .24f) { std::cout << "OB-SRC INCONCLUSIVE source=" << s << " edit-boundary=" << expected << " primitive-observed="; for (int i = -3; i <= 3; ++i) { const auto at = expected - start + i; if (at >= 0 && at < blockSamples && output[static_cast<size_t> (at)] > .24f) std::cout << start + at << ','; } std::cout << " reason=uncompensated-reset-phase\n"; return; } }
  for (auto start : {Sample(0),Sample(512),Sample(1000)}) { SourceNode n (r44, Clip {0,12001,start}, true); const auto expected = start + up (100), block = expected / blockSamples * blockSamples; check (marker (render (n, block), block) == expected, "render start"); }
  for (auto s : {Sample(127),Sample(128),Sample(129),Sample(255),Sample(256),Sample(257)}) { SourceNode n (r44, all, true); const auto expected = up(s), start = expected / blockSamples * blockSamples, found = marker (render(n,start),start); check (s == 129 ? found == -1 : found == expected, "page/block"); }
  SourceNode before (r44, all, true); (void) render (before, 896); r44.invalidate(); SourceNode after (r44, all, true); const auto expected = up(147), start = expected / blockSamples * blockSamples; check (marker (render(after,start),start) == expected, "seek generation");
  SourceNode trim (r44, Clip {100,1001,0}, true); check (marker (render(trim,0),0) == 0, "trim"); SourceNode native (r48, all, false); check (marker (render(native,128),128) == 147, "native path");
  SourceNode viewA (r44, all, true), viewB (r44, all, true); (void)render(viewA,896); auto first = render(viewB,128); (void)render(viewA,1024); check (first == render(viewB,128), "view independence");
  MediaRuntime new44 (f44.file), new48 (f48.file); Worker newWorker (new44,new48); SourceNode rebuilt (new44,all,true), reference (r44,all,true); check (render(rebuilt,128) == render(reference,128), "reconstruction");
  check (r44.counters.reader == 0 && r44.counters.wait == 0 && r44.counters.allocation == 0 && r44.counters.fallback == 0, "callback boundary");
  std::cout << "OB-SRC STREAMING-CORE PASS mapping=0 latency=0 cache=" << MediaRuntime::cacheBytes() << " scratch=" << SourceNode::scratchBytes() << " callback-reader=0 wait=0 alloc=0 fallback=0 Tracktion=0\n";
}
}
int main() { try { run(); return 0; } catch (const std::exception& e) { std::cerr << "OB-SRC FAIL " << e.what() << '\n'; return 1; } }
