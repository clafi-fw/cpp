export module ThisApp.History;

import ClaFi.Controls.Base.SliderBase;

import ClaFi.Core.TextEngine.Text;

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
        // Takes the state an edit left and its name; held steps share a state until one settles.
        void record(const T& state, EditPhase, const Text& what);
        // The state that many steps back, or null where the history does not reach.
        [[nodiscard]] const T* undo(std::size_t steps);
        // The state that many undone steps forward, or null where the history does not reach.
        [[nodiscard]] const T* redo(std::size_t steps);
        [[nodiscard]] std::size_t undoDepth() const { return m_current; }
        [[nodiscard]] std::size_t redoDepth() const;
        [[nodiscard]] bool canUndo() const { return undoDepth() != 0ull; }
        [[nodiscard]] bool canRedo() const { return redoDepth() != 0ull; }
        // What the step that many back is called, the newest at 0.
        [[nodiscard]] const Text& undoStep(std::size_t i) const;
        // What the undone step that many on is called, the nearest at 0.
        [[nodiscard]] const Text& redoStep(std::size_t i) const;
    private:
        // One state and what the step that produced it was called - nothing for the first.
        struct Entry
        {
            T state{};
            Text what{};
        };
        using Entries = RingBuffer<Entry>;
    private:
        // Makes room for one more state, growing towards k_maxStates; full there, the oldest goes.
        void reserveForEntry();
    private:
        // A session of edits deep, and a copy of the subject per step still a few megabytes.
        static constexpr std::size_t k_maxStates = 128;
        static constexpr std::size_t k_firstStates = 8; // what the first state reserves
    private:
        Entries m_entries{ 0 };
        std::size_t m_current{ 0 }; // the state the subject is in
        bool m_runOpen{ false }; // whether the newest state is still the pointer's to move
    };


    //----------------------------------------------------------------------------


    template<typename T>
    void History<T>::reset(const T& state)
    {
        m_entries.clear();
        reserveForEntry();
        m_entries.push_back(Entry{ .state = state });
        m_current = 0ull;
        m_runOpen = false;
    }

    template<typename T>
    void History<T>::record(const T& state, const EditPhase phase, const Text& what)
    {
        if (m_runOpen)
        {
            m_entries[m_current].state = state;
            m_runOpen = phase == EditPhase::Held;
            return;
        }

        // What was undone is dropped: those states branch off a past this edit has left.
        m_entries.resize(m_current + 1ull);
        reserveForEntry();
        if (!m_entries.full())
            ++m_current;
        m_entries.push_back(Entry{ .state = state, .what = what });
        m_runOpen = phase == EditPhase::Held;
    }

    template<typename T>
    const T* History<T>::undo(const std::size_t steps)
    {
        if (steps == 0ull || steps > undoDepth())
            return nullptr;
        m_runOpen = false;
        m_current -= steps;
        return &m_entries[m_current].state;
    }

    template<typename T>
    const T* History<T>::redo(const std::size_t steps)
    {
        if (steps == 0ull || steps > redoDepth())
            return nullptr;
        m_runOpen = false;
        m_current += steps;
        return &m_entries[m_current].state;
    }

    template<typename T>
    std::size_t History<T>::redoDepth() const
    {
        if (m_entries.empty())
            return 0ull;
        return m_entries.size() - m_current - 1ull;
    }

    template<typename T>
    const Text& History<T>::undoStep(const std::size_t i) const
    {
        return m_entries[m_current - i].what;
    }

    template<typename T>
    const Text& History<T>::redoStep(const std::size_t i) const
    {
        return m_entries[m_current + 1ull + i].what;
    }

    template<typename T>
    void History<T>::reserveForEntry()
    {
        if (m_entries.size() != m_entries.capacity())
            return;
        if (m_entries.capacity() == k_maxStates)
            return;

        m_entries.set_capacity(std::min(k_maxStates,
            std::max(k_firstStates, m_entries.capacity() * 2ull)));
    }
}
