#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace audionle::source_runtime {

// Integer-only range planning shared by source reconstruction and its tests.
// P/Q is project/native after reduction. PrerollPolicyV1 is backend-neutral.
struct PhysicalSrcRangePlan {
  std::int64_t physicalStart{};
  std::int64_t logicalNativeStart{};
  std::int64_t outputDiscard{};
  std::uint32_t nativeFrames{};
  std::uint32_t projectFrames{};
};

class PhysicalSrcRangePlanner final {
 public:
  static constexpr std::uint32_t kNativeCapacity = 2048;
  static constexpr std::uint32_t kProjectCapacity = 512;
  static constexpr std::int64_t kPrerollNativeFrames = 1024;
  [[nodiscard]] static constexpr std::int64_t ceilDivide(std::int64_t n, std::int64_t d) noexcept { return n / d + (n % d != 0 ? 1 : 0); }
  [[nodiscard]] bool plan(std::int64_t sourceStart, std::int64_t timelineOffset, std::uint32_t p, std::uint32_t q, std::uint32_t requestedProjectFrames, std::int64_t sourceFrames, PhysicalSrcRangePlan& out) const noexcept {
    if (p == 0 || q == 0 || requestedProjectFrames == 0 || requestedProjectFrames > kProjectCapacity || sourceStart < 0 || timelineOffset < 0 || sourceFrames < 0 || sourceStart >= sourceFrames) return false;
    if (timelineOffset > (std::numeric_limits<std::int64_t>::max() - sourceStart) / q) return false;
    const auto logical = sourceStart + (timelineOffset * q) / p;
    if (logical >= sourceFrames) return false;
    const auto prerollStart = std::max(sourceStart, logical - kPrerollNativeFrames);
    const auto physical = prerollStart;
    // This is the V1 reconstruction discard rule.  It intentionally retains
    // the timeline phase instead of forcing an additional q-aligned preroll.
    const auto discard = timelineOffset - ((physical - sourceStart) * p) / q;
    const auto required = (logical - physical) + ceilDivide(std::int64_t(requestedProjectFrames) * q, p) + q;
    if (required <= 0 || required > kNativeCapacity) return false;
    const auto available = sourceFrames - physical;
    out = {physical, logical, discard, static_cast<std::uint32_t>(std::min<std::int64_t>(required, available)), requestedProjectFrames};
    return out.nativeFrames != 0;
  }
};
} // namespace audionle::source_runtime
