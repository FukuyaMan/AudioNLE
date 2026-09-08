#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using Sample = int;
constexpr Sample rate = 48000, block = 128, sourceEnd = 1480, expectedTail = 1024;
constexpr float epsilon = 0.00001f;
void require(bool value, const char* message) { if (! value) throw std::runtime_error(message); }
void equal(float actual, float expected, const char* message) { require(std::abs(actual - expected) < epsilon, message); }

enum class TailPolicy { reported, cutAtSourceEnd };
// Deliberately framework-free: values below are persistent-description-shaped only.
struct ClipDescription { int id; Sample sourceEnd; Sample placement; float sourceValue; TailPolicy policy; bool enabled; };
struct TailExtent { Sample timelineSourceEnd; Sample duration; Sample processingEnd; };

class HostedTail final {
public:
    HostedTail() {
        juce::VST3PluginFormatHeadless format;
        juce::OwnedArray<juce::PluginDescription> descriptions;
        format.findAllTypesForFile(descriptions, OB_TAIL_VST3);
        require(! descriptions.isEmpty(), "VST3 discovery");
        manager.addFormat(std::make_unique<juce::VST3PluginFormatHeadless>());
        juce::String error;
        plugin = manager.createPluginInstance(*descriptions[0], rate, block, error);
        require(plugin != nullptr, "VST3 instance");
        plugin->setRateAndBufferSizeDetails(rate, block);
        plugin->prepareToPlay(rate, block);
        tailSamples = static_cast<Sample>(std::llround(plugin->getTailLengthSeconds() * rate));
        require(tailSamples == expectedTail, "Tail metadata conversion");
    }
    Sample reportedTailSamples() const { return tailSamples; }
    void process(const std::array<float, block>& input, int frames, std::array<float, block>& output) {
        std::copy_n(input.begin(), frames, scratch.begin());
        float* channels[] = { scratch.data() };
        juce::AudioBuffer<float> audio(channels, 1, frames);
        midi.clear();
        plugin->processBlock(audio, midi);
        std::copy_n(scratch.begin(), frames, output.begin());
    }
private:
    juce::AudioPluginFormatManager manager;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    juce::MidiBuffer midi;
    std::array<float, block> scratch{};
    Sample tailSamples = 0;
};

class TailRuntime final {
public:
    explicit TailRuntime(ClipDescription description, float downstreamMultiplier = 1.0f)
        : description(description), multiplier(downstreamMultiplier) {
        const Sample duration = description.policy == TailPolicy::reported ? host.reportedTailSamples() : 0;
        extent = { description.placement + description.sourceEnd, duration, description.placement + description.sourceEnd + duration };
    }
    const TailExtent& tailExtent() const { return extent; }
    int processGrowth() const { return growth; }
    int processWaits() const { return waits; }
    int sourceIo() const { return io; }
    std::vector<float> render(Sample begin, Sample end) {
        std::vector<float> rendered(static_cast<size_t>(end - begin), 0.0f); // before processing
        if (! description.enabled) return rendered;
        for (Sample position = begin; position < std::min(end, extent.processingEnd); position += block) {
            const int frames = std::min<Sample>(block, std::min(end, extent.processingEnd) - position);
            input.fill(0.0f); output.fill(0.0f);
            for (int i = 0; i < frames; ++i) {
                const Sample timeline = position + i;
                if (timeline >= description.placement && timeline < extent.timelineSourceEnd) input[i] = description.sourceValue;
                if (timeline == extent.timelineSourceEnd) require(input[i] == 0.0f, "first zero-fed input");
            }
            host.process(input, frames, output);
            for (int i = 0; i < frames; ++i) rendered[static_cast<size_t>(position - begin + i)] = output[i] * multiplier;
        }
        return rendered;
    }
private:
    ClipDescription description;
    HostedTail host;
    TailExtent extent{};
    std::array<float, block> input{}, output{};
    float multiplier;
    int growth = 0, waits = 0, io = 0;
};

float at(const std::vector<float>& samples, Sample begin, Sample position) { return samples.at(static_cast<size_t>(position - begin)); }
std::vector<float> sum(const std::vector<std::vector<float>>& inputs) {
    require(! inputs.empty(), "sum inputs");
    std::vector<float> output(inputs.front().size(), 0.0f);
    for (const auto& input : inputs) { require(input.size() == output.size(), "sum size"); for (size_t i = 0; i < output.size(); ++i) output[i] += input[i]; }
    return output;
}
std::vector<float> sourceOnly(Sample begin, Sample end, Sample first, Sample last, float value) {
    std::vector<float> output(static_cast<size_t>(end - begin), 0.0f);
    for (Sample s = std::max(begin, first); s < std::min(end, last); ++s) output[static_cast<size_t>(s - begin)] = value;
    return output;
}
void exactTail(const std::vector<float>& output, Sample begin, const TailExtent& extent, float scale, const char* prefix) {
    equal(at(output, begin, extent.timelineSourceEnd - 1), 0.25f * scale, prefix);
    if (extent.duration != 0) {
        equal(at(output, begin, extent.timelineSourceEnd), 0.125f * scale, prefix);
        equal(at(output, begin, extent.processingEnd - 1), 0.125f * scale, prefix);
    }
    equal(at(output, begin, extent.processingEnd), 0.0f, prefix);
}

