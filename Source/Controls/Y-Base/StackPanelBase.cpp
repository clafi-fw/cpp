module ClaFi.Controls.Base.StackPanelBase;

import ClaFi.Controls.Base.Container;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // CanFocusItemEvent

    CanFocusItemEvent::CanFocusItemEvent(StackPanelBase& itemsView,
        const Control& item, bool canFocus)
        :
        itemsView{ itemsView },
        item{ item },
        canFocus{ canFocus }
    {
    }

    // CurrentItemChangeEvent

    CurrentItemChangeEvent::CurrentItemChangeEvent(StackPanelBase& itemsView, Control* previousItem)
        :
        itemsView{ itemsView },
        previousItem{ previousItem }
    {
    }

    // StackPanelBase

    void StackPanelBase::setCurrentItem(Control* item)
    {
        Control* previousItem = currentItem();
        recordCurrentItem(item);
        // No modifiers, because there was no gesture to carry - nothing else separates a pick
        // made here from one the user made.
        currentItemPicked(item, previousItem, {}, true);
        // The reveal is requested, not measured here. A caller picking an item programmatically is
        // usually still building the view around it, and whatever it shows or hides next only
        // invalidates the alignment - so where the item sits is the pass's to give.
        if (item)
            item->scrollIntoViewOnAlign();
    }

    void StackPanelBase::recordCurrentItem(Control* value)
    {
        if (m_currentItem == value)
            return;

        if (isNotNullAndCannotBeCurrent(value))
            return;

        Control* previousValue = m_currentItem;
        m_currentItem = value;

        if (previousValue)
            previousValue->invalidateState();

        if (m_currentItem)
            m_currentItem->invalidateState();

        doCurrentItemChanged(previousValue);
    }

    bool StackPanelBase::canFocusItem(Control* value)
    {
        if (!value)
            return true;

        CanFocusItemEvent event{
            *this,
            *value,
            defaultCanFocusItem(*value)
        };
        emitEvent(event);
        return event.canFocus;
    }

    Control* StackPanelBase::itemAt(Control* control)
    {
        while (control && control != this)
        {
            if (canFocusItem(control))
                return control;

            control = control->parent();
        }
        return nullptr;
    }

    bool StackPanelBase::isNotNullAndCannotBeCurrent(Control* value)
    {
        return value && !canFocusItem(value);
    }

    void StackPanelBase::currentItemChanged(CurrentItemChangeEvent&)
    {
    }

    // A container that does not move its current item itself has none to offer, and inside one
    // that does, an item is a control the user can land on rather than a caption or a panel
    // sitting between them.
    bool StackPanelBase::defaultCanFocusItem(Control& value)
    {
        return followsUser() && value.interactivity() == Interactivity::Focusable;
    }

    bool StackPanelBase::isItemSelected(const Control& item) const
    {
        return m_currentItem == &item;
    }

    void StackPanelBase::nestedControlDeleted(Control* item)
    {
        Container::nestedControlDeleted(item);
        if (item == m_currentItem)
        {
            m_currentItem = nullptr;
            doCurrentItemChanged(item);
        }
    }

    void StackPanelBase::getControlState(GetStateEvent& event) const
    {
        Container::getControlState(event);
        if (event.propagationStopped())
            return;

        // The container that moves the current item is the one that answers for which item is
        // selected. One that does not leaves the state to whoever does.
        if (!followsUser())
            return;

        event.state.selected = isItemSelected(event.control);
        event.stopPropagation();
    }

    Control* StackPanelBase::focusDelegate()
    {
        return m_currentItem ? m_currentItem : this;
    }

    void StackPanelBase::click(ClickEvent& event)
    {
        //if (Input::device() == InputDevice::Keyboard)
        {
            // SetFocus handles this on mouse clicks
            bool handled{};
            autoSelectItem(*event.control, event.modifiers, handled);
        }
        Container::click(event);
    }

    void StackPanelBase::nestedControlFocusing(FocusEvent& event)
    {
        bool handled = (event.control == this || event.control == currentItem());
        if (!handled)
        {
            const KeyModifiers modifiers = event.modifiers;
            const bool applyGesture = appliesGesture(*event.control, modifiers);
            autoSelectItem(*event.control, modifiers, handled, applyGesture);
        }
        if (handled)
        {
            event.control = this;
        }
        Container::nestedControlFocusing(event);
    }

    bool StackPanelBase::followsUser() const
    {
        return interactivity() == Interactivity::ActiveContainer;
    }

    void StackPanelBase::autoSelectItem(Control& item, KeyModifiers modifiers, bool& handled, bool applyGesture)
    {
        if (!followsUser())
            return;

        // Nothing is recorded when the user reaches something that cannot be current, so the
        // current item is still whatever it was - which is also what a handler would call the
        // previous one, there being no move between them.
        if (!canFocusItem(&item))
        {
            currentItemPicked(nullptr, currentItem(), modifiers, applyGesture);
            return;
        }

        handled = true;
        Control* previousItem = currentItem();
        recordCurrentItem(&item);
        currentItemPicked(&item, previousItem, modifiers, applyGesture);
    }

    void StackPanelBase::doCurrentItemChanged(Control* previous)
    {
        CurrentItemChangeEvent event{ *this, previous };
        currentItemChanged(event);
        emitEvent(event);
    }

    // Whether the focus pass is the one that reads a gesture whole. Both devices name a place
    // with one action and give the command with another wherever the two are separable, and
    // this answers which of the two the container is looking at.
    //
    // Ctrl on the keyboard separates them: a navigation key under Ctrl moves the current item
    // and issues no selection command, and Space or Return is the command it sets up - so
    // Ctrl+arrow walks the items and Ctrl+Space toggles the one walked to. Shift is not
    // separable that way: a range is named by the item it ends on, so the navigation key that
    // ends it IS the command, and Ctrl+Shift+arrow extends the range without giving up what
    // stands.
    //
    // A mouse press waits for the click pass wherever what it means depends on what the
    // pointer does next:
    //
    // - a press carrying a modifier may become a rubber band, which adds a rectangle rather
    //   than naming a range or toggling the one item
    // - a press INSIDE the selection may become a drag of the whole selection, and a press
    //   that acted would collapse the selection to the one item under the pointer first
    //
    // A plain press outside the selection means the same thing whatever follows it, so it is
    // read here and the item is selected under the pointer before any drag of it begins.
    //
    // For a container keeping a selection of one this is the current item, and a press on that
    // never reaches here - adjustFocus answers it before asking.
    bool StackPanelBase::appliesGesture(const Control& item, KeyModifiers modifiers) const
    {
        if (Input::device() != InputDevice::Mouse)
            return modifiers.shift || !modifiers.ctrl;

        if (modifiers.ctrl || modifiers.shift)
            return false;

        return !isItemSelected(item);
    }

}
