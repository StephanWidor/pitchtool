#include <gtest/gtest.h>
#include <map>
#include <sw/notes.hpp>

namespace {

const std::map<double, std::string> frequencyNoteMap = {
  {4186.01, "C 8"},  {3951.07, "B 7"},  {3729.31, "A# 7"}, {3520.00, "A 7"},  {3322.44, "G# 7"}, {3135.96, "G 7"},
  {2959.96, "F# 7"}, {2793.83, "F 7"},  {2637.02, "E 7"},  {2489.02, "D# 7"}, {2349.32, "D 7"},  {2217.46, "C# 7"},
  {2093.00, "C 7"},  {1975.53, "B 6"},  {1864.66, "A# 6"}, {1760.00, "A 6"},  {1661.22, "G# 6"}, {1567.98, "G 6"},
  {1479.98, "F# 6"}, {1396.91, "F 6"},  {1318.51, "E 6"},  {1244.51, "D# 6"}, {1174.66, "D 6"},  {1108.73, "C# 6"},
  {1046.50, "C 6"},  {987.767, "B 5"},  {932.328, "A# 5"}, {880.000, "A 5"},  {830.609, "G# 5"}, {783.991, "G 5"},
  {739.989, "F# 5"}, {698.456, "F 5"},  {659.255, "E 5"},  {622.254, "D# 5"}, {587.330, "D 5"},  {554.365, "C# 5"},
  {523.251, "C 5"},  {493.883, "B 4"},  {466.164, "A# 4"}, {440.000, "A 4"},  {438.000, "A 4"},  {415.305, "G# 4"},
  {391.995, "G 4"},  {369.994, "F# 4"}, {349.228, "F 4"},  {329.628, "E 4"},  {311.127, "D# 4"}, {293.665, "D 4"},
  {277.183, "C# 4"}, {261.626, "C 4"},  {246.942, "B 3"},  {233.082, "A# 3"}, {220.000, "A 3"},  {207.652, "G# 3"},
  {195.998, "G 3"},  {184.997, "F# 3"}, {174.614, "F 3"},  {164.814, "E 3"},  {155.563, "D# 3"}, {146.832, "D 3"},
  {138.591, "C# 3"}, {130.813, "C 3"},  {123.471, "B 2"},  {116.541, "A# 2"}, {110.000, "A 2"},  {103.826, "G# 2"},
  {97.9989, "G 2"},  {92.4986, "F# 2"}, {87.3071, "F 2"},  {82.4069, "E 2"},  {77.7817, "D# 2"}, {73.4162, "D 2"},
  {69.2957, "C# 2"}, {65.4064, "C 2"},  {61.7354, "B 1"},  {58.2705, "A# 1"}, {55.0000, "A 1"},  {51.9130, "G# 1"},
  {48.9995, "G 1"},  {46.2493, "F# 1"}, {43.6536, "F 1"},  {41.2035, "E 1"},  {38.8909, "D# 1"}, {36.7081, "D 1"},
  {34.6479, "C# 1"}, {32.7032, "C 1"},  {30.8677, "B 0"},  {29.1352, "A# 0"}, {27.5000, "A 0"},  {25.9565, "G# 0"},
  {24.4997, "G 0"},  {23.1247, "F# 0"}, {21.8268, "F 0"},  {20.6017, "E 0"},  {19.4454, "D# 0"}, {18.3540, "D 0"},
  {17.3239, "C# 0"}, {16.3516, "C 0"}};

}    // namespace

namespace sw::tests {

TEST(NotesTest, backAndForth)
{
    const auto testSingleFrequency = [](double frequency, const std::string &expectedString,
                                        double standardPitch = 440.0) {
        double semitoneDeviation{0.0};
        const auto note = toNote(frequency, standardPitch, &semitoneDeviation);
        const auto stringBack = toString(note);
        EXPECT_EQ(stringBack, expectedString);

        const auto frequencyBack = toFrequency(note, standardPitch, semitoneDeviation);
        EXPECT_TRUE(math::equal(frequencyBack, frequency));

        EXPECT_TRUE(math::isZero(semitoneDeviation, 0.5));
    };

    for (const auto &[frequency, noteString] : frequencyNoteMap)
        testSingleFrequency(frequency, noteString);
}

}    // namespace sw::tests
