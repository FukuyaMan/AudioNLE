#include <samplerate.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
using I = std::int64_t;
constexpr I kOutput = 512, kNativeCapacity = 2048, kSearchMaximum = 8192;
constexpr double kTolerance = 2e-5;
struct Delete { void operator()(SRC_STATE* s) const { src_delete(s); } };
using State = std::unique_ptr<SRC_STATE, Delete>;
struct Render { std::vector<float> samples; };
void require(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
std::vector<float> signal() { std::vector<float> x(48000); std::uint32_t z=0x51a7u; for (size_t i=0;i<x.size();++i) { z=1664525u*z+1013904223u; x[i]=float((int(z>>16)-32768)/131072.0); } return x; }
Render render(int converter, std::vector<float> const& input, double ratio) {
 int error{}; State state(src_new(converter, 1, &error)); require(state && error==0,"src_new"); Render r; size_t offset{};
 while(offset<input.size()) { std::array<float,4096> out{}; SRC_DATA d{}; d.data_in=input.data()+offset; d.input_frames=long(input.size()-offset); d.data_out=out.data(); d.output_frames=long(out.size()); d.src_ratio=ratio; d.end_of_input=1; require(src_process(state.get(),&d)==0,"src_process"); require(d.input_frames_used || d.output_frames_gen,"src_process progress"); offset+=size_t(d.input_frames_used); r.samples.insert(r.samples.end(),out.begin(),out.begin()+d.output_frames_gen); }
 return r;
}
struct Difference { bool pass{}; I first{-1}; double expected{}, actual{}, maximum{}, rms{}; };
Difference compare(Render const& reference, I referenceAt, Render const& reconstructed, I reconstructedAt) {
 constexpr I count=128; Difference d; if(referenceAt<0||reconstructedAt<0||size_t(referenceAt+count)>reference.samples.size()||size_t(reconstructedAt+count)>reconstructed.samples.size()) return d;
 double sum{}; d.pass=true; for(I i=0;i<count;++i){ double a=reference.samples[size_t(referenceAt+i)],b=reconstructed.samples[size_t(reconstructedAt+i)],e=std::abs(a-b); sum+=(a-b)*(a-b); if(e>d.maximum){d.maximum=e;if(d.first<0){d.first=i;d.expected=a;d.actual=b;}} } d.rms=std::sqrt(sum/count); d.pass=d.maximum<=kTolerance&&d.rms<=kTolerance; return d;
}
I physicalStart(I timeline, I p, I q, I history) { const I logical=timeline*q/p; return std::max<I>(0,logical-history); }
I outputDiscard(I timeline, I p, I q, I physical) { return timeline-(physical*p/q); }
Difference trial(int converter, int p, int q, I timeline, I history, Render const& reference, std::vector<float> const& source) { I physical=physicalStart(timeline,p,q,history); std::vector<float> tail(source.begin()+physical,source.end()); auto reconstructed=render(converter,tail,double(p)/q); return compare(reference,timeline,reconstructed,outputDiscard(timeline,p,q,physical)); }
struct Policy { I history{-1}; Difference diagnostic{}; };
Policy findPolicy(int converter, int p, int q, Render const& reference, std::vector<float> const& source) {
 constexpr I t=12000; Policy result; for(I h=0;h<=kSearchMaximum;h+=64){auto d=trial(converter,p,q,t,h,reference,source);if(d.pass){for(I exact=std::max<I>(0,h-63);exact<=h;++exact){auto e=trial(converter,p,q,t,exact,reference,source);if(e.pass){result={exact,e};return result;}}} result.diagnostic=d;} return result;
}
bool validatePhases(int converter,int p,int q,Policy policy,Render const& reference,std::vector<float> const& source) {
 for(I phase=0;phase<p;++phase) for(I t : {I(4096)+phase,I(12000)+phase}) { if(!trial(converter,p,q,t,policy.history,reference,source).pass) return false; }
 for(I t : {I(0),I(1),I(160),I(161),I(4096),I(4097),I(5003),I(12000),I(4096),I(4256)}) if(!trial(converter,p,q,t,policy.history,reference,source).pass) return false;
 return true;
}
void mode(char const* name,int converter) { auto source=signal(); for(auto const& rate:std::array<std::array<int,2>,6>{{{{160,147}},{{147,160}},{{320,147}},{{147,320}},{{2,1}},{{1,2}}}}){int p=rate[0],q=rate[1];auto reference=render(converter,source,double(p)/q);auto v1=trial(converter,p,q,4096,1024,reference,source);auto policy=findPolicy(converter,p,q,reference,source);const auto physical=policy.history<0?-1:physicalStart(4096,p,q,policy.history);const auto discard=policy.history<0?-1:outputDiscard(4096,p,q,physical);const I capacity=policy.history<0?-1:policy.history+(kOutput*q+p-1)/p+q;bool phases=policy.history>=0&&validatePhases(converter,p,q,policy,reference,source); std::cout<<"mode-history mode="<<name<<" P/Q="<<p<<'/'<<q<<" v1-first="<<v1.first<<" v1-expected="<<v1.expected<<" v1-actual="<<v1.actual<<" v1-error="<<v1.maximum<<" history="<<policy.history<<" physical="<<physical<<" discard="<<discard<<" capacity="<<capacity<<" phases="<<(phases?"PASS":"FAIL")<<" status="<<((phases&&capacity<=kNativeCapacity)?"VALIDATED":"CAPACITY_OR_TIMING_FAIL")<<'\n'; }
}
}
int main(){try{mode("MEDIUM",SRC_SINC_MEDIUM_QUALITY);mode("FASTEST",SRC_SINC_FASTEST);return 0;}catch(std::exception const&e){std::cerr<<"mode-history FAIL "<<e.what()<<'\n';return 1;}}
