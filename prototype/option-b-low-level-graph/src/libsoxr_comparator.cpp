#include <soxr.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace {
constexpr size_t nativeCapacity = 2048, outputCapacity = 512;
bool qualityFailure = false;
void require(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
using Soxr = std::unique_ptr<std::remove_pointer_t<soxr_t>, decltype(&soxr_delete)>;
struct Runtime {
  Soxr state{nullptr, soxr_delete};
  std::array<float, nativeCapacity> input{};
  std::array<float, outputCapacity> output{};
  Runtime(int source, int project) {
    soxr_error_t error = nullptr;
    auto io = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
    auto quality = soxr_quality_spec(SOXR_VHQ, 0);
    auto runtime = soxr_runtime_spec(1);
    state.reset(soxr_create(source, project, 1, &error, &io, &quality, &runtime));
    require(state && !error, "soxr_create");
  }
  std::pair<size_t, size_t> process(float const* data, size_t available, bool final) {
    size_t used = 0, generated = 0;
    require(available <= input.size(), "input capacity");
    auto error = soxr_process(state.get(), final ? nullptr : data, final ? 0 : available, &used,
                              output.data(), output.size(), &generated);
    require(!error, error ? error : "soxr_process");
    return {used, generated};
  }
};
std::vector<float> tone(double hz, double rate) { std::vector<float> result(32000); for (size_t i=0;i<result.size();++i) result[i]=float(.5*std::sin(2*3.141592653589793*hz*i/rate)); return result; }
std::vector<float> content() { std::vector<float> result(32000); for(size_t i=0;i<result.size();++i) result[i]=float(.35*std::sin(i*.013)+.2*std::sin(i*.071)); return result; }
std::vector<float> render(int source, int project, std::vector<float> const& input, size_t maximumChunk) {
  Runtime runtime(source, project); std::vector<float> result; size_t offset = 0;
  while (offset < input.size()) {
    const auto offered = std::min({maximumChunk, input.size()-offset, runtime.input.size()});
    auto [used, generated] = runtime.process(input.data()+offset, offered, false);
    require(used || generated, "soxr progress"); offset += used;
    result.insert(result.end(), runtime.output.begin(), runtime.output.begin()+generated);
  }
  for (;;) { auto [used, generated] = runtime.process(nullptr, 0, true); require(!used, "flush consumes no input"); result.insert(result.end(), runtime.output.begin(), runtime.output.begin()+generated); if (!generated) break; }
  return result;
}
double amplitude(std::vector<float> const& x, double hz, double rate) { const size_t begin=3000, count=8000; require(x.size() >= begin+count, "quality coverage"); double s=0,c=0; for(size_t i=0;i<count;++i){double a=2*3.141592653589793*hz*(begin+i)/rate;s+=x[begin+i]*std::sin(a);c+=x[begin+i]*std::cos(a);} return 2*std::sqrt(s*s+c*c)/count; }
double fold(double hz, double rate) { hz=std::fmod(hz,rate); return hz>rate/2 ? rate-hz : hz; }
void timing(int source, int project) {
  auto input=content(), a=render(source,project,input,2048), b=render(source,project,input,257), c=render(source,project,input,2048);
  require(a==c && !b.empty() && std::all_of(a.begin(),a.end(),[](float v){return std::isfinite(v);}), "fresh deterministic/finite");
  Runtime state(source,project); std::cout << "soxr timing source=" << source << " project=" << project << " P/Q=" << project << '/' << source << " fresh=deterministic partition-frames=" << b.size() << " delay-output=" << soxr_delay(state.state.get()) << " capacity=2048/512 PASS\n";
}
void quality(int source, int project) {
  double base=0,ripple=0,worst=0; bool first=true;
  for (double hz : {100.,1000.,5000.,10000.,15000.}) { if (hz >= .8*std::min(source,project)/2) continue; auto y=render(source,project,tone(hz,source),512); const auto db=20*std::log10(amplitude(y,hz,project)/.5); if(first){base=db;first=false;} ripple=std::max(ripple,std::abs(db-base)); worst=std::max(worst,std::abs(db)); }
  double rejection=-300;
  if(project<source) { double hz=std::min(source/2.-100.,project/2.+1000.); auto y=render(source,project,tone(hz,source),512); rejection=20*std::log10(std::max(amplitude(y,fold(hz,project),project),1e-15)/.5); }
  else { auto y=render(source,project,tone(10000,source),512); rejection=20*std::log10(std::max(amplitude(y,fold(source-10000.,project),project),1e-15)/.5); }
  bool pass=worst<=.10&&ripple<=.05&&rejection<=-60.; qualityFailure|=!pass;
  std::cout << "soxr quality source="<<source<<" project="<<project<<" gain-db="<<worst<<" ripple-db="<<ripple<<" rejection-db="<<rejection<<" threshold="<<(pass?"PASS":"FAIL")<<'\n';
}
}
int main() { try { for (auto const& r : std::array<std::array<int,2>,6>{{{{44100,48000}},{{48000,44100}},{{44100,96000}},{{96000,44100}},{{48000,96000}},{{96000,48000}}}}) { timing(r[0],r[1]); quality(r[0],r[1]); } std::cout << "libsoxr comparator version=" << soxr_version() << " recipe=VHQ threads=1 timing=PASS quality=" << (qualityFailure?"FAIL":"PASS") << '\n'; return 0; } catch(std::exception const& e) { std::cerr << "libsoxr comparator FAIL " << e.what() << '\n'; return 1; } }
