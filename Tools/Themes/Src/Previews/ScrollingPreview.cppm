export module Themes_App.ScrollingPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.TextBox;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A log longer and wider than the box it stands in, so the box carries both bars.
    export class ScrollingPreview : public PreviewPageBase
    {
    public:
        using PreviewPageBase::PreviewPageBase;
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        static constexpr std::wstring_view k_whatItShows{ L"Scroll buttons and thumbs" };
        static constexpr float k_boxHeight = 170.0f;

        static constexpr std::wstring_view k_log{
            L"05:40 - mist on the lake, and the pike rose twice by the reeds on the north bank\n"
            L"06:15 - perch shoaling under the jetty, thirty or more, all facing the current\n"
            L"07:02 - a heron took the shallows, and every fish went down to the weed bed\n"
            L"08:30 - the tench came up for bread at the lilies and left again at the splash\n"
            L"09:45 - the water at eleven degrees, clear down to the second rung of the ladder\n"
            L"11:20 - the carp basked at the surface in the sun until the boat came by\n"
            L"13:05 - a small eel crossed the gravel by the outflow and went under the stone\n"
            L"15:40 - the wind turned east, and the ripples hid everything below them\n"
            L"18:10 - the pike again at dusk, larger than the morning one, hunting the margins"
        };

        // A box that scrolls has to state a maximum on the axis it scrolls.
        ScrollBoxWith<TextBox>& m_box{ add<ScrollBoxWith<TextBox>>(
            HostProps{
                ScrollBars::Both,
                MaxSize{ k_maxFloat, k_boxHeight },
                HorizontalAlign::Fill
            },
            BodyProps{
                Text{ k_log },
                ReadOnly::Yes,
                WordWrap::No,
                Padding{ 8.0f }
            }
        ) };
    };
}