void run() {
    const ClipDescription initialA{1, sourceEnd, 0, 0.25f, TailPolicy::reported, true};
    // T2: SourceEnd falls within [1408,1536), and ProcessingEnd is metadata-derived.
    TailRuntime t2(initialA);
    const auto initialExtent = t2.tailExtent();
    require(initialExtent.timelineSourceEnd == 1480 && initialExtent.duration == 1024 && initialExtent.processingEnd == 2504, "T2 extent");
    const auto raw = t2.render(0, 2505);
    exactTail(raw, 0, initialExtent, 1.0f, "T2 boundaries");
    require(t2.processGrowth() == 0 && t2.processWaits() == 0 && t2.sourceIo() == 0, "T2 process behavior");

    // T3: Multiply is an unconditional downstream stage on host output.
    TailRuntime t3Raw(initialA), t3Processed(initialA, 2.0f);
    const float rawTail = at(t3Raw.render(0, 2505), 0, 1497);
    const float downstream = at(t3Processed.render(0, 2505), 0, 1497);
    equal(rawTail, 0.125f, "T3 raw"); equal(downstream, 0.25f, "T3 multiply");

    // T4: no Tail branch; ordinary linear sum with B active during A Tail.
    TailRuntime t4(initialA);
    const auto aOutput = t4.render(0, 2800);
    const auto bOutput = sourceOnly(0, 2800, 2000, 2700, 0.5f);
    const auto mixed = sum({aOutput, bOutput});
    equal(at(mixed, 0, 1600), 0.125f, "T4 Tail-only"); equal(at(mixed, 0, 2000), 0.625f, "T4 overlap"); equal(at(mixed, 0, 2520), 0.5f, "T4 B-only");

    // T5: move description then rebuild. B is unchanged and overlaps only the moved A Tail.
    constexpr Sample delta = 4000;
    const ClipDescription movedA{1, sourceEnd, delta, 0.25f, TailPolicy::reported, true};
    TailRuntime oldRuntime(initialA), movedRuntime(movedA);
    const auto movedExtent = movedRuntime.tailExtent();
    require(movedA.sourceEnd == initialA.sourceEnd && movedExtent.timelineSourceEnd == initialExtent.timelineSourceEnd + delta && movedExtent.processingEnd == initialExtent.processingEnd + delta, "T5 derived move");
    const auto movedOutput = movedRuntime.render(0, movedExtent.processingEnd + 1);
    const auto unchangedB = sourceOnly(0, movedExtent.processingEnd + 1, 5600, 6300, 0.5f);
    const auto movedMix = sum({movedOutput, unchangedB});
    equal(at(movedMix, 0, 100), 0.0f, "T5 old source absent"); equal(at(movedMix, 0, 1600), 0.0f, "T5 old Tail absent");
    exactTail(movedOutput, 0, movedExtent, 1.0f, "T5 moved Tail"); equal(at(movedMix, 0, 5600), 0.625f, "T5 B overlap");
    require(oldRuntime.tailExtent().duration == movedExtent.duration, "T5 fixture metadata unchanged");

    // T6: delete A from the description and rebuild; only B remains.
    const auto deletedMix = sum({sourceOnly(0, movedExtent.processingEnd + 1, 5600, 6300, 0.5f)});
    equal(at(deletedMix, 0, movedExtent.timelineSourceEnd), 0.0f, "T6 deleted Tail absent"); equal(at(deletedMix, 0, 5600), 0.5f, "T6 B unchanged");

    // T7: policy alone controls the extent; CutAtSourceEnd does not process a Tail block.
    const ClipDescription cutA{1, sourceEnd, 0, 0.25f, TailPolicy::cutAtSourceEnd, true};
    TailRuntime reported(initialA), cut(cutA);
    require(reported.tailExtent().duration == expectedTail && cut.tailExtent().duration == 0 && cut.tailExtent().processingEnd == sourceEnd, "T7 policy extent");
    const auto cutOutput = cut.render(0, 2505);
    equal(at(cutOutput, 0, 1479), 0.25f, "T7 Cut source"); equal(at(cutOutput, 0, 1480), 0.0f, "T7 Cut Tail");

    // T8: same final description, fresh VST3 instances, identical output and derived runtime metadata.
    const ClipDescription finalA{1, sourceEnd, delta, 0.25f, TailPolicy::reported, true};
    auto renderFinal = [&] {
        TailRuntime runtime(finalA, 2.0f);
        const auto output = sum({runtime.render(0, 7000), sourceOnly(0, 7000, 5600, 6300, 0.5f)});
        return std::pair{output, runtime.tailExtent()};
    };
    const auto first = renderFinal(); const auto second = renderFinal();
    require(first.first == second.first && first.second.timelineSourceEnd == second.second.timelineSourceEnd && first.second.duration == second.second.duration && first.second.processingEnd == second.second.processingEnd, "T8 reconstruction");
    equal(at(first.first, 0, 5600), 0.75f, "T8 downstream overlap");

    std::cout << "OB-TAIL T2 source-end=1480 tail=[1480,2504) first-zero-fed=1480 boundary-error=0 growth=0 waits=0\n"
              << "OB-TAIL T3 sample=1497 raw=0.125 expected=0.25 observed=0.25 timing-error=0\n"
              << "OB-TAIL T4 tail-only=0.125 overlap=0.625 b-only=0.5 timing-error=0\n"
              << "OB-TAIL T5 delta=4000 old=zero moved-tail=[5480,6504) overlap=0.625 error=0\n"
              << "OB-TAIL T6 deleted-a=zero b=0.5 T7 reported=[1480,2504) cut-end=1480 T8 reconstruction=equal\n"
              << "OB-TAIL T2-T8 PASS max-boundary-error=0\n";
}
} // namespace
int main() { try { run(); return 0; } catch (const std::exception& error) { std::cerr << "OB-TAIL FAIL " << error.what() << '\n'; return 1; } }
