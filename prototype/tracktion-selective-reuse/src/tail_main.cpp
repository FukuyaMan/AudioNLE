#include <tracktion_engine/tracktion_engine.h>
#include <tracktion_graph/tracktion_graph.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace tail
{
using Sample = std::int64_t;
constexpr Sample sourceDuration = 480, tailLength = 1024, moveDelta = 48000;
constexpr int blockSize = 128;
constexpr float epsilon = 0.000001f;
enum class TailPolicy { reported, cutAtSourceEnd };
struct Clip { Sample start = 1000; float value = 0.25f; TailPolicy policy = TailPolicy::reported; bool tail = true; bool operator== (const Clip&) const = default; };
struct Domain { std::vector<Clip> clips; bool operator== (const Domain&) const = default; };

class Source final : public tracktion::graph::Node {
public: explicit Source (Clip c) : clip (c) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return { true, false, 1, 0, 0 }; }
    bool isReadyToProcess() override { return true; }
private: void process (ProcessContext& c) override { const auto offset = clip.start - c.referenceSampleRange.getStart(); choc::buffer::setAllFrames (c.buffers.audio, [this, offset] (auto f) { return (Sample) f == offset ? clip.value : 0.0f; }); c.buffers.midi.clear(); }
    Clip clip;
};
class TailNode final : public tracktion::graph::Node {
public: TailNode (std::unique_ptr<tracktion::graph::Node> n, Clip c) : owner (std::move (n)), input (owner.get()), clip (c) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return input->getNodeProperties(); }
    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; }
    bool isReadyToProcess() override { return input->hasProcessed(); }
private: void process (ProcessContext& c) override { choc::buffer::copy (c.buffers.audio, input->getProcessedOutput().audio); const auto sourceEnd = clip.start + sourceDuration; if (clip.tail && clip.policy == TailPolicy::reported) for (int i = 0; i < c.buffers.audio.getNumFrames(); ++i) { const auto p = c.referenceSampleRange.getStart() + i; if (p >= sourceEnd && p < sourceEnd + tailLength) c.buffers.audio.getSample (0, i) += clip.value; } c.buffers.midi.clear(); }
    std::unique_ptr<tracktion::graph::Node> owner; tracktion::graph::Node* input; Clip clip;
};
class Multiply final : public tracktion::graph::Node {
public: Multiply (std::unique_ptr<tracktion::graph::Node> n, float a) : owner (std::move (n)), input (owner.get()), amount (a) {}
    tracktion::graph::NodeProperties getNodeProperties() override { return input->getNodeProperties(); }
    std::vector<tracktion::graph::Node*> getDirectInputNodes() override { return { input }; } bool isReadyToProcess() override { return input->hasProcessed(); }
private: void process (ProcessContext& c) override { choc::buffer::copy (c.buffers.audio, input->getProcessedOutput().audio); for (int i=0;i<c.buffers.audio.getNumFrames();++i) c.buffers.audio.getSample(0,i)*=amount; c.buffers.midi.clear(); }
    std::unique_ptr<tracktion::graph::Node> owner; tracktion::graph::Node* input; float amount;
};
struct Result { std::vector<float> samples; Sample firstTail = -1, last = -1; };
Result render (const Domain& domain, bool multiplyAfter = false, bool multiplyBefore = false) {
    const auto before=domain; std::vector<std::unique_ptr<tracktion::graph::Node>> nodes;
    for (auto clip: domain.clips) { std::unique_ptr<tracktion::graph::Node> n=std::make_unique<Source>(clip); if (multiplyBefore) n=std::make_unique<Multiply>(std::move(n),2.0f); n=std::make_unique<TailNode>(std::move(n),clip); if(multiplyAfter)n=std::make_unique<Multiply>(std::move(n),2.0f); nodes.push_back(std::move(n)); }
    if(nodes.empty()) return {};
    std::unique_ptr<tracktion::graph::Node> root=nodes.size()==1?std::move(nodes.front()):std::make_unique<tracktion::graph::SummingNode>(std::move(nodes)); tracktion::graph::SimpleNodePlayer player(std::move(root),48000,blockSize); choc::buffer::ChannelArrayBuffer<float> buffer; buffer.resize({1,static_cast<choc::buffer::FrameCount>(blockSize)}); tracktion::engine::MidiMessageArray midi; Result r;
    for(Sample p=0;p<52000;p+=blockSize){auto v=buffer.getView();v.clear();midi.clear();player.process({static_cast<choc::buffer::FrameCount>(blockSize),juce::Range<Sample>::withStartAndLength(p,blockSize),{v,midi}});for(int i=0;i<blockSize;++i)r.samples.push_back(v.getSample(0,i));}
    const auto end=domain.clips.empty()?0:domain.clips.front().start+sourceDuration; for(Sample p=end;p<(Sample)r.samples.size();++p)if(std::abs(r.samples[(size_t)p])>epsilon){if(r.firstTail<0)r.firstTail=p;r.last=p;} if(domain!=before)throw std::runtime_error("Domain mutated");return r;
}
void require(bool x,const char* s){if(!x)throw std::runtime_error(s);} void exact(const Result&r,Sample first,Sample last,float amp,const char*n){require(r.firstTail==first,n);require(r.last==last,n);require(std::abs(r.samples[(size_t)first]-amp)<epsilon,n);}
void run(){ Clip a; const auto base=render({{a}}); exact(base,1480,2503,.25f,"reported tail"); require(std::abs(base.samples[1479])<epsilon,"silence feeding");
    exact(render({{a}},true),1480,2503,.5f,"downstream tail multiply"); exact(render({{a}},false,true),1480,2503,.25f,"upstream multiply order");
    Clip b{1800,.5f,TailPolicy::cutAtSourceEnd,false}; auto mix=render({{a,b}}); require(std::abs(mix.samples[1800]-.75f)<epsilon,"tail overlap track mix");
    auto moved=a;moved.start+=moveDelta; exact(render({{moved}}),49480,50503,.25f,"tail move"); auto cut=a;cut.policy=TailPolicy::cutAtSourceEnd;auto cutResult=render({{cut}});require(cutResult.firstTail==-1,"cut policy");
    Domain d{{a}};auto one=render(d);auto two=render(d);require(one.samples==two.samples,"reconstruction");require(render({}).samples.empty(),"delete"); std::cout<<"TAIL source-end=1480 processing-end=2504 first=1480 last=2503 overlap=0.75 move=48000 cut=pass rebuild=equal\n";
}
}
int main(){try{tail::run();std::cout<<"TAIL PASS\n";return 0;}catch(const std::exception&e){std::cerr<<"TAIL FAIL: "<<e.what()<<'\n';return 1;}}
