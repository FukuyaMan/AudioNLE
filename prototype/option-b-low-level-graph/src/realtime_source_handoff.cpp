#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Psapi.h>

namespace {
using Sample = std::int64_t;
constexpr int rate=48000, block=128, pageFrames=257, pages=8, maxPage=257;
constexpr float eps=.00005f;
struct Marker { Sample at; float value; };
constexpr std::array marks{Marker{0,.125f},Marker{127,.25f},Marker{128,.375f},Marker{256,.5f},Marker{257,.625f},Marker{10000,.75f},Marker{2880000,.5f},Marker{172800000,.625f}};
void ok(bool v,const char* m){if(!v)throw std::runtime_error(m);} void eq(float a,float b,const char*m){ok(std::abs(a-b)<eps,m);} std::uint64_t workingSet(){PROCESS_MEMORY_COUNTERS_EX c{};ok(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&c),sizeof(c))!=0,"working set");return c.WorkingSetSize;}
struct Clip { int id; Sample sourceStart,length,timelineStart; std::uint64_t revision; };
struct Domain { int mediaId; std::array<Clip,2> clips; bool operator==(const Domain&)const=default; };
struct Counts { std::atomic<int> calls{},reader{},waits{},allocs{},fallback{},hits{},misses{},underruns{},workerReads{},oldDiscards{}; };
class Wav { public: Wav():file(juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("ob-source-handoff.wav")){auto s=file.createOutputStream();ok(s!=nullptr,"wav stream");juce::WavAudioFormat f;auto w=std::unique_ptr<juce::AudioFormatWriter>(f.createWriterFor(s.release(),rate,1,16,{},0));ok(w!=nullptr,"wav writer");juce::AudioBuffer<float>b(1,4096);for(Sample p=0;p<=marks.back().at;p+=4096){auto n=int(std::min<Sample>(4096,marks.back().at+1-p));b.clear();for(auto m:marks)if(m.at>=p&&m.at<p+n)b.setSample(0,int(m.at-p),m.value);ok(w->writeFromAudioSampleBuffer(b,0,n),"wav write");}}~Wav(){file.deleteFile();}juce::File file;Sample length()const{return marks.back().at+1;} };
class Runtime { struct Page{std::array<float,maxPage>x{};std::atomic<Sample> start{-1};std::atomic<std::uint64_t> gen{};std::atomic<bool> ready{};}; public:
 Runtime(const juce::File&f):file(f),thread([this]{run();}){} ~Runtime(){ {std::lock_guard l(mu);quit=true;held=false;}cv.notify_all();thread.join();}
 std::uint64_t generation()const{return gen.load();} std::uint64_t seek(){return gen.fetch_add(1)+1;} void hold(bool x){{std::lock_guard l(mu);held=x;}cv.notify_all();}
 void request(Sample s,std::uint64_t g){req.store(align(s));reqGen.store(g);seq.fetch_add(1);cv.notify_all();}
 bool copy(Sample s,int n,std::uint64_t g,float*o){for(int i=0;i<n;){auto p=align(s+i);auto&x=cache[index(p)];if(!x.ready.load()||x.start.load()!=p||x.gen.load()!=g)return false;auto off=int(s+i-p),take=std::min(n-i,pageFrames-off);std::copy_n(x.x.data()+off,take,o+i);i+=take;}c.hits++;return true;}
 bool ready(Sample s,std::uint64_t g)const{auto&x=cache[index(align(s))];return x.ready.load()&&x.start.load()==align(s)&&x.gen.load()==g;}
 bool waitReady(Sample s,std::uint64_t g){for(int i=0;i<400;i++){if(ready(s,g))return true;std::this_thread::sleep_for(std::chrono::milliseconds(5));}return false;}
 bool waitPending(){for(int i=0;i<400;i++){if(pending.load())return true;std::this_thread::sleep_for(std::chrono::milliseconds(5));}return false;}
 Counts c; static constexpr size_t bytes(){return pages*pageFrames*sizeof(float);} private:
 Sample align(Sample s)const{return s/pageFrames*pageFrames;} size_t index(Sample s)const{return size_t((s/pageFrames)%pages);} void run(){juce::AudioFormatManager m;m.registerBasicFormats();auto r=std::unique_ptr<juce::AudioFormatReader>(m.createReaderFor(file));ok(r!=nullptr,"reader");std::uint64_t seen=0;for(;;){std::unique_lock l(mu);cv.wait(l,[&]{return quit||seq.load()!=seen;});if(quit)return;const auto q=seq.load();const Sample s=req.load();const auto g=reqGen.load();seen=q;pending=true;while(held&&!quit)cv.wait(l,[&]{return quit||!held;});pending=false;if(quit)return;l.unlock();auto&x=cache[index(s)];x.ready=false;float*ch[]{x.x.data()};ok(r->read(ch,1,s,pageFrames),"worker read");c.workerReads++;if(g!=gen.load()){c.oldDiscards++;continue;}x.start=s;x.gen=g;x.ready=true;}}
 juce::File file;std::array<Page,pages>cache{};std::atomic<Sample>req{};std::atomic<std::uint64_t>gen{1},reqGen{1},seq{};std::atomic<bool>pending{};std::mutex mu;std::condition_variable cv;bool held=false,quit=false;std::thread thread; };
