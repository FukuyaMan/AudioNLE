#include <array>
#include <iostream>
#include <numeric>
#include <stdexcept>

#include "native_source_service.hpp"
#include "physical_src_range_planner.hpp"

namespace {
using namespace audionle::source_runtime;
void require(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
void plannerCase(std::uint32_t nativeRate, std::uint32_t projectRate) {
  const auto divisor=std::gcd(nativeRate,projectRate); const auto p=projectRate/divisor,q=nativeRate/divisor;
  PhysicalSrcRangePlanner planner; PhysicalSrcRangePlan first{},interior{},end{},boundary{};
  require(planner.plan(0,0,p,q,512,500000,end)==false || end.nativeFrames<=2048,"capacity");
  require(planner.plan(0,0,p,q,256,500000,first),"source start");
  require(planner.plan(0,10000,p,q,256,500000,interior),"interior");
  require(interior.physicalStart<=interior.logicalNativeStart&&interior.outputDiscard==10000-((interior.physicalStart*p)/q),"mapping");
  const auto fits=planner.plan(0,10000,p,q,512,500000,boundary);
  require(!fits || boundary.nativeFrames<=2048,"boundary");
  require(planner.plan(0,500000*p/q-1,p,q,1,500000,end),"near end");
}
void serviceCase() {
  FixtureNativeProvider provider; NativeSourceService service(provider,44,1); std::array<float,32> out{};
  // Eight unique callback misses fill the table; a ninth is nonblocking and
  // diagnosable. A duplicate request is coalesced rather than duplicated.
  for(unsigned n=0;n<NativeSourceService::requestCapacity;++n) require(!service.copy(std::int64_t(n)*NativeSourceService::pageFrames,1,1,out.data()),"miss");
  require(!service.copy(0,1,1,out.data()),"duplicate miss");
  require(!service.copy(std::int64_t(NativeSourceService::requestCapacity)*NativeSourceService::pageFrames,1,1,out.data()),"full miss");
  auto d=service.diagnostics(); require(d.coalesced>0&&d.queueFull>0,"queue metrics");
  while(service.serviceOne()){};
  require(!service.copy(0,1,1,out.data()),"post-drain request");
  require(service.serviceOne()&&service.copy(0,1,1,out.data()),"worker publication");
  service.reconfigureGeneration(2); require(!service.copy(0,1,1,out.data()),"generation invalidates page");
  while(service.serviceOne()){}; d=service.diagnostics(); require(d.workerCompleted>0,"completion");
}
}
int main(){try{plannerCase(44100,48000);plannerCase(48000,44100);plannerCase(44100,96000);plannerCase(96000,44100);plannerCase(48000,96000);plannerCase(96000,48000);serviceCase();std::cout<<"physical-src-range-planner ratios=6 positions=start,interior,end,capacity PASS\nnative-source-service requests=8 coalescing=PASS saturation=recoverable PASS\n";return 0;}catch(std::exception const&e){std::cerr<<e.what()<<'\n';return 1;}}
