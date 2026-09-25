module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.ComboBox;

import ClaFi.Controls.TextItems;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Base.DropdownControlBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.StackView;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.ScrollBox;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export class ComboBox;

    // ComboboxEventBase

    class ComboBoxEventBase : public Event
    {
    public:
        explicit ComboBoxEventBase(const ComboBox& combobox);
        const ComboBox& comboBox() const { return m_comboBox; }
    private:
        const ComboBox& m_comboBox;
    };

    // ComboboxChangeEvent

    // The combobox has settled on a different item.
    export class ComboboxChangeEvent : public ComboBoxEventBase
    {
    public:
        using ComboBoxEventBase::ComboBoxEventBase;
    };

    // ComboboxAcceptTextEvent

    // The text typed over the face, before it is looked up among the items. See Controls
    export class ComboboxAcceptTextEvent : public ComboBoxEventBase
    {
    public:
        ComboboxAcceptTextEvent(const ComboBox&, AcceptEditEvent&);
        AcceptEditEvent& accept; // the typed text, and the refusal that answers it
    };

    // TextItemEventBase

    export class TextItemEventBase : public ComboBoxEventBase
    {
    public:
        TextItemEventBase(const ComboBox&, const TextItem&);
        const TextItem& item() const { return m_item; }
    private:
        const TextItem& m_item;
    };

    // PreviewItemEvent

    // An item is being looked at before it is chosen.
    export class PreviewItemEvent : public TextItemEventBase
    {
    public:
        PreviewItemEvent(ComboBox&, std::size_t itemIndex);
        std::size_t itemIndex() const { return m_itemIndex; }
    private:
        std::size_t m_itemIndex;
    };

    // AdjustItemTextEvent

    // The text one item is about to be written with.
    export class AdjustItemTextEvent : public TextItemEventBase
    {
    public:
        AdjustItemTextEvent(const ComboBox&, const TextItem&, Text&, EventPhase);
        Text& text() const { return m_text; }
        EventPhase phase() const { return m_phase; };
    private:
        Text& m_text;
        EventPhase m_phase;
    };

    // How many items one lane of a popup list takes, and how that count is read.
    export struct ItemsLaneSize {
        std::size_t value{ k_maxSize };
        LaneSizing sizing{ LaneSizing::Exact };
    };

    // How large the popup list may become. Unconstrained by default: what stops a list
    // growing is the room the placement found for it, and the bar comes up for what it cut.
    export struct DropdownMaxSize : public DesignDimensions
    {
        using DesignDimensions::DesignDimensions;
    };

    // PaintItemIconEvent

    // The icon one item is drawn with.
    export class PaintItemIconEvent : public PaintIconEvent
    {
    public:
        PaintItemIconEvent(PaintIconEvent&, const TextItem&);
        const TextItem& item() const { return m_item; }
    private:
        const TextItem& m_item;
    };

    // ComboboxItemControl

    // One line of the combobox's popup list.
    export class ComboboxItemControl : public Button
    {
    public:
        ComboboxItemControl(const CreateParams&, ComboBox&, std::size_t itemIndex);
        // Which of the combobox's items this control stands for. The dropdown reads it back off
        // the item the preview settled on, that preview naming a control and the combobox's own
        // event an index.
        [[nodiscard]] std::size_t itemIndex() const { return m_itemIndex; }
    protected:
        void paintIcon(PaintIconEvent&) override;
        void getText(GetTextEvent&) const override;
        void getTooltip(GetTooltipEvent&) override;
        void click(ClickEvent&) override;
        ScaledDimensions calculateContent(AlignEvent& event) override {
            return Button::calculateContent(event);
        }
    private:
        ComboBox& m_combobox;
        std::size_t m_itemIndex;
    };

    // ComboboxDropdownStack // The 'Stack' part here is very misleading

    class ComboboxDropdownStack : public StackView
    {
    public:
        ComboboxDropdownStack(const CreateParams&, ComboBox&);
    protected:
        void previewItemChanged(PreviewEvent&) override;
    private:
        // Brings the item the combobox is wearing into view, once the layout has settled.
        void scrollToCurrentItem();
    private:
        ComboBox& m_combobox;
        ScopedEventConnection m_aligned{};
        bool m_scrolledToCurrent{ false };
    };

    // The list as it is dropped: a box that scrolls, holding the stack of items.
    using ComboboxDropdown = ScrollBoxWith<ComboboxDropdownStack>;

    // Combobox

    using ComboboxBaseClass = WithInPlaceEdit<DropdownControlBase>;
    // A button that drops a list of texts and shows the one chosen.
    export class ComboBox : public ComboboxBaseClass, public TextItemsContainer
    {
        friend ComboboxItemControl;
        friend ComboboxDropdownStack;
    public:
        // Shared items
        template <typename... Args>
        ComboBox(const CreateParams&, TextItems&, Args&&...);
        // Own items
        template <typename... Args>
        ComboBox(const CreateParams&, TextItems&&, Args&&...);
    public:
        // Which editor the face opens, if any. See UI-Types
        DECLARE_PROPERTY_STORAGE(EditorMode, editorMode, EditorMode::None)
    public:
        // The combobox has settled on a different item.
        DECLARE_EVENT(ComboboxChangeEvent, OnChange, onChange)
        // The text one item is about to be written with.
        DECLARE_EVENT(AdjustItemTextEvent, OnAdjustItemText, onAdjustItemText)
        // The icon one item is drawn with.
        DECLARE_EVENT(PaintItemIconEvent, OnPaintItemIcon, onPaintItemIcon)
        // An item is being looked at before it is chosen.
        DECLARE_EVENT(PreviewItemEvent, OnPreviewItem, onPreviewItem)
        // The text typed over the face, before it is looked up among the items. See Controls
        DECLARE_EVENT(ComboboxAcceptTextEvent, OnAcceptText, onAcceptText)
    public:
        [[nodiscard]] EditorMode editorMode() const override { return m_editorMode; }
    protected:
        // Whether a press on the face drops the list rather than opening the editor.
        [[nodiscard]] bool dropOnPrimaryPress() const override;
        void showDropdown(Control& initiator) override;
        void itemIndexChanged() override;
        void paintItemIcon(PaintItemIconEvent&) const;
        void paintIcon(PaintIconEvent&) override;
        void getMainText(GetTextEvent&) const override;
        // The widest face of any item, so that a pick never changes the size. See Controls
        CalculatedDimensions measureText(AlignEvent&, ScaledDimensions asked, const Text&) override;
        void adjustPaint(AdjustPaintEvent&) override;
        [[nodiscard]] bool clickOpensEditor() const override;
        void getEditorText(Text&) const override;
        void acceptEditorText(AcceptEditEvent&) override;
        [[nodiscard]] FloatPoint editorMaxTextSize(const FloatRect&) const override;
        // The items, listed under the editor as their names are typed.
        [[nodiscard]] const TextItems* editorSuggestions() const override;
        void keyDown(KeyDownEvent&) override;
        void charPress(CharPressEvent&) override;
    private:
        template <typename... Args>
        ComboBox(const CreateParams&, TextItems* ownItems, TextItems* sharedItems, Args&&...);
        void adjustItemText(const TextItem&, Text&, EventPhase) const;
        // What the face and the list show for an item - its placeholder while its text is empty.
        void itemFaceText(const TextItem&, Text&, EventPhase) const;
        void previewItem(std::size_t itemIndex);
        // The picked item's text, in the plain text a user would type over it.
        [[nodiscard]] std::wstring valueText() const;
        [[nodiscard]] ItemIndexValue lookUpItem(std::wstring_view typed) const;
    private:
        inline static constexpr IconSize s_defaultIconSize{ 16.0f, 16.0f };
        inline static constexpr DropdownMaxSize s_defaultDropdownMaxSize{ k_maxFloat, k_maxFloat };
        inline static constexpr ButtonViewMode s_defaultViewMode = ButtonViewMode::TextLabel;
        ItemIndexValue m_previewedItemIndex;
        PlaceHolderText m_placeHolderText{};
        IconSize m_itemsIconSize{ s_defaultIconSize };
        ButtonViewMode m_itemsViewMode{ s_defaultViewMode };
        ItemsLaneSize m_itemsLaneSize;
        DropdownMaxSize m_dropdownMaxSize;
    };


    //-------------------------------------------------------------------------

    // ComboboxChangeEvent

    ComboBoxEventBase::ComboBoxEventBase(const ComboBox& combobox)
        : m_comboBox{ combobox }
    {
    }

    // ComboboxItemEventBase

    TextItemEventBase::TextItemEventBase(const ComboBox& combobox, const TextItem& item)
        :
        ComboBoxEventBase{ combobox },
        m_item{ item }
    {
    }

    // AdjustItemTextEvent

    AdjustItemTextEvent::AdjustItemTextEvent(const ComboBox& combobox,
        const TextItem& item, Text& text, EventPhase phase)
        :
        TextItemEventBase{ combobox, item },
        m_text{ text },
        m_phase{ phase }
    {
    }

    // PaintItemIconEvent

    PaintItemIconEvent::PaintItemIconEvent(PaintIconEvent& event, const TextItem& item)
        :
        PaintIconEvent{ event },
        m_item{ item }
    {
    }

    // PreviewItemEvent

    PreviewItemEvent::PreviewItemEvent(ComboBox& combobox, std::size_t itemIndex)
        :
        TextItemEventBase{ combobox, combobox.items()[itemIndex] },
        m_itemIndex{ itemIndex }
    {
    }

    // ComboboxItemControl

    ComboboxItemControl::ComboboxItemControl(const CreateParams& params, ComboBox& combobox, std::size_t itemIndex)
        :
        Button{ params,
            //IndicatorVisibility::Hover,
            // Not a tool, so not a ToolButton - but it wears the same look, and the property is
            // the whole of that look.
            ShowSurfaceAtRest::No,
            ShowSelectionOnSurface::Yes,
            combobox.m_itemsViewMode,
            combobox.m_itemsIconSize,
            Tag{ itemIndex }
        },
        m_combobox{ combobox },
        m_itemIndex{ itemIndex }
    {
    }

    void ComboboxItemControl::paintIcon(PaintIconEvent& event)
    {
        PaintItemIconEvent event2{ event, m_combobox.items()[m_itemIndex] };
        m_combobox.paintItemIcon(event2);
    }

    void ComboboxItemControl::getText(GetTextEvent& event) const
    {
        m_combobox.itemFaceText(m_combobox.items()[m_itemIndex], event.text, event.phase());
    }

    void ComboboxItemControl::getTooltip(GetTooltipEvent& event)
    {
        event.text << m_combobox.items()[m_itemIndex].tooltipText();
        Button::getTooltip(event);
    }

    void ComboboxItemControl::click(ClickEvent& event)
    {
        m_combobox.setItemIndex(m_itemIndex);
        event.closeForm();
    }

    // ComboboxDropdownStack

    ComboboxDropdownStack::ComboboxDropdownStack(const CreateParams& params, ComboBox& combobox)
        :
        StackView{ params,
            Orientation::VerticalWrap,
            PlaceHolderText{ L"No items" },
            LaneSize{ combobox.m_itemsLaneSize.value, combobox.m_itemsLaneSize.sizing },
            // A SCROLLED BODY KEEPS THE SIZE IT MEASURED. The box states its viewport as a
            // wrapping body's maximum width, and a list measured against a viewport the bar
            // has already taken its strip out of comes back a bar narrower than the window.
            WordWrap::No,
            // The item under the pointer is the one the list is asked about, and the keyboard
            // reaches it the same way - Control::setFocus carries the hover with the focus while
            // the keys drive, so arrowing down the list previews what it stops on.
            PreviewMode::Hover
        },
        m_combobox{ combobox }
    {
        for (std::size_t i = 0; i != combobox.items().size(); ++i)
        {
            Control& newItem = add<ComboboxItemControl>(combobox, i);
            if (combobox.itemIndex().has_value() && combobox.itemIndex().value() == i)
            {
                recordCurrentItem(newItem);
                // The combobox is already wearing this item, so reaching it asks for nothing.
                recordPreviewItem(newItem);
            }
        }
        // Sized by its items, so a bar the list needs is room the window grows by. See Controls
        params.form.setAutoFit(AutoFit::Yes);
        // ASKED OF THE PASS THAT SETTLES THE FORM. A pass measuring what to ask the placement
        // for is bounded by nothing, so nothing overruns and there is nothing to scroll yet.
        m_aligned = params.form.onAligned([this](FormAlignedEvent&) {
            scrollToCurrentItem();
        });
    }

    void ComboboxDropdownStack::previewItemChanged(PreviewEvent& event)
    {
        StackView::previewItemChanged(event);
        // Every control this stack holds is one of the combobox's items, so the item the preview
        // settled on is one of them and nothing else can reach here.
        m_combobox.previewItem(static_cast<ComboboxItemControl&>(event.item).itemIndex());
    }

    void ComboboxDropdownStack::scrollToCurrentItem()
    {
        if (m_scrolledToCurrent)
            return;
        m_scrolledToCurrent = true;
        if (Control* item = currentItem())
            item->scrollIntoView();
    }

    template<typename ...Args>
    ComboBox::ComboBox(const CreateParams& params, TextItems& items, Args&&... args)
        :
        ComboBox{ params, nullptr, &items, std::forward<Args>(args)... }
    {
    }

    template<typename ...Args>
    ComboBox::ComboBox(const CreateParams& params, TextItems&& items, Args && ... args)
        :
        ComboBox{
            params,
            new TextItems(std::move(items)),
            nullptr,
            std::forward<Args>(args)...
        }
    {
    }

    void ComboBox::showDropdown(Control& initiator)
    {
        m_previewedItemIndex.reset();
        dropPopup<ComboboxDropdown>(
            form(),
            initiator,
            HostProps{
                themeMetrics().secondaryWindow,
                themeMetrics().secondaryWindowShadow,
                UiElement::Section,
                // A LIST TALLER THAN THE ROOM IT FELL INTO SCROLLS. The placement cuts the
                // window to the room the side it took had, and the pass that follows is
                // measured against that window - which is where Auto reads the overrun.
                ScrollBars::Auto,
                MaxSize{ m_dropdownMaxSize },
                // The items carry the focus themselves, and the box around them is not a
                // focus scope of its own.
                Interactivity::MouseOnly
            },
            BodyProps{ *this }
        );

        // Restore preview after execute.
        // That needs when popup is canceled with keyboard
        if (itemIndex().has_value() && m_previewedItemIndex.has_value())
          if (itemIndex().value() != m_previewedItemIndex.value())
              previewItem(itemIndex().value());
    }

    void ComboBox::itemIndexChanged()
    {
        ComboboxChangeEvent event{ *this };
        emitEvent<ComboboxChangeEvent>(event);
        invalidate();
    }

    void ComboBox::paintItemIcon(PaintItemIconEvent& event) const
    {
        emitEvent(event);
    }

    void ComboBox::paintIcon(PaintIconEvent& event)
    {
        if (itemIndex().has_value())
        {
            PaintItemIconEvent event2{ event, items()[itemIndex().value()] };
            paintItemIcon(event2);
        }
    }

    void ComboBox::getMainText(GetTextEvent& event) const
    {
        // Only the selected item. Where the mark goes, and whether it goes here or on a strip,
        // is the base's business.
        // TODO: selected index routine
        if (itemIndex())
        {
            const TextItem& item = items()[itemIndex().value()];
            itemFaceText(item, event.text, event.phase());
        }
        else
            event.text << m_placeHolderText;
    }

    CalculatedDimensions ComboBox::measureText(AlignEvent& event, ScaledDimensions asked,
        const Text& text)
    {
        // The face as it reads now counts too - a derived control may word it unlike any item.
        CalculatedDimensions result = ComboboxBaseClass::measureText(event, asked, text);
        for (const TextItem& item : items())
        {
            Text face{};
            itemFaceText(item, face, EventPhase::Calculate);
            if (arrowPlacement() == ArrowPlacement::InText)
                appendInTextMark(face);

            const CalculatedDimensions faceSize =
                ComboboxBaseClass::measureText(event, asked, face);
            result.x = std::max(result.x, faceSize.x);
            result.y = std::max(result.y, faceSize.y);
        }
        return result;
    }

    void ComboBox::adjustPaint(AdjustPaintEvent& event)
    {
        // A COMBOBOX WEARS A BUTTON'S COLOURS, and it has to say so itself: the chain above
        // it names no rule set, so a combobox that states none paints against an empty one -
        // no surface in any state, whatever ShowSurfaceAtRest asks for, and no border.
        ComboboxBaseClass::adjustPaint(event);
        event.setColorRules(UiElement::Button);
    }

    // Combobox

    template <typename ... Args>
    ComboBox::ComboBox(const CreateParams& params, TextItems* ownItems, TextItems* sharedItems, Args&&... args)
        :
        ComboboxBaseClass{
            params,
            params.themeMetrics().button,
            Interactivity::Focusable,
            // The face is one line. A label too wide for it fades at the end rather than breaking,
            // which a control of one line's height would collapse.
            WordWrap::No,
            // What Auto means for a combobox. It comes before the caller's own arguments, so a
            // caller that asks for a strip still gets one - Props takes the last match.
            //ArrowPlacement::InText,

            // The size the combobox draws its own icon at.
            READ_PROPERTY(IconSize, s_defaultIconSize),
            // How the combobox lays that icon out against its text.
            READ_PROPERTY(ButtonViewMode, s_defaultViewMode),
            std::forward<Args>(args)...
        },
        TextItemsContainer{ ownItems, sharedItems, std::forward<Args>(args)... },
        INIT_PROPERTY(editorMode),
        // The size the items of the popup list draw their icons at.
        m_itemsIconSize{ READ_PROPERTY(ItemsIconSize, s_defaultIconSize) },
        // How those items lay an icon out against their text.
        m_itemsViewMode{ READ_PROPERTY(ItemsViewMode, s_defaultViewMode).value },
        // How many items one lane of the popup list takes, and how that count is read.
        m_itemsLaneSize{ READ_PROPERTY(ItemsLaneSize, ItemsLaneSize{}) },
        // How large that list may become.
        m_dropdownMaxSize{ READ_PROPERTY(DropdownMaxSize, s_defaultDropdownMaxSize) }
    {
    }

    void ComboBox::adjustItemText(const TextItem& item, Text& text, EventPhase phase) const
    {
        text << item.text();
        AdjustItemTextEvent event{ *this, item, text, phase };
        emitEvent(event);
    }

    void ComboBox::previewItem(std::size_t itemIndex)
    {
        m_previewedItemIndex = itemIndex;
        PreviewItemEvent event{ *this, itemIndex };
        emitEvent(event);
    }

}
