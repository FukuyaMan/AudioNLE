#include <tracktion_engine/tracktion_engine.h>

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;
    tracktion::engine::Engine engine { "AudioNLE Tracktion feasibility bootstrap" };
    return 0;
}
