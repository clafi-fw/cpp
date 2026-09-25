export module ThisApp.PreviewPanel;

import ClaFi;
import ClaFi.Controls.TextBox;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.InkWell;

namespace ThisApp
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

        StackPanel& m_stackPanel{ createBody<StackPanel>(
            Padding{ 12.0 },
            Spacing{ 8.0 }
        ) };

        Label& m_titleLabel{ m_stackPanel.add<Label>(
            Text{ TextAlign::Center, TextStyleId::SubTitle, L"Live preview" },
            WordWrap::No,
            HorizontalAlign::Center,
            VerticalAlign::Center,
            UiElement::Header,
            themeMetrics().page
        ) };

        // Consumes every excessive height
        FlexSpacer& m_flexSpacer1{ m_stackPanel.add<FlexSpacer>() };
        Divider& m_divider1{ m_stackPanel.add<Divider>(VerticalAlign::Center) };

        TextBox& m_textBox{ m_stackPanel.add<TextBox>(
            Padding{ 4.0f },
            Text{
                TextAlign::Center,
                TextStyleId::SubBody, InkWell::textInk(InkGrade::Muted), L"A color is judged\n", PopColor{}, PopTextStyle{},
                L"In Context\n",
                TextStyleId::SubBody, InkWell::textInk(InkGrade::Muted), L"and never", PopColor{}, PopTextStyle{}, TextStyleId::Heading, InkWell::textInk(InkGrade::Muted), L" in isolation\n", PopColor{}, PopTextStyle{},
                TextStyleId::Title, InkWell::spotInk(), L"Select", PopColor{}, PopTextStyle{}, TextStyleId::SubBody, InkWell::textInk(InkGrade::Muted), L" any part of\n",
                L"this passage to see how\n", PopColor{}, PopTextStyle{},
                L"The Theme Responds"
            }
        ) };

        Divider& m_divider2{ m_stackPanel.add<Controls::Divider>() };
        // Consumes every excessive height
        FlexSpacer& m_flexSpacer2{ m_stackPanel.add<FlexSpacer>() };

        Button& m_button1{ m_stackPanel.add<Button>(
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

        Checkbox2& m_checkBox{ m_stackPanel.add<Checkbox2>(
            L"CheckBox",
            Checked::Yes
        ) };

        RadioButton& m_radioButton1{ m_stackPanel.add<RadioButton>(
            L"RadioButton 1",
            OnEvent{ [](ClickEvent& event) {
                m_selectedRb = false;
                event.control->parent()->invalidateChildrenStates();
            } },
            OnGetState{ [](GetStateEvent& event) { event.state.selected = !m_selectedRb; }}
        ) };
        RadioButton& m_radioButton2{ m_stackPanel.add<RadioButton>(
            L"RadioButton 2",
            OnEvent{ [](ClickEvent& event) {
                m_selectedRb = true;
                event.control->parent()->invalidateChildrenStates();
            } },
            OnGetState{ [](GetStateEvent& event) { event.state.selected = m_selectedRb; }}
        ) };
    };
}
