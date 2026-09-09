#include <samplerate.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace production_src {
using Sample = std::int64_t;
struct Generation { std::uint64_t value{}; };
enum class Status { ValidatedV1, UnvalidatedConfiguration, UnsupportedConfiguration };
enum class Failure { UnsupportedConfiguration, InvalidRate, CapacityExceeded, StaleGeneration, PreparedInputUnavailable, BackendPreparationFailure, BackendProcessFailure };
struct BackendConfig { const char* backend{"libsamplerate"}; const char* pin{"0.2.2/b9c20b93660c3683fda12e3c2a01f0021bf96c56"}; const char* mode{"SRC_SINC_BEST_QUALITY"}; Sample nativeRate{}, projectRate{}, p{}, q{}; std::uint32_t prerollPolicy{1}, boundaryPolicy{1}; Status status{}; };
struct ViewCoordinates { Sample timelineStart{}, sourceStart{}; Generation generation{}; };
struct PhysicalInputPlan { Sample physicalStart{}, logicalSource{}, discard{}; Sample nativeFrames{}; Generation generation{}; };
struct PreparedNativeInput { const float* data{}; Sample frames{}; Generation generation{}; };
struct FixedOutput { static constexpr Sample capacity = 512; std::array<float, capacity> data{}; Sample frames{}; };
struct Capacity { Sample nativeFrames{2048}, stagedProjectFrames{512}, plannerScratch{0}, runtimeScratch{0}; };
void require(bool b, Failure e) { if (!b) throw e; }
Sample gcd(Sample a, Sample b) { while (b) { Sample t=a%b; a=b;b=t; } return a; }
BackendConfig config(Sample nativeRate, Sample projectRate) { require(nativeRate>0&&projectRate>0,Failure::InvalidRate); Sample g=gcd(nativeRate,projectRate); BackendConfig c; c.nativeRate=nativeRate;c.projectRate=projectRate;c.p=projectRate/g;c.q=nativeRate/g;c.status=((nativeRate==44100&&projectRate==48000)||(nativeRate==48000&&projectRate==44100))?Status::ValidatedV1:Status::UnvalidatedConfiguration;return c; }
class PhysicalSrcRangePlanner { public: PhysicalInputPlan plan(const BackendConfig& c, ViewCoordinates v, Sample requested) const { require(requested>=0,Failure::InvalidRate); Sample logical=v.timelineStart*c.q/c.p; Sample start=std::max<Sample>(0,logical-1024)/c.q*c.q; return {start,logical,v.timelineStart-(start/c.q)*c.p,std::min<Sample>(2048,requested*c.q/c.p+1024+c.q),v.generation}; } };
class NativeSourceService { public: PreparedNativeInput prepare(const std::vector<float>& source,const PhysicalInputPlan& p) const { require(p.physicalStart>=0&&p.physicalStart<Sample(source.size()),Failure::PreparedInputUnavailable); return {source.data()+p.physicalStart,std::min<Sample>(p.nativeFrames,Sample(source.size())-p.physicalStart),p.generation}; } };
class BandlimitedSrcRuntime { public:
 void configure(const BackendConfig& c) { require(c.status!=Status::UnsupportedConfiguration,Failure::UnsupportedConfiguration); config_=c; int error{}; state_.reset(src_new(SRC_SINC_BEST_QUALITY,1,&error)); require(state_&&error==0,Failure::BackendPreparationFailure); configured_=true; }
 Capacity requiredCapacity() const { require(configured_,Failure::BackendPreparationFailure); return {}; }
 void prepare(Generation g) { require(configured_,Failure::BackendPreparationFailure); generation_=g; prepared_=true; }
 void invalidate(Generation g) { generation_=g; prepared_=false; state_.reset(); }
 void reconstruct(const BackendConfig& c, Generation g) { configure(c); prepare(g); }
 void process(const PreparedNativeInput& input, FixedOutput& out) { require(prepared_&&input.generation.value==generation_.value,Failure::StaleGeneration); SRC_DATA d{};d.data_in=input.data;d.input_frames=long(input.frames);d.data_out=out.data.data();d.output_frames=long(out.data.size());d.src_ratio=double(config_.p)/config_.q;require(src_process(state_.get(),&d)==0,Failure::BackendProcessFailure);out.frames=d.output_frames_gen; }
 private: struct Delete { void operator()(SRC_STATE*s)const{src_delete(s);} }; BackendConfig config_{};std::unique_ptr<SRC_STATE,Delete> state_{nullptr};Generation generation_{};bool configured_{},prepared_{}; };
}
namespace { using namespace production_src; void ok(bool b){if(!b)throw std::runtime_error("check");} std::vector<float> signal(){std::vector<float>x(8000);for(size_t i=0;i<x.size();++i)x[i]=float((i%127)/127.0-.5);return x;}
void matrix(Sample n,Sample r){auto c=config(n,r);PhysicalSrcRangePlanner planner;for(Sample phase=0;phase<c.p;++phase){ViewCoordinates v{3600+phase,0,{7}};auto p=planner.plan(c,v,512);ok(p.physicalStart>=0&&p.physicalStart%c.q==0&&p.discard==v.timelineStart-(p.physicalStart/c.q)*c.p&&p.nativeFrames<=2048);}for(Sample hours:{1,3,6,12}){ViewCoordinates v{hours*r*3600,0,{9}};auto p=planner.plan(c,v,512);ok(p.logicalSource==v.timelineStart*c.q/c.p&&p.physicalStart>=0);}auto p=planner.plan(c,{4096,0,{1}},512);NativeSourceService service;auto source=signal();auto in=service.prepare(source,p);BandlimitedSrcRuntime runtime;runtime.configure(c);runtime.prepare({1});FixedOutput out;runtime.process(in,out);ok(out.frames>=0&&out.frames<=FixedOutput::capacity);runtime.invalidate({2});try{runtime.process(in,out);throw std::runtime_error("stale accepted");}catch(Failure f){ok(f==Failure::StaleGeneration);}runtime.reconstruct(c,{2});auto p2=planner.plan(c,{4096,0,{2}},512);ok(p.physicalStart==p2.physicalStart&&p.discard==p2.discard);std::cout<<"matrix "<<n<<"->"<<r<<" status="<<(c.status==Status::ValidatedV1?"validated":"unvalidated")<<" phases="<<c.p<<" capacity=2048/512 PASS\n";}
}
int main(){try{matrix(44100,48000);matrix(48000,44100);matrix(44100,96000);matrix(96000,44100);matrix(48000,96000);matrix(96000,48000);std::cout<<"production-src-skeleton PASS callback_lifecycle=separated\n";return 0;}catch(...){std::cerr<<"production-src-skeleton FAIL\n";return 1;}}
