#pragma once
#include <filesystem>
#include <memory>
#include <vector>
#include "native_source_service.hpp"
namespace audionle::source_runtime {
struct AudioStreamDescriptor { unsigned index{}, channels{}, sampleRate{}; std::uint64_t stableId{}, layout{}; };
struct FfmpegProviderDiagnostics { std::uint64_t seekRequests{}, decodedFrames{}; unsigned maxScratchFrames{}; };
std::vector<AudioStreamDescriptor> enumerateAudioStreams(std::filesystem::path const&);
class FfmpegNativeSourceProvider final : public NativeSourceProvider {
 public: FfmpegNativeSourceProvider(std::filesystem::path, AudioStreamDescriptor); ~FfmpegNativeSourceProvider() override;
 NativeReadResult read(std::int64_t,unsigned,float*) noexcept override;
 [[nodiscard]] FfmpegProviderDiagnostics diagnostics() const noexcept;
 private: struct Impl; std::unique_ptr<Impl> impl_;
}; }
