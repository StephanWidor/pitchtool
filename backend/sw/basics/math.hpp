#pragma once
#include <cmath>
#include <complex>
#include <concepts>
#include <numbers>
#include <type_traits>

namespace sw::math {

template<typename C>
concept Complex = std::same_as<std::decay_t<C>, std::complex<typename C::value_type>>;

template<typename T>
concept Number = std::is_floating_point_v<T> || std::is_integral_v<T>;

template<typename T>
struct underlying;

template<Number N>
struct underlying<N>
{
    using Type = N;
};

template<Complex C>
struct underlying<C>
{
    using Type = typename C::value_type;
};

template<typename T>
using underlying_t = typename underlying<T>::Type;

template<std::floating_point F = double>
constexpr F pi = std::numbers::pi_v<F>;

template<std::floating_point F = double>
constexpr F twoPi = static_cast<F>(2.0 * pi<double>);

template<std::floating_point F = double>
constexpr F piHalf = static_cast<F>(0.5 * pi<double>);

template<Number N>
constexpr N zero = static_cast<N>(0);

template<Number N>
constexpr N one = static_cast<N>(1);

template<std::floating_point F>
constexpr F oneHalf = static_cast<F>(0.5);

namespace detail {

template<typename T>
struct DefaultTolerance;

template<Number N>
struct DefaultTolerance<N>
{
    using Type = underlying_t<N>;
    static constexpr Type value = static_cast<Type>(1e5) * std::numeric_limits<Type>::epsilon();
};

template<>
struct DefaultTolerance<float>
{
    using Type = float;
    static constexpr Type value = 1e-5f;
};

template<Complex C>
struct DefaultTolerance<C>
{
    using Type = underlying_t<C>;
    static constexpr Type value = DefaultTolerance<Type>::value;
};

}    // namespace detail

template<typename T>
constexpr underlying_t<T> defaultTolerance = detail::DefaultTolerance<T>::value;

template<typename T>
constexpr underlying_t<T> defaultToleranceQ = detail::DefaultTolerance<T>::value *detail::DefaultTolerance<T>::value;

template<Complex C>
constexpr bool isZero(const C &c, const underlying_t<C> tolerance = defaultTolerance<C>)
{
    return std::norm(c) <= tolerance * tolerance;
}

template<typename T>
constexpr bool isZero(const T &t, const underlying_t<T> tolerance = defaultTolerance<T>)
{
    return std::abs(t) <= tolerance;
}

template<typename T>
constexpr bool equal(const T &f0, const T &f1, const underlying_t<T> tolerance = defaultTolerance<T>)
{
    return isZero<T>(f0 - f1, tolerance);
}

constexpr bool isPowerOfTwo(const std::integral auto x)
{
    return (x != 0 && (x & (x - 1)) == 0);
}

template<std::floating_point F>
constexpr F maxRatio(const F f0, const F f1)
{
    return f0 > f1 ? f0 / f1 : f1 / f0;
}

template<typename T>
void safeAssign(const T &from, T *o_to)
{
    if (o_to != nullptr)
        *o_to = from;
}

}    // namespace sw::math
