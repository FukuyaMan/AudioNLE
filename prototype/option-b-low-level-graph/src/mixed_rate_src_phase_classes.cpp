#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using I = std::int64_t;
constexpr I P = 160, Q = 147;
I ceilMap (I s) { return (s * P + Q - 1) / Q; }
I floorMap (I t) { return t * Q / P; }

I firstTransition (I transition, I sourceStart, int outputs)
{
    std::array<float, 1024> input {}, output {};
    for (int i = 0; i < 1024; ++i)
        input[static_cast<size_t> (i)] = sourceStart + i >= transition ? 1.0f : 0.0f;
    juce::ZeroOrderHoldInterpolator zoh;
    zoh.process (static_cast<double> (Q) / P, input.data(), output.data(), outputs);
    for (int i = 0; i < outputs; ++i)
        if (output[static_cast<size_t> (i)] > 0.5f)
            return i;
    return -1;
}

I continuousTransition (I transition)
{
    return firstTransition (transition, 0, static_cast<int> (ceilMap (transition) + 8));
}

I directImpulse (I impulse)
{
    std::array<float, 1024> input {}, output {};
    input[static_cast<size_t> (impulse)] = 1.0f;
    juce::ZeroOrderHoldInterpolator zoh;
    zoh.process (static_cast<double> (Q) / P, input.data(), output.data(), static_cast<int> (ceilMap (impulse) + 8));
    for (int i = 0; i < static_cast<int> (ceilMap (impulse) + 8); ++i)
        if (output[static_cast<size_t> (i)] > 0.5f)
            return i;
    return -1;
}

I anchoredTransition (I transition)
{
    const I expected = ceilMap (transition);
    const I anchor = expected / P * P;
    return anchor + firstTransition (transition, floorMap (anchor), static_cast<int> (P + 8));
}

void run()
{
    std::map<I, int> resetErrors;
    std::map<I, int> continuousErrors;
    std::cout << "S,modQ,floor,ceil,remainder,continuous,reset,error,inputStart,phase\n";
    for (I s = 0; s != 2 * Q; ++s)
    {
        const I expected = ceilMap (s);
        const I continuous = continuousTransition (s);
        const I anchored = anchoredTransition (s);
        const I remainder = (s * P) % Q;
        ++resetErrors[anchored - expected];
        ++continuousErrors[continuous - expected];
        std::cout << s << ',' << s % Q << ',' << (s * P / Q) << ',' << expected << ',' << remainder << ','
                  << continuous << ',' << anchored << ',' << anchored - expected << ','
                  << floorMap (expected / P * P) << ",1.0\n";
    }
    std::cout << "RESET-HIST";
    for (const auto& [error, count] : resetErrors) std::cout << ' ' << error << ':' << count;
    std::cout << "\nCONTINUOUS-HIST";
    for (const auto& [error, count] : continuousErrors) std::cout << ' ' << error << ':' << count;
    std::cout << "\nIMPULSE S=147 expected=" << ceilMap (147) << " observed=" << directImpulse (147) << "\n";
}
}
int main() { try { run(); return 0; } catch (const std::exception& e) { std::cerr << "OB-SRC-PHASE FAIL " << e.what() << '\n'; return 1; } }
