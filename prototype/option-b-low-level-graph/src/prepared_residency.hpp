#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>

namespace audionle::source_runtime {

struct PreparedArtifactKey {
  std::uint64_t source{}, generation{}, config{};
  std::uint32_t channels{}, projectRate{};
};

class PreparedResidency;
using PreparedResidencyPtr = std::shared_ptr<PreparedResidency>;

PreparedResidencyPtr makePreparedResidency(std::filesystem::path artifact, PreparedArtifactKey key);
bool preparedCallback(PreparedResidency& residency, std::uint32_t frame, std::uint32_t frames, float* output) noexcept;
void servicePrepared(PreparedResidency& residency);
void drainPrepared(PreparedResidency& residency);
bool preparedMatches(PreparedResidency const& residency, PreparedArtifactKey key) noexcept;
std::uint32_t preparedCoalesced(PreparedResidency const& residency) noexcept;
std::uint32_t preparedSubmitted(PreparedResidency const& residency) noexcept;
std::uint32_t preparedCompleted(PreparedResidency const& residency) noexcept;
std::uint32_t preparedQueueFull(PreparedResidency const& residency) noexcept;
bool preparedHasDemand(PreparedResidency const& residency) noexcept;
void writePreparedFixtureArtifact(std::filesystem::path const& path, PreparedArtifactKey key, std::uint32_t pages);
void writeSparsePreparedFixtureArtifact(std::filesystem::path const& path, PreparedArtifactKey key, std::uint32_t pages);

} // namespace audionle::source_runtime
