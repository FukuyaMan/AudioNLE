#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#ifndef OB_LATENCY_SAMPLES
#error OB_LATENCY_SAMPLES must be defined
#endif
class OptionBLatencyFixture final : public juce::AudioProcessor {
public:
 OptionBLatencyFixture():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::mono(),true).withOutput("Output",juce::AudioChannelSet::mono(),true)){setLatencySamples(OB_LATENCY_SAMPLES);}
 const juce::String getName() const override{return "Option B Latency";} void prepareToPlay(double,int) override{delay.fill(0);write=0;} void releaseResources() override{}
 bool isBusesLayoutSupported(const BusesLayout& l) const override{return l.getMainInputChannelSet()==juce::AudioChannelSet::mono()&&l.getMainOutputChannelSet()==juce::AudioChannelSet::mono();}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) override{auto*x=b.getWritePointer(0);for(int i=0;i<b.getNumSamples();++i){auto v=x[i];x[i]=delay[write];delay[write]=v;write=(write+1)%OB_LATENCY_SAMPLES;}}
 bool hasEditor() const override{return false;} juce::AudioProcessorEditor* createEditor() override{return nullptr;} double getTailLengthSeconds() const override{return 0;} bool acceptsMidi() const override{return false;} bool producesMidi() const override{return false;} int getNumPrograms() override{return 1;} int getCurrentProgram() override{return 0;} void setCurrentProgram(int) override{} const juce::String getProgramName(int) override{return{};} void changeProgramName(int,const juce::String&) override{} void getStateInformation(juce::MemoryBlock&) override{} void setStateInformation(const void*,int) override{}
private: std::array<float,OB_LATENCY_SAMPLES> delay{};int write=0;};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new OptionBLatencyFixture();}
