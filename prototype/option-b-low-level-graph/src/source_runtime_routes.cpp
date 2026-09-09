#include "source_runtime_engine.hpp"
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace audionle::source_runtime;
namespace { void require(bool v,const char*m){if(!v)throw std::runtime_error(m);} void run(){
  auto root=std::filesystem::temp_directory_path()/"AudioNLE-source-runtime";std::filesystem::create_directories(root);auto file=root/"prepared.anleprp";PreparedArtifactKey key{3,1,77,1,48000};writePreparedFixtureArtifact(file,key,20);
  auto ni=makeRuntimeIdentity(1,48000,Admission::NativeRate),ri=makeRuntimeIdentity(2,44100,Admission::GuaranteedRealtime),pi=makeRuntimeIdentity(3,48000,Admission::PreparedRequired);SourceRuntime native(ni),realtime(ri),prepared(pi,file);SharedSourceWorker worker;worker.add(native);worker.add(realtime);worker.add(prepared);ClipRuntimeView nv{0,0,512,ni},rv{0,0,512,ri},pv{0,0,512,pi};SourceNode n(native),r(realtime),p(prepared);std::array<float,256>a{},b{},c{};
  require(n.process(nv,0,256,a.data())==SourceRenderResult::Ready,"native");require(r.process(rv,0,256,b.data())==SourceRenderResult::Ready,"realtime");require(p.process(pv,0,256,c.data())==SourceRenderResult::Unavailable,"prepared cold");worker.drain();require(p.process(pv,0,256,c.data())==SourceRenderResult::Ready,"prepared reload");
  auto shared=std::make_shared<SharedNativeSourceRuntime>(22,1);auto si=makeRuntimeIdentity(22,44100,Admission::GuaranteedRealtime);SourceRuntime first(si,{},shared),second(si,{},shared);SourceNode fn(first),sn(second);ClipRuntimeView sv{0,0,512,si};require(fn.process(sv,0,256,a.data())==SourceRenderResult::Ready&&sn.process(sv,0,256,b.data())==SourceRenderResult::Ready,"siblings");
  auto residency=makePreparedResidency(file,key);SourceRuntime pf(pi,file,{},residency),ps(pi,file,{},residency);SourceNode pfn(pf),psn(ps);require(pfn.process(pv,256,64,a.data())==SourceRenderResult::Unavailable&&psn.process(pv,256,64,b.data())==SourceRenderResult::Unavailable,"prepared shared cold");drainPrepared(*residency);require(pfn.process(pv,256,64,a.data())==SourceRenderResult::Ready&&psn.process(pv,256,64,b.data())==SourceRenderResult::Ready,"prepared shared ready");
  require(native.realtimeCalls()==0&&prepared.realtimeCalls()==0&&realtime.realtimeCalls()>0&&pf.preparedShared()==ps.preparedShared(),"route isolation");std::error_code ec;std::filesystem::remove_all(root,ec);std::cout<<"source runtime routes PASS\n";}}
int main(){try{run();return 0;}catch(std::exception const&e){std::cerr<<"source runtime FAIL "<<e.what()<<'\n';return 1;}}
