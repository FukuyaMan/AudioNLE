#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

namespace audionle::source_runtime {
enum class NativeReadResult { Ready, Unavailable, Failed, Stale, CapacityExceeded };
class NativeSourceProvider { public: virtual ~NativeSourceProvider()=default; virtual NativeReadResult read(std::int64_t start,unsigned frames,float* destination) noexcept=0; };
class FixtureNativeProvider final : public NativeSourceProvider { public: explicit FixtureNativeProvider(std::int64_t frames=4096):frames_(frames){} NativeReadResult read(std::int64_t s,unsigned n,float*d)noexcept override{++reads_;if(s<0||s+std::int64_t(n)>frames_)return NativeReadResult::Unavailable;for(unsigned i=0;i<n;++i)d[i]=float(((s+i)%97)+1)/100.0f;return NativeReadResult::Ready;} [[nodiscard]] unsigned reads()const noexcept{return reads_.load();} private:std::int64_t frames_{};std::atomic<unsigned>reads_{}; };
struct NativeSourceDiagnostics { std::uint64_t requests{},coalesced{},queueFull{},workerCompleted{},workerFailed{},staleRejected{}; };

// Callback code only scans fixed arrays, publishes/coalesces a page request,
// and copies a published page. Provider access belongs exclusively to serviceOne.
class NativeSourceService final {
 public:
  static constexpr unsigned pageFrames=2048,pages=2,requestCapacity=8;
  NativeSourceService(NativeSourceProvider&p,std::uint64_t source,std::uint64_t g) noexcept : provider_(p),source_(source),generation_(g) {}
  void reconfigureGeneration(std::uint64_t g) noexcept { generation_.store(g,std::memory_order_release); }
  bool copy(std::int64_t start,unsigned n,std::uint64_t g,float*d) noexcept {
    if(!d||n==0||n>pageFrames||g!=generation_.load(std::memory_order_acquire)||start<0)return false;const auto page=start/pageFrames*pageFrames;if(start+n>page+pageFrames)return false;
    for(auto&slot:slots_)if(slot.state.load(std::memory_order_acquire)==2&&slot.page==page&&slot.generation==g){std::copy_n(slot.data.data()+start-page,n,d);return true;}publish(page,g);return false;
  }
  bool serviceOne() noexcept {
    for(auto&r:requestTable_){unsigned expected=1;if(!r.state.compare_exchange_strong(expected,2,std::memory_order_acq_rel))continue;const auto current=generation_.load(std::memory_order_acquire);if(r.generation!=current||r.source!=source_){r.state.store(0,std::memory_order_release);++staleRejected_;return true;}auto&slot=slots_[nextPage_++%pages];slot.state.store(1,std::memory_order_release);const auto result=provider_.read(r.start,r.frames,slot.data.data());if(result==NativeReadResult::Ready&&r.generation==generation_.load(std::memory_order_acquire)){slot.page=r.start;slot.generation=r.generation;slot.state.store(2,std::memory_order_release);++workerCompleted_;}else{slot.state.store(0,std::memory_order_release);if(result==NativeReadResult::Stale)++staleRejected_;else ++workerFailed_;}r.state.store(0,std::memory_order_release);return true;}return false;
  }
  [[nodiscard]] bool hasPending() const noexcept { for(auto const&r:requestTable_)if(r.state.load(std::memory_order_acquire)==1)return true; return false; }
  void prime(std::int64_t start=0) noexcept { publish(start/pageFrames*pageFrames,generation_.load(std::memory_order_acquire)); }
  [[nodiscard]] NativeSourceDiagnostics diagnostics()const noexcept{return{requests_.load(),coalesced_.load(),queueFull_.load(),workerCompleted_.load(),workerFailed_.load(),staleRejected_.load()};}
  [[nodiscard]] static constexpr size_t bytes()noexcept{return pages*pageFrames*sizeof(float);} [[nodiscard]] static constexpr size_t requestBytes()noexcept{return requestCapacity*sizeof(Request);}
 private:
  struct Page{std::array<float,pageFrames>data{};std::int64_t page{-1};std::uint64_t generation{};std::atomic<unsigned>state{0};};
  struct Request{std::uint64_t source{},generation{},token{};std::int64_t start{};unsigned frames{};std::atomic<unsigned>state{0};};
  void publish(std::int64_t page,std::uint64_t g)noexcept{for(auto&r:requestTable_){const auto state=r.state.load(std::memory_order_acquire);if(state!=0&&r.source==source_&&r.generation==g&&r.start==page&&r.frames==pageFrames){++coalesced_;return;}}for(auto&r:requestTable_){unsigned expected=0;if(r.state.compare_exchange_strong(expected,3,std::memory_order_acq_rel)){r.source=source_;r.generation=g;r.start=page;r.frames=pageFrames;r.token=++token_;r.state.store(1,std::memory_order_release);++requests_;return;}}++queueFull_;}
  NativeSourceProvider&provider_;std::uint64_t source_{};std::atomic<std::uint64_t>generation_{};std::array<Page,pages>slots_{};std::array<Request,requestCapacity>requestTable_{};unsigned nextPage_{};std::atomic<std::uint64_t>token_{},requests_{},coalesced_{},queueFull_{},workerCompleted_{},workerFailed_{},staleRejected_{};
};
} // namespace audionle::source_runtime
