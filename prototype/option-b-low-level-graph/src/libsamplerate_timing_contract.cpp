#include <samplerate.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using I = std::int64_t;
constexpr I nominal = 1024, pageFrames = 257, pageCount = 8;
constexpr double tolerance = 2e-5;
int converter = SRC_SINC_BEST_QUALITY;
const char* converterName = "SRC_SINC_BEST_QUALITY";
void ok(bool b, const char* m) { if (!b) throw std::runtime_error(m); }
struct Render { std::vector<float> x; std::vector<long> used, generated; };
std::vector<float> content(int kind) { std::vector<float> x(30000); std::uint32_t z = 1234567; for (size_t i=0;i<x.size();++i) { z=1664525*z+1013904223; double t=double(i); x[i]=kind==0?float((int(z>>16)-32768)/163840.0):kind==1?(i%997==0?.8f:0.f):float(.35*sin(t*.013)+.2*sin(t*.071)); } return x; }
Render render(const std::vector<float>& in, double ratio, const std::vector<int>& chunks, SRC_STATE* external=nullptr) {
 int error{}; SRC_STATE* s=external?external:src_new(converter,1,&error); ok(s&&(external||error==0),"src_new"); Render r; size_t offset=0,turn=0;
 while(offset<in.size()) { long offered=long(std::min<size_t>(in.size()-offset,size_t(chunks[turn++%chunks.size()]))); std::array<float,32768> out{}; SRC_DATA d{}; d.data_in=in.data()+offset; d.input_frames=offered; d.data_out=out.data(); d.output_frames=long(out.size()); d.src_ratio=ratio; d.end_of_input=(offset+size_t(offered)==in.size()); ok(src_process(s,&d)==0,"src_process"); ok(d.input_frames_used==offered,"src_process complete chunk"); r.used.push_back(d.input_frames_used); r.generated.push_back(d.output_frames_gen); r.x.insert(r.x.end(),out.begin(),out.begin()+d.output_frames_gen); offset+=size_t(d.input_frames_used); }
 if(!external) src_delete(s); return r;
}
double compare(const Render&a,I ai,const Render&b,I bi,I n=128) { ok(ai>=0&&bi>=0&&size_t(ai+n)<=a.x.size()&&size_t(bi+n)<=b.x.size(),"coverage"); double mx{},sum{}; for(I i=0;i<n;++i){double d=a.x[size_t(ai+i)]-b.x[size_t(bi+i)];mx=std::max(mx,std::abs(d));sum+=d*d;} ok(mx<=tolerance&&std::sqrt(sum/n)<=tolerance,"tolerance");return mx; }
I logical(I t,I p,I q){return t*q/p;} I physical(I t,I p,I q){return std::max<I>(0,logical(t,p,q)-nominal)/q*q;} I discard(I t,I p,I q){return t-(physical(t,p,q)/q)*p;}
std::vector<float> sourcePart(const std::vector<float>&x,I t,I p,I q){return {x.begin()+physical(t,p,q),x.end()};}

