#pragma once
#include "sw/basics/ranges.hpp"
#include <array>
#include <concepts>
#include <functional>
#include <tuple>

namespace sw::containers {

template<size_t... I>
constexpr auto makeArray(std::invocable<size_t> auto &&initFunction, std::index_sequence<I...>)
{
    return std::array{initFunction(I)...};
}
template<size_t N>
constexpr auto makeArray(std::invocable<size_t> auto &&initFunction)
{
    return makeArray(initFunction, std::make_index_sequence<N>{});
}

template<size_t N, typename T>
constexpr std::array<T, N> makeFilledArray(const T &value)
{
    return makeArray<N>([&value](size_t) { return value; });
}

template<typename T>
void ringPush(std::vector<T> &io_vector, const T &element, const size_t num = 1u)
{
    const auto size = io_vector.size();
    if (size == 0u)
        return;
    if (num >= size)
        io_vector.clear();
    else
        io_vector.erase(io_vector.begin(), io_vector.begin() + static_cast<int>(num));
    io_vector.resize(size, element);
}

template<typename T>
void ringPush(std::vector<T> &io_vector, ranges::TypedInputRange<T> auto &&elements)
{
    const auto vectorSize = std::ranges::ssize(io_vector);
    const auto numElements = std::ranges::ssize(elements);
    if (numElements >= vectorSize)
    {
        const auto numDiff = numElements - vectorSize;
        std::ranges::copy(elements.begin() + numDiff, elements.end(), io_vector.begin());
    }
    else
    {
        std::copy(io_vector.begin() + numElements, io_vector.end(), io_vector.begin());
        std::ranges::copy(elements, io_vector.end() - numElements);
    }
}

}    // namespace sw::containers
