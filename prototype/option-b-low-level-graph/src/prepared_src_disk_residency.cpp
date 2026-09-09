#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "prepared_residency.hpp"

namespace {
constexpr std::uint32_t kPageFrames = 256, kSlots = 16, kRequests = 8, kMaxChannels = 2;
constexpr std::array<char, 8> kMagic{'A','N','L','E','P','R','P','1'};
struct Key { std::uint64_t source{}, generation{}, config{}; std::uint32_t channels{}, rate{}; };
struct Header { std::array<char,8> magic{kMagic}; std::uint32_t version{1}; Key key{}; std::uint32_t pageFrames{kPageFrames}, totalFrames{}; };
struct Record { std::uint32_t page{}, frames{}, sum{}; };
static_assert(std::is_trivially_copyable_v<Header>);
std::uint32_t sum(float const* x, size_t n) { std::uint32_t h=2166136261u; auto p=reinterpret_cast<unsigned char const*>(x); for(size_t i=0;i<n*sizeof(float);++i) h=(h^p[i])*16777619u; return h; }
void need(bool ok, char const* what) { if (!ok) throw std::runtime_error(what); }
float sample(std::uint32_t page, std::uint32_t frame, std::uint32_t channel) { return float(page * 1000 + frame * 3 + channel); }
std::uint64_t recordBytes(std::uint32_t channels) { return sizeof(Record) + std::uint64_t(kPageFrames) * channels * sizeof(float); }
std::uint64_t offset(std::uint32_t page, std::uint32_t channels) { return sizeof(Header) + std::uint64_t(page) * recordBytes(channels); }

void writeArtifact(std::filesystem::path const& path, Key key, std::uint32_t pages, bool sparse = false) {
  Header h{}; h.key=key; h.totalFrames=pages*kPageFrames;
  std::fstream out(path, std::ios::binary|std::ios::out|std::ios::trunc); need(bool(out), "write open");
  out.write(reinterpret_cast<char const*>(&h), sizeof h);
  const auto emit = [&](std::uint32_t p) { std::array<float,kPageFrames*kMaxChannels> data{}; for(std::uint32_t f=0;f<kPageFrames;++f) for(std::uint32_t c=0;c<key.channels;++c) data[f*key.channels+c]=sample(p,f,c); Record r{p,kPageFrames,sum(data.data(),size_t(kPageFrames)*key.channels)}; out.seekp(std::streamoff(offset(p,key.channels))); out.write(reinterpret_cast<char const*>(&r),sizeof r); out.write(reinterpret_cast<char const*>(data.data()),std::streamsize(kPageFrames*key.channels*sizeof(float))); };
  if (sparse) { emit(0); emit(pages/2); emit(pages-1); } else for(std::uint32_t p=0;p<pages;++p) emit(p);
  out.close(); need(bool(out), "write artifact");
}

struct Diagnostics { std::atomic<std::uint32_t> hits{}, misses{}, requests{}, coalesced{}, success{}, failure{}, prefetch{}, evictions{}, pinnedNoSlot{}, stale{}, corrupt{}, unavailable{}, fallback{}; };
struct Request { std::atomic<unsigned> state{}; std::uint32_t page{}, generation{}; }; // 1 demand, 2 prefetch, 3 worker-owned
struct RequestData { std::uint32_t page{}, generation{}; };
struct Slot { std::atomic<unsigned> state{}; std::atomic<std::uint32_t> page{}, generation{}, version{1}, pins{}; std::atomic<std::uint64_t> age{}; std::array<float,kPageFrames*kMaxChannels> data{}; }; // Empty, Loading, Ready

class Runtime {
 public:
  Runtime(std::filesystem::path path, Key expected) : path_(std::move(path)), key_(expected), generation_(expected.generation) {
    std::ifstream in(path_,std::ios::binary); Header h{}; in.read(reinterpret_cast<char*>(&h),sizeof h);
    need(bool(in) && h.magic==kMagic && h.version==1 && std::memcmp(&h.key,&key_,sizeof key_)==0 && h.pageFrames==kPageFrames && key_.channels>0 && key_.channels<=kMaxChannels,"artifact header"); totalPages_=(h.totalFrames+kPageFrames-1)/kPageFrames;
  }
  bool callback(std::uint32_t frame, std::uint32_t frames, float* out) noexcept {
    if (!frames || frame+frames > totalPages_*kPageFrames || frames > kPageFrames*2) { zero(out,frames); ++d_.unavailable; return false; }
    const auto first=frame/kPageFrames, last=(frame+frames-1)/kPageFrames; Slot* found[2]{};
    for (std::uint32_t p=first;p<=last;++p) { found[p-first]=acquire(p); if (!found[p-first]) { for(auto* s:found) release(s); request(p,false); zero(out,frames); ++d_.misses; ++d_.unavailable; return false; } }
    for(std::uint32_t f=0;f<frames;++f) { auto absolute=frame+f, p=absolute/kPageFrames-first, local=absolute%kPageFrames; std::memcpy(out+size_t(f)*key_.channels, found[p]->data.data()+size_t(local)*key_.channels, key_.channels*sizeof(float)); }
    for(auto* s:found) release(s); ++d_.hits; return true;
  }
  void serviceOne() { // worker/control thread only: all file operations and eviction are here.
    int chosen=-1; unsigned requestKind{}; for(unsigned priority=1;priority<=2 && chosen<0;++priority) for(unsigned i=0;i<kRequests;++i) { unsigned expected=priority; if(q_[i].state.compare_exchange_strong(expected,3,std::memory_order_acq_rel)) { chosen=int(i); requestKind=priority; break; } }
    if(chosen<0) return; const RequestData r{q_[chosen].page,q_[chosen].generation}; q_[chosen].state.store(0,std::memory_order_release);
    if(r.generation!=generation_.load(std::memory_order_acquire)) { ++d_.stale; return; }
    Slot* slot=chooseSlot(); if(!slot) { ++d_.pinnedNoSlot; request(r.page,false); return; }
    slot->state.store(1,std::memory_order_release); std::array<float,kPageFrames*kMaxChannels> privatePage{};
    std::ifstream in(path_,std::ios::binary); Record rec{}; in.seekg(std::streamoff(offset(r.page,key_.channels))); in.read(reinterpret_cast<char*>(&rec),sizeof rec); in.read(reinterpret_cast<char*>(privatePage.data()),std::streamsize(kPageFrames*key_.channels*sizeof(float)));
    if(!in || rec.page!=r.page || rec.frames==0 || rec.frames>kPageFrames || sum(privatePage.data(),size_t(rec.frames)*key_.channels)!=rec.sum) { slot->state.store(0,std::memory_order_release); ++d_.failure; ++d_.corrupt; return; }
    if(r.generation!=generation_.load(std::memory_order_acquire)) { slot->state.store(0,std::memory_order_release); ++d_.stale; return; }
    std::copy_n(privatePage.data(),size_t(kPageFrames)*key_.channels,slot->data.data()); slot->page.store(r.page,std::memory_order_relaxed); slot->generation.store(r.generation,std::memory_order_relaxed); slot->version.fetch_add(1,std::memory_order_relaxed); slot->age.store(++clock_,std::memory_order_relaxed); slot->state.store(2,std::memory_order_release); ++d_.success;
    if(requestKind==1 && r.page+1<totalPages_) request(r.page+1,true);
  }
  void drain(unsigned max=128) { for(unsigned i=0;i<max;++i) serviceOne(); }
  void setGeneration(std::uint32_t g) { generation_.store(g,std::memory_order_release); }
  bool hold(std::uint32_t p) { if(auto* s=acquire(p)) { held_=s; return true; } return false; }
  void unhold() { release(held_); held_=nullptr; }
  Diagnostics const& diagnostics() const { return d_; }
  bool hasDemand() const noexcept { for(auto const& request:q_)if(request.state.load(std::memory_order_acquire)==1)return true; return false; }
  bool matches(Key const& key) const noexcept { return std::memcmp(&key_,&key,sizeof key)==0; }
  unsigned ready() const { unsigned n{}; for(auto const&s:slots_) n+=s.state.load()==2; return n; }
 private:
  void zero(float* out,std::uint32_t frames) noexcept { std::fill_n(out,size_t(frames)*key_.channels,0.0f); }
  void request(std::uint32_t page,bool prefetch) noexcept { const auto gen=generation_.load(std::memory_order_relaxed); for(auto&r:q_) { auto state=r.state.load(std::memory_order_acquire); if((state==1 || state==2) && r.page==page && r.generation==gen) { ++d_.coalesced; return; } } for(auto&r:q_) { unsigned expected=0; if(r.state.compare_exchange_strong(expected,3,std::memory_order_acq_rel)) { r.page=page; r.generation=gen; r.state.store(prefetch?2u:1u,std::memory_order_release); ++d_.requests; if(prefetch)++d_.prefetch; return; } } ++d_.unavailable; }
  Slot* acquire(std::uint32_t page) noexcept { const auto gen=generation_.load(std::memory_order_acquire); for(auto&s:slots_) { if(s.state.load(std::memory_order_acquire)!=2 || s.page.load()!=page || s.generation.load()!=gen) continue; const auto version=s.version.load(); s.pins.fetch_add(1,std::memory_order_acq_rel); if(s.state.load(std::memory_order_acquire)==2 && s.page.load()==page && s.generation.load()==gen && s.version.load()==version) return &s; s.pins.fetch_sub(1,std::memory_order_release); } return nullptr; }
  void release(Slot* s) noexcept { if(s) s->pins.fetch_sub(1,std::memory_order_release); }
  Slot* chooseSlot() { Slot* best=nullptr; for(auto&s:slots_) { if(s.state.load(std::memory_order_acquire)==0) return &s; if(s.state.load(std::memory_order_acquire)==2 && s.pins.load(std::memory_order_acquire)==0 && (!best || s.age.load()<best->age.load())) best=&s; } if(best) ++d_.evictions; return best; }
  std::filesystem::path path_; Key key_{}; std::atomic<std::uint32_t> generation_; std::uint32_t totalPages_{}; std::array<Slot,kSlots> slots_{}; std::array<Request,kRequests> q_{}; Diagnostics d_{}; std::uint64_t clock_{}; Slot* held_{};
};

void expectPage(Runtime& r,std::uint32_t page,std::uint32_t channels) { std::array<float,kPageFrames*kMaxChannels> out{}; if(!r.callback(page*kPageFrames,kPageFrames,out.data())) r.drain(); need(r.callback(page*kPageFrames,kPageFrames,out.data()),"published ready"); for(unsigned f=0;f<kPageFrames;++f) for(unsigned c=0;c<channels;++c) need(out[f*channels+c]==sample(page,f,c),"payload exact"); }
void run() {
  auto root=std::filesystem::temp_directory_path()/"AudioNLE-prepared-residency"; std::filesystem::create_directories(root); Key key{11,7,99,2,48000}; auto artifact=root/"stereo.anleprp"; writeArtifact(artifact,key,24);
  Runtime r(artifact,key); std::array<float,kPageFrames*kMaxChannels> out{};
  need(!r.callback(0,kPageFrames,out.data()) && std::all_of(out.begin(),out.end(),[](float x){return x==0;}),"cold silence");
  need(!r.callback(0,kPageFrames,out.data()),"duplicate cold"); r.drain(); need(r.diagnostics().coalesced>0,"coalesce"); need(r.callback(250,12,out.data()),"stereo seam"); for(unsigned f=0;f<12;++f) for(unsigned c=0;c<2;++c) need(out[f*2+c]==sample((250+f)/kPageFrames,(250+f)%kPageFrames,c),"seam exact");
  for(unsigned p=1;p<22;++p) expectPage(r,p,key.channels); need(r.ready()==kSlots && r.diagnostics().evictions>0,"fixed eviction");
  expectPage(r,0,key.channels); need(r.hold(0),"pin acquire"); for(unsigned p=22;p<24;++p) expectPage(r,p,key.channels); need(r.callback(0,kPageFrames,out.data()),"pinned survives eviction"); r.unhold();
  Runtime stale(artifact,key); need(!stale.callback(5*kPageFrames,kPageFrames,out.data()),"stale request cold"); stale.setGeneration(8); stale.serviceOne(); need(stale.diagnostics().stale>0,"stale reject"); const auto staleRejects=stale.diagnostics().stale.load();
  Runtime shared(artifact,key); need(!shared.callback(3*kPageFrames,kPageFrames,out.data()),"view one cold"); need(!shared.callback(3*kPageFrames,kPageFrames,out.data()),"view two cold"); shared.drain(); need(shared.callback(3*kPageFrames,kPageFrames,out.data()),"shared view");
  Runtime reopened(artifact,key); expectPage(reopened,4,key.channels);
  auto corrupt=root/"corrupt.anleprp"; writeArtifact(corrupt,key,2); { std::fstream f(corrupt,std::ios::binary|std::ios::in|std::ios::out); f.seekp(std::streamoff(offset(1,key.channels)+sizeof(Record))); char x=char(0xff); f.write(&x,1); } Runtime bad(corrupt,key); need(!bad.callback(kPageFrames,kPageFrames,out.data()),"bad cold"); bad.drain(); need(bad.diagnostics().failure>0,"bad not published");
  auto longKey=key; longKey.channels=1; longKey.generation=9; auto longFile=root/"three-hours-sparse.anleprp"; constexpr unsigned longPages=(3u*60u*60u*48000u)/kPageFrames; writeArtifact(longFile,longKey,longPages,true); Runtime longRun(longFile,longKey); for(auto p:{0u,longPages/2,longPages-1}) expectPage(longRun,p,1); need(longRun.ready()<=kSlots,"long bounded");
  std::atomic<bool> done{}; std::thread worker([&]{ while(!done.load()) { r.serviceOne(); } }); for(unsigned i=0;i<2000;++i) { r.callback((i%24)*kPageFrames,16,out.data()); } done.store(true); worker.join();
  std::cout<<"prepared disk residency pages=24 slots="<<kSlots<<" coalesced="<<r.diagnostics().coalesced<<" evictions="<<r.diagnostics().evictions<<" stale-rejects="<<staleRejects<<" PASS\n";
  std::cout<<"prepared long-form hours=3 pages="<<longPages<<" resident="<<longRun.ready()<<" pcm-bytes-per-slot="<<kPageFrames*kMaxChannels*sizeof(float)<<" index-bytes=0 request-bytes="<<sizeof(std::array<Request,kRequests>)<<" PASS\n";
  std::error_code e; std::filesystem::remove_all(root,e);
}
}
#if defined(AUDIONLE_PREPARED_RESIDENCY_ENGINE)
namespace audionle::source_runtime {
class PreparedResidency final {
 public:
  PreparedResidency(std::filesystem::path artifact, PreparedArtifactKey key)
      : runtime_(std::move(artifact), Key{key.source, key.generation, key.config, key.channels, key.projectRate}) {}
  Runtime runtime_;
};

PreparedResidencyPtr makePreparedResidency(std::filesystem::path artifact, PreparedArtifactKey key) {
  return std::make_shared<PreparedResidency>(std::move(artifact), key);
}
bool preparedCallback(PreparedResidency& residency, std::uint32_t frame, std::uint32_t frames, float* output) noexcept {
  return residency.runtime_.callback(frame, frames, output);
}
void servicePrepared(PreparedResidency& residency) { residency.runtime_.serviceOne(); }
void drainPrepared(PreparedResidency& residency) { residency.runtime_.drain(); }
bool preparedMatches(PreparedResidency const& residency, PreparedArtifactKey key) noexcept {
  return residency.runtime_.matches(Key{key.source, key.generation, key.config, key.channels, key.projectRate});
}
std::uint32_t preparedCoalesced(PreparedResidency const& residency) noexcept { return residency.runtime_.diagnostics().coalesced.load(); }
std::uint32_t preparedSubmitted(PreparedResidency const& residency) noexcept { return residency.runtime_.diagnostics().requests.load(); }
std::uint32_t preparedCompleted(PreparedResidency const& residency) noexcept { return residency.runtime_.diagnostics().success.load(); }
std::uint32_t preparedQueueFull(PreparedResidency const& residency) noexcept { return residency.runtime_.diagnostics().unavailable.load(); }
bool preparedHasDemand(PreparedResidency const& residency) noexcept { return residency.runtime_.hasDemand(); }
void writePreparedFixtureArtifact(std::filesystem::path const& path, PreparedArtifactKey key, std::uint32_t pages) {
  writeArtifact(path, Key{key.source, key.generation, key.config, key.channels, key.projectRate}, pages);
}
void writeSparsePreparedFixtureArtifact(std::filesystem::path const& path, PreparedArtifactKey key, std::uint32_t pages) {
  writeArtifact(path, Key{key.source, key.generation, key.config, key.channels, key.projectRate}, pages, true);
}
} // namespace audionle::source_runtime
#endif
#ifndef AUDIONLE_PREPARED_RESIDENCY_LIBRARY
int main(){try{run();return 0;}catch(std::exception const&e){std::cerr<<"prepared disk residency FAIL "<<e.what()<<'\n';return 1;}}
#endif
