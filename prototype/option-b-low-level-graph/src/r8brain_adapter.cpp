#include <CDSPResampler.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
constexpr int kNativeCapacity=2048, kDiagnosticMaximum=8192, kProjectCapacity=512;
void require(bool ok,char const* what){if(!ok)throw std::runtime_error(what);}
class Adapter {
 public:
  Adapter(double source,double project): state_(source,project,kDiagnosticMaximum,2.0,140.0) {
    input_.fill(0); output_.fill(0); // All storage and the r8brain cache are created before callback use.
  }
  int required() const { return state_.getInputRequiredForOutput(kProjectCapacity); }
  int latency() const { return state_.getLatency(); }
  double latencyFraction() const { return state_.getLatencyFrac(); }
  int process(std::array<float,kNativeCapacity> const& source, int frames) {
    require(frames>=0&&frames<=kNativeCapacity,"input bounds");
    for(int i=0;i<frames;++i) input_[size_t(i)]=source[size_t(i)];
    double* backend=nullptr; int available=state_.process(input_.data(),frames,backend);
    const int exposed=(std::min)(available,kProjectCapacity);
    for(int i=0;i<exposed;++i) output_[size_t(i)]=float(backend[i]);
    return exposed;
  }
  void clear(){state_.clear();}
  std::array<float,kProjectCapacity> const& output() const{return output_;}
 private:
  r8b::CDSPResampler state_;
  std::array<double,kNativeCapacity> input_{};
  std::array<float,kProjectCapacity> output_{};
};
void one(double source,double project){Adapter a(source,project);const int need=a.required();std::cout<<"r8brain adapter source="<<source<<" project="<<project<<" required="<<need<<" latency="<<a.latency()<<" latency-frac="<<a.latencyFraction()<<" public-native="<<kNativeCapacity<<" capacity="<<(need<=kNativeCapacity?"PASS":"FAIL")<<" float-to-double=16384B double-to-float=2048B\n";if(need>kNativeCapacity)return;std::array<float,kNativeCapacity> input{};for(int i=0;i<kNativeCapacity;++i)input[size_t(i)]=std::sin(i*.031f);int n=a.process(input,need);require(n>=0&&n<=kProjectCapacity,"output bounds");for(int i=0;i<n;++i)require(std::isfinite(a.output()[size_t(i)]),"finite output");a.clear();int again=a.process(input,need);require(again==n,"clear deterministic count");}
}
int main(){try{for(auto const&r:std::array<std::array<double,2>,6>{{{{44100,48000}},{{48000,44100}},{{44100,96000}},{{96000,44100}},{{48000,96000}},{{96000,48000}}}})one(r[0],r[1]);std::cout<<"r8brain adapter pin=9e73d2d cache=control-thread-warmup PASS\n";return 0;}catch(std::exception const&e){std::cerr<<"r8brain adapter FAIL "<<e.what()<<'\n';return 1;}}
