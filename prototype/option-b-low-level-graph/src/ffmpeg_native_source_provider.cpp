#include "ffmpeg_native_source_provider.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace audionle::source_runtime {
namespace {
std::uint64_t streamId(AVStream const* stream, unsigned index) noexcept {
  return (std::uint64_t(index) << 32) ^ std::uint64_t(stream->codecpar->codec_id);
}
void closeFormat(AVFormatContext* value) noexcept { if (value) avformat_close_input(&value); }
void freeCodec(AVCodecContext* value) noexcept { if (value) avcodec_free_context(&value); }
void freeResampler(SwrContext* value) noexcept { if (value) swr_free(&value); }
void freePacket(AVPacket* value) noexcept { if (value) av_packet_free(&value); }
void freeFrame(AVFrame* value) noexcept { if (value) av_frame_free(&value); }

bool appendFrame(SwrContext* resampler, AVFrame const* frame, unsigned channels, std::int64_t start,
                 unsigned count, std::int64_t& nativePosition, unsigned& written, float* output) {
  const int capacity = swr_get_out_samples(resampler, frame->nb_samples);
  if (capacity <= 0) return false;
  std::vector<float> converted(size_t(capacity) * channels);
  uint8_t* destination = reinterpret_cast<uint8_t*>(converted.data());
  const int received = swr_convert(resampler, &destination, capacity,
                                   const_cast<uint8_t const**>(frame->extended_data), frame->nb_samples);
  if (received < 0) return false;
  for (int index = 0; index < received; ++index, ++nativePosition) {
    if (nativePosition >= start && written < count) {
      std::copy_n(converted.data() + size_t(index) * channels, channels,
                  output + size_t(written) * channels);
      ++written;
    }
  }
  return true;
}
} // namespace

std::vector<AudioStreamDescriptor> enumerateAudioStreams(std::filesystem::path const& path) {
  AVFormatContext* raw{};
  if (avformat_open_input(&raw, path.string().c_str(), nullptr, nullptr) < 0) throw std::runtime_error("unable to open media");
  std::unique_ptr<AVFormatContext, decltype(&closeFormat)> format(raw, closeFormat);
  if (avformat_find_stream_info(raw, nullptr) < 0) throw std::runtime_error("unable to read stream information");
  std::vector<AudioStreamDescriptor> result;
  for (unsigned index = 0; index < raw->nb_streams; ++index) {
    const auto* stream = raw->streams[index]; const auto* codec = stream->codecpar;
    if (codec->codec_type == AVMEDIA_TYPE_AUDIO && codec->sample_rate > 0 &&
        codec->ch_layout.nb_channels > 0 && codec->ch_layout.nb_channels <= 2)
      result.push_back({index, unsigned(codec->ch_layout.nb_channels), unsigned(codec->sample_rate),
                        streamId(stream, index), codec->ch_layout.u.mask});
  }
  return result;
}

struct FfmpegNativeSourceProvider::Impl {
  std::filesystem::path path; AudioStreamDescriptor stream;
  Impl(std::filesystem::path input, AudioStreamDescriptor selected) : path(std::move(input)), stream(selected) {}
};
FfmpegNativeSourceProvider::FfmpegNativeSourceProvider(std::filesystem::path path, AudioStreamDescriptor stream)
    : impl_(std::make_unique<Impl>(std::move(path), stream)) {}
FfmpegNativeSourceProvider::~FfmpegNativeSourceProvider() = default;

NativeReadResult FfmpegNativeSourceProvider::read(std::int64_t start, unsigned count, float* output) noexcept {
  // Worker-side only: decoded-frame order, not PTS/timebase, defines native samples.
  try {
    if (!output || start < 0 || !count || !impl_->stream.channels || impl_->stream.channels > 2 || !impl_->stream.sampleRate)
      return NativeReadResult::Failed;
    AVFormatContext* rawFormat{};
    if (avformat_open_input(&rawFormat, impl_->path.string().c_str(), nullptr, nullptr) < 0) return NativeReadResult::Failed;
    std::unique_ptr<AVFormatContext, decltype(&closeFormat)> format(rawFormat, closeFormat);
    if (avformat_find_stream_info(rawFormat, nullptr) < 0 || impl_->stream.index >= rawFormat->nb_streams) return NativeReadResult::Failed;
    auto* stream = rawFormat->streams[impl_->stream.index]; auto* parameters = stream->codecpar;
    if (parameters->codec_type != AVMEDIA_TYPE_AUDIO || streamId(stream, impl_->stream.index) != impl_->stream.stableId ||
        unsigned(parameters->ch_layout.nb_channels) != impl_->stream.channels || unsigned(parameters->sample_rate) != impl_->stream.sampleRate ||
        parameters->ch_layout.u.mask != impl_->stream.layout) return NativeReadResult::Stale;
    const AVCodec* decoder = avcodec_find_decoder(parameters->codec_id); if (!decoder) return NativeReadResult::Failed;
    AVCodecContext* rawCodec = avcodec_alloc_context3(decoder); if (!rawCodec) return NativeReadResult::Failed;
    std::unique_ptr<AVCodecContext, decltype(&freeCodec)> codec(rawCodec, freeCodec);
    if (avcodec_parameters_to_context(rawCodec, parameters) < 0 || avcodec_open2(rawCodec, decoder, nullptr) < 0) return NativeReadResult::Failed;
    AVChannelLayout layout{};
    av_channel_layout_default(&layout, int(impl_->stream.channels));
    SwrContext* rawResampler{};
    if (swr_alloc_set_opts2(&rawResampler, &layout, AV_SAMPLE_FMT_FLT, int(impl_->stream.sampleRate), &rawCodec->ch_layout,
                            rawCodec->sample_fmt, rawCodec->sample_rate, 0, nullptr) < 0 || !rawResampler) return NativeReadResult::Failed;
    std::unique_ptr<SwrContext, decltype(&freeResampler)> resampler(rawResampler, freeResampler);
    if (swr_init(rawResampler) < 0) return NativeReadResult::Failed;
    AVPacket* rawPacket = av_packet_alloc(); AVFrame* rawFrame = av_frame_alloc();
    if (!rawPacket || !rawFrame) { freePacket(rawPacket); freeFrame(rawFrame); return NativeReadResult::Failed; }
    std::unique_ptr<AVPacket, decltype(&freePacket)> packet(rawPacket, freePacket);
    std::unique_ptr<AVFrame, decltype(&freeFrame)> frame(rawFrame, freeFrame);
    std::int64_t nativePosition{}; unsigned written{};
    auto receive = [&] {
      while (true) {
        const int status = avcodec_receive_frame(rawCodec, rawFrame);
        if (status == AVERROR(EAGAIN) || status == AVERROR_EOF) return true;
        if (status < 0 || !appendFrame(rawResampler, rawFrame, impl_->stream.channels, start, count, nativePosition, written, output)) return false;
        av_frame_unref(rawFrame); if (written == count) return true;
      }
    };
    while (written < count && av_read_frame(rawFormat, rawPacket) >= 0) {
      if (rawPacket->stream_index == int(impl_->stream.index) && (avcodec_send_packet(rawCodec, rawPacket) < 0 || !receive())) return NativeReadResult::Failed;
      av_packet_unref(rawPacket);
    }
    if (written < count && (avcodec_send_packet(rawCodec, nullptr) < 0 || !receive())) return NativeReadResult::Failed;
    return written == count ? NativeReadResult::Ready : NativeReadResult::Unavailable;
  } catch (...) { return NativeReadResult::Failed; }
}
} // namespace audionle::source_runtime
