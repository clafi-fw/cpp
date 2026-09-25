module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.StackView;

import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Whether a view holds selected items on top of the current item. See Selection-Model
    export enum class SelectionMode
    {
        None,   // the current item is the only one marked
        Multi   // any number of items are held selected alongside the current item
    };

    // What a drag that starts on an item does. See Selection-Model
    export enum class DragMode
    {
        EasySelect,   // an item is one more place to start a selection rectangle from
        EasyDrag      // an item is the selection's handle, and the drag carries what is selected
    };

    export class StackView;

    export class ItemsViewSelection
    {
        friend StackView;
    public:
        using UnorderedSet = std::unordered_set<Control*>;
    public:
        ItemsViewSelection(StackView&);
        [[nodiscard]] bool empty() const { return m_set.empty(); }
        [[nodiscard]] UnorderedSet::iterator begin() { return m_set.begin(); }
        [[nodiscard]] UnorderedSet::iterator end() { return m_set.end(); }
        [[nodiscard]] const UnorderedSet::iterator begin() const { return m_set.begin(); }
        [[nodiscard]] const UnorderedSet::iterator end() const { return m_set.end(); }
        [[nodiscard]] bool contains(const Control* value) const;
    private:
        void changed() const;
        bool tryAdd(Control&);
        bool tryRemove(Control&);
        bool tryClear();
    private:
        StackView& m_owner;
        UnorderedSet m_set{};
    };

    // The view whose set of selected items has just changed.
    export struct SelectionChangeEvent : public Event
    {
        explicit SelectionChangeEvent(StackView& control);
        StackView& control;   // the view the selection belongs to
    };

    // May this item join the selection? See Selection-Model
    export struct CanSelectItemEvent : public Event
    {
        CanSelectItemEvent(StackView&, const Control&, bool canSelect);
        StackView& itemsView;   // the view asking
        const Control& item;    // the item the answer is about
        bool canSelect;         // the answer, as the view itself gave it before any handler saw it
    };

    // A StackPanel that traps child focus and holds several items selected. See Selection-Model
    export class StackView : public StackPanel
    {
        friend ItemsViewSelection;
    public:
        using StackPanel::recordCurrentItem;
    public:
        template <typename... Args>
        explicit StackView(const CreateParams& params, Args&&... args);
    public:
        // Whether the view holds a selection on top of the current item.
        DECLARE_PROPERTY(SelectionMode, selectionMode, SelectionMode::None)
        // What a drag that starts on an item does.
        DECLARE_PROPERTY(DragMode, dragMode, DragMode::EasySelect)
    public:
        // Raised after the set of selected items has changed.
        DECLARE_EVENT(SelectionChangeEvent, OnSelectionChange, onSelectionChange)
        // Asked before an item joins the selection.
        DECLARE_EVENT(CanSelectItemEvent, OnCanSelectItem, onCanSelectItem)
    public:
        std::wstring_view diagnosticText() const override { return L"StackView"; }
        [[nodiscard]] ItemsViewSelection& selection() { return m_selection; }
    protected:
        void toggleItemSelection(Control*, bool keepSelection = false);
        // A drag that started on an item, under DragMode::EasyDrag. Called on every move of
        // that drag, so it both begins the carry and continues it.
        virtual void dragSelection(DragEvent&) {}
        void selectRange(Control* anchor, Control* item, bool keepSelection = false);
        virtual void selectionChanged() {}
        // The answer after the handlers have had it; nullptr answers no, there being nothing
        // to add. defaultCanSelectItem is the answer before any handler sees it.
        [[nodiscard]] bool canSelectItem(Control*);
        virtual bool defaultCanSelectItem(Control&);
        void currentItemPicked(Control*, Control* previousItem, KeyModifiers, bool applyGesture) override;
        [[nodiscard]] bool isItemSelected(const Control&) const override;
        void adjustNestedControlVisualState(const Control&, VisualState&) const override;
        void nestedControlDeleted(Control*) override;
        void drag(DragEvent&) override;
        void pressUp(PressUpEvent&) override;
        void paintChildren(PaintEvent&) override;
    private:
        [[nodiscard]] bool isMultiSelect() const { return m_selectionMode == SelectionMode::Multi; }
        void doSelectionChanged();
        // Where the next Shift range starts, with the set it is measured against. Taken after
        // the pick has had its say, so an item the same gesture added is part of what a later
        // Ctrl+Shift range builds on.
        void recordSelectionAnchor(Control*);
        // The item a press landed on: the control itself where the view takes that as an item,
        // and the nearest ancestor it does otherwise. A press names whichever leaf is under the
        // pointer, and an item built out of parts is not that leaf. nullptr says the press
        // landed on the surface.
        [[nodiscard]] Control* pressedItem(Control*);
        void selectItemsInRect(FloatRect&, KeyModifiers);
        bool replaceSelection(const ItemsViewSelection::UnorderedSet&);
        bool clearSelection();
        bool tryAddToSelection(Control* value);
        // Every item the view holds, whatever is on screen.
        void selectAll();
        // Whether there is anything to select at all. Not const: canSelectItem emits, and a
        // handler answers for the view as it stands.
        [[nodiscard]] bool hasSelectableItem();
        // The view volunteering as the subject of the commands that act on a selection. Connected
        // from the constructor, the way TextBox connects the edit actions.
        void connectSelectionActions();
        void invalidateSelectionOverlay();
    private:
        // TODO: move the drag threshold to the application accessibility settings
        static constexpr float k_dragThreshold = 16.0f;
    private:
        const inline static ItemsViewSelection::UnorderedSet s_emptySelectionBuffer{};
        inline static ItemsViewSelection::UnorderedSet s_tempSelectionBuffer{};
        inline static ItemsViewSelection::UnorderedSet s_initialSelectionBuffer{};
        inline static ItemsViewSelection::UnorderedSet s_previousSelectionBuffer{};
        //
        std::optional<FloatRect> m_selectionOverlayRect;
        // The bounds of the items the band reaches, rebuilt by selectItemsInRect on every move
        // and empty while no band is up. This is what a band invalidates, rather than its own
        // rect - see invalidateSelectionOverlay for why an item is asked for whole.
        FloatRect m_selectionOverlayItemsRect{};
        ItemsViewSelection m_selection{ *this };
        // Per view rather than shared the way the scratch buffers above are: an anchor stands
        // between one gesture and the next, so a second view setting its own would take this
        // one with it.
        Control* m_selectionAnchor{};
        ItemsViewSelection::UnorderedSet m_anchorSelection{};
    };


    //-------------------------------------------------------------------------


    // StackView
    //
    // The constructor is the one definition that has to stay here: it is a template, so every
    // caller instantiates it from this interface. Every other body lives in StackView.cpp.

    template<typename ...Args>
    StackView::StackView(const CreateParams& params, Args && ...args)
        :
        StackPanel{ params,
            Interactivity::ActiveContainer, //to traps children focus on keyboard navigation
            std::forward<Args>(args)... },
        INIT_PROPERTY(selectionMode),
        INIT_PROPERTY(dragMode)
    {
        connectSelectionActions();
    }

}
