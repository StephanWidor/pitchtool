#pragma once
#include "basics/math.hpp"
#include "basics/ranges.hpp"

namespace sw {

template<std::floating_point F>
F phaseAngle(const F frequency, const F timeDiff)
{
    return frequency * timeDiff * math::twoPi<F>;
};

template<std::floating_point F>
F frequency(const F phaseAngle, const F timeDiff)
{
    return phaseAngle / (timeDiff * math::twoPi<F>);
};

template<std::floating_point F>
F standardized(const F phaseAngle)
{
    const auto positiveSmallerEqTwoPi = phaseAngle - std::floor(phaseAngle / math::twoPi<F>) * math::twoPi<F>;
    return positiveSmallerEqTwoPi <= math::pi<F> ? positiveSmallerEqTwoPi : positiveSmallerEqTwoPi - math::twoPi<F>;
}

template<std::floating_point F>
void shiftPhases(ranges::TypedInputRange<F> auto &&phases, ranges::TypedInputRange<F> auto &&frequencies,
                 const F timeDiff, ranges::TypedOutputRange<F> auto &&o_shiftedPhases)
{
    assert(std::ranges::size(phases) == std::ranges::size(frequencies) &&
           std::ranges::size(phases) == std::ranges::size(o_shiftedPhases));
    std::transform(phases.begin(), phases.end(), frequencies.begin(), o_shiftedPhases.begin(),
                   [timeDiff](const auto phase, const auto frequency) {
                       return standardized(phase + phaseAngle(frequency, timeDiff));
                   });
}

}    // namespace sw
