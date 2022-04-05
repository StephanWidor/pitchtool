#pragma once
#include <algorithm>
#include <complex>
#include <concepts>
#include <ranges>
#include <span>
#include <vector>

namespace sw::ranges {

template<typename R, typename T>
concept TypedRange = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, std::decay_t<T>>;

template<typename R, typename T>
concept TypedInputRange = std::ranges::input_range<R> && std::same_as<std::ranges::range_value_t<R>, std::decay_t<T>>;

// don't know why, but std::ranges::output_range<R, T> still seems to leave ambiguity, e.g. calling
// f(vector<std::complex>) will give compiler error for "ambiguous call..." if
// f(output_range<double>&&) and f(output_range<std::complex<double>>&&) is declared
template<typename R, typename T>
concept TypedOutputRange =
  std::ranges::output_range<R, T> && std::same_as<std::ranges::range_value_t<R>, std::decay_t<T>>;

template<std::ranges::input_range R>
std::vector<std::ranges::range_value_t<R>> to_vector(R &&range)
{
    if constexpr (std::ranges::common_range<R>)
        return std::vector<std::ranges::range_value_t<R>>(range.begin(), range.end());
    else if constexpr (std::ranges::sized_range<R>)
    {
        std::vector<std::ranges::range_value_t<R>> vector(std::ranges::size(range));
        std::ranges::copy(range, vector.begin());
    }
    else
    {
        std::vector<std::ranges::range_value_t<R>> vector;
        std::ranges::copy(range, std::back_inserter(vector));
    }
}

}    // namespace sw::ranges
