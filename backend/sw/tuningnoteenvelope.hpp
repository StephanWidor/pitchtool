#pragma once
#include "sw/notes.hpp"

namespace sw {

template<std::floating_point F>
class TuningNoteEnvelope
{
public:
    /// Going from 0 to 1 in attackTime if note is not invalid and stays the same, otherwise resetting to 0
    F process(const Note note, const F attackTime, const F timeDiff)
    {
        assert(math::zero<F> <= attackTime);
        assert(timeDiff > math::zero<F>);
        if (note.name == Note::Name::Invalid || note.name != m_currentNote.name || note.level != m_currentNote.level)
            m_elapsed = math::zero<F>;
        else
            m_elapsed += timeDiff;
        m_currentNote = note;
        if (m_elapsed < attackTime)
            return math::oneHalf<F> - math::oneHalf<F> * std::cos(math::pi<F> * m_elapsed / attackTime);
        return math::one<F>;
    }

private:
    Note m_currentNote;
    F m_elapsed{math::zero<F>};
};

}    // namespace sw
