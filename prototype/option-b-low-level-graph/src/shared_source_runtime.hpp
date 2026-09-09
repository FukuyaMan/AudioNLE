#pragma once

#include <memory>
#include "native_source_service.hpp"

namespace audionle::source_runtime {
// Source-level lifetime boundary. It is constructed/reconfigured on the
// control side; ClipRuntimeView-local resamplers only borrow it at callback.
class SharedNativeSourceRuntime final {
 public:
  SharedNativeSourceRuntime(std::uint64_t source, std::uint64_t generation, std::int64_t frames=4096)
      : source_(source), generation_(generation), frames_(frames), provider_(frames), service_(provider_,source,generation) {}
  void prime() noexcept { service_.prime(); service_.serviceOne(); }
  void reconfigure(std::uint64_t generation) noexcept { generation_=generation; service_.reconfigureGeneration(generation); }
  [[nodiscard]] NativeSourceService& service() noexcept { return service_; }
  [[nodiscard]] FixtureNativeProvider const& provider() const noexcept { return provider_; }
  [[nodiscard]] std::uint64_t source() const noexcept { return source_; }
  [[nodiscard]] std::uint64_t generation() const noexcept { return generation_; }
  [[nodiscard]] std::int64_t sourceFrames() const noexcept { return frames_; }
 private:
  std::uint64_t source_{},generation_{}; std::int64_t frames_{}; FixtureNativeProvider provider_; NativeSourceService service_;
};
using SharedNativeSourceRuntimePtr=std::shared_ptr<SharedNativeSourceRuntime>;
} // namespace audionle::source_runtime
