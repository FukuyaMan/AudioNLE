#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>

// Fixture-local timing primitive. P is TimelineRate/gcd and Q is SourceRate/gcd.
// It selects source floor(T * Q / P); it intentionally owns no interpolation,
// buffering, allocation, or source I/O.
class IntegerRationalZoh final
{
public:
    using Sample = std::int64_t;

    IntegerRationalZoh (Sample timelineFactor, Sample sourceFactor)
        : p (timelineFactor), q (sourceFactor)
    {
        if (p <= 0 || q <= 0)
            throw std::invalid_argument ("ZOH factors must be positive");
        if (q > std::numeric_limits<Sample>::max() - (p - 1))
            throw std::overflow_error ("ZOH phase addition");
    }

    static Sample referenceHeldSource (Sample timeline, Sample timelineFactor, Sample sourceFactor)
    {
        if (timeline < 0 || timelineFactor <= 0 || sourceFactor <= 0
            || timeline > std::numeric_limits<Sample>::max() / sourceFactor)
            throw std::overflow_error ("ZOH reference multiplication");
        return timeline * sourceFactor / timelineFactor;
    }

    void reset (Sample timeline)
    {
        source = referenceHeldSource (timeline, p, q);
        // This multiplication has the same checked bound as the closed form.
        phase = (timeline * q) % p;
    }

    [[nodiscard]] Sample currentSource() const noexcept { return source; }

    void advance()
    {
        const auto sum = phase + q;
        const auto increment = sum / p;
        if (source > std::numeric_limits<Sample>::max() - increment)
            throw std::overflow_error ("ZOH source advance");
        source += increment;
        phase = sum % p;
    }

private:
    Sample p, q;
    Sample source {}, phase {};
};
