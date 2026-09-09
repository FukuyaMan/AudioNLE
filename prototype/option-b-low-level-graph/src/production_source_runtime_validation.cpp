#include "source_runtime_engine.hpp"

#include <array>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <stdexcept>

using namespace audionle::source_runtime;
namespace {
constexpr std::int64_t kThreeHours48 = 3LL * 60 * 60 * 48000;
void require(bool value, char const* message) { if (!value) throw std::runtime_error(message); }
RuntimeIdentity identity(std::uint64_t source, std::uint64_t generation, std::uint32_t sourceRate, std::uint32_t projectRate, Admission admission) {
  auto value=makeRuntimeIdentity(source,sourceRate,admission); const auto divisor=std::gcd(sourceRate,projectRate); value.generation=generation; value.projectRate=projectRate; value.p=projectRate/divisor; value.q=sourceRate/divisor; return value;
}
struct LowLevelGraph final {
  std::array<SourceNode*,3> nodes{}; std::array<ClipRuntimeView,3> views{};
  void render(std::int64_t timeline, unsigned frames, float* output) noexcept { std::fill_n(output,frames,0.0f); std::array<float,512> scratch{}; for(unsigned i=0;i<nodes.size();++i) { nodes[i]->process(views[i],timeline,frames,scratch.data()); for(unsigned f=0;f<frames;++f) output[f]+=scratch[f]; } }
};
SourceRenderResult renderAfterDrain(SourceNode& node, ClipRuntimeView const& view, std::int64_t time, SharedSourceWorker& worker, float* output) {
  auto result=node.process(view,time,64,output); for(unsigned attempt=0;result==SourceRenderResult::Unavailable&&attempt<4;++attempt) { worker.drain(); result=node.process(view,time,64,output); } return result;
}
void run() {
  const auto root=std::filesystem::temp_directory_path()/"AudioNLE-production-source-runtime"; std::filesystem::create_directories(root);
  const PreparedArtifactKey preparedKey{300,1,77,1,48000}; const auto preparedFile=root/"three-hours.anleprp"; writeSparsePreparedFixtureArtifact(preparedFile,preparedKey,unsigned(kThreeHours48/256));
  const auto nativeId=identity(100,1,48000,48000,Admission::NativeRate), realtimeId=identity(200,1,44100,48000,Admission::GuaranteedRealtime), preparedId=identity(300,1,48000,48000,Admission::PreparedRequired);
  auto nativeShared=std::make_shared<SharedNativeSourceRuntime>(100,1,kThreeHours48+4096); auto realtimeShared=std::make_shared<SharedNativeSourceRuntime>(200,1,kThreeHours48+4096);
  SourceRuntime native(nativeId,{},nativeShared), realtime(realtimeId,{},realtimeShared), prepared(preparedId,preparedFile); SourceNode nativeNode(native), realtimeNode(realtime), preparedNode(prepared);
  LowLevelGraph graph{{&nativeNode,&realtimeNode,&preparedNode},{{{0,0,kThreeHours48,nativeId},{0,0,kThreeHours48,realtimeId},{0,0,kThreeHours48,preparedId}}}}; SharedSourceWorker worker; worker.add(native); worker.add(realtime); worker.add(prepared);
  std::array<float,512> output{}; const std::array<std::int64_t,3> seeks{0,kThreeHours48/2,kThreeHours48-512};
  for(unsigned round=0;round<12;++round) for(const auto time:seeks) { graph.render(time,64,output.data()); worker.drain(); graph.render(time,64,output.data()); require(native.nativeCalls()>0&&realtime.realtimeCalls()>0,"mixed graph native/realtime continue"); }
  // Lifecycle views are integer-coordinate-only edits; no route state moves in callback.
  auto view=graph.views[0]; view.timelineStart=1024; require(renderAfterDrain(nativeNode,view,1024,worker,output.data())==SourceRenderResult::Ready,"move"); view.sourceStart=128; require(renderAfterDrain(nativeNode,view,1024,worker,output.data())==SourceRenderResult::Ready,"trim/re-expand"); view.length=256; require(renderAfterDrain(nativeNode,view,1024,worker,output.data())==SourceRenderResult::Ready,"split");
  SourceRuntime sibling(realtimeId,{},realtimeShared); SourceNode siblingNode(sibling); require(renderAfterDrain(siblingNode,graph.views[1],0,worker,output.data())==SourceRenderResult::Ready,"shared sibling");
  // G1 queued work must never become G2 output. Recreation is control-side only.
  graph.views[0].identity.generation=2; require(nativeNode.process(graph.views[0],0,64,output.data())==SourceRenderResult::Stale,"G1 stale"); const auto g2Native=identity(100,2,48000,48000,Admission::NativeRate); auto g2Shared=std::make_shared<SharedNativeSourceRuntime>(100,2,kThreeHours48+4096); SourceRuntime recreated(g2Native,{},g2Shared); SourceNode recreatedNode(recreated); ClipRuntimeView g2View{0,0,kThreeHours48,g2Native}; SharedSourceWorker g2Worker; g2Worker.add(recreated); require(renderAfterDrain(recreatedNode,g2View,0,g2Worker,output.data())==SourceRenderResult::Ready,"G2 native isolated");
  graph.views[0].identity=nativeId;
  const auto g2PreparedKey=PreparedArtifactKey{300,2,77,1,48000}; const auto g2PreparedFile=root/"g2.anleprp"; writeSparsePreparedFixtureArtifact(g2PreparedFile,g2PreparedKey,unsigned(kThreeHours48/256)); const auto g2PreparedId=identity(300,2,48000,48000,Admission::PreparedRequired); SourceRuntime g2Prepared(g2PreparedId,g2PreparedFile); SourceNode g2PreparedNode(g2Prepared); ClipRuntimeView g2PreparedView{0,0,kThreeHours48,g2PreparedId}; g2Worker.add(g2Prepared); require(renderAfterDrain(g2PreparedNode,g2PreparedView,0,g2Worker,output.data())==SourceRenderResult::Ready&&g2Prepared.realtimeCalls()==0,"G2 prepared isolated");
  // Project rate and admission reconstruction uses fresh identities/SRC/artifacts.
  const auto rate96=identity(200,1,44100,96000,Admission::BestEffortRealtime); auto shared96=std::make_shared<SharedNativeSourceRuntime>(200,1,kThreeHours48*2+4096); SourceRuntime realtime96(rate96,{},shared96); SourceNode realtime96Node(realtime96); ClipRuntimeView rate96View{0,0,kThreeHours48*2,rate96}; SharedSourceWorker rateWorker; rateWorker.add(realtime96); require(renderAfterDrain(realtime96Node,rate96View,0,rateWorker,output.data())==SourceRenderResult::Ready,"48 to 96"); const auto rate48=identity(200,1,44100,48000,Admission::GuaranteedRealtime); SourceRuntime realtime48(rate48,{},realtimeShared); SourceNode realtime48Node(realtime48); require(renderAfterDrain(realtime48Node,graph.views[1],0,worker,output.data())==SourceRenderResult::Ready,"96 to 48");
  // Sustain mixed demand through bounded queues, then prove drain recovery and fair service.
  for(unsigned i=0;i<16;++i) { nativeNode.process(graph.views[0],i*256,64,output.data()); preparedNode.process(graph.views[2],i*256,64,output.data()); } worker.drain(); const auto metrics=worker.metrics(); require(metrics.submitted>0&&metrics.completed>0&&!metrics.starvationObserved,"fair bounded worker"); require(renderAfterDrain(nativeNode,graph.views[0],0,worker,output.data())==SourceRenderResult::Ready,"native recovery");
  require(prepared.preparedCalls()>0&&prepared.realtimeCalls()==0,"prepared required never fallback"); require(realtime.realtimeCalls()>0&&native.realtimeCalls()==0,"route SRC counts");
  std::cout<<"production source runtime fairness submitted="<<metrics.submitted<<" coalesced="<<metrics.coalesced<<" completed="<<metrics.completed<<" queue-full="<<metrics.queueFull<<" max-gap="<<metrics.maxObservedServiceGap<<" starvation="<<metrics.starvationObserved<<" three-hour-samples="<<kThreeHours48<<" shared-native-bytes="<<NativeSourceService::bytes()<<" prepared-pages=16 PASS\n";
  std::error_code error; std::filesystem::remove_all(root,error);
}
}
int main(){try{run();return 0;}catch(std::exception const& error){std::cerr<<"production source runtime FAIL "<<error.what()<<'\n';return 1;}}
