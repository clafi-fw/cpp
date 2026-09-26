module ThisApp.ValueRuleControl;

import ThisApp.Utils;

import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.LabeledDivider;
import ClaFi.Controls.Slider;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
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

        // A glyph in the spot ink and bold, the way the operation comboboxes write theirs.
        void writeGlyph(Text& text, const std::wstring_view glyph)
        {
            text << InkWell::spotInk()
                << TextOp::PushBold
                << glyph
                << TextOp::PopBold
                << PopColor{};
        }

        // No change in words, having no value to show, and anything else as its glyph and value.
        void writeValueContent(Text& text, const ColorRuleValue& rule)
        {
            if (rule.operation() == ColorRuleOp::NoChange)
            {
                text << InkWell::textInk(InkGrade::Muted)
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

    // One operation in the popup, marked while the rule applies it.
    class OperationItem : public Button
    {
    public:
        OperationItem(const CreateParams&, ValueRulePopup&, ColorRuleOp);
    protected:
        [[nodiscard]] MinSize indicatorSize(const AppTheme&) const override;
        void getText(GetTextEvent&) const override;
        void getTooltip(GetTooltipEvent&) override;
        void getControlState(GetStateEvent&) const override;
        void click(ClickEvent&) override;
    private:
        ValueRulePopup& m_popup;
        ColorRuleOp m_operation;
    };

    // ValueRuleControl

    void ValueRuleControl::bind(ColorRuleValue& value, OnValueRuleChanged onChanged)
    {
        m_value = &value;
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

    void ValueRuleControl::getTooltip(GetTooltipEvent& event)
    {
        if (m_value)
            event.text << k_operationNames[static_cast<std::size_t>(operation())];
        DropdownControlBase::getTooltip(event);
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
        Slider& slider = add<Slider>(
            ScrollButtons::No,
            HorizontalAlign::Fill,
            Padding{ 4.0f }
        );
        slider.setMaxPosition(1.0f);
        slider.setRelativePosition(m_owner.normalizedValue(), false);
        slider.connectEvent([this](GetStateEvent& event) {
            event.state.enabled = m_owner.operation() != ColorRuleOp::NoChange;
        });
        slider.onChange([this, &slider](SliderChangeEvent&) {
            m_owner.setNormalizedValue(slider.relativePosition());
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

    void ValueRulePopup::refreshItems()
    {
        for (Control* item : m_stateItems)
            item->invalidateState();
    }

    // OperationItem

    OperationItem::OperationItem(const CreateParams& params, ValueRulePopup& popup,
        const ColorRuleOp operation)
        :
        Button{ params,
            ShowSurfaceAtRest::No,
            IndicatorVisibility::Always,
            IndicatorStyle::Radio,
            ShowSelectionOnSurface::Yes,
            k_harmonyItemSize,
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

    void OperationItem::getText(GetTextEvent& event) const
    {
        event.text << TextStyleId::Title;
        writeGlyph(event.text, operationGlyph(m_operation, 0.0f));
        event.text << PopTextStyle{};
    }

    void OperationItem::getTooltip(GetTooltipEvent& event)
    {
        event.text << k_operationNames[static_cast<std::size_t>(m_operation)];
        Button::getTooltip(event);
    }

    // Stopped here, so the tile row's current item is not written over the mark.
    void OperationItem::getControlState(GetStateEvent& event) const
    {
        if (&event.control != this)
            return;
        event.state.selected = m_popup.owner().operation() == m_operation;
        event.stopPropagation();
    }

    void OperationItem::click(ClickEvent&)
    {
        m_popup.pick(m_operation);
    }
}
