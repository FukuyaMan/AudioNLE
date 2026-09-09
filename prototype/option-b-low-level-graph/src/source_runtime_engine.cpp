#include "source_runtime_engine.hpp"

#include <samplerate.h>

#include <algorithm>
#include <array>
#include <numeric>
#include <stdexcept>

#include "physical_src_range_planner.hpp"

namespace audionle::source_runtime {
namespace {
void require(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
}

class SourceRuntime::Impl final {
 public:
  Impl(RuntimeIdentity identity, std::filesystem::path artifact, SharedNativeSourceRuntimePtr shared,
       PreparedResidencyPtr prepared)
      : id(std::move(identity)), shared(shared ? std::move(shared) : std::make_shared<SharedNativeSourceRuntime>(id.source, id.generation)) {
    if (this->shared->source() != id.source || this->shared->generation() != id.generation) throw std::runtime_error("shared runtime identity");
    this->shared->prime();
    if (id.admission == Admission::PreparedRequired) {
      const PreparedArtifactKey key{id.source, id.generation, id.config, id.channels, id.projectRate};
      if (prepared && !preparedMatches(*prepared, key)) throw std::runtime_error("shared prepared identity");
      this->prepared = prepared ? std::move(prepared) : makePreparedResidency(std::move(artifact), key);
    }
    if (id.admission == Admission::GuaranteedRealtime || id.admission == Admission::BestEffortRealtime) {
      int error{}; src = src_new(SRC_SINC_BEST_QUALITY, 1, &error); require(src && error == 0, "src configure");
    }
  }
  ~Impl() { if (src) src_delete(src); }
  RuntimeIdentity id; SharedNativeSourceRuntimePtr shared; PhysicalSrcRangePlanner planner{};
  std::array<float, 2048> nativeInput{}; SRC_STATE* src{}; PreparedResidencyPtr prepared{};
  unsigned native{}, realtime{}, preparedCalls{}, preparedMisses{};
};

SourceRuntime::SourceRuntime(RuntimeIdentity identity, std::filesystem::path artifact, SharedNativeSourceRuntimePtr shared,
                             PreparedResidencyPtr prepared)
    : impl_(std::make_unique<Impl>(identity, std::move(artifact), std::move(shared), std::move(prepared))) {}
SourceRuntime::~SourceRuntime() = default;
SourceRenderResult SourceRuntime::render(ClipRuntimeView const& view, std::int64_t timeline, unsigned frames, float* output) noexcept {
  auto& r = *impl_; std::fill_n(output, frames, 0.0f);
  if (view.identity.generation != r.id.generation) return SourceRenderResult::Stale;
  const auto source = view.sourceAt(timeline); if (source < 0) return SourceRenderResult::Unavailable;
  if (r.id.admission == Admission::NativeRate) {
    if (!r.shared->service().copy(source, frames, r.id.generation, output)) return SourceRenderResult::Unavailable;
    ++r.native; return SourceRenderResult::Ready;
  }
  if (r.id.admission == Admission::PreparedRequired) {
    if (!preparedCallback(*r.prepared, std::uint32_t(source), frames, output)) { ++r.preparedMisses; return SourceRenderResult::Unavailable; }
    ++r.preparedCalls; return SourceRenderResult::Ready;
  }
  PhysicalSrcRangePlan plan{};
  if (!r.planner.plan(view.sourceStart, timeline - view.timelineStart, r.id.p, r.id.q, frames, r.shared->sourceFrames(), plan) || !r.src ||
      frames * 2 > r.nativeInput.size() || !r.shared->service().copy(source, frames * 2, r.id.generation, r.nativeInput.data())) return SourceRenderResult::Unavailable;
  SRC_DATA data{}; data.data_in = r.nativeInput.data(); data.input_frames = frames * 2; data.data_out = output; data.output_frames = frames;
  data.src_ratio = double(r.id.projectRate) / r.id.sourceRate;
  if (src_process(r.src, &data) != 0) return SourceRenderResult::Failed;
  ++r.realtime; return data.output_frames_gen == long(frames) ? SourceRenderResult::Ready : SourceRenderResult::Unavailable;
}
bool SourceRuntime::serviceNative() noexcept { return impl_->shared->service().serviceOne(); }
bool SourceRuntime::servicePrepared() noexcept { if (!impl_->prepared) return false; if (!preparedHasDemand(*impl_->prepared)) return false; audionle::source_runtime::servicePrepared(*impl_->prepared); return true; }
bool SourceRuntime::hasNativeDemand() const noexcept { return impl_->shared->service().hasPending(); }
bool SourceRuntime::hasPreparedDemand() const noexcept { return impl_->prepared && preparedHasDemand(*impl_->prepared); }
unsigned SourceRuntime::nativeCalls() const noexcept { return impl_->native; }
unsigned SourceRuntime::realtimeCalls() const noexcept { return impl_->realtime; }
unsigned SourceRuntime::preparedCalls() const noexcept { return impl_->preparedCalls; }
unsigned SourceRuntime::preparedMisses() const noexcept { return impl_->preparedMisses; }
SharedNativeSourceRuntimePtr const& SourceRuntime::shared() const noexcept { return impl_->shared; }
PreparedResidencyPtr const& SourceRuntime::preparedShared() const noexcept { return impl_->prepared; }
void SharedSourceWorker::add(SourceRuntime& runtime) { require(count_ < std::size(sources_), "worker capacity"); sources_[count_++] = &runtime; }
bool SharedSourceWorker::serviceNext() noexcept {
  const unsigned slots = count_ * 2; if (!slots) return false;
  for (unsigned offset = 0; offset < slots; ++offset) {
    const unsigned candidate = (cursor_ + offset) % slots; auto& source = *sources_[candidate / 2];
    const bool native = candidate % 2 == 0; if (native ? !source.hasNativeDemand() : !source.hasPreparedDemand()) continue;
    cursor_ = (candidate + 1) % slots; const bool serviced = native ? source.serviceNative() : source.servicePrepared();
    if (serviced) { metrics_.completed++; metrics_.maxObservedServiceGap = std::max<std::uint64_t>(metrics_.maxObservedServiceGap, turnsSinceService_); turnsSinceService_ = 0; }
    return serviced;
  }
  return false;
}
void SharedSourceWorker::drain() { for (unsigned turn = 0; turn < 512; ++turn) { ++turnsSinceService_; if (!serviceNext()) break; } }
SharedSourceWorker::Metrics SharedSourceWorker::metrics() const noexcept {
  Metrics result = metrics_;
  for (unsigned i=0;i<count_;++i) { const auto native=sources_[i]->shared()->service().diagnostics(); result.submitted+=native.requests; result.coalesced+=native.coalesced; result.queueFull+=native.queueFull; if (auto const& prepared=sources_[i]->preparedShared()) { result.submitted+=preparedSubmitted(*prepared); result.coalesced+=preparedCoalesced(*prepared); result.queueFull+=preparedQueueFull(*prepared); } }
  result.starvationObserved = result.maxObservedServiceGap > count_ * 2; return result;
}
SourceNode::SourceNode(SourceRuntime& runtime) noexcept : runtime_(runtime) {}
SourceRenderResult SourceNode::process(ClipRuntimeView const& view, std::int64_t timeline, unsigned frames, float* output) noexcept { return runtime_.render(view, timeline, frames, output); }
RuntimeIdentity makeRuntimeIdentity(std::uint64_t source, std::uint32_t sourceRate, Admission admission) noexcept {
  const auto gcd = std::gcd(sourceRate, 48000u); return {source, 1, 0, 77, sourceRate, 48000, 48000u / gcd, sourceRate / gcd, 1, admission};
}
} // namespace audionle::source_runtime
