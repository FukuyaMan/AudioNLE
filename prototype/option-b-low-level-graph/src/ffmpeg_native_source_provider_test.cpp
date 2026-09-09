#include "ffmpeg_native_source_provider.hpp"
#include "shared_source_runtime.hpp"
#include "source_runtime_engine.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace audionle::source_runtime;
namespace {
void require(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
void put16(std::ofstream& output, std::uint16_t value) { output.put(char(value)); output.put(char(value >> 8)); }
void put32(std::ofstream& output, std::uint32_t value) { put16(output, std::uint16_t(value)); put16(output, std::uint16_t(value >> 16)); }
void writeWave(std::filesystem::path const& path, unsigned rate, unsigned channels) {
  constexpr unsigned frames = 8192; const auto bytes = frames * channels * 2;
  std::ofstream output(path, std::ios::binary); require(bool(output), "fixture open");
  output.write("RIFF", 4); put32(output, 36 + bytes); output.write("WAVEfmt ", 8); put32(output, 16);
  put16(output, 1); put16(output, std::uint16_t(channels)); put32(output, rate); put32(output, rate * channels * 2);
  put16(output, std::uint16_t(channels * 2)); put16(output, 16); output.write("data", 4); put32(output, bytes);
  for (unsigned frame = 0; frame < frames; ++frame) for (unsigned channel = 0; channel < channels; ++channel) {
    const auto sample = std::int16_t((int(frame * 37 + channel * 1009) % 30000) - 15000); put16(output, std::uint16_t(sample));
  }
}
std::string quote(std::filesystem::path const& path) { return "\"" + path.string() + "\""; }
void encode(std::filesystem::path const& ffmpeg, std::filesystem::path const& stereo, std::filesystem::path const& mono,
            std::filesystem::path const& output) {
  const auto command = ffmpeg.string() + " -y -v error -i " + quote(stereo) + " -i " + quote(mono) +
      " -map 0:a -map 1:a -c:a aac " + quote(output);
  require(std::system(command.c_str()) == 0, "container fixture encode");
}
AudioStreamDescriptor selected(std::filesystem::path const& path, unsigned channels, unsigned rate) {
  const auto streams = enumerateAudioStreams(path); for (const auto& stream : streams)
    if (stream.channels == channels && stream.sampleRate == rate) return stream;
  throw std::runtime_error("selected stream absent");
}
void decodeChecks(std::filesystem::path const& path, bool multiple) {
  const auto streams = enumerateAudioStreams(path); require(!streams.empty(), "audio enumeration");
  if (multiple) require(streams.size() == 2, "explicit multi-stream enumeration");
  const auto stereo = selected(path, 2, 48000); const auto mono = multiple ? selected(path, 1, 44100) : AudioStreamDescriptor{};
  std::array<float, 128 * 2> first{}, repeat{}, seek{};
  FfmpegNativeSourceProvider stereoProvider(path, stereo);
  require(stereoProvider.read(0, 128, first.data()) == NativeReadResult::Ready, "stereo decode");
  require(stereoProvider.read(0, 128, repeat.data()) == NativeReadResult::Ready && first == repeat, "stereo reconstruction");
  require(stereoProvider.read(1024, 128, seek.data()) == NativeReadResult::Ready && seek != first, "integer native seek coordinate");
  require(std::abs(first[0] - first[1]) > 0.001f, "stereo channels preserved");
  if (multiple) { std::array<float, 128> output{}; FfmpegNativeSourceProvider monoProvider(path, mono);
    require(monoProvider.read(0, 128, output.data()) == NativeReadResult::Ready, "explicit mono selection");
    require(std::abs(output[0]) > 0.001f, "mono data"); }
}
void runtimeAndGraph(std::filesystem::path const& path) {
  const auto stream = selected(path, 2, 48000); auto provider = std::make_unique<FfmpegNativeSourceProvider>(path, stream);
  auto shared = std::make_shared<SharedNativeSourceRuntime>(91, 7, 8192, 2, std::move(provider));
  auto identity = makeRuntimeIdentity(91, 48000, Admission::NativeRate); identity.generation = 7; identity.channels = 2; identity.channelLayout = stream.layout;
  SourceRuntime runtime(identity, {}, shared); SourceNode node(runtime); ClipRuntimeView view{0, 2048, 6144, identity};
  std::array<float, 128 * 2> output{}; AudioBuffer buffer{output.data(), 128, 2, true};
  require(node.process(view, 0, buffer) == SourceRenderResult::Unavailable, "callback only queues native page");
  SharedSourceWorker worker; worker.add(runtime); worker.drain();
  require(node.process(view, 0, buffer) == SourceRenderResult::Ready && std::abs(output[0] - output[1]) > 0.001f,
          "selected stream reaches SourceRuntime and graph source node");
  view.identity.generation = 8; require(node.process(view, 0, buffer) == SourceRenderResult::Stale, "generation invalidation");
}
void run() {
  const auto root = std::filesystem::temp_directory_path() / "AudioNLE-ffmpeg-native-source";
  std::filesystem::create_directories(root); const auto stereo = root / "stereo-48k.wav"; const auto mono = root / "mono-44k.wav";
  writeWave(stereo, 48000, 2); writeWave(mono, 44100, 1); const auto ffmpeg = std::filesystem::path(std::getenv("ComSpec") ? "ffmpeg.exe" : "ffmpeg.exe");
  const std::array<std::string, 3> names{"fixture.mp4", "fixture.mkv", "fixture.mov"};
  for (const auto& name : names) { const auto fixture = root / name; encode(ffmpeg, stereo, mono, fixture); decodeChecks(fixture, true); }
  decodeChecks(stereo, false); runtimeAndGraph(root / names.front());
  std::error_code error; std::filesystem::remove_all(root, error);
  std::cout << "FFmpeg native source MP4/MKV/MOV/WAV mono/stereo/multi-stream 44.1/48 kHz PASS\n";
}
}
int main() { try { run(); return 0; } catch (const std::exception& error) { std::cerr << "ffmpeg native source FAIL " << error.what() << '\n'; return 1; } }
