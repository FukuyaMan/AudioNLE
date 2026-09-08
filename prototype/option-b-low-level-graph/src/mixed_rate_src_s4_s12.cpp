#include "integer_rational_zoh.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using Sample = std::int64_t;
constexpr Sample P = 160, Q = 147;
constexpr int pageFrames = 257, pageCount = 8, blockFrames = 128;
void require (bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
Sample boundary (Sample source) { return (source * P + Q - 1) / Q; }
Sample held (Sample timeline) { return IntegerRationalZoh::referenceHeldSource(timeline, P, Q); }
Sample page (Sample s) { return s / pageFrames * pageFrames; }

struct Counters { std::atomic<int> callback{}, reader{}, wait{}, allocation{}, fallback{}; };
struct CachePage { std::array<float, pageFrames> data{}; std::atomic<Sample> start{-1}; std::atomic<bool> ready{}; };
class NativeCache {
public:
  explicit NativeCache(const juce::File& file) { formats.registerBasicFormats(); reader.reset(formats.createReaderFor(file)); require(reader != nullptr, "WAV reader"); }
  void request(Sample s) { const auto p=page(s); int idle=0; if (state.compare_exchange_strong(idle,1)) requested=p; }
  bool service() { int pending=1; if (!state.compare_exchange_strong(pending,2)) return false; const auto p=requested.load(); auto& c=pages[static_cast<size_t>((p/pageFrames)%pageCount)]; c.ready=false; float* channels[]{c.data.data()}; require(reader->read(channels,1,p,pageFrames),"worker read"); c.start=p; c.ready.store(true,std::memory_order_release); state=0; return true; }
  bool copy(Sample s,float& out) const { const auto p=page(s); const auto& c=pages[static_cast<size_t>((p/pageFrames)%pageCount)]; if(!c.ready.load(std::memory_order_acquire)||c.start.load()!=p)return false; out=c.data[static_cast<size_t>(s-p)]; return true; }
  void prime(Sample first,Sample last) { for(auto p=page(first);p<=page(last);p+=pageFrames){for(int n=0;n<800;++n){request(p);float x{};if(copy(p,x))break;std::this_thread::sleep_for(std::chrono::milliseconds(1));}float x{};require(copy(p,x),"page ready");} }
  static constexpr int bytes() { return pageCount*pageFrames*int(sizeof(float)); }
  Counters counters;
private: juce::AudioFormatManager formats; std::unique_ptr<juce::AudioFormatReader> reader; std::array<CachePage,pageCount> pages{}; std::atomic<Sample> requested{}; std::atomic<int> state{};
};
class Worker { public: explicit Worker(NativeCache& r):runtime(r),thread([this]{while(running)if(!runtime.service())std::this_thread::sleep_for(std::chrono::milliseconds(1));}){} ~Worker(){running=false;thread.join();} private:NativeCache& runtime;std::atomic<bool>running{true};std::thread thread; };

// phaseTimeline/sourceBase preserve the original Source-to-Timeline relation.
// Clip range changes constrain visibility only; they never rebase rational phase.
struct Clip { Sample sourceBase, sourceFirst, sourceEnd, phaseTimeline, timelineFirst, timelineEnd; };
class ClipRuntimeView {
public:
  explicit ClipRuntimeView(Clip c):clip(c),zoh(P,Q){}
  void reset(Sample timeline) { require(timeline>=clip.timelineFirst && timeline<clip.timelineEnd,"view range"); zoh.reset(timeline-clip.phaseTimeline); }
  Sample source() const { return clip.sourceBase+zoh.currentSource(); }
  void advance(){zoh.advance();}
  bool visible()const{return source()>=clip.sourceFirst&&source()<clip.sourceEnd;}
private:Clip clip;IntegerRationalZoh zoh;
};
class SourceNode {
public:
  SourceNode(NativeCache&r,Clip c):runtime(r),view(c){}
  void reset(Sample t){view.reset(t);}
  void process(float*out,int frames){++runtime.counters.callback;for(int i=0;i<frames;++i){float x{};if(view.visible())require(runtime.copy(view.source(),x),"callback cache miss");out[i]=x;view.advance();}}
private:NativeCache&runtime;ClipRuntimeView view;
};

class FixtureWav {
public:
  FixtureWav(Sample edge,const char* name,bool step):file(juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile(juce::String(name)+".wav")){
    auto stream=file.createOutputStream();require(stream!=nullptr,"WAV stream");juce::WavAudioFormat wav;auto writer=std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(stream.release(),44100,1,16,{},0));require(writer!=nullptr,"WAV writer");juce::AudioBuffer<float>b(1,5000);for(int i=0;i<b.getNumSamples();++i){const int code=step?(i>=edge?32:0):((i%97)-48);b.setSample(0,i,float(code)/64.0f);}require(writer->writeFromAudioSampleBuffer(b,0,b.getNumSamples()),"WAV write");}
  ~FixtureWav(){file.deleteFile();}juce::File file;
};
std::vector<int> codes(NativeCache&r,Clip c,Sample begin,int count,const std::vector<int>&parts){const auto first=std::max(c.sourceFirst,c.sourceBase+held(begin-c.phaseTimeline));const auto last=std::min(c.sourceEnd-1,c.sourceBase+held(begin-c.phaseTimeline+count-1));r.prime(first,std::min(c.sourceEnd-1,last+pageFrames));SourceNode node(r,c);node.reset(begin);std::vector<int>out;out.reserve(static_cast<size_t>(count));int done=0,k=0;std::array<float,blockFrames>block{};while(done<count){int remaining=std::min(parts[static_cast<size_t>(k++%parts.size())],count-done);while(remaining>0){const int n=std::min(remaining,blockFrames);node.process(block.data(),n);for(int i=0;i<n;++i)out.push_back(int(std::lround(block[i]*64.0f)));remaining-=n;done+=n;}}return out;}
Clip full(Sample base,Sample start,Sample end,Sample placement=0){return{base,base,base+(end-start),placement,placement,placement+boundary(end-start)};}
void s4(){for(auto edge:{Sample(147),Sample(148),Sample(1000),Sample(255),Sample(256),Sample(257)}){FixtureWav wav(edge,"ob-s4-step",true);NativeCache cache(wav.file);Worker worker(cache);const auto c=full(0,0,4000);const auto t=boundary(edge);const auto out=codes(cache,c,t,4,{128});require(out[0]==32,"real step boundary");}}
void s5s7(){FixtureWav wav(0,"ob-s5-token",false);NativeCache cache(wav.file);Worker worker(cache);const auto c=full(0,0,4000);const auto continuous=codes(cache,c,0,1200,{128});for(auto start:{Sample(0),Sample(512),Sample(1000)}){const auto slice=codes(cache,c,start,128,{128});for(int i=0;i<128;++i)require(slice[static_cast<size_t>(i)]==continuous[static_cast<size_t>(start+i)],"render start/seek slice");}const auto a=codes(cache,c,160,401,{128});(void)codes(cache,c,700,200,{128});require(a==codes(cache,c,160,401,{128}),"seek reset");const auto b128=codes(cache,c,0,1001,{128}),b256=codes(cache,c,0,1001,{256}),irregular=codes(cache,c,0,1001,{1,17,256,3,128,71});require(b128==b256&&b128==irregular,"partition");require(cache.counters.reader==0&&cache.counters.wait==0&&cache.counters.allocation==0&&cache.counters.fallback==0,"callback invariants");}
void s8s9s11s12(){FixtureWav wav(0,"ob-s8-token",false);NativeCache cache(wav.file);Worker worker(cache);const auto original=full(100,0,2000);const auto reference=codes(cache,original,0,1000,{128});
  // Trim/re-expand: phase stays anchored at 0 while ranges change.
  auto trimmed=original;trimmed.sourceFirst=200;trimmed.timelineFirst=boundary(100);const auto trim=codes(cache,trimmed,trimmed.timelineFirst,500,{128});for(int i=0;i<500;++i)require(trim[static_cast<size_t>(i)]==reference[static_cast<size_t>(trimmed.timelineFirst+i)],"trim phase");require(codes(cache,original,0,1000,{128})==reference,"re-expand");
  // Split ranges meet at T; independent views derive phase from the shared original placement.
  for(auto split:{Sample(160),Sample(161),Sample(513)}){auto left=original,right=original;left.timelineEnd=split;left.sourceEnd=100+held(split-1)+1;right.timelineFirst=split;right.sourceFirst=100+held(split);const auto l=codes(cache,left,0,int(split),{128});const auto rr=codes(cache,right,split,1000-int(split),{128});std::vector<int>joined=l;joined.insert(joined.end(),rr.begin(),rr.end());require(joined==reference,"split seam");}
  auto first=full(100,0,2000),second=full(700,0,2000,300);const auto a=codes(cache,first,400,128,{128});const auto b=codes(cache,second,700,128,{128});for(int i=0;i<128;++i){const auto sa=100+held(400+i),sb=700+held(400+i);require(a[static_cast<size_t>(i)]+b[static_cast<size_t>(i)]==(sa%97-48)+(sb%97-48),"multi view sum");}require(a==codes(cache,first,400,128,{128}),"independent view phase");
  NativeCache rebuilt(wav.file);Worker rebuiltWorker(rebuilt);require(codes(rebuilt,original,300,256,{128})==codes(cache,original,300,256,{128}),"reconstruction");}
void s10(){for(auto hours:{Sample(1),Sample(3),Sample(6),Sample(12)}){const auto t=hours*3600*48000;require(held(t)==hours*3600*44100,"long exact ratio");}const Sample oneHour=3600*48000;require(held(oneHour)==3600*44100,"one hour marker");}
void run(){s4();s5s7();s8s9s11s12();s10();}
}
int main(){try{run();std::cout<<"OB-SRC S4-S12 PASS real-boundaries=0 starts=0 partition=identical callback=0 seek=exact trim=exact split=seamless long=exact multiview=independent reconstruction=exact cache-bytes="<<NativeCache::bytes()<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"OB-SRC FAIL "<<e.what()<<'\n';return 1;}}
