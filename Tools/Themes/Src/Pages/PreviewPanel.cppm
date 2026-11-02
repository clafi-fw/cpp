export module Themes_App.PreviewPanel;

import ClaFi;
import ClaFi.Controls.TextBox;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.InkWell;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    export class PreviewPanel : public Panel
    {
    public:
        using Panel::Panel;
    public:
        ~PreviewPanel() override { m_button1Click2.disconnect(); }
    private:
        inline static bool m_selectedRb{ true };

        StackPanel& m_stack{ createBody<StackPanel>(
            Padding{ 12.0 },
            Spacing{ 8.0 }
        ) };

        Label& m_titleLabel{ m_stack.add<Label>(
            Text{ TextAlign::Center, StyledText{ TextStyles::SubTitle, L"Live preview" } },
            WordWrap::No,
            HorizontalAlign::Center,
            VerticalAlign::Center,
            UiElement::SectionHeader,
            themeMetrics().page
        ) };

        // Consumes every excessive height
        FlexSpacer& m_flexSpacer1{ m_stack.add<FlexSpacer>() };
        Divider& m_divider1{ m_stack.add<Divider>(VerticalAlign::Center) };

        TextBox& m_textBox{ m_stack.add<TextBox>(
            Padding{ 4.0f },
            TextFormat{ TextAlign::Center },
            Text{
                StyledText{
                    TextStyles::SubBody,
                    ColoredText{ InkGrade::Muted, L"A color is judged\n" }
                },
                L"In Context\n",
                StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, L"and never" } },
                StyledText{
                    TextStyles::Heading,
                    ColoredText{ InkGrade::Muted, L" in isolation\n" }
                },
                StyledText{ TextStyles::Title, ColoredText{ InkWell::spotInk(), L"Select" } },
                StyledText{
                    TextStyles::SubBody,
                    ColoredText{ InkGrade::Muted, L" any part of\nthis passage to see how\n" }
                },
                L"The Theme Responds"
            }
        ) };

        Divider& m_divider2{ m_stack.add<Controls::Divider>() };
        // Consumes every excessive height
        FlexSpacer& m_flexSpacer2{ m_stack.add<FlexSpacer>() };

        Button& m_button1{ m_stack.add<Button>(
            Text{ TextAlign::Center, L"Test Button" }
        ) };

        ScopedEventConnection m_button1Click1{
            m_button1.onClick([](ClickEvent&) {
                })
        };

        EventConnection m_button1Click2{
            m_button1.onClick([](ClickEvent&) {
                })
        };

        EventConnection m_button1Click3{ m_button1.onClick([](ClickEvent&) {
            }) };

        CheckBox2& m_checkBox{ m_stack.add<CheckBox2>(
            L"CheckBox",
            Checked::Yes
        ) };

        RadioButton& m_radioButton1{ m_stack.add<RadioButton>(
            L"RadioButton 1",
            OnEvent{ [](ClickEvent& event) {
                m_selectedRb = false;
                event.control->parent()->invalidateChildrenStates();
            } },
            OnGetState{ [](GetStateEvent& event) { event.state.selected = !m_selectedRb; }}
        ) };
        RadioButton& m_radioButton2{ m_stack.add<RadioButton>(
            L"RadioButton 2",
            OnEvent{ [](ClickEvent& event) {
                m_selectedRb = true;
                event.control->parent()->invalidateChildrenStates();
            } },
            OnGetState{ [](GetStateEvent& event) { event.state.selected = m_selectedRb; }}
        ) };

        // A button's metrics give a stroke something to be drawn at; the focus gives the states.
        RichControl& m_testee{ m_stack.add<RichControl>(
            themeMetrics().button,
            NavigationReach::Yes,
            MouseFocus::Self,
            ClickTarget::Self,
            UiElement::Testee,
            Text{ L"Testee" }
        ) };

        RichControl& m_bestee{ m_stack.add<RichControl>(
            themeMetrics().button,
            NavigationReach::Yes,
            MouseFocus::Self,
            ClickTarget::Self,
            UiElement::Bestee,
            Text{ L"Bestee" }
        ) };
    };
}
