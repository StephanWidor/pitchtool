#pragma once
#include "sw/basics/ranges.hpp"
#include "sw/containers/utils.hpp"
#include <atomic>
#include <cassert>
#include <functional>
#include <span>
#include <vector>

namespace sw::containers {

template<typename ValueType>
class SpinLockedBuffer
{
public:
    SpinLockedBuffer(const size_t size, const ValueType initValue): m_inBuffer(size, initValue), m_outBuffer(size) {}

    template<ranges::TypedInputRange<ValueType> R>
    SpinLockedBuffer(R &&range): m_inBuffer(ranges::to_vector(std::forward<R>(range))), m_outBuffer(m_inBuffer.size())
    {}

    /// set data from feeding thread
    void set(ranges::TypedInputRange<ValueType> auto &&values)
    {
        lock();
        m_inBuffer.resize(std::ranges::size(values));
        std::copy(values.begin(), values.end(), m_inBuffer.begin());
        m_newDataAvailable = true;
        unlock();
    }

    /// ring push data from feeding thread
    template<ranges::TypedInputRange<ValueType> R>
    void ringPush(R &&values)
    {
        lock();
        containers::ringPush(m_inBuffer, std::forward<R>(values));
        m_newDataAvailable = true;
        unlock();
    }

    void clear()
    {
        lock();
        m_inBuffer.clear();
        m_newDataAvailable = true;
        unlock();
    }

    /// apply callback to data from feeding thread
    void apply(std::invocable<std::vector<ValueType> &> auto &&callback)
    {
        lock();
        callback(m_inBuffer);
        m_newDataAvailable = true;
        unlock();
    }

    /// only call from feeding thread
    const std::vector<ValueType> &inBuffer() const { return m_inBuffer; }

    /// get data from consumer thread (thread safe in respect of pushing/setting/changing data from feeding thread,
    /// as long as there is only one consumer thread)
    const std::vector<ValueType> &outBuffer() const
    {
        if (m_newDataAvailable)
        {
            lock();
            m_outBuffer.resize(m_inBuffer.size());
            std::copy(m_inBuffer.begin(), m_inBuffer.end(), m_outBuffer.begin());
            m_newDataAvailable = false;
            unlock();
        }
        return m_outBuffer;
    }

private:
    void lock() const
    {
        while (m_locked.test_and_set(std::memory_order_acquire))
            ;
    }
    void unlock() const { m_locked.clear(std::memory_order_release); }

    std::vector<ValueType> m_inBuffer;
    mutable std::vector<ValueType> m_outBuffer;
    mutable std::atomic_flag m_locked;
    mutable std::atomic<bool> m_newDataAvailable{true};
};

}    // namespace sw::containers
