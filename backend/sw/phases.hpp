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

}    // namespace sw
