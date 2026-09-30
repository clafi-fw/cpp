module ClaFi.Controls.HistoryButton;

import ClaFi.Controls.SplitButton;
import ClaFi.Controls.StepsMenu;

import ClaFi.StdActions;

import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    void HistoryButton::dropdown(DropdownEvent& event)
    {
        // The history is the one the face's click would act on: the same walk, from the strip.
        const Action& action = actionOf(m_direction);
        Control* subject = action.subject(event.form, event.control);
        if (!subject)
            return;
        GetEditHistoryEvent request{};
        subject->emitEvent(request);
        if (!request.history)
            return;

        IEditHistory& history = *request.history;
        const bool undoing = m_direction == HistoryDirection::Undo;
        const std::size_t depth = undoing ? history.undoDepth() : history.redoDepth();
        StepsMenu menu{ *event.control, action.text() };
        for (std::size_t i = 0; i != depth; ++i)
        {
            Text step{};
            if (undoing)
                history.writeUndoStep(i, step);
            else
                history.writeRedoStep(i, step);
            menu.add(step);
        }

        const std::size_t taken = menu.executeUnder(event.button());
        if (taken == 0)
            return;
        if (undoing)
            history.undo(taken);
        else
            history.redo(taken);
    }

    Action& HistoryButton::actionOf(const HistoryDirection direction)
    {
        if (direction == HistoryDirection::Undo)
            return StdActions::undo;
        return StdActions::redo;
    }
}
