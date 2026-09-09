#include "ffmpeg_native_source_provider.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace audionle::source_runtime {
namespace {
constexpr int kPrerollNativeFrames = 8192;
constexpr int kMaxConvertedFrameSamples = 8192;
std::uint64_t streamId(AVStream const* stream, unsigned index) noexcept { return (std::uint64_t(index) << 32) ^ std::uint64_t(stream->codecpar->codec_id); }
void closeFormat(AVFormatContext* value) noexcept { if (value) avformat_close_input(&value); }
void freeCodec(AVCodecContext* value) noexcept { if (value) avcodec_free_context(&value); }
void freeResampler(SwrContext* value) noexcept { if (value) swr_free(&value); }
void freePacket(AVPacket* value) noexcept { if (value) av_packet_free(&value); }
void freeFrame(AVFrame* value) noexcept { if (value) av_frame_free(&value); }

std::int64_t sampleTimestamp(std::int64_t sample, int rate, AVRational timeBase, std::int64_t origin) noexcept {
  return origin + av_rescale_q(sample, AVRational{1, rate}, timeBase);
}

std::uint64_t layoutMask(AVChannelLayout const& layout) noexcept {
  if (layout.order == AV_CHANNEL_ORDER_NATIVE) return layout.u.mask;
  // The V1 contract supports only mono/stereo.  For codecs that omit a native
  // mask, that cardinality has one canonical layout; custom layouts remain out
  // of scope and are rejected by enumeration.
  if (layout.order != AV_CHANNEL_ORDER_UNSPEC || layout.nb_channels == 0 || layout.nb_channels > 2) return 0;
  AVChannelLayout canonical{};
  av_channel_layout_default(&canonical, layout.nb_channels);
  return canonical.u.mask;
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
    const auto layout = layoutMask(codec->ch_layout);
    if (codec->codec_type == AVMEDIA_TYPE_AUDIO && codec->sample_rate > 0 && layout &&
        codec->ch_layout.nb_channels > 0 && codec->ch_layout.nb_channels <= 2)
      result.push_back({index, unsigned(codec->ch_layout.nb_channels), unsigned(codec->sample_rate), streamId(stream, index), layout});
  }
  return result;
}

struct FfmpegNativeSourceProvider::Impl {
  std::filesystem::path path; AudioStreamDescriptor stream;
  std::atomic<std::uint64_t> seekRequests{}, decodedFrames{}; std::atomic<unsigned> maxScratchFrames{};
  Impl(std::filesystem::path input, AudioStreamDescriptor selected) : path(std::move(input)), stream(selected) {}
};
FfmpegNativeSourceProvider::FfmpegNativeSourceProvider(std::filesystem::path path, AudioStreamDescriptor stream) : impl_(std::make_unique<Impl>(std::move(path), stream)) {}
FfmpegNativeSourceProvider::~FfmpegNativeSourceProvider() = default;
FfmpegProviderDiagnostics FfmpegNativeSourceProvider::diagnostics() const noexcept { return {impl_->seekRequests.load(), impl_->decodedFrames.load(), impl_->maxScratchFrames.load()}; }