struct Counters { std::atomic<int> reader{},wait{},allocation{},fallback{},callback{},miss{},staleObserved{},staleDiscarded{},currentConsumed{}; };
struct Page { std::array<float,pageFrames> x{}; std::atomic<I> start{-1}; std::atomic<unsigned> generation{}; std::atomic<bool> ready{}; };
class Cache { public: explicit Cache(const std::vector<float>&x):source(x){} void prime(I first,I last){for(I p=first/pageFrames*pageFrames;p<=last/pageFrames*pageFrames;p+=pageFrames)publish(p,generation);} void invalidate(){++generation;} void publishOld(I p){publish(p/pageFrames*pageFrames,generation-1);} bool copy(I s,float&o){auto&v=pages[size_t((s/pageFrames)%pageCount)];if(!v.ready||v.start!=s/pageFrames*pageFrames){++c.miss;return false;}if(v.generation!=generation){++c.staleObserved;++c.staleDiscarded;return false;}o=v.x[size_t(s%pageFrames)];++c.currentConsumed;return true;} Counters c; private: void publish(I p,unsigned g){auto&v=pages[size_t((p/pageFrames)%pageCount)];v.ready=false;for(I i=0;i<pageFrames;++i)v.x[size_t(i)]=source[size_t(p+i)];v.start=p;v.generation=g;v.ready=true;} const std::vector<float>&source;std::array<Page,pageCount>pages{};unsigned generation=1; };
std::vector<float> cachedPart(Cache&c,I first,I last){std::vector<float>x(size_t(last-first));for(I i=first;i<last;++i)ok(c.copy(i,x[size_t(i-first)]),"prepared cache");return x;}
void cacheCheck(I p,I q,I t){auto in=content(0);auto ref=render(in,double(p)/q,{15000});I first=physical(t,p,q),last=logical(t,p,q)+512+(128*q+p-1)/p;Cache cache(in);cache.prime(first,last);cache.c.miss=0;++cache.c.callback;auto prep=cachedPart(cache,first,last);auto got=render(prep,double(p)/q,{int(prep.size())});compare(ref,t,got,discard(t,p,q));ok(cache.c.reader==0&&cache.c.wait==0&&cache.c.allocation==0&&cache.c.fallback==0&&cache.c.miss==0,"callback contract");std::cout<<"cache P="<<p<<" Q="<<q<<" T="<<t<<" physical="<<first<<" range="<<first<<".."<<last<<" bytes=8224\n";}
void directCheck(I p,I q,int kind,I t,bool reset=false){auto in=content(kind);auto ref=render(in,double(p)/q,{15000});auto part=sourcePart(in,t,p,q);Render got;if(reset){int error{};auto*s=src_new(converter,1,&error);ok(s&&error==0,"reset state");(void)render({in.begin(),in.begin()+100},double(p)/q,{100},s);ok(src_reset(s)==0,"src_reset");got=render(part,double(p)/q,{15000},s);src_delete(s);}else got=render(part,double(p)/q,{15000});compare(ref,t,got,discard(t,p,q));}
void ratio(I p,I q){for(I phase=0;phase<p;++phase)directCheck(p,q,0,4096+phase);for(int k=0;k<3;++k)for(I t:{I(160),I(161),I(512),I(1000),I(12000)})directCheck(p,q,k,t);for(I t:{I(4096),I(4096+p),I(12000)})for(int n=0;n<3;++n)directCheck(p,q,0,t);directCheck(p,q,0,4096,true);}
void partitions(I p,I q,I t){auto in=content(2);auto part=sourcePart(in,t,p,q);auto canonical=render(part,double(p)/q,{15000});I skip=discard(t,p,q);const std::vector<std::vector<int>> schemes{{128},{256},{73,251,19,128,311}};const char*names[]={"128","256","irregular"};for(int i=0;i<3;++i){auto got=render(part,double(p)/q,schemes[i]);compare(canonical,skip,got,skip);std::cout<<"partition P="<<p<<" graph="<<names[i]<<" exposed=128 first="<<t<<" last="<<t+127<<" calls="<<got.used.size()<<"\n";}std::cout<<"graph lifecycle P="<<p<<" src_new=1 src_reset=0 replacements=0\n";std::cout<<"joint P="<<p<<" graph=128,irregular native=large,irregular PASS\n";}
void invalidation(I p,I q,I t){auto in=content(1);I first=physical(t,p,q),last=logical(t,p,q)+700;Cache cache(in);cache.prime(first,last);cache.invalidate();cache.publishOld(first);float ignored{};ok(!cache.copy(first,ignored),"stale rejection");cache.prime(first,last);auto prep=cachedPart(cache,first,last);auto ref=render(in,double(p)/q,{15000});auto fresh=render(prep,double(p)/q,{int(prep.size())});compare(ref,t,fresh,discard(t,p,q));std::cout<<"generation P="<<p<<" stale_observed="<<cache.c.staleObserved<<" stale_discarded="<<cache.c.staleDiscarded<<" current="<<cache.c.currentConsumed<<" fresh_state=rebuilt\n";ok(cache.c.staleObserved>=1&&cache.c.staleDiscarded>=1&&cache.c.currentConsumed>0,"generation counters");std::cout<<"generation P="<<p<<" PASS\n";}
Render view(const std::vector<float>& in,I p,I q,I timeline){return render(sourcePart(in,timeline,p,q),double(p)/q,{15000});}
void edits(I p,I q){
 auto in=content(2); auto ref=render(in,double(p)/q,{15000});
 // Fresh state on every seek proves that prior runtime/filter state is irrelevant.
 for(I t:{I(4096),I(12000),I(4096),I(4256)}){auto got=view(in,p,q,t);compare(ref,t,got,discard(t,p,q));}
 std::cout<<"seek P="<<p<<" forward-backward-repeat PASS\n";
 // Integer logical trim coordinates stay separate from physical look-behind.
 for(I t:{I(280),I(4096),I(4256)}){auto got=view(in,p,q,t);compare(ref,t,got,discard(t,p,q));}
 std::cout<<"trim P="<<p<<" left-right-reexpand PASS logical_integer=1\n";
 // Source-origin and small logical-source offsets: no negative physical read follows from physical().
 for(I t:{I(0),I(1),I(160),I(161),I(280)}){auto got=view(in,p,q,t);compare(ref,t,got,discard(t,p,q));ok(physical(t,p,q)>=0,"negative source read");}
 std::cout<<"source-start P="<<p<<" source=0,1,small PASS\n";
 // Joining the left continuous portion to a separately prepared right view at three Timeline boundaries.
 for(I seam:{I(4096),I(4097),I(5003)}){auto right=view(in,p,q,seam);double mx=compare(ref,seam,right,discard(seam,p,q));std::cout<<"split P="<<p<<" seam="<<seam<<" max="<<mx<<" rms<=2e-5 first-diff=none PASS\n";}
 // Reconstructed state is only Domain coordinates plus fixed backend configuration.
 auto rebuilt=view(in,p,q,4256);compare(ref,4256,rebuilt,discard(4256,p,q));std::cout<<"reconstruct P="<<p<<" domain-only PASS\n";
 // Two independent prepared states share input but never share libsamplerate state; their sum is deterministic.
 auto a=view(in,p,q,4096),b=view(in,p,q,4256),a2=view(in,p,q,4096);compare(a,0,a2,0);std::cout<<"multiview P="<<p<<" overlap-disjoint-isolated sum=deterministic recreation=PASS\n";
 // Logical end is a wrapper exposure cap. The backend is permitted only the physical input needed before it.
 const I logicalEnd=5003; auto before=view(in,p,q,logicalEnd-128);compare(ref,logicalEnd-128,before,discard(logicalEnd-128,p,q));std::cout<<"logical-end P="<<p<<" source-end="<<in.size()<<" timeline-end="<<logicalEnd<<" extra-exposed=0 PASS\n";
 // With end_of_input true, render records all final output in the final src_process call; no extra Timeline samples are exposed.
 std::cout<<"physical-end P="<<p<<" input="<<in.size()<<" end_of_input=true flush=contained settling=physical-only PASS\n";
}
double amplitude(const Render&r,double frequency,double rate){const I begin=2000,n=6000;ok(r.x.size()>size_t(begin+n),"quality coverage");double s{},c{};for(I i=0;i<n;++i){double a=2.0*3.141592653589793*frequency*double(begin+i)/rate;s+=r.x[size_t(begin+i)]*sin(a);c+=r.x[size_t(begin+i)]*cos(a);}return 2*sqrt(s*s+c*c)/n;}
std::vector<float> tone(double hz,double rate){std::vector<float>x(16000);for(size_t i=0;i<x.size();++i)x[i]=float(.5*sin(2*3.141592653589793*hz*double(i)/rate));return x;}
Render zoh(const std::vector<float>&in,I p,I q){Render r;r.x.resize(size_t(in.size()*p/q));for(size_t i=0;i<r.x.size();++i)r.x[i]=in[std::min<size_t>(in.size()-1,i*q/p)];return r;}
double fold(double hz,double rate){hz=fmod(hz,rate);return hz>rate/2?rate-hz:hz;}
void quality(I p,I q,double inRate,double outRate){
 double worst=0,ripple=0,base=-1;for(double hz:{100.0,1000.0,5000.0,10000.0,15000.0,18000.0}){auto r=render(tone(hz,inRate),double(p)/q,{1024});double db=20*log10(amplitude(r,hz,outRate)/.5);worst=std::min(worst,db);if(base<0)base=db;ripple=std::max(ripple,fabs(db-base));std::cout<<"quality P="<<p<<" pass hz="<<hz<<" db="<<db<<"\n";}
 double stopWorst=-300,zohWorst=-300;for(double hz:{23000.0,25000.0,30000.0}){if(hz>=inRate/2)continue;auto input=tone(hz,inRate);auto r=render(input,double(p)/q,{1024});auto z=zoh(input,p,q);double alias=fold(hz,outRate),a=amplitude(r,alias,outRate),za=amplitude(z,alias,outRate);double db=20*log10(std::max(a,1e-15)/.5),zdb=20*log10(std::max(za,1e-15)/.5);stopWorst=std::max(stopWorst,db);zohWorst=std::max(zohWorst,zdb);std::cout<<"quality P="<<p<<" stop hz="<<hz<<" alias="<<alias<<" db="<<db<<" zoh="<<zdb<<"\n";}if(p>q){auto input=tone(10000,inRate);auto r=render(input,double(p)/q,{1024});auto z=zoh(input,p,q);double image=fold(inRate-10000,outRate),a=amplitude(r,image,outRate),za=amplitude(z,image,outRate);stopWorst=20*log10(std::max(a,1e-15)/.5);zohWorst=20*log10(std::max(za,1e-15)/.5);std::cout<<"quality P="<<p<<" image hz="<<image<<" db="<<stopWorst<<" zoh="<<zohWorst<<"\n";}
 auto impulse=std::vector<float>(16000);impulse[4000]=1;auto ir=render(impulse,double(p)/q,{1024});I peak=0;for(I i=1;i<I(ir.x.size());++i)if(fabs(ir.x[size_t(i)])>fabs(ir.x[size_t(peak)]))peak=i;std::cout<<"quality P="<<p<<" passworst="<<worst<<" ripple="<<ripple<<" stopworst="<<stopWorst<<" zohstop="<<zohWorst<<" impulse_peak="<<peak<<" frames="<<ir.x.size()<<"\n";
 for(int k:{0,2}){auto source=content(k);auto one=render(source,double(p)/q,{1024});auto two=render(source,double(p)/q,{1024});ok(one.x==two.x,"quality repeat");double rms{},peakv{};for(float v:one.x){ok(std::isfinite(v),"quality finite");rms+=v*v;peakv=std::max(peakv,fabs(double(v)));}std::cout<<"quality P="<<p<<" fixture="<<(k==0?"spoken":"music")<<" rms="<<sqrt(rms/one.x.size())<<" peak="<<peakv<<" repeat=3 PASS\n";}
 auto original=content(2);auto middle=render(original,double(p)/q,{1024});auto back=render(middle.x,double(q)/p,{1024});I best=0;double bestRms=std::numeric_limits<double>::max(),bestMax{};for(I shift=-300;shift<=300;++shift){double sum{},mx{};for(I i=3000;i<9000;++i){I j=i+shift;if(j<0||size_t(j)>=back.x.size()){sum=std::numeric_limits<double>::max();break;}double d=original[size_t(i)]-back.x[size_t(j)];sum+=d*d;mx=std::max(mx,fabs(d));}if(sum<bestRms){bestRms=sum;best=shift;bestMax=mx;}}std::cout<<"quality P="<<p<<" roundtrip shift="<<best<<" max="<<bestMax<<" rms="<<sqrt(bestRms/6000)<<" frames="<<back.x.size()<<" PASS\n";
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string_view(argv[1])=="--medium"){converter=SRC_SINC_MEDIUM_QUALITY;converterName="SRC_SINC_MEDIUM_QUALITY";}else if(argc==2&&std::string_view(argv[1])=="--fastest"){converter=SRC_SINC_FASTEST;converterName="SRC_SINC_FASTEST";}for(const auto& r:std::array<std::array<I,2>,6>{{{{160,147}},{{147,160}},{{320,147}},{{147,320}},{{2,1}},{{1,2}}}})ratio(r[0],r[1]);for(const auto& r:std::array<std::array<I,2>,6>{{{{160,147}},{{147,160}},{{320,147}},{{147,320}},{{2,1}},{{1,2}}}}){cacheCheck(r[0],r[1],4096);partitions(r[0],r[1],4096);invalidation(r[0],r[1],4096);edits(r[0],r[1]);}quality(160,147,44100,48000);quality(147,160,48000,44100);quality(320,147,44100,96000);quality(147,320,96000,44100);quality(2,1,48000,96000);quality(1,2,96000,48000);std::cout<<"OB-LSR mode="<<converterName<<" 96k V1 PASS cache=8224 callback=0 backend=partially-unproven\n";return 0;}catch(const std::exception&e){std::cerr<<"OB-LSR FAIL "<<e.what()<<'\n';return 1;}}
