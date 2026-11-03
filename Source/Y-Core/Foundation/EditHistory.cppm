export module ClaFi.Core.Foundation.EditHistory;

import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.StepHistory;

import ClaFi.StdLib;

namespace ClaFi
{
    // Where an edit stands: still held, or complete. See Control-Foundation
    export enum class EditPhase
    {
        Held,    // the pointer or the edit mode holds the value, and nothing records it yet
        Settled  // a step of its own, or the one step a hold makes as it ends
    };

    // What the undo list and the undo actions read of a history. See Control-Foundation
    export class IEditHistory
    {
    public:
        virtual ~IEditHistory() = default;
    public:
        [[nodiscard]] virtual std::size_t undoDepth() const = 0;
        [[nodiscard]] virtual std::size_t redoDepth() const = 0;
        virtual void writeUndoStep(std::size_t i, Text&) const = 0; // the newest at 0
        virtual void writeRedoStep(std::size_t i, Text&) const = 0; // the nearest at 0
        // Takes that many steps back; more than undoDepth takes none.
        virtual void undo(std::size_t steps) = 0;
        // Does that many undone steps again; more than redoDepth does none.
        virtual void redo(std::size_t steps) = 0;
    };

    // The history a control's undo and redo act on, asked of their subject. See Control-Foundation
    export struct GetEditHistoryEvent : public Event
    {
        IEditHistory* history{};   // the answer, null until a control gives one
    };

    // The states a subject has been through, a whole copy per step. See Control-Foundation
    export template<typename State, typename Place>
    class StateHistory
    {
    public:
        // Where a walk lands: the state to put on, and where the last step it crossed was made.
        struct Landing
        {
            const State* state{};
            const Place* place{};
            [[nodiscard]] explicit operator bool() const { return state != nullptr; }
        };
    public:
        // Forgets every step and stands on one state, the first an undo can reach.
        void reset(const State&);
        // Takes the state an edit left, its name and where it was made; a held edit makes none.
        void record(const State&, EditPhase, const Text& what, Place);
        [[nodiscard]] Landing undo(std::size_t steps);
        [[nodiscard]] Landing redo(std::size_t steps);
        [[nodiscard]] std::size_t undoDepth() const { return m_steps.undoDepth(); }
        [[nodiscard]] std::size_t redoDepth() const { return m_steps.redoDepth(); }
        [[nodiscard]] bool canUndo() const { return undoDepth() != 0; }
        [[nodiscard]] bool canRedo() const { return redoDepth() != 0; }
        [[nodiscard]] const Text& undoStep(std::size_t i) const; // the newest at 0
        [[nodiscard]] const Text& redoStep(std::size_t i) const; // the nearest at 0
    private:
        // The state a step left, what it was called and where it was made.
        struct Step
        {
            State state{};
            Text what{};
            Place place{};
        };
    private:
        // A session of edits deep, and a copy of the subject per step still a few megabytes.
        static constexpr std::size_t k_maxStates = 128;
        static constexpr std::size_t k_firstStates = 8; // what the first step reserves
    private:
        using Steps = StepHistory<Step, k_firstStates, k_maxStates>;
    private:
        State m_first{}; // the state before the oldest step, where undo bottoms out
        Steps m_steps{};
    };


    //----------------------------------------------------------------------------


    template<typename State, typename Place>
    void StateHistory<State, Place>::reset(const State& state)
    {
        m_steps.clear();
        m_first = state;
    }

    template<typename State, typename Place>
    void StateHistory<State, Place>::record(const State& state, const EditPhase phase,
        const Text& what, Place place)
    {
        if (phase == EditPhase::Held)
            return;
        std::optional<Step> pushedOut = m_steps.push(Step{
            .state = state,
            .what = what,
            .place = std::move(place),
        }, false); // a hold reports its one step as it ends, so no run is left open
        if (pushedOut.has_value())
            m_first = std::move(pushedOut->state);
    }

    // Lands on where the oldest step undone was made, which is where the user was left.
    template<typename State, typename Place>
    typename StateHistory<State, Place>::Landing StateHistory<State, Place>::undo(
        const std::size_t steps)
    {
        if (steps == 0 || steps > undoDepth())
            return {};
        const Place* place = &m_steps.undoStep(steps - 1).place;
        m_steps.back(steps);
        const State* state = undoDepth() == 0 ? &m_first : &m_steps.undoStep(0).state;
        return { state, place };
    }

    // Lands on where the newest step redone was made.
    template<typename State, typename Place>
    typename StateHistory<State, Place>::Landing StateHistory<State, Place>::redo(
        const std::size_t steps)
    {
        if (steps == 0 || steps > redoDepth())
            return {};
        m_steps.forward(steps);
        const Step& newest = m_steps.undoStep(0);
        return { &newest.state, &newest.place };
    }

    template<typename State, typename Place>
    const Text& StateHistory<State, Place>::undoStep(const std::size_t i) const
    {
        return m_steps.undoStep(i).what;
    }

    template<typename State, typename Place>
    const Text& StateHistory<State, Place>::redoStep(const std::size_t i) const
    {
        return m_steps.redoStep(i).what;
    }
}
