#include "ffmpeg_native_source_provider.hpp"
#include "shared_source_runtime.hpp"
#include "source_runtime_engine.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
}

#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace audionle::source_runtime;
namespace {
constexpr unsigned kShortFrames = 8192, kLongFrames = 48 * 30000;
void require(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
void put16(std::ofstream& output, std::uint16_t value) { output.put(char(value)); output.put(char(value >> 8)); }
void put32(std::ofstream& output, std::uint32_t value) { put16(output, std::uint16_t(value)); put16(output, std::uint16_t(value >> 16)); }
void writeWave(std::filesystem::path const& path, unsigned rate, unsigned channels, unsigned frames) {
  const auto bytes = frames * channels * 2; std::ofstream output(path, std::ios::binary); require(bool(output), "fixture open");
  output.write("RIFF", 4); put32(output, 36 + bytes); output.write("WAVEfmt ", 8); put32(output, 16); put16(output, 1); put16(output, std::uint16_t(channels)); put32(output, rate); put32(output, rate * channels * 2); put16(output, std::uint16_t(channels * 2)); put16(output, 16); output.write("data", 4); put32(output, bytes);
  for (unsigned frame = 0; frame < frames; ++frame) for (unsigned channel = 0; channel < channels; ++channel) { const auto sample = std::int16_t((int(frame * 37 + channel * 1009) % 30000) - 15000); put16(output, std::uint16_t(sample)); }
}
void check(int result, char const* message) { require(result >= 0, message); }
struct FixtureStream { unsigned rate{}, channels{}, frames{}, offset{}; };
struct Encoder {
  AVCodecContext* context{}; AVStream* stream{}; AVFrame* frame{}; AVPacket* packet{};
  FixtureStream fixture{}; unsigned produced{};
  ~Encoder() { av_packet_free(&packet); av_frame_free(&frame); avcodec_free_context(&context); }
};
float fixtureSample(unsigned frame, unsigned channel) {
  return float((int(frame * 37 + channel * 1009) % 30000) - 15000) / 32768.0f;
}
std::unique_ptr<Encoder> makeEncoder(AVFormatContext* format, FixtureStream fixture) {
  const auto* codec = avcodec_find_encoder(AV_CODEC_ID_AAC); require(codec, "AAC encoder available from pinned FFmpeg");
  auto writer = std::make_unique<Encoder>(); writer->fixture = fixture;
  writer->context = avcodec_alloc_context3(codec); require(writer->context, "AAC encoder allocation");
  writer->context->sample_fmt = AV_SAMPLE_FMT_FLTP; writer->context->sample_rate = int(fixture.rate);
  writer->context->time_base = AVRational{1, int(fixture.rate)}; writer->context->bit_rate = 128000;
  av_channel_layout_default(&writer->context->ch_layout, int(fixture.channels));
  if (format->oformat->flags & AVFMT_GLOBALHEADER) writer->context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  check(avcodec_open2(writer->context, codec, nullptr), "open AAC fixture encoder");
  writer->stream = avformat_new_stream(format, nullptr); require(writer->stream, "fixture stream allocation");
  writer->stream->time_base = writer->context->time_base;
  check(avcodec_parameters_from_context(writer->stream->codecpar, writer->context), "fixture stream parameters");
  writer->frame = av_frame_alloc(); writer->packet = av_packet_alloc(); require(writer->frame && writer->packet, "fixture frame allocation");
  writer->frame->format = writer->context->sample_fmt; writer->frame->sample_rate = writer->context->sample_rate;
  check(av_channel_layout_copy(&writer->frame->ch_layout, &writer->context->ch_layout), "fixture channel layout");
  writer->frame->nb_samples = writer->context->frame_size;
  check(av_frame_get_buffer(writer->frame, 0), "fixture frame buffer");
  return writer;
}
void writePackets(AVFormatContext* format, Encoder& writer) {
  while (true) {
    const auto result = avcodec_receive_packet(writer.context, writer.packet);
    if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) return;
    check(result, "receive AAC fixture packet");
    av_packet_rescale_ts(writer.packet, writer.context->time_base, writer.stream->time_base);
    writer.packet->stream_index = writer.stream->index;
    check(av_interleaved_write_frame(format, writer.packet), "mux AAC fixture packet");
    av_packet_unref(writer.packet);
  }
}
void encodeFrame(AVFormatContext* format, Encoder& writer) {
  check(av_frame_make_writable(writer.frame), "writable fixture frame");
  for (unsigned channel = 0; channel < writer.fixture.channels; ++channel) {
    auto* samples = reinterpret_cast<float*>(writer.frame->extended_data[channel]);
    for (int index = 0; index < writer.frame->nb_samples; ++index) {
      const auto source = writer.produced + unsigned(index);
      samples[index] = source < writer.fixture.frames ? fixtureSample(source, channel) : 0.0f;
    }
  }
  writer.frame->pts = std::int64_t(writer.produced) + writer.fixture.offset;
  writer.produced += unsigned(writer.frame->nb_samples);
  check(avcodec_send_frame(writer.context, writer.frame), "send AAC fixture frame"); writePackets(format, writer);
}
void makeContainer(std::filesystem::path const& path, std::initializer_list<FixtureStream> fixtures) {
  AVFormatContext* raw{}; check(avformat_alloc_output_context2(&raw, nullptr, nullptr, path.string().c_str()), "allocate fixture muxer");
  std::unique_ptr<AVFormatContext, decltype(&avformat_free_context)> format(raw, avformat_free_context);
  std::vector<std::unique_ptr<Encoder>> writers; for (const auto fixture : fixtures) writers.push_back(makeEncoder(raw, fixture));
  if (!(raw->oformat->flags & AVFMT_NOFILE)) check(avio_open(&raw->pb, path.string().c_str(), AVIO_FLAG_WRITE), "open fixture output");
  check(avformat_write_header(raw, nullptr), "write fixture header");
  bool pending = true; while (pending) { pending = false; for (auto& writer : writers) if (writer->produced < writer->fixture.frames) { encodeFrame(raw, *writer); pending = true; } }
  for (auto& writer : writers) { check(avcodec_send_frame(writer->context, nullptr), "flush AAC fixture encoder"); writePackets(raw, *writer); }
  check(av_write_trailer(raw), "write fixture trailer");
  if (!(raw->oformat->flags & AVFMT_NOFILE)) check(avio_closep(&raw->pb), "close fixture output");
}
AudioStreamDescriptor selected(std::filesystem::path const& path, unsigned channels, unsigned rate) { const auto streams = enumerateAudioStreams(path); for (const auto& stream : streams) if (stream.channels == channels && stream.sampleRate == rate) return stream; throw std::runtime_error("selected stream absent"); }
void decodeChecks(std::filesystem::path const& path, bool multiple) {
  const auto streams = enumerateAudioStreams(path); require(!streams.empty(), "audio enumeration"); if (multiple) require(streams.size() == 2, "explicit multi-stream enumeration");
  const auto stereo = selected(path, 2, 48000); std::array<float, 256> first{}, repeat{}, forward{}, backward{}, eof{}; FfmpegNativeSourceProvider provider(path, stereo);
  require(provider.read(0, 128, first.data()) == NativeReadResult::Ready, "stereo decode"); require(provider.read(1024, 128, forward.data()) == NativeReadResult::Ready, "forward seek"); require(provider.read(128, 128, backward.data()) == NativeReadResult::Ready, "backward seek"); require(provider.read(1024, 128, repeat.data()) == NativeReadResult::Ready && repeat == forward, "repeat/same-position reconstruction"); require(provider.read(kShortFrames - 256, 128, eof.data()) == NativeReadResult::Ready, "EOF-near seek"); require(first != forward && std::abs(first[0] - first[1]) > 0.001f, "stereo channels preserved"); require(provider.diagnostics().seekRequests >= 4, "FFmpeg seek used");
  if (multiple) { const auto mono = selected(path, 1, 44100); std::array<float, 128> output{}; FfmpegNativeSourceProvider monoProvider(path, mono); require(monoProvider.read(0, 128, output.data()) == NativeReadResult::Ready && std::abs(output[0]) > 0.001f, "explicit mono selection"); }
}
class ReplacingProvider final : public NativeSourceProvider { public: NativeSourceProvider& old; NativeSourceService* service{}; NativeSourceProvider* replacement{}; ReplacingProvider(NativeSourceProvider& value) : old(value) {} NativeReadResult read(std::int64_t start, unsigned frames, float* output) noexcept override { const auto result=old.read(start,frames,output); service->reconfigure(*replacement,77,6,1); return result; } };
void selectionInvalidation(std::filesystem::path const& path) {
  FfmpegNativeSourceProvider oldStream(path, selected(path,2,48000)), newStream(path, selected(path,1,44100)); NativeSourceService service(oldStream,77,1,2); std::array<float,NativeSourceService::pageFrames*2> output{};
  require(!service.copy(0,1,1,output.data()),"old queued"); service.reconfigure(newStream,77,2,1); require(service.serviceOne(),"old queue serviced stale"); require(oldStream.diagnostics().decodedFrames==0&&service.diagnostics().staleRejected>=1,"queued old stream rejected");
  require(!service.copy(0,1,2,output.data())&&service.serviceOne()&&service.copy(0,1,2,output.data())&&std::abs(output[0])>.001f,"new selected stream current");
  service.reconfigure(oldStream,77,4,2); require(!service.copy(0,1,4,output.data())&&service.serviceOne(),"old page not reused"); require(service.copy(0,1,4,output.data())&&std::abs(output[1]-output[0])>.001f,"published old page identity rejected");
  ReplacingProvider inFlight(oldStream); inFlight.service=&service; inFlight.replacement=&newStream; service.reconfigure(inFlight,77,5,2); require(!service.copy(2048,1,5,output.data())&&service.serviceOne(),"in-flight old result serviced"); require(!service.copy(2048,1,5,output.data()),"in-flight result not published as current"); require(service.diagnostics().staleRejected>=2,"provider result stale rejected");
}
void longContainer(std::filesystem::path const& path) {
  const auto stream = selected(path, 2, 48000); FfmpegNativeSourceProvider provider(path, stream); std::array<float, 256> begin{}, middle{}, end{}, backward{}, repeat{};
  require(provider.read(128, 128, begin.data()) == NativeReadResult::Ready, "large beginning seek"); require(provider.read(kLongFrames / 2, 128, middle.data()) == NativeReadResult::Ready, "large middle seek"); require(provider.read(kLongFrames - 512, 128, end.data()) == NativeReadResult::Ready, "large end seek"); require(provider.read(kLongFrames / 4, 128, backward.data()) == NativeReadResult::Ready, "large backward seek"); require(provider.read(kLongFrames / 2, 128, repeat.data()) == NativeReadResult::Ready && repeat == middle, "large repeat reconstruction"); const auto d=provider.diagnostics(); require(d.seekRequests >= 5 && d.maxScratchFrames <= 8192 && d.decodedFrames < 100000, "duration-independent provider working set");
}
void runtimeAndGraph(std::filesystem::path const& path) {
  const auto stream = selected(path, 2, 48000); auto provider = std::make_unique<FfmpegNativeSourceProvider>(path, stream); auto shared = std::make_shared<SharedNativeSourceRuntime>(91, 7, kShortFrames, 2, std::move(provider)); auto identity = makeRuntimeIdentity(91, 48000, Admission::NativeRate); identity.generation=7; identity.channels=2; identity.channelLayout=stream.layout; SourceRuntime runtime(identity, {}, shared); SourceNode node(runtime); ClipRuntimeView view{0,2048,kShortFrames-2048,identity}; std::array<float,256> output{}; AudioBuffer buffer{output.data(),128,2,true}; require(node.process(view,0,buffer)==SourceRenderResult::Unavailable,"callback queues only"); SharedSourceWorker worker; worker.add(runtime); worker.drain(); require(node.process(view,0,buffer)==SourceRenderResult::Ready&&std::abs(output[0]-output[1])>.001f,"provider SourceRuntime SourceNode"); view.identity.generation=8; require(node.process(view,0,buffer)==SourceRenderResult::Stale,"generation invalidation"); }
void run() {
  const auto root=std::filesystem::temp_directory_path()/"AudioNLE-ffmpeg-native-source"; std::filesystem::create_directories(root); const auto stereo=root/"stereo-48k.wav", mono=root/"mono-44k.wav"; writeWave(stereo,48000,2,kShortFrames); writeWave(mono,44100,1,kShortFrames); const std::array<std::string,3> names{"fixture.mp4","fixture.mkv","fixture.mov"}; for(const auto& name:names){const auto fixture=root/name;makeContainer(fixture,{{48000,2,kShortFrames,0},{44100,1,kShortFrames,0}});decodeChecks(fixture,true);} decodeChecks(stereo,false); const auto delayed=root/"nonzero-start.mkv";makeContainer(delayed,{{48000,2,kShortFrames,24000}});decodeChecks(delayed,false); runtimeAndGraph(root/names.front()); selectionInvalidation(root/names.front()); const auto longWav=root/"long-48k.wav",longMp4=root/"long.mp4";writeWave(longWav,48000,2,kLongFrames);makeContainer(longMp4,{{48000,2,kLongFrames,0}});longContainer(longMp4); std::error_code error;std::filesystem::remove_all(root,error);std::cout<<"FFmpeg API fixture mux/encode MP4/MKV/MOV/WAV mono/stereo/multi-stream nonzero-start 44.1/48k selection-invalidation long-container fixed-scratch PASS\n";
}
}
int main(){try{run();return 0;}catch(const std::exception& error){std::cerr<<"ffmpeg native source FAIL "<<error.what()<<'\n';return 1;}}
