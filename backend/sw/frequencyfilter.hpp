#pragma once
#include "sw/basics/math.hpp"
#include "sw/signals.hpp"
#include <concepts>
#include <vector>

namespace sw {

template<std::floating_point F>
class FrequencyFilter
{
public:
    FrequencyFilter(const size_t initialCapacity) { m_buffer.reserve(initialCapacity); }

    F process(const F frequency, const F averagingTime, const F sampleTime)
    {
        assert(averagingTime >= math::zero<F>);

        const auto size = std::max(math::one<size_t>, static_cast<size_t>(std::round(averagingTime / sampleTime)));
        m_buffer.push_back(frequency);
        if (size < m_buffer.size())
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<int>(m_buffer.size() - size));

        const auto average =
          geometricAverage<F>(m_buffer | std::views::filter([](const auto f) { return f != math::zero<F>; }));
        if (average > math::one<F>)
            return average;

        return math::zero<F>;
    }

    void clearBuffer() { m_buffer.clear(); }

private:
    std::vector<F> m_buffer;
};

}    // namespace sw