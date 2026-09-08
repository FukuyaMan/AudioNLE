#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace { using I=std::int64_t; constexpr int pg=257, np=2, n=401; constexpr I P=160,Q=147;
I held(I t){return t*Q/P;} I boundary(I s){return(s*P+Q-1)/Q;} void ok(bool x,const char*m){if(!x)throw std::runtime_error(m);}
struct C{std::atomic<int> callback{},generator{},wait{},allocation{},fallback{},resets{};};
struct Page{std::array<float,pg>x{};std::atomic<I> start{-1};std::atomic<bool>ready{};};
class Cache { public:
  void request(I s){I p=s/pg*pg;int z=0;if(state.compare_exchange_strong(z,1))req.store(p);}
  bool serve(){int z=1;if(!state.compare_exchange_strong(z,2))return false;const I start=req.load();auto&p=page[(start/pg)%np];p.ready=false;for(int i=0;i<pg;++i)p.x[i]=float(start+i);p.start.store(start);p.ready=true;state=0;return true;}
  bool get(I s,float&o)const{auto&p=page[(s/pg)%np];if(!p.ready||p.start!=s/pg*pg)return false;o=p.x[s%pg];return true;}
  void prime(I s){for(int i=0;i<800;++i){request(s);if(get(s,dummy))return;std::this_thread::sleep_for(std::chrono::milliseconds(1));}ok(false,"page");}
  C c; private:std::array<Page,np>page{};std::atomic<I>req{};std::atomic<int>state{};mutable float dummy{};};
class Worker{public:Worker(Cache&a):c(a),t([this]{while(go){if(!c.serve())std::this_thread::sleep_for(std::chrono::milliseconds(1));}}){}~Worker(){go=false;t.join();}private:Cache&c;std::atomic<bool>go{true};std::thread t;};
void run(){Cache cache;Worker worker(cache);cache.prime(0);cache.prime(257);for(I s:{I(0),I(5),I(143),I(151),I(255),I(256),I(257),I(258),I(290),I(298)}){float v;ok(cache.get(s,v)&&v==float(s),"cache identity");}
 std::array<float,512> input{},direct{},cached{};for(int i=0;i<512;++i)input[i]=float(i);juce::ZeroOrderHoldInterpolator d,w;d.process(double(Q)/P,input.data(),direct.data(),n);I cursor=0;for(int begin=0;begin<n;begin+=128){int count=std::min(128,n-begin),need=(count*147+159)/160+2;for(int i=0;i<need;++i){float v;ok(cache.get(cursor+i,v),"callback cache");input[i]=v;}cache.c.callback++;int used=w.process(double(Q)/P,input.data(),cached.data()+begin,count);cursor+=used;}
 int match=0,miss=0,exact=0,fractional=0;std::array<I,2> positions{};int k=0;for(int t=0;t<n;++t){I a=I(std::lround(cached[t])),ref=I(std::lround(direct[t])),e=held(t);ok(a==ref,"direct/cache divergence");if(a==e)++match;else{++miss;if(boundary(e)==t)++exact;else ++fractional;if(k<2)positions[k++]=t;}}
 ok(match==399&&miss==2&&exact==2&&fractional==0&&positions[0]==160&&positions[1]==320,"distribution");ok(cache.c.generator==0&&cache.c.wait==0&&cache.c.allocation==0&&cache.c.fallback==0,"callback invariant");
 std::cout<<"OB-SRC-WC total=401 matched=399 mismatched=2 positions=160,320 exact=2 fractional=0 resets=0 callback-generator=0 wait=0 alloc=0 fallback=0 Tracktion=0\n";for(int t:{157,158,159,160,161,162,163,317,318,319,320,321,322,323})std::cout<<"T="<<t<<" expected="<<held(t)<<" direct="<<I(std::lround(direct[t]))<<" cache="<<I(std::lround(cached[t]))<<"\n";}
}
void runWorkerCacheIdentityTrace(){run();}
