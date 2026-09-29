export module ThisApp.History;

import ClaFi.Controls.Base.SliderBase;

import ClaFi.Core.System.RingBuffer;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;

    // Where an edit stands: still under the pointer, or complete.
    export enum class EditPhase
    {
        Held,    // the pointer holds the value, and every step joins the one before it
        Settled  // the step stands on its own, or closes the run the pointer made
    };

    // The phase a slider's change reports: held while the pointer has the position.
    export [[nodiscard]] EditPhase editPhaseOf(const Controls::SliderBase&);

    // The states a subject has been through, one whole copy each, walked by undo and redo.
    export template<typename T>
    class History
    {
    public:
        // Forgets every step and stands on one state, the first an undo can reach.
        void reset(const T& state);
        // Takes the state an edit left. Held steps share a state until a settled one ends the run.
        void record(const T& state, EditPhase);
        [[nodiscard]] const T* undo(); // the state before the newest step, or null with none
        [[nodiscard]] const T* redo(); // the state the newest undone step left, or null with none
        [[nodiscard]] bool canUndo() const { return m_current != 0; }
        [[nodiscard]] bool canRedo() const { return m_current + 1 < m_states.size(); }
    private:
        using States = RingBuffer<T>;
    private:
        // Makes room for one more state, growing towards k_maxStates; full there, the oldest goes.
        void reserveForState();
    private:
        // A session of edits deep, and a copy of the subject per step still a few megabytes.
        static constexpr std::size_t k_maxStates = 128;
        static constexpr std::size_t k_firstStates = 8; // what the first state reserves
    private:
        States m_states{ 0 };
        std::size_t m_current{ 0 }; // the state the subject is in
        bool m_runOpen{ false }; // whether the newest state is still the pointer's to move
    };


    //----------------------------------------------------------------------------


    template<typename T>
    void History<T>::reset(const T& state)
    {
        m_states.clear();
        reserveForState();
        m_states.push_back(state);
        m_current = 0;
        m_runOpen = false;
    }

    template<typename T>
    void History<T>::record(const T& state, const EditPhase phase)
    {
        if (m_runOpen)
        {
            m_states[m_current] = state;
            m_runOpen = phase == EditPhase::Held;
            return;
        }

        // What was undone is dropped: those states branch off a past this edit has left.
        m_states.resize(m_current + 1ull);
        reserveForState();
        if (!m_states.full())
            ++m_current;
        m_states.push_back(state);
        m_runOpen = phase == EditPhase::Held;
    }

    template<typename T>
    const T* History<T>::undo()
    {
        if (!canUndo())
            return nullptr;
        m_runOpen = false;
        --m_current;
        return &m_states[m_current];
    }

    template<typename T>
    const T* History<T>::redo()
    {
        if (!canRedo())
            return nullptr;
        m_runOpen = false;
        ++m_current;
        return &m_states[m_current];
    }

    template<typename T>
    void History<T>::reserveForState()
    {
        if (m_states.size() != m_states.capacity())
            return;
        if (m_states.capacity() == k_maxStates)
            return;

        m_states.set_capacity(std::min(k_maxStates,
            std::max(k_firstStates, m_states.capacity() * 2ull)));
    }
}
