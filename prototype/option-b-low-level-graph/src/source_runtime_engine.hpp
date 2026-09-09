#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

#include "prepared_residency.hpp"
#include "shared_source_runtime.hpp"
#include "source_runtime_contract.hpp"

namespace audionle::source_runtime {

class SourceRuntime final {
 public:
  SourceRuntime(RuntimeIdentity identity, std::filesystem::path artifact = {},
                SharedNativeSourceRuntimePtr shared = {}, PreparedResidencyPtr prepared = {});
  ~SourceRuntime();
  SourceRuntime(SourceRuntime const&) = delete;
  SourceRuntime& operator=(SourceRuntime const&) = delete;
  SourceRenderResult render(ClipRuntimeView const&, std::int64_t timeline, unsigned frames, float*) noexcept;
  bool serviceNative() noexcept;
  bool servicePrepared() noexcept;
  [[nodiscard]] bool hasNativeDemand() const noexcept;
  [[nodiscard]] bool hasPreparedDemand() const noexcept;
  [[nodiscard]] unsigned nativeCalls() const noexcept;
  [[nodiscard]] unsigned realtimeCalls() const noexcept;
  [[nodiscard]] unsigned preparedCalls() const noexcept;
  [[nodiscard]] unsigned preparedMisses() const noexcept;
  [[nodiscard]] SharedNativeSourceRuntimePtr const& shared() const noexcept;
  [[nodiscard]] PreparedResidencyPtr const& preparedShared() const noexcept;
 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

class SharedSourceWorker final {
 public:
  struct Metrics { std::uint64_t submitted{}, coalesced{}, completed{}, queueFull{}, maxObservedServiceGap{}; bool starvationObserved{}; };
  void add(SourceRuntime&);
  void drain();
  [[nodiscard]] Metrics metrics() const noexcept;
 private:
  bool serviceNext() noexcept;
  SourceRuntime* sources_[8]{};
  unsigned count_{}, cursor_{}, turnsSinceService_{};
  Metrics metrics_{};
};

class SourceNode final {
 public:
  explicit SourceNode(SourceRuntime&) noexcept;
  SourceRenderResult process(ClipRuntimeView const&, std::int64_t timeline, unsigned frames, float*) noexcept;
 private:
  SourceRuntime& runtime_;
};

RuntimeIdentity makeRuntimeIdentity(std::uint64_t source, std::uint32_t sourceRate, Admission) noexcept;

} // namespace audionle::source_runtime
