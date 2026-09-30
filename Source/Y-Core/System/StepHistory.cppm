export module ClaFi.Core.System.StepHistory;

import ClaFi.Core.System.RingBuffer;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    // Steps split by a cursor into those in force and those undone. See Control-Foundation
    export template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    class StepHistory
    {
    public:
        [[nodiscard]] bool empty() const { return m_steps.empty(); }
        [[nodiscard]] std::size_t undoDepth() const { return m_next; }
        [[nodiscard]] std::size_t redoDepth() const { return m_steps.size() - m_next; }
        [[nodiscard]] const Step& undoStep(std::size_t i) const; // the newest at 0
        [[nodiscard]] const Step& redoStep(std::size_t i) const; // the nearest at 0
        [[nodiscard]] Step* newestStep(); // the newest step in force, null with none
        [[nodiscard]] Step* openStep(); // the newest step while its run is open, null otherwise
        // Drops what was undone and takes the step as the newest. Answers what the ring let go.
        std::optional<Step> push(Step, bool opensRun);
        void closeRun() { m_open = false; }
        // Moves that many steps from in force to undone, once the owner has taken them back.
        void back(std::size_t steps);
        // Moves that many steps from undone to in force, once the owner has made them again.
        void forward(std::size_t steps);
        void clear();
    private:
        using Steps = RingBuffer<Step>;
    private:
        // Grows the store towards maxSteps. Full there, the next step takes the oldest one's place.
        void reserveForStep();
    private:
        Steps m_steps{ 0 };
        std::size_t m_next{ 0 }; // how many steps are in force, and the index of the next redo
        bool m_open{ false }; // whether the newest step is still open to its run
    };


    //----------------------------------------------------------------------------


    template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    const Step& StepHistory<Step, firstSteps, maxSteps>::undoStep(const std::size_t i) const
    {
        return m_steps[m_next - 1 - i];
    }

    template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    const Step& StepHistory<Step, firstSteps, maxSteps>::redoStep(const std::size_t i) const
    {
        return m_steps[m_next + i];
    }

    template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    Step* StepHistory<Step, firstSteps, maxSteps>::newestStep()
    {
        if (m_next == 0)
            return nullptr;
        return &m_steps[m_next - 1];
    }

    template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    Step* StepHistory<Step, firstSteps, maxSteps>::openStep()
    {
        if (!m_open)
            return nullptr;
        return newestStep();
    }

    template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    std::optional<Step> StepHistory<Step, firstSteps, maxSteps>::push(Step step,
        const bool opensRun)
    {
        // What was undone is dropped: those steps branch off a past this one has left.
        m_steps.resize(m_next);
        reserveForStep();

        std::optional<Step> pushedOut{};
        if (m_steps.full())
        {
            pushedOut = std::move(m_steps.front());
            m_steps.pop_front();
        }
        m_steps.push_back(std::move(step));
        m_next = m_steps.size();
        m_open = opensRun;
        return pushedOut;
    }

    template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    void StepHistory<Step, firstSteps, maxSteps>::back(const std::size_t steps)
    {
        if (steps > m_next)
            unreachable("A history was walked back past its oldest step");
        m_open = false;
        m_next -= steps;
    }

    template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    void StepHistory<Step, firstSteps, maxSteps>::forward(const std::size_t steps)
    {
        if (steps > redoDepth())
            unreachable("A history was walked forward past its newest step");
        m_open = false;
        m_next += steps;
    }

    template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    void StepHistory<Step, firstSteps, maxSteps>::clear()
    {
        // The steps go, the block they stood in stays: a history is cleared because its subject
        // moved, which says nothing about whether it is still being edited.
        m_steps.clear();
        m_next = 0;
        m_open = false;
    }

    template<typename Step, std::size_t firstSteps, std::size_t maxSteps>
    void StepHistory<Step, firstSteps, maxSteps>::reserveForStep()
    {
        if (m_steps.size() != m_steps.capacity())
            return;
        if (m_steps.capacity() == maxSteps)
            return;

        m_steps.set_capacity(std::min(maxSteps, std::max(firstSteps, m_steps.capacity() * 2)));
    }
}
