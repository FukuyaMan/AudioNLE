#include <speex_resampler.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
constexpr int quality = 10;
constexpr int nativeCapacity = 2048, outputCapacity = 512;
bool qualityFailed{};
using State = std::unique_ptr<SpeexResamplerState, decltype(&speex_resampler_destroy)>;
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Runtime {
    State state{nullptr, speex_resampler_destroy};
    std::array<float, nativeCapacity> input{};
    std::array<float, outputCapacity> output{};
    Runtime(int source, int project) {
        int error{};
        state.reset(speex_resampler_init_frac(1, unsigned(source), unsigned(project), unsigned(source), unsigned(project), quality, &error));
        check(state && error == RESAMPLER_ERR_SUCCESS, "speex init");
    }
    std::pair<unsigned, unsigned> process(unsigned available, unsigned requested) {
        check(available <= input.size() && requested <= output.size(), "capacity");
        auto in = available, out = requested;
        check(speex_resampler_process_float(state.get(), 0, input.data(), &in, output.data(), &out) == RESAMPLER_ERR_SUCCESS, "speex process");
        return {in, out};
    }
};
std::vector<float> source() { std::vector<float> x(24000); for (size_t i=0;i<x.size();++i) x[i]=float(.35*std::sin(i*.013)+.2*std::sin(i*.071)); return x; }
std::vector<float> render(int sourceRate, int projectRate, const std::vector<float>& input, int chunk) {
    Runtime runtime(sourceRate, projectRate); std::vector<float> result; size_t offset{};
    const unsigned nativeRequest = unsigned(std::ceil(double(outputCapacity) * sourceRate / projectRate) + 4);
    check(nativeRequest <= runtime.input.size(), "native request capacity");
    while (offset < input.size()) { const unsigned offered = unsigned(std::min<size_t>(nativeRequest, input.size()-offset)); std::copy_n(input.data()+offset, offered, runtime.input.data()); const auto [used, generated] = runtime.process(offered, outputCapacity); check(used > 0 || generated > 0, "speex progress"); result.insert(result.end(), runtime.output.begin(), runtime.output.begin()+generated); offset += used; }
    return result;
}
double amplitude(const std::vector<float>& samples, double hz, double rate) { const size_t begin=2000, count=std::min<size_t>(6000, samples.size()-2000); double s{},c{}; for(size_t i=0;i<count;++i){double a=2*3.141592653589793*hz*(begin+i)/rate;s+=samples[begin+i]*std::sin(a);c+=samples[begin+i]*std::cos(a);} return 2*std::sqrt(s*s+c*c)/count; }
std::vector<float> tone(double hz, double rate) { std::vector<float> x(24000); for(size_t i=0;i<x.size();++i)x[i]=float(.5*std::sin(2*3.141592653589793*hz*i/rate)); return x; }
void timing(int sourceRate,int projectRate) { const auto input=source(); const auto reference=render(sourceRate,projectRate,input,2048); const auto partitioned=render(sourceRate,projectRate,input,257); check(reference==render(sourceRate,projectRate,input,2048),"determinism"); check(!partitioned.empty() && std::all_of(reference.begin(),reference.end(),[](float x){return std::isfinite(x);}),"finite"); Runtime reset(sourceRate,projectRate); check(speex_resampler_get_input_latency(reset.state.get()) >= 0 && speex_resampler_get_output_latency(reset.state.get()) >= 0,"latency query"); speex_resampler_reset_mem(reset.state.get()); std::cout<<"speex timing source="<<sourceRate<<" project="<<projectRate<<" P/Q="<<projectRate<<'/'<<sourceRate<<" fresh=deterministic partition-frames="<<partitioned.size()<<" latency-in="<<speex_resampler_get_input_latency(reset.state.get())<<" latency-out="<<speex_resampler_get_output_latency(reset.state.get())<<" capacity=2048/512 PASS\n"; }
double fold(double hz,double rate){hz=std::fmod(hz,rate);return hz>rate/2?rate-hz:hz;}
void qualityCheck(int sourceRate,int projectRate) { double first{}, ripple{}, worstGain{}; for(double hz:{100.0,1000.0,5000.0,10000.0}){if(hz>=.8*std::min(sourceRate,projectRate)/2)continue;auto pass=render(sourceRate,projectRate,tone(hz,sourceRate),512);const double gain=20*std::log10(amplitude(pass,hz,projectRate)/.5);if(first==0)first=gain;ripple=std::max(ripple,std::abs(gain-first));worstGain=std::max(worstGain,std::abs(gain));}double rejection=-300;if(projectRate<sourceRate){const double test=std::min(sourceRate/2.-100.,projectRate/2.+1000.);auto blocked=render(sourceRate,projectRate,tone(test,sourceRate),512);rejection=20*std::log10(std::max(amplitude(blocked,fold(test,projectRate),projectRate),1e-15)/.5);}else{auto image=render(sourceRate,projectRate,tone(10000,sourceRate),512);rejection=20*std::log10(std::max(amplitude(image,fold(sourceRate-10000.,projectRate),projectRate),1e-15)/.5);}const bool pass=worstGain<=.10&&ripple<=.05&&rejection<=-60.;qualityFailed|=!pass;std::cout<<"speex quality source="<<sourceRate<<" project="<<projectRate<<" gain-db="<<worstGain<<" ripple-db="<<ripple<<" rejection-db="<<rejection<<" threshold="<<(pass?"PASS":"FAIL")<<"\n"; }
}
int main(){try{for(const auto& rate:std::array<std::array<int,2>,6>{{{{44100,48000}},{{48000,44100}},{{44100,96000}},{{96000,44100}},{{48000,96000}},{{96000,48000}}}}){timing(rate[0],rate[1]);qualityCheck(rate[0],rate[1]);}std::cout<<"speex comparator quality=10 timing=PASS quality="<<(qualityFailed?"FAIL":"PASS")<<"\n";return 0;}catch(const std::exception&e){std::cerr<<"speex comparator FAIL "<<e.what()<<'\n';return 1;}}
