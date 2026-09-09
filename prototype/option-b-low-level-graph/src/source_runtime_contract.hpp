#pragma once

#include <cstdint>

namespace audionle::source_runtime {

// This is the callback-facing engine contract.  It intentionally contains no
// decoder, SRC, artifact, worker, or OS type.
enum class Admission { NativeRate, GuaranteedRealtime, BestEffortRealtime, PreparedRequired, Unsupported };
enum class SourceRenderResult { Ready, Unavailable, Failed, Stale, Unsupported };

struct RuntimeIdentity {
  std::uint64_t source{}, generation{}, stream{}, config{};
  std::uint32_t sourceRate{}, projectRate{}, p{}, q{}, channels{};
  Admission admission{};
};

struct ClipRuntimeView {
  std::int64_t timelineStart{}, sourceStart{}, length{};
  RuntimeIdentity identity{};
  [[nodiscard]] std::int64_t sourceAt(std::int64_t timeline) const noexcept {
    return sourceStart + timeline - timelineStart;
  }
};

} // namespace audionle::source_runtime
