#include <gtest/gtest.h>
#include <random>
#include <sw/phases.hpp>

namespace sw::tests {

template<std::floating_point F>
class PhasesTest : public ::testing::Test
{};
using FloatTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(PhasesTest, FloatTypes);

TYPED_TEST(PhasesTest, standardized)
{
    using F = TypeParam;

    const auto checkAngle = [](const F angle) {
        const auto expectedPhase = std::arg(std::polar(math::one<F>, angle));
        const auto phase = standardized(angle);
        EXPECT_NEAR(phase, expectedPhase, math::defaultTolerance<F>);
    };

    std::mt19937 randGen(1u);
    std::uniform_real_distribution distribution(static_cast<F>(-100), static_cast<F>(100));

    for (auto i = 0u; i < 100u; ++i)
        checkAngle(distribution(randGen));
}

}    // namespace sw::tests
