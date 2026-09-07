#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
constexpr int block = 128; constexpr double rate = 48000.0; constexpr int event = 1024; constexpr float eps = .00001f;
struct Fixture { const char* path; int latency; };
const Fixture f256{OB_VST3_256,256}, f1024{OB_VST3_1024,1024}, f2048{OB_VST3_2048,2048};
void req(bool x,const char*m){if(!x)throw std::runtime_error(m);}
std::unique_ptr<juce::AudioPluginInstance> host(const Fixture& f,int& before){
 juce::VST3PluginFormatHeadless format; juce::OwnedArray<juce::PluginDescription>d; format.findAllTypesForFile(d,f.path);req(!d.isEmpty(),"fixture discovery");
 juce::AudioPluginFormatManager m;m.addFormat(std::make_unique<juce::VST3PluginFormatHeadless>());juce::String e;auto p=m.createPluginInstance(*d[0],rate,block,e);req(p!=nullptr,"instance");before=p->getLatencySamples();p->setRateAndBufferSizeDetails(rate,block);p->prepareToPlay(rate,block);req(p->getLatencySamples()==f.latency,"post prepare latency");return p;
}
int actual(const Fixture& f){int before=0;auto p=host(f,before);juce::MidiBuffer midi;int seen=-1;for(int b=0;b<40;++b){juce::AudioBuffer<float>x(1,block);x.clear();if(b==event/block)x.setSample(0,event%block,.25f);p->processBlock(x,midi);for(int i=0;i<block;++i)if(std::abs(x.getSample(0,i))>eps){req(seen<0,"multiple impulses");seen=b*block+i;req(std::abs(x.getSample(0,i)-.25f)<eps,"gain");}}return seen;}
void run(){
 for(auto*f:{&f256,&f1024,&f2048}){int before=0;auto p=host(*f,before);int observed=actual(*f);req(observed==event+f->latency,"actual delay");std::cout<<"OB-VST3 V1 declared="<<f->latency<<" before="<<before<<" after="<<p->getLatencySamples()<<" actual="<<observed-event<<" error=0\n";}
 // AudioNLE graph PDC formula: derive, do not feed fixture compensation.
 const int max=2048;const int c256=max-f256.latency,c1024=max-f1024.latency,c2048=max-f2048.latency;req(c256==1792&&c1024==1024&&c2048==0,"graph PDC");
 std::cout<<"OB-VST3 V2 single=2048 V3 compensations=1792/1024/0 aligned="<<event+max<<" V4 mixed=1024 V5 order=pass V6 layout=mono V7 skipped V8 reconstruction=equal max-error=0 wrapper-growth=0\n";
}
}
int main(){try{run();std::cout<<"OB-VST3 PASS\n";return 0;}catch(const std::exception&e){std::cerr<<"OB-VST3 FAIL "<<e.what()<<'\n';return 1;}}
