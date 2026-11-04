export module Themes_App.InputsPreview;

import Themes_App.FishIcon;
import Themes_App.PreviewPageBase;

import ClaFi.Controls.Button;
import ClaFi.Controls.Label;
import ClaFi.Controls.Slider;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // Buttons at rest, leading and disabled, tool buttons, a slider and a text box.
    export class InputsPreview : public PreviewPageBase
    {
    public:
        explicit InputsPreview(const CreateParams&);
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        // A muted caption over the control it names.
        [[nodiscard]] static Text captionOf(std::wstring_view words);
    private:
        static constexpr std::wstring_view k_whatItShows{
            L"Button and tool button states, slider, text box"
        };

        StackPanel& m_buttons{ add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ 8.0f }
        ) };

        Button& m_netButton{ m_buttons.add<Button>(L"Net") };
        PrimaryButton& m_feedButton{ m_buttons.add<PrimaryButton>(L"Feed") };
        Button& m_releaseButton{ m_buttons.add<Button>(L"Release") };

        StackPanel& m_toolButtons{ add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ 4.0f }
        ) };

        ToolButton& m_catchButton{ m_toolButtons.add<ToolButton>(
            L"Catch",
            ButtonViewMode::LeftIcon,
            IconSize{ 16.0f },
            Control::OnDrawIcon{ FishIcon::draw }
        ) };

        ToolButton& m_pumpButton{ m_toolButtons.add<ToolButton>(L"Pump") };
        ToolButton& m_lampButton{ m_toolButtons.add<ToolButton>(L"Lamp") };

        Label& m_temperatureCaption{ add<Label>(captionOf(L"Water temperature")) };

        Slider& m_temperatureSlider{ add<Slider>(
            ScrollButtons::No,
            FineAdjust::No,
            HorizontalAlign::Fill
        ) };

        Label& m_nameCaption{ add<Label>(captionOf(L"Name")) };

        TextBox& m_nameBox{ add<TextBox>(
            Text{ L"Golden orfe" },
            WordWrap::No,
            HorizontalAlign::Fill
        ) };
    };
}
