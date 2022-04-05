#include <gtest/gtest.h>
#include <list>
#include <sw/dft/transform.hpp>
#include <sw/signals.hpp>

namespace sw::dft::tests {

template<typename F>
class TransformTest : public ::testing::Test
{};

using FloatTypes = ::testing::Types<double, float>;
TYPED_TEST_SUITE(TransformTest, FloatTypes);

namespace {

template<std::floating_point F>
bool isCoefficientsForRealSignal(ranges::TypedInputRange<std::complex<F>> auto &&coefficients)
{
    const auto N = std::ranges::size(coefficients);
    const auto n = N / 2;
    if (!math::isZero(coefficients[0].imag()))
        return false;
    if (!math::isZero(coefficients[n].imag()))
        return false;
    for (auto k = 1u; k < n; ++k)
    {
        const auto &c0 = coefficients[k];
        const auto &c1 = coefficients[N - k];
        if (!math::isZero(c0 - std::conj(c1)))
            return false;
    }
    return true;
}

template<std::floating_point F>
constexpr std::string_view floatString()
{
    if constexpr (std::is_same_v<F, float>)
        return "float";
    else if constexpr (std::is_same_v<F, double>)
        return "double";
    else
        return "unknown";
}

}    // namespace

TYPED_TEST(TransformTest, RealVsComplexValues)
{
    using F = TypeParam;

    auto check = [&](const auto &signal, auto &processor) {
        const auto signalComplex = signal | std::views::transform([](const auto real) {
                                       return std::complex<F>{real, math::zero<F>};
                                   });
        std::vector<std::complex<F>> coefficientsReal(signal.size()), coefficientsComplex(signal.size());

        processor.transform(signalComplex, coefficientsComplex);
        processor.transform(signal, coefficientsReal);

        EXPECT_TRUE(equal<std::complex<F>>(coefficientsReal, coefficientsComplex));
        EXPECT_TRUE(isCoefficientsForRealSignal<F>(coefficientsReal));
        EXPECT_TRUE(isCoefficientsForRealSignal<F>(coefficientsComplex));

        std::vector<std::complex<F>> signalInv(coefficientsReal.size());
        processor.transform_inverse(coefficientsReal, signalInv);
        EXPECT_TRUE(equal<std::complex<F>>(signalComplex, signalInv));

        processor.transform_inverse(coefficientsComplex, signalInv);
        EXPECT_TRUE(equal<std::complex<F>>(signalComplex, signalInv));
    };

    for (auto N = 2u; N <= 256; N *= 2u)
    {
        const auto signalAsVector = makeRandomRealSignal<F>(1.0, N, N);
        std::list<F> signalAsList(signalAsVector.begin(), signalAsVector.end());

        DFT<F> dft(N);
        check(signalAsVector, dft);
        check(signalAsList, dft);

        FFT<F> fft(N);
        check(signalAsVector, fft);
        check(signalAsList, fft);
    }
}

TYPED_TEST(TransformTest, OnlyFirstHalf)
{
    using F = TypeParam;

    auto check = [&](const auto &signal, auto &processor) {
        const auto N = signal.size();
        const auto n = N / 2u + 1u;
        std::vector<std::complex<F>> coefficients(N), coefficientsShort(n);
        processor.transform(signal, coefficients);
        processor.transform(signal, coefficientsShort);
        EXPECT_TRUE(std::equal(coefficientsShort.begin(), coefficientsShort.end(), coefficients.begin(),
                               [&](const auto &v0, const auto &v1) { return sw::math::equal(v0, v1); }));
    };

    for (auto N = 2u; N <= 256; N *= 2u)
    {
        const auto signal = makeRandomRealSignal(static_cast<F>(1), N, N);

        DFT<F> dft(signal.size());
        check(signal, dft);

        FFT<F> fft(signal.size());
        check(signal, fft);
    }
}

TYPED_TEST(TransformTest, RoundtripOfRealSignals)
{
    using F = TypeParam;

    auto check = [&](const std::vector<F> &signal, auto &processor) {
        std::vector<std::complex<F>> coefficients(dft::nyquistLength(signal.size()));
        std::vector<F> signalBack(signal.size());
        processor.transform(signal, coefficients);
        processor.transform_inverse(coefficients, signalBack);
        EXPECT_TRUE(std::equal(signal.begin(), signal.end(), signalBack.begin(),
                               [&](const auto &v0, const auto &v1) { return sw::math::equal(v0, v1); }));
    };

    for (auto N = 2u; N <= 256; N *= 2u)
    {
        const auto signal = makeRandomRealSignal(static_cast<F>(1), N, 2 * N);

        DFT<F> dft(signal.size());
        check(signal, dft);

        FFT<F> fft(signal.size());
        check(signal, fft);
    }
}

TYPED_TEST(TransformTest, CrossValidate)
{
    using F = TypeParam;
    using ComplexSignal = std::vector<std::complex<F>>;

    auto crossValidate = [&](const ComplexSignal &signal) {
        const auto N = signal.size();
        DFT<F> dft(N);
        FFT<F> fft(N);
        ComplexSignal coefficientsDFT(N), coefficientsFFT(N);

        dft.transform(signal, coefficientsDFT);
        fft.transform(signal, coefficientsFFT);
        EXPECT_TRUE(equal<std::complex<F>>(coefficientsDFT, coefficientsFFT));

        ComplexSignal signalInv(N);
        dft.transform_inverse(coefficientsFFT, signalInv);
        EXPECT_TRUE(equal<std::complex<F>>(signal, signalInv));

        fft.transform_inverse(coefficientsDFT, signalInv);
        EXPECT_TRUE(equal<std::complex<F>>(signal, signalInv));
    };

    const auto maxN = 1024;
    for (auto N = 2u; N <= maxN; N *= 2u)
        crossValidate(makeRandomComplexSignal(static_cast<F>(1), N, N));
}

TYPED_TEST(TransformTest, TransformDirac)
{
    using F = TypeParam;

    const auto signal = makeDirac(static_cast<F>(1), 2048u);
    std::vector<std::complex<F>> coefficients(signal.size());
    FFT<F>(signal.size()).transform(signal, coefficients);
    const auto allIsOne =
      std::all_of(coefficients.begin(), coefficients.end(),
                  [oneComplex = std::complex<F>{math::one<F>}](const auto c) { return math::equal(c, oneComplex); });
    EXPECT_TRUE(allIsOne);
}

TYPED_TEST(TransformTest, TransformDirectCurrent)
{
    using F = TypeParam;

    const auto signalLength = 2048u;
    const std::vector<F> signal(signalLength, 1.0);
    std::vector<std::complex<F>> coefficients(signal.size());
    FFT<F>(signal.size()).transform(signal, coefficients);
    EXPECT_TRUE(math::equal(coefficients.front(), std::complex<F>{static_cast<F>(signalLength)}));
    EXPECT_TRUE(std::all_of(
      coefficients.begin() + 1, coefficients.end(),
      [zeroComplex = std::complex<F>{static_cast<F>(0)}](const auto c) { return math::equal(c, zeroComplex); }));
}

TYPED_TEST(TransformTest, SingleSineWaves)
{
    using F = TypeParam;

    constexpr size_t N = 32;
    constexpr auto n = N / 2;
    constexpr auto sampleRate = static_cast<F>(44100.0);
    FFT<F> fft(N);
    const auto frequencies = dft::makeBinFrequencies<F>(N, sampleRate, n);
    std::vector<std::complex<F>> coefficients(N);
    for (auto i = 1u; i < frequencies.size(); ++i)
    {
        const auto infiniteSignal = sineWave(static_cast<F>(1), frequencies[i], sampleRate);
        const auto signal = std::views::counted(infiniteSignal.begin(), static_cast<int>(N));

        fft.transform(signal, coefficients);
        for (auto j = 1u; j < frequencies[j]; ++j)
        {
            if (j == i)
                EXPECT_TRUE(math::equal(coefficients[j].imag(), -static_cast<F>(n)));
            EXPECT_TRUE(math::isZero(coefficients[j] + coefficients[N - j]));
        }
    }
}

}    // namespace sw::dft::tests
