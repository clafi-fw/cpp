module ClaFi.Controls.Grids;

import :Columns;
import :Descriptor;
import :RowContainer;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Controls::Grids
{
    namespace
    {
        // The keys the grid moves the selection by, and Tab, which leaves the grid. Held with Alt,
        // a key is the cell's control's instead: Alt+Down drops a combobox's list.
        [[nodiscard]] bool gridMovesBy(const KeyDownEvent& event)
        {
            if (event.modifiers.alt)
                return false;
            switch (event.key)
            {
                case Keys::Left:
                case Keys::Right:
                case Keys::Up:
                case Keys::Down:
                case Keys::Home:
                case Keys::End:
                case Keys::Prior:
                case Keys::Next:
                case Keys::Tab:
                    return true;
            }
            return false;
        }
    }

    // RowContainer

    void RowContainer::nestedControlPressed(PressedEvent& event)
    {
        if (keepsPress(*event.control))
        {
            event.stopPropagation();
            return;
        }
        Row::nestedControlPressed(event);
    }

    // The grid completes a pick in the click pass, and it can make only a row current - handed
    // the control, it would read the click as landing on nothing and drop the selection.
    void RowContainer::click(ClickEvent& event)
    {
        event.control = this;
        Row::click(event);
    }

    void RowContainer::keyDown(KeyDownEvent& event)
    {
        Control* control = gridMovesBy(event) ? nullptr : selectedCellControl();
        if (!control)
        {
            Row::keyDown(event);
            return;
        }
        forwardKeyDown(*control, event);
        if (event.handled)
            return;
        // Return and Space press the control that has the focus - see FocusNavigator - and this
        // control has it through the row. Marked handled before the press, which may run a loop
        // this row does not outlive.
        if (event.key == Keys::Return || event.key == Keys::Space)
        {
            event.handled = true;
            control->animatedClick(form(), event.stamp);
            return;
        }
        Row::keyDown(event);
    }

    void RowContainer::charPress(CharPressEvent& event)
    {
        Control* control = selectedCellControl();
        if (!control)
        {
            Row::charPress(event);
            return;
        }
        CharPressEvent forwarded{ event.formContext(), *control, event.character() };
        forwardCharPress(*control, forwarded);
    }

    void RowContainer::contextPopup(ContextPopupEvent& event)
    {
        Control* control = event.mousePos ? nullptr : selectedCellControl();
        if (control && event.control == this)
        {
            event.control = control;
            forwardContextPopup(*control, event);
            if (event.propagationStopped())
                return;
        }
        Row::contextPopup(event);
    }

    Control* RowContainer::selectedCellControl()
    {
        const Column* column = descriptor().selectedColumn();
        if (descriptor().selectedRow() != this || !column)
            return nullptr;
        Control* control = controlAtColumn(*column);
        // A disabled control could not hold the focus, so it is handed no key either.
        if (!control || !control->enabled(true))
            return nullptr;
        return control;
    }

    // A part with a command of its own, a dropdown strip, still acts on the press that picks its
    // cell. Nothing in the cell acts on a press that changes the selection.
    bool RowContainer::keepsPress(const Control& acting) const
    {
        for (const auto& [column, index] : m_controlMap)
        {
            const Control& hosted = *m_controls[index];
            if (!hosted.containsNested(acting))
                continue;
            switch (pressOn(*column))
            {
                case CellPressKind::Pick:
                    return &acting == &hosted;
                case CellPressKind::Select:
                    return true;
                case CellPressKind::Act:
                    return false;
            }
        }
        return false;
    }
}