NativeReadResult FfmpegNativeSourceProvider::read(std::int64_t start, unsigned count, float* output) noexcept {
  // Worker-side only. FFmpeg timestamps choose a demux entry point; native coordinates
  // are integer sample positions and frame samples before `start` are discarded.
  try {
    if (!output || start < 0 || !count || !impl_->stream.channels || impl_->stream.channels > 2 || !impl_->stream.sampleRate) return NativeReadResult::Failed;
    AVFormatContext* rawFormat{};
    if (avformat_open_input(&rawFormat, impl_->path.string().c_str(), nullptr, nullptr) < 0) return NativeReadResult::Failed;
    std::unique_ptr<AVFormatContext, decltype(&closeFormat)> format(rawFormat, closeFormat);
    if (avformat_find_stream_info(rawFormat, nullptr) < 0 || impl_->stream.index >= rawFormat->nb_streams) return NativeReadResult::Failed;
    auto* stream = rawFormat->streams[impl_->stream.index]; auto* parameters = stream->codecpar;
    if (parameters->codec_type != AVMEDIA_TYPE_AUDIO ||
        streamId(stream, impl_->stream.index) != impl_->stream.stableId ||
        unsigned(parameters->ch_layout.nb_channels) != impl_->stream.channels ||
        unsigned(parameters->sample_rate) != impl_->stream.sampleRate ||
        layoutMask(parameters->ch_layout) != impl_->stream.layout) return NativeReadResult::Stale;
    const AVCodec* decoder = avcodec_find_decoder(parameters->codec_id); if (!decoder) return NativeReadResult::Failed;
    AVCodecContext* rawCodec = avcodec_alloc_context3(decoder); if (!rawCodec) return NativeReadResult::Failed;
    std::unique_ptr<AVCodecContext, decltype(&freeCodec)> codec(rawCodec, freeCodec);
    if (avcodec_parameters_to_context(rawCodec, parameters) < 0 || avcodec_open2(rawCodec, decoder, nullptr) < 0) return NativeReadResult::Failed;
    AVChannelLayout layout{}; av_channel_layout_default(&layout, int(impl_->stream.channels));
    SwrContext* rawResampler{};
    if (swr_alloc_set_opts2(&rawResampler, &layout, AV_SAMPLE_FMT_FLT, int(impl_->stream.sampleRate), &rawCodec->ch_layout, rawCodec->sample_fmt, rawCodec->sample_rate, 0, nullptr) < 0 || !rawResampler) return NativeReadResult::Failed;
    std::unique_ptr<SwrContext, decltype(&freeResampler)> resampler(rawResampler, freeResampler);
    if (swr_init(rawResampler) < 0) return NativeReadResult::Failed;
    AVPacket* rawPacket = av_packet_alloc(); AVFrame* rawFrame = av_frame_alloc();
    if (!rawPacket || !rawFrame) { freePacket(rawPacket); freeFrame(rawFrame); return NativeReadResult::Failed; }
    std::unique_ptr<AVPacket, decltype(&freePacket)> packet(rawPacket, freePacket); std::unique_ptr<AVFrame, decltype(&freeFrame)> frame(rawFrame, freeFrame);

    const auto origin = stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time;
    const auto seekStart = std::max<std::int64_t>(0, start - kPrerollNativeFrames);
    const auto seekTimestamp = sampleTimestamp(seekStart, int(impl_->stream.sampleRate), stream->time_base, origin);
    if (start > 0) {
      if (avformat_seek_file(rawFormat, int(impl_->stream.index), std::numeric_limits<std::int64_t>::min(), seekTimestamp, seekTimestamp, AVSEEK_FLAG_BACKWARD) < 0) return NativeReadResult::Unavailable;
      avcodec_flush_buffers(rawCodec); ++impl_->seekRequests;
    }

    std::array<float, size_t(kMaxConvertedFrameSamples) * NativeSourceService::maxChannels> converted{};
    unsigned written{};
    // Timestamp conversion is strictly a one-time seek-recovery alignment.  The
    // source contract and all subsequent discard/copy decisions use this integer
    // cursor, never a per-frame timestamp.
    std::int64_t nativePosition = seekStart;
    bool positioned{};
    auto receive = [&] {
      while (true) {
        const int status = avcodec_receive_frame(rawCodec, rawFrame);
        if (status == AVERROR(EAGAIN) || status == AVERROR_EOF) return true;
        if (status < 0) return false;
        const int capacity = swr_get_out_samples(rawResampler, rawFrame->nb_samples);
        if (capacity <= 0 || capacity > kMaxConvertedFrameSamples) return false;
        unsigned prior = impl_->maxScratchFrames.load(); while (prior < unsigned(capacity) && !impl_->maxScratchFrames.compare_exchange_weak(prior, unsigned(capacity))) {}
        uint8_t* destination = reinterpret_cast<uint8_t*>(converted.data());
        const int received = swr_convert(rawResampler, &destination, capacity, const_cast<uint8_t const**>(rawFrame->extended_data), rawFrame->nb_samples);
        if (received < 0) return false;
        if (!positioned) {
          const auto timestamp = rawFrame->best_effort_timestamp;
          if (timestamp != AV_NOPTS_VALUE)
            nativePosition = av_rescale_q(timestamp - origin, stream->time_base,
                                          AVRational{1, int(impl_->stream.sampleRate)});
          positioned = true;
        }
        for (int index = 0; index < received && written < count; ++index) {
          if (nativePosition >= start) {
            std::copy_n(converted.data() + size_t(index) * impl_->stream.channels,
                        impl_->stream.channels,
                        output + size_t(written) * impl_->stream.channels);
            ++written;
          }
          ++nativePosition;
        }
        impl_->decodedFrames += unsigned(received); av_frame_unref(rawFrame);
        if (written == count) return true;
      }
    };
    while (written < count && av_read_frame(rawFormat, rawPacket) >= 0) { if (rawPacket->stream_index == int(impl_->stream.index) && (avcodec_send_packet(rawCodec, rawPacket) < 0 || !receive())) return NativeReadResult::Failed; av_packet_unref(rawPacket); }
    if (written < count && (avcodec_send_packet(rawCodec, nullptr) < 0 || !receive())) return NativeReadResult::Failed;
    return written == count ? NativeReadResult::Ready : NativeReadResult::Unavailable;
  } catch (...) { return NativeReadResult::Failed; }
}
} // namespace audionle::source_runtime
