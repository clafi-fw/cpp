module ThisApp.ValueRuleControl;

import ThisApp.RuleSlider;
import ThisApp.RuleText;
import ThisApp.Utils;

import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.LabeledDivider;
import ClaFi.Controls.Slider;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace ThisApp
{
    namespace
    {
        constexpr std::size_t k_operationCount{ static_cast<std::size_t>(ColorRuleOp::Set) + 1ull };

        // What each operation does, as its tooltip names it.
        constexpr std::array<std::wstring_view, k_operationCount> k_operationNames{
            L"No change",
            L"Add offset",
            L"Scale value",
            L"Set exact value"
        };

        // The operations the popup offers as tiles, in the order they read.
        constexpr std::array k_tileOperations{
            ColorRuleOp::Offset,
            ColorRuleOp::Scale,
            ColorRuleOp::Set
        };

        // The mark against a tile of k_harmonyItemSize, as the hue popup and the harmony picker.
        constexpr MinSize k_indicatorSize{ 16.0f };

        // The character an operation is marked with, an offset's taking the sign of its value.
        [[nodiscard]] std::wstring_view operationGlyph(const ColorRuleOp operation,
            const float value)
        {
            switch (operation)
            {
                case ColorRuleOp::Offset:
                    return value < 0.0f ? L"\u2212" : L"+";
                case ColorRuleOp::Scale:
                    return L"\u00D7";
                case ColorRuleOp::Set:
                    return L"=";
                case ColorRuleOp::NoChange:
                    break;
            }
            return {};
        }

        // A glyph in the spot ink and bold, the way the operation combo boxes write theirs.
        void writeGlyph(Text& text, const std::wstring_view glyph)
        {
            text << InkWell::spotInk()
                << TextOp::PushBold
                << glyph
                << TextOp::PopBold
                << PopColor{};
        }

        // A value as it would be typed over the face: its mark and its number.
        [[nodiscard]] std::wstring typedValue(const ColorRuleValue& rule)
        {
            const float value = rule.value();
            switch (rule.operation())
            {
                case ColorRuleOp::Offset:
                    return std::format(L"{:+.2f}", value);
                case ColorRuleOp::Scale:
                    return std::format(L"\u00D7{:.2f}", value);
                case ColorRuleOp::Set:
                    return std::format(L"={:.2f}", value);
                case ColorRuleOp::NoChange:
                    break;
            }
            return {};
        }

        // No change in words, having no value to show, and anything else as its glyph and value.
        void writeValueContent(Text& text, const ColorRuleValue& rule)
        {
            if (rule.operation() == ColorRuleOp::NoChange)
            {
                text << InkWell::textInk(InkGrade::Subtle)
                    << TextStyleId::SubBody
                    << k_operationNames[static_cast<std::size_t>(ColorRuleOp::NoChange)]
                    << PopTextStyle{}
                    << PopColor{};
                return;
            }
            const float value = rule.value();
            writeGlyph(text, operationGlyph(rule.operation(), value));
            text << TextStyleId::Code
                << std::format(L"{:.2f}", std::abs(value))
                << PopTextStyle{};
        }

        // The operation's glyph on a blob in the muted spot ink, shaped as the hue popup's.
        void paintOperationBlob(PaintIconEvent& event, const ColorRuleOp operation)
        {
            const FloatRect& blob = event.iconRect();
            const float radius = blob.height() * k_blobCornerShare;
            const Color spot = event.spotRgb(InkGrade::Muted);
            event.canvas().fillRoundedRectangle(blob, radius, radius, spot);

            Text glyph;
            glyph << k_blobGlyphStyle;
            writeBlobGlyph(glyph, operationGlyph(operation, 0.0f));
            glyph << PopTextStyle{};
            Control::textEngine().drawText(
                event.controlContext(),
                blob,
                glyph,
                { VerticalTextAnchor::Center, HorizontalTextAnchor::Center }
            );
        }
    }

    // The three operations as tiles, a slider for the value, and the Clear and Close commands.
    class ValueRulePopup : public StackPanel
    {
    public:
        ValueRulePopup(const CreateParams&, ValueRuleControl&);
    public:
        [[nodiscard]] const ValueRuleControl& owner() const { return m_owner; }
        void pick(ColorRuleOp);
    private:
        void refreshItems();
    private:
        static constexpr float k_minWidth{ 220.0f };
        static constexpr float k_spacing{ 4.0f };
        ValueRuleControl& m_owner;
        std::vector<Control*> m_stateItems{}; // the tiles and the slider
    };

    // One operation in the popup: its mark on a blob over the value it gives the rule.
    class OperationItem : public Button
    {
    public:
        OperationItem(const CreateParams&, ValueRulePopup&, ColorRuleOp);
    protected:
        [[nodiscard]] MinSize indicatorSize(const AppTheme&) const override;
        void getText(GetTextEvent&) const override;
        void paintIcon(PaintIconEvent&) override;
        void nestedGetTooltip(GetTooltipEvent&) override;
        void getControlState(GetStateEvent&) const override;
        void nestedClick(ClickEvent&) override;
    private:
        ValueRulePopup& m_popup;
        ColorRuleOp m_operation;
    };

    // ValueRuleControl

    void ValueRuleControl::bind(ColorRuleValue& value, const RuleChannel channel,
        OnGetRuleBase ruleBase, OnValueRuleChanged onChanged)
    {
        m_value = &value;
        m_channel = channel;
        m_ruleBase = std::move(ruleBase);
        m_onChanged = std::move(onChanged);
        invalidate();
    }

    ColorRuleOp ValueRuleControl::operation() const
    {
        return m_value->operation();
    }

    float ValueRuleControl::normalizedValue() const
    {
        return m_value->normalizedValue();
    }

    // Another operation is another run on the face, so the row is laid out again.
    void ValueRuleControl::setOperation(const ColorRuleOp value)
    {
        if (m_value->operation() == value)
            return;
        m_value->setOperation(value);
        invalidateFormAlign();
        changed();
    }

    void ValueRuleControl::setNormalizedValue(const float value)
    {
        m_value->setNormalizedValue(value);
        changed();
    }

    EditorMode ValueRuleControl::editorMode() const
    {
        return m_value ? EditorMode::Editable : EditorMode::None;
    }

    void ValueRuleControl::showDropdown(Control& initiator)
    {
        if (!m_value)
            return;
        dropPopup<ValueRulePopup>(form(), initiator, *this);
    }

    void ValueRuleControl::getMainText(GetTextEvent& event) const
    {
        if (m_value)
            writeValueContent(event.text, *m_value);
    }

    void ValueRuleControl::nestedGetTooltip(GetTooltipEvent& event)
    {
        if (m_value)
            event.text << k_operationNames[static_cast<std::size_t>(operation())];
        DropdownControlBase::nestedGetTooltip(event);
    }

    void ValueRuleControl::getEditorText(Text& text) const
    {
        if (m_value)
            text << typedValue(*m_value);
    }

    // A value left as it reads is not taken, or the two decimals it shows would round the rule.
    void ValueRuleControl::acceptEditorText(AcceptEditEvent& event)
    {
        if (!m_value)
            return;
        std::wstring_view typed = event.text.plainText();
        trimLeft(typed);
        trimRight(typed);
        if (typed == typedValue(*m_value))
            return;
        if (typed.empty())
        {
            setOperation(ColorRuleOp::NoChange);
            return;
        }
        const std::optional<ColorRuleValue> rule = typedRule(event, *m_value);
        if (!rule.has_value())
        {
            if (!event.refused())
                event.refuse(L"Type a mark and a value, such as +0.02 or =0.66.");
            return;
        }
        *m_value = rule.value();
        invalidateFormAlign();
        changed();
    }

    // The face is one line, so the editor grows to the right with no ceiling of its own.
    FloatPoint ValueRuleControl::editorMaxTextSize(const FloatRect&) const
    {
        return {};
    }

    // Return types over the face, the way it does over an editable combo box.
    void ValueRuleControl::nestedKeyDown(KeyDownEvent& event)
    {
        if (event.key == Keys::Return && openEditor())
        {
            event.handled = true;
            return;
        }
        ValueRuleControlBase::nestedKeyDown(event);
    }

    // A character that can start a value opens the editor with it, and a space presses the face.
    void ValueRuleControl::charPress(CharPressEvent& event)
    {
        const wchar_t character = event.character();
        if (character != L' ' && std::iswprint(character) && openEditor({ &character, 1 }))
            return;
        ValueRuleControlBase::charPress(event);
    }

    void ValueRuleControl::changed()
    {
        invalidate();
        if (m_onChanged)
            m_onChanged();
    }

    // ValueRulePopup

    ValueRulePopup::ValueRulePopup(const CreateParams& params, ValueRuleControl& owner)
        :
        StackPanel{ params,
            Orientation::Vertical,
            Interactivity::ActiveContainer,
            params.themeMetrics().secondaryWindow,
            params.themeMetrics().secondaryWindowShadow,
            UiElement::Menu,
            Padding{ k_spacing },
            Spacing{ k_spacing },
            MinSize{ k_minWidth, 0.0f }
        },
        m_owner{ owner }
    {
        add<LabeledDivider>(
            L"Operation",
            Padding{ 0.0f, k_spacing }
        );

        StackPanel& tileRow = add<StackPanel>(
            Orientation::HorizontalWrap,
            Interactivity::ActiveContainer,
            LaneSize{ k_tileOperations.size() },
            Spacing{ k_spacing }
        );
        for (const ColorRuleOp operation : k_tileOperations)
            m_stateItems.push_back(&tileRow.add<OperationItem>(*this, operation));

        add<LabeledDivider>(
            L"Value",
            Padding{ 0.0f, k_spacing }
        );

        // The value belongs to the picked operation, so with none there is nothing to move.
        RuleSlider& slider = add<RuleSlider>(
            ScrollButtons::No,
            HorizontalAlign::Fill,
            Padding{ 4.0f }
        );
        slider.bind(m_owner.value(), m_owner.channel(), m_owner.ruleBase());
        slider.setMaxPosition(1.0f);
        slider.setRelativePosition(m_owner.normalizedValue(), false);
        slider.connectEvent([this](GetStateEvent& event) {
            event.state.enabled = m_owner.operation() != ColorRuleOp::NoChange;
        });
        // Every tile's caption is a value the slider sets, so the row is painted again with it.
        slider.onChange([this, &slider, &tileRow](SliderChangeEvent&) {
            m_owner.setNormalizedValue(slider.relativePosition());
            tileRow.invalidate();
        });
        m_stateItems.push_back(&slider);

        StackPanel& commandBar = add<StackPanel>(
            UiElement::Section,
            Orientation::Horizontal,
            Spacing{ 12.0f },
            Padding{ 12.0f },
            ItemSizing::Equal
        );
        commandBar.add<Button>(
            OnClick{ [this](ClickEvent& event) {
                pick(ColorRuleOp::NoChange);
                event.closeForm();
            } },
            Text{ L"Clear" },
            HorizontalTextAnchor::Center
        );
        commandBar.add<Button>(
            OnClick{ [](ClickEvent& event) {
                event.closeForm();
            } },
            Text{ L"Close" },
            HorizontalTextAnchor::Center
        );
    }

    // The popup stays up through a pick, so the tile that lost its mark is still in view.
    void ValueRulePopup::pick(const ColorRuleOp value)
    {
        m_owner.setOperation(value);
        refreshItems();
    }

    // The ramp is drawn through the operation, so the slider is painted again as well.
    void ValueRulePopup::refreshItems()
    {
        for (Control* item : m_stateItems)
        {
            item->invalidateState();
            item->invalidate();
        }
    }

    // OperationItem

    // As wide as the harmony picker's tile, and as tall as the blob and its caption come to.
    OperationItem::OperationItem(const CreateParams& params, ValueRulePopup& popup,
        const ColorRuleOp operation)
        :
        Button{ params,
            params.themeMetrics().toolButton,
            UiElement::ToolButton,
            IndicatorVisibility::Always,
            IndicatorStyle::Radio,
            ShowSelectionOnSurface::Yes,
            ButtonViewMode::TopCenterIcon,
            IconSize{ k_blobSize },
            MinSize{ k_harmonyItemSize.x, k_harmonyItemSize.y },
            MaxSize{ k_harmonyItemSize.x, k_maxFloat },
            HorizontalTextAnchor::Center,
            VerticalTextAnchor::Center
        },
        m_popup{ popup },
        m_operation{ operation }
    {
    }

    MinSize OperationItem::indicatorSize(const AppTheme&) const
    {
        return k_indicatorSize;
    }

    // The value the rule takes under this operation, sign included - the blob holds only the mark.
    void OperationItem::getText(GetTextEvent& event) const
    {
        const float value = m_popup.owner().value().value(m_operation);
        event.text << TextStyleId::SubBody
            << std::format(L"{:.2f}", value)
            << PopTextStyle{};
    }

    void OperationItem::paintIcon(PaintIconEvent& event)
    {
        paintOperationBlob(event, m_operation);
    }

    void OperationItem::nestedGetTooltip(GetTooltipEvent& event)
    {
        event.text << k_operationNames[static_cast<std::size_t>(m_operation)];
        Button::nestedGetTooltip(event);
    }

    // Stopped here, so the tile row's current item is not written over the mark.
    void OperationItem::getControlState(GetStateEvent& event) const
    {
        if (&event.control != this)
            return;
        event.state.selected = m_popup.owner().operation() == m_operation;
        event.stopPropagation();
    }

    void OperationItem::nestedClick(ClickEvent&)
    {
        m_popup.pick(m_operation);
    }
}
