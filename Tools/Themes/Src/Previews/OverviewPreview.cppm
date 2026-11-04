export module Themes_App.OverviewPreview;

import Themes_App.PreviewPageBase;
import Themes_App.WaterChoice;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.TextBox;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A passage in several voices and a few controls, for a node with no control of its own.
    export class OverviewPreview : public PreviewPageBase
    {
    public:
        using PreviewPageBase::PreviewPageBase;
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        static constexpr std::wstring_view k_whatItShows{
            L"Text grades, spot ink, button, selection indicators"
        };

        FlexSpacer& m_spaceAbove{ add<FlexSpacer>() };
        Divider& m_dividerAbove{ add<Divider>(VerticalAlign::Center) };

        TextBox& m_passage{ add<TextBox>(
            Padding{ 4.0f },
            TextFormat{ TextAlign::Center },
            Text{
                StyledText{
                    TextStyles::SubBody,
                    ColoredText{ InkGrade::Muted, L"A pike waits\n" }
                },
                L"In Still Water\n",
                StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, L"and never" } },
                StyledText{
                    TextStyles::Heading,
                    ColoredText{ InkGrade::Muted, L" in the shallows\n" }
                },
                StyledText{ TextStyles::Title, ColoredText{ InkWell::spotInk(), L"Watch" } },
                StyledText{
                    TextStyles::SubBody,
                    ColoredText{ InkGrade::Muted, L" the reeds by\nthe north bank to see how\n" }
                },
                L"The Water Moves"
            }
        ) };

        Divider& m_dividerBelow{ add<Divider>() };
        FlexSpacer& m_spaceBelow{ add<FlexSpacer>() };

        Button& m_feedButton{ add<Button>(
            Text{ TextAlign::Center, L"Feed the fish" }
        ) };

        CheckBox2& m_lampCheck{ add<CheckBox2>(
            L"Lamp on",
            Checked::Yes
        ) };

        WaterChoice& m_waterChoice{ add<WaterChoice>() };
    };
}
