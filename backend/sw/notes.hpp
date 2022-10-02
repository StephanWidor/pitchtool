#pragma once
#include "sw/basics/math.hpp"
#include <array>
#include <cstdint>
#include <string>

namespace sw {

template<std::floating_point F = double>
constexpr F logSemitone = static_cast<F>(1.0 / 12.0);

#ifdef __GNUC__
template<std::floating_point F = double>
constexpr F semitoneRatio = std::pow(static_cast<F>(2), logSemitone<F>);

template<std::floating_point F = double>
constexpr F sqrtSemitoneRatio = std::sqrt(semitoneRatio<F>);
#else
template<std::floating_point F = double>
const F semitoneRatio = std::pow(static_cast<F>(2), logSemitone<F>);

template<std::floating_point F = double>
const F sqrtSemitoneRatio = std::sqrt(semitoneRatio<F>);
#endif

template<std::floating_point F>
F semitonesToFactor(const F semitones)
{
    return std::pow(static_cast<F>(2), semitones * logSemitone<F>);
}

template<std::floating_point F>
F factorToSemitones(const F factor)
{
    return factor <= math::zero<F> ? -std::numeric_limits<F>::infinity() : std::log2(factor) * static_cast<F>(12);
}

template<std::floating_point F>
constexpr F isHarmonic(const F fundamentalFrequency, const F otherFrequency,
                       const F ratioTolerance = sqrtSemitoneRatio<F>)
{
    return math::maxRatio(std::round(otherFrequency / fundamentalFrequency) * fundamentalFrequency, otherFrequency) <=
           ratioTolerance;
}

struct Note
{
    enum class Name : std::uint8_t
    {
        C = 0,
        Cis = 1,
        D = 2,
        Dis = 3,
        E = 4,
        F = 5,
        Fis = 6,
        G = 7,
        Gis = 8,
        A = 9,
        Ais = 10,
        H = 11,
        Invalid = 12
    };
    static constexpr std::array<std::string_view, 13> nameStrings{"C", "C#", "D", "D#", "E", "F", "F#",
                                                                  "G", "G#", "A", "A#", "B", ""};

    Name name{Name::A};
    int level{4};
};

inline std::string toString(const Note &note)
{
    return note.name == Note::Name::Invalid ?
             "" :
             std::string(Note::nameStrings[static_cast<size_t>(note.name)]) + " " + std::to_string(note.level);
}

constexpr int toMidi(const Note &note)
{
    return (note.level + 1) * 12 + static_cast<int>(note.name);
}

constexpr Note fromMidi(const int midiNumber)
{
    return {Note::Name(midiNumber > 0 ? midiNumber % 12 : midiNumber % 12 + 12), (midiNumber / 12) - 1};
}

template<std::floating_point F>
constexpr F midiPitchBendToSemitones(const int pitchBend, const F fullBendInSemitones = static_cast<F>(2))
{
    return fullBendInSemitones * static_cast<F>(pitchBend - 8192) / static_cast<F>(8192);
}

template<std::floating_point F>
Note toNote(const F frequency, const F standardPitch = static_cast<F>(440), F *o_semitoneDeviation = nullptr)
{
    if (frequency <= math::zero<F> || standardPitch <= math::zero<F>)
    {
        math::safeAssign(math::zero<F>, o_semitoneDeviation);
        return {Note::Name::Invalid, 0u};
    }
    const auto diffSemitones = factorToSemitones(frequency / standardPitch);
    const auto roundDiffSemitones = std::round(diffSemitones);
    math::safeAssign(diffSemitones - roundDiffSemitones, o_semitoneDeviation);
    return fromMidi(69 + static_cast<int>(roundDiffSemitones));
}

template<std::floating_point F>
F toFrequency(const Note &note, const F standardPitch = static_cast<F>(440), const F semitoneDeviation = math::zero<F>)
{
    const auto diffSemitones = static_cast<F>(toMidi(note) - 69) + semitoneDeviation;
    return standardPitch * semitonesToFactor(diffSemitones);
}

}    // namespace sw