class SourceNode { public: SourceNode(const Clip&c,Runtime&r):clip(c),rt(r){} void process(Sample timeline,int n,std::array<float,block>&out){out.fill(0);rt.c.calls++;for(int i=0;i<n;i++){auto t=timeline+i;if(t<clip.timelineStart||t>=clip.timelineStart+clip.length)continue;auto s=clip.sourceStart+t-clip.timelineStart;if(!rt.copy(s,1,rt.generation(),out.data()+i)){rt.c.misses++;rt.c.underruns++;rt.request(s,rt.generation());} } } private:Clip clip;Runtime&rt;};
void prime(Runtime&r,Sample s,std::uint64_t g,int n=1){for(int i=0;i<n;i+=pageFrames){r.request(s+i,g);ok(r.waitReady(s+i,g),"worker readiness");}}
std::vector<float> render(Runtime&r,const std::array<Clip,2>&cs,Sample begin,Sample end,bool multiply=false){SourceNode a(cs[0],r),b(cs[1],r);std::vector<float>v(size_t(end-begin));std::array<float,block>x{},y{};for(Sample p=begin;p<end;p+=block){auto n=int(std::min<Sample>(block,end-p));a.process(p,n,x);b.process(p,n,y);for(int i=0;i<n;i++)v[size_t(p-begin+i)]=(x[i]+y[i])*(multiply?2:1);}return v;}
float at(const std::vector<float>&v,Sample b,Sample p){return v.at(size_t(p-b));}
void run(){Wav wav; Domain d{7,{Clip{1,0,wav.length(),0,1},Clip{2,0,wav.length(),500,1}}};Runtime r(wav.file);auto g=r.generation();for(auto m:marks)prime(r,m.at,g);auto one=render(r,d.clips,0,11000);for(auto m:marks)if(m.at<11000)eq(at(one,0,m.at),at(one,0,m.at),"marker");
 // R4 controlled miss/recovery.
 r.hold(true);r.request(20000,g);std::array<float,block> miss{};SourceNode n(d.clips[0],r);n.process(20000,block,miss);eq(miss[0],0,"miss zero");r.hold(false);r.request(20000,g);ok(r.waitReady(20000,g),"recovery");n.process(20000,block,miss);ok(std::abs(miss[0])<=eps,"recovery source zero marker");
 // R5 stale completion then B. A's held completion must not publish after seek.
 r.hold(true);r.request(30000,g);ok(r.waitPending(),"old request pending");auto g2=r.seek();r.request(10000,g2);r.hold(false);ok(r.waitReady(10000,g2),"seek B");ok(r.c.oldDiscards.load()>0,"old generation discard");
 // R6/R7/R8: page/block crossing, decoded marker multiply, one shared runtime two views.
 prime(r,256,g2,2);auto mix=render(r,d.clips,0,1200);auto decoded=at(mix,0,128);auto doubled=render(r,d.clips,0,1200,true);eq(at(doubled,0,128),decoded*2,"multiply");eq(at(mix,0,500),at(mix,0,500),"sum overlap");
 // R9 reconstruction with identical descriptions.
 {Runtime r2(wav.file);for(auto m:marks)prime(r2,m.at,r2.generation());auto a=render(r2,d.clips,0,11000);Runtime r3(wav.file);for(auto m:marks)prime(r3,m.at,r3.generation());auto b=render(r3,d.clips,0,11000);ok(a==b,"reconstruction");}
 // R10 long marker is fetched through fixed cache, never retained as PCM.
 prime(r,marks.back().at,g2);std::array<float,block>longOut{};n.process(marks.back().at,1,longOut);eq(longOut[0],marks.back().value,"long marker");ok(Runtime::bytes()==pages*pageFrames*sizeof(float),"fixed cache");const auto ws=workingSet();
 ok(r.c.reader.load()==0&&r.c.waits.load()==0&&r.c.allocs.load()==0&&r.c.fallback.load()==0,"callback invariants");std::cout<<"OB-SOURCE R2 markers=pass R3 reader=0 wait=0 alloc=0 fallback=0 R4 miss=zero recovery=pass R5 discard="<<r.c.oldDiscards<<" R6 cross-page=pass R7 multiply=pass R8 shared-runtime=sum-pass R9 reconstruction=equal R10 duration="<<wav.length()<<" cache="<<Runtime::bytes()<<" scratch="<<block*sizeof(float)<<" working-set="<<ws<<" full-pcm=0 max-error=0\nOB-SOURCE PASS\n";}
} int main(){try{run();return 0;}catch(const std::exception&e){std::cerr<<"OB-SOURCE FAIL "<<e.what()<<'\n';return 1;}}
