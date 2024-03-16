#pragma once
#include <chrono>
#include <concepts>

namespace sw::chrono {

class StopWatch
{
public:
    using clock = std::chrono::steady_clock;

    StopWatch(): m_startTime(clock::now()) {}

    void reset() { m_startTime = clock::now(); }

    double elapsed() const
    {
        return 1E-3 * static_cast<double>(
                        std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - m_startTime).count());
    }

private:
    std::chrono::time_point<clock> m_startTime;
};

inline double getTime(std::invocable auto &&function)
{
    StopWatch stopWatch;
    function();
    return stopWatch.elapsed();
}

}    // namespace sw::chrono
