export module Themes_App.WindowsPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.Button;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Label;
import ClaFi.Controls.Menu;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;

import ClaFi.Icons.XMark;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A dialog, a menu and a tooltip, each a control dressed as its window and casting its shadow.
    export class WindowsPreview : public PreviewPageBase
    {
    public:
        explicit WindowsPreview(const CreateParams&);
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    protected:
        void paintChildren(PaintEvent&) override;
    private:
        // Draws the shadow the window a mockup stands for casts, under the mockup.
        void drawWindowShadow(PaintEvent&, const RichControl& mockup, UiElement,
            const WindowShadow&);
        // A menu line, which keeps the click from closing the form it stands in.
        MenuItem& addMenuLine(std::wstring_view caption);
        static void drawMinimizeMark(DrawIconEvent&);
        static void drawMaximizeMark(DrawIconEvent&);
    private:
        static constexpr std::wstring_view k_whatItShows{
            L"Window surfaces, dialog title, window shadows"
        };
        static constexpr IconSize k_titleIconSize{ 12.0f };
        static constexpr float k_tooltipWidth = 180.0f;

        Panel& m_dialog{ add<Panel>(
            UiElement::Dialog,
            MetricsSet::PrimaryWindow
        ) };

        // Dressed as DialogTitle: a real title moves and closes the window it stands in.
        Panel& m_dialogTitle{ m_dialog.createTopBar<Panel>(
            UiElement::DialogTitle,
            MetricsSet::DialogTitle,
            Text{ L"Aquarium" },
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            WordWrap::No
        ) };

        StackPanel& m_titleButtons{ m_dialogTitle.createRightBar<StackPanel>(
            Orientation::Horizontal,
            Padding{ 4.0f, 4.0f }
        ) };

        ToolButton& m_minimizeButton{ m_titleButtons.add<ToolButton>(
            ButtonViewMode::IconOnly,
            k_titleIconSize,
            ShowSelectionOnSurface::No,
            Control::OnDrawIcon{ drawMinimizeMark }
        ) };

        ToolButton& m_maximizeButton{ m_titleButtons.add<ToolButton>(
            ButtonViewMode::IconOnly,
            k_titleIconSize,
            ShowSelectionOnSurface::No,
            Control::OnDrawIcon{ drawMaximizeMark }
        ) };

        ToolButton& m_closeButton{ m_titleButtons.add<ToolButton>(
            ButtonViewMode::IconOnly,
            k_titleIconSize,
            ShowSelectionOnSurface::No,
            Control::OnDrawIcon{ Icons::XMark::draw }
        ) };

        StackPanel& m_dialogBody{ m_dialog.createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 14.0f, 12.0f },
            Spacing{ 14.0f }
        ) };

        Label& m_question{ m_dialogBody.add<Label>(
            L"Put the pike back in the lake?"
        ) };

        StackPanel& m_answers{ m_dialogBody.add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ 8.0f },
            HorizontalAlign::Right
        ) };

        PrimaryButton& m_releaseButton{ m_answers.add<PrimaryButton>(L"Release") };
        Button& m_keepButton{ m_answers.add<Button>(L"Keep") };

        StackPanel& m_menu{ add<StackPanel>(
            Orientation::Vertical,
            UiElement::Menu,
            MetricsSet::SecondaryWindow,
            Padding{ 4.0f },
            HorizontalAlign::Left,
            ClickTarget::Self,
            MouseFocus::Keep,
            FocusScope::Yes
        ) };

        MenuItem& m_feedLine{ addMenuLine(L"Feed") };
        MenuItem& m_weighLine{ addMenuLine(L"Weigh") };
        MenuItem& m_measureLine{ addMenuLine(L"Measure") };
        Divider& m_menuDivider{ m_menu.add<Divider>() };
        MenuItem& m_releaseLine{ addMenuLine(L"Release") };

        Label& m_tooltip{ add<Label>(
            UiElement::Tooltip,
            MetricsSet::SecondaryWindow,
            Text{ L"Weighs the catch on the brass scale" },
            HorizontalAlign::Right,
            MaxSize{ k_tooltipWidth, k_maxFloat }
        ) };
    };
}
