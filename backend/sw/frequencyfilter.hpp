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

    F process(const F frequency, const F sampleTime, const F averagingTime, const F holdTime)
    {
        assert(sampleTime > math::zero<F>);
        assert(averagingTime >= math::zero<F>);
        assert(holdTime >= math::zero<F>);

        const auto holdLength = static_cast<size_t>(std::round(holdTime / sampleTime));
        if (frequency <= math::zero<F>)
        {
            if (m_holdCount < holdLength)
            {
                m_buffer.push_back(m_out);
                ++m_holdCount;
            }
            else
            {
                m_buffer.push_back(math::zero<F>);
                m_holdCount = 0u;
            }
        }
        else
        {
            m_buffer.push_back(frequency);
            m_holdCount = 0u;
        }

        const auto averagingLength = static_cast<size_t>(std::round(averagingTime / sampleTime)) + math::one<size_t>;
        if (averagingLength < m_buffer.size())
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<int>(m_buffer.size() - averagingLength));

        m_out = std::ranges::all_of(m_buffer, [](const auto f) { return f == math::zero<F>; }) ?
                  math::zero<F> :
                  geometricAverage<F>(m_buffer | std::views::filter([](const auto f) { return f != math::zero<F>; }));
        return m_out;
    }

    void clearBuffer() { m_buffer.clear(); }

private:
    std::vector<F> m_buffer;
    size_t m_holdCount = 0u;
    F m_out = math::zero<F>;
};

}    // namespace sw