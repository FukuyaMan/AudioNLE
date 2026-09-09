#include <samplerate.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace {
using I = std::int64_t;
constexpr I kPageFrames=256, kPages=16, kOutput=512;
enum class State : unsigned { NotRequested, Queued, Preparing, Ready, Failed, Stale, Unavailable };
struct Key { std::uint64_t source{}, generation{}, config{}; int sourceRate{}, projectRate{}, p{}, q{}, channels{1}; bool operator==(Key const&) const = default; };
struct Page { std::array<float,kPageFrames> data{}; std::atomic<State> state{State::NotRequested}; std::atomic<I> start{-1}; Key key{}; };
struct Metrics { std::atomic<unsigned> hits{}, misses{}, unavailable{}, stale{}, unexpectedRealtimeSrc{}; };
void require(bool value, char const* what="check") { if(!value) throw std::runtime_error(what); }
class Store {
 public:
  // Worker-only: build privately, then release-publish the complete immutable page.
  void publish(Key const& key, I start, float const* src, I count) {
    require(start%kPageFrames==0&&count==kPageFrames,"page shape"); auto& page=pages_[size_t((start/kPageFrames)%kPages)];
    page.state.store(State::Preparing,std::memory_order_relaxed); page.start.store(-1,std::memory_order_relaxed); page.key=key;
    std::copy_n(src,kPageFrames,page.data.begin()); page.start.store(start,std::memory_order_relaxed); page.state.store(State::Ready,std::memory_order_release);
  }
  void invalidate(std::uint64_t generation) { currentGeneration_.store(generation,std::memory_order_release); }
  void fail(Key const& key, I start) { auto& page=pages_[size_t((start/kPageFrames)%kPages)]; page.key=key; page.start.store(start,std::memory_order_relaxed); page.state.store(State::Failed,std::memory_order_release); }
  State read(Key const& key,I start,I count,std::array<float,kOutput>& out) {
    if(count<0||count>kOutput){++metrics_.unavailable;return State::Unavailable;} const I end=start+count;
    // Verify complete coverage before copying: ranges spanning an unpublished page return silence, never partial audio.
    for(I p=start/kPageFrames*kPageFrames;p<end;p+=kPageFrames){auto const& page=pages_[size_t((p/kPageFrames)%kPages)];const auto state=page.state.load(std::memory_order_acquire);if(state==State::Failed){++metrics_.unavailable;return State::Failed;}if(state!=State::Ready||page.start.load(std::memory_order_relaxed)!=p){++metrics_.unavailable;return State::Unavailable;}if(!(page.key==key)||key.generation!=currentGeneration_.load(std::memory_order_acquire)){++metrics_.stale;return State::Stale;}}
    for(I i=0;i<count;++i){auto const& page=pages_[size_t(((start+i)/kPageFrames)%kPages)];out[size_t(i)]=page.data[size_t((start+i)%kPageFrames)];}++metrics_.hits;return State::Ready;
  }
  Metrics& metrics(){return metrics_;}
 private: std::array<Page,kPages> pages_{}; std::atomic<std::uint64_t> currentGeneration_{1}; Metrics metrics_{};
};
std::vector<float> source(size_t n){std::vector<float>x(n);for(size_t i=0;i<n;++i)x[i]=float(.3*std::sin(i*.017)+.2*std::sin(i*.071));return x;}
std::vector<float> reference(std::vector<float> const& in,double ratio){int error{};SRC_STATE*s=src_new(SRC_SINC_BEST_QUALITY,1,&error);require(s&&error==0,"src_new");std::vector<float>out;size_t offset{};bool flushing=false;while(!flushing){std::array<float,1024> b{};SRC_DATA d{};d.data_in=offset<in.size()?in.data()+offset:nullptr;d.input_frames=offset<in.size()?long(std::min<size_t>(1024,in.size()-offset)):0;d.data_out=b.data();d.output_frames=long(b.size());d.src_ratio=ratio;d.end_of_input=offset+size_t(d.input_frames)>=in.size();require(src_process(s,&d)==0,"src_process");offset+=size_t(d.input_frames_used);out.insert(out.end(),b.begin(),b.begin()+d.output_frames_gen);flushing=offset==in.size()&&d.output_frames_gen==0;require(d.input_frames_used||d.output_frames_gen||flushing,"src_process progress");}src_delete(s);return out;}
void fixture(int sourceRate,int projectRate){auto native=source(12000);auto direct=reference(native,double(projectRate)/sourceRate);Store store;Key key{17,1,0xB9C20B,sourceRate,projectRate,projectRate,std::gcd(sourceRate,projectRate),1};store.invalidate(1);const I pageCount=std::min<I>(kPages,I(direct.size())/kPageFrames);for(I p=0;p<pageCount;++p)store.publish(key,p*kPageFrames,direct.data()+p*kPageFrames,kPageFrames);
 std::array<float,kOutput> out{};require(store.read(key,128,512,out)==State::Ready,"spanning read");double max=0,rms=0;for(I i=0;i<512;++i){double d=out[size_t(i)]-direct[size_t(128+i)];max=std::max(max,std::abs(d));rms+=d*d;}require(max<=2e-5&&std::sqrt(rms/512)<=2e-5,"reference seam");
 // Trim/re-expand/split and two views are all logical offsets over the same shared immutable pages.
 require(store.read(key,256,128,out)==State::Ready&&store.read(key,256,128,out)==State::Ready,"repeat/seek");require(store.read(key,3840,512,out)==State::Unavailable,"partial availability");store.fail(key,3840);require(store.read(key,3840,128,out)==State::Failed,"worker failure");
 Key stale=key;stale.generation=2;store.invalidate(2);require(store.read(key,128,128,out)==State::Stale,"generation reject");store.publish(stale,0,direct.data(),kPageFrames);require(store.read(stale,0,128,out)==State::Ready,"retry current generation");Key rateMismatch=stale;rateMismatch.projectRate+=1;require(store.read(rateMismatch,0,128,out)==State::Stale,"project-rate key reject");
 std::cout<<"prepared cache source="<<sourceRate<<" project="<<projectRate<<" pages="<<pageCount<<" seam-max="<<max<<" hits="<<store.metrics().hits<<" unavailable="<<store.metrics().unavailable<<" stale="<<store.metrics().stale<<" src-callback=0 PASS\n";
}
void longForm(){Store store;Key key{99,1,0xB9C20B,44100,48000,160,147,1};store.invalidate(1);std::array<float,kPageFrames> page{};const I threeHours=I(48000)*3600*3;for(I p:{I(0),threeHours/kPageFrames*kPageFrames})store.publish(key,p,page.data(),kPageFrames);std::array<float,kOutput>out{};require(store.read(key,0,128,out)==State::Ready,"long start");std::cout<<"prepared longform duration-project="<<threeHours<<" resident-pages="<<kPages<<" page-bytes="<<kPageFrames*sizeof(float)<<" working-native=1024 working-output=1024 PASS\n";}
}
int main(){try{fixture(44100,48000);fixture(44100,96000);fixture(96000,44100);longForm();return 0;}catch(std::exception const&e){std::cerr<<"prepared cache FAIL "<<e.what()<<'\n';return 1;}}
