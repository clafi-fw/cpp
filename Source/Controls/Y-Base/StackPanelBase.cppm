module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Base.StackPanelBase;

import ClaFi.Controls.Base.Container;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export class StackPanelBase;

    // May this item be the container's current one? See Item-Containers
    export struct CanFocusItemEvent : public Event
    {
        CanFocusItemEvent(StackPanelBase&, const Control&, bool canFocus);
        StackPanelBase& itemsView;   // the container asking
        const Control& item;         // the item the answer is about
        bool canFocus;               // the answer, as the container gave it before any handler
    };

    // The container whose current item has just changed.
    export struct CurrentItemChangeEvent : public Event
    {
        CurrentItemChangeEvent(StackPanelBase& itemsView, Control* previousItem);
        StackPanelBase& itemsView;   // the container the current item belongs to
        Control* previousItem;       // what was current before, null where nothing was
    };

    // A base for StackPanel and PageControl, owning the current item. See Item-Containers
    export class StackPanelBase : public Container
    {
    public:
        template <typename... Args>
        explicit StackPanelBase(const CreateParams&, Args&&...);
    public:
        // Asked before an item becomes the current one.
        DECLARE_EVENT(CanFocusItemEvent, OnCanFocusItem, onCanFocusItem)
        // Raised after the current item has changed.
        DECLARE_EVENT(CurrentItemChangeEvent, OnCurrentItemChange, onCurrentItemChange)
    public:
        // The one item this container treats as current - the page a PageControl shows, the tab
        // a TabStrip has open, the item the focus goes to when the container is entered. Current
        // is not focused: it stays current while the focus is somewhere else entirely, which is
        // why the cue for it, VisualStateIndex::Current, is not gated on the keyboard.
        [[nodiscard]] Control* currentItem() const { return m_currentItem; }
        // Makes item the current one from code: records it, tells the container the user was not
        // what moved it, and scrolls it into view. This is the one to call from outside.
        void setCurrentItem(Control*);
        void setCurrentItem(Control& item) { setCurrentItem(&item); }
    protected:
        // Records the change and nothing more - the state of both items is invalidated and the
        // change event raised. Every path funnels through here, a click and a key as much as
        // setCurrentItem, so whatever a particular path owes on top of it stays with that path.
        void recordCurrentItem(Control*);
        void recordCurrentItem(Control& value) { recordCurrentItem(&value); }
        // The answer after the handlers have had it. Nullptr answers yes: clearing the current
        // item is always allowed, and it is not an item to ask about.
        [[nodiscard]] bool canFocusItem(Control*);
        // The item this control belongs to: itself where the container takes it for one, and the
        // nearest ancestor below the container that it does otherwise. Whatever the pointer or the
        // focus names is a leaf, and an item built out of parts is not that leaf - nor is a
        // container's direct child, where the items are grouped into panels inside it.
        //
        // Nullptr where nothing on the way up is an item: the control is a caption or a panel
        // between them, or it is not inside this container at all.
        [[nodiscard]] Control* itemAt(Control*);
    protected:
        [[nodiscard]] bool isNotNullAndCannotBeCurrent(Control*);
        virtual void currentItemChanged(CurrentItemChangeEvent&);
        // The answer before any handler sees it.
        virtual bool defaultCanFocusItem(Control&);
        // The user's gesture, or a call to setCurrentItem, settled on item - and a container
        // that keeps more than one item brings its selection in line here. StackView grows,
        // toggles or replaces it.
        //
        // The gesture comes with it, because that is the whole of what there is to read: Shift
        // extends a range from previousItem, Ctrl adds or removes the one item, neither replaces
        // the set with it. A pick made from code carries no modifiers and so lands on the last
        // of those - which is all "from code" ever meant, and why it needs no hook of its own.
        //
        // item is null when nothing was picked: the user reached empty space or a caption
        // between the items, or setCurrentItem was passed nullptr. Those two differ on the base
        // side - the first records nothing, so the current item is still previousItem, while the
        // second really did clear it - but neither leaves a selection anything to anchor on, so
        // they answer alike.
        // applyGesture says this pass is the one that reads the gesture whole - see
        // appliesGesture for which pass that is. A pass that does not apply the gesture still
        // moved the current item, and a handler is told either way.
        virtual void currentItemPicked(Control* /*item*/, Control* /*previousItem*/, KeyModifiers, bool /*applyGesture*/) {}
        // Which items this container reports as selected. The current item is the answer here
        // unless a derived class keeps a selection set of its own.
        [[nodiscard]] virtual bool isItemSelected(const Control&) const;
        void nestedControlDeleted(Control*) override;
        void getControlState(GetStateEvent&) const override;
        Control* focusDelegate() override;
        void click(ClickEvent&) override;
        void nestedControlFocusing(FocusEvent&) override;
    private:
        // Whether this container moves its current item itself, following whatever the user acts
        // on - a click, or the focus arriving from the keyboard - and so also answers for which
        // of its items is selected. That is what Interactivity::ActiveContainer means, in its
        // own words: a container that manages focus traversal and child selection. A container
        // saying it is one and then not tracking anything is the two halves disagreeing, so
        // there is nothing else to set.
        //
        // A container whose current item is settled elsewhere is not one: PageControl's is
        // whichever page was made visible, and the tab strip is what makes one visible.
        [[nodiscard]] bool followsUser() const;
        void autoSelectItem(Control&, KeyModifiers, bool& handled, bool applyGesture = true);
        void doCurrentItemChanged(Control* previous);
    private:
        [[nodiscard]] bool appliesGesture(const Control& item, KeyModifiers) const;
    private:
        Control* m_currentItem{ nullptr };
    };


    // ------------------------------------------------------------------------


    // StackPanelBase
    //
    // The constructor is the one definition that has to stay here: it is a template, so every
    // caller instantiates it from this interface. Every other body lives in StackPanelBase.cpp.

    template<typename ...Args>
    StackPanelBase::StackPanelBase(const CreateParams& params, Args && ...args)
        :
        Container{ params, std::forward<Args>(args)...}
    {
    }

}
