export module Themes_App.SurfacesPreview;

import Themes_App.FishIcon;
import Themes_App.PreviewPageBase;

import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Label;
import ClaFi.Controls.Stack;
import ClaFi.Controls.ToolBar;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A tool bar over a heading, a divider, and two sections with their headers - one open, one
    // shut.
    export class SurfacesPreview : public PreviewPageBase
    {
    public:
        explicit SurfacesPreview(const CreateParams&);
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        using Section = ExpanderWith<StackPanel>;
    private:
        // A section holding one paragraph, VerticalAlign::Top so that it grows with its words.
        Section& addSection(std::wstring_view caption, std::wstring_view paragraph);
    private:
        static constexpr std::wstring_view k_whatItShows{
            L"Page, tool bar and section surfaces, headers, dividers"
        };

        ToolBar& m_toolBar{ add<ToolBar>() };

        ToolButton& m_feedButton{ m_toolBar.add<ToolButton>(
            L"Feed",
            ButtonViewMode::LeftIcon,
            IconSize{ 16.0f },
            Control::OnDrawIcon{ FishIcon::draw }
        ) };

        ToolButton& m_cleanButton{ m_toolBar.add<ToolButton>(L"Clean") };
        ToolButton& m_lampButton{ m_toolBar.add<ToolButton>(L"Lamp") };

        Label& m_heading{ add<Label>(
            Text{ StyledText{ TextStyles::Heading, L"The north tank" } }
        ) };

        Label& m_paragraph{ add<Label>(
            Text{ ColoredText{ InkGrade::Muted, L"Six fish share it, and the plants keep to the "
                L"corners." } }
        ) };

        Divider& m_divider{ add<Divider>() };

        Section& m_orfeSection{ addSection(
            L"Golden orfe",
            L"Hides under the slate in the afternoon and rises at dusk."
        ) };

        Section& m_bitterlingSection{ addSection(
            L"Bitterling",
            L"Lays its eggs inside a freshwater mussel."
        ) };
    };
}
