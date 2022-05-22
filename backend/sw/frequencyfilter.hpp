#pragma once
#include "sw/basics/math.hpp"
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

        if (frequency <= math::zero<F>)
        {
            m_buffer.clear();
            return math::zero<F>;
        }

        m_buffer.push_back(frequency);
        m_size = std::max(math::one<size_t>, static_cast<size_t>(std::round(averagingTime / sampleTime)));

        if (m_size < m_buffer.size())
            m_buffer.erase(m_buffer.begin(), m_buffer.begin() + static_cast<int>(m_buffer.size() - m_size));

        return geometricAverage<F>(m_buffer);
    }

    void clearBuffer() { m_buffer.clear(); }

private:
    size_t m_size{0u};
    std::vector<F> m_buffer;
};

}    // namespace sw