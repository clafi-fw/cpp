export module Themes_App.DialogMockup;

import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;

import ClaFi.Icons.XMark;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // Whether the window a mockup draws holds the focus.
    export enum class WindowFocus
    {
        Focused,
        Unfocused
    };

    // States the bare colour of the mode as what a control stands on and the ink it starts
    // from: a window stands on nothing, wherever a mockup of one is placed.
    export void standOnBareColor(AdjustPaintEvent&);

    // A dialog drawn as a control, at the window focus it states.
    export class DialogMockup : public WithBody<Panel, StackPanel>
    {
    public:
        explicit DialogMockup(const CreateParams&, std::wstring_view title, WindowFocus);
    public:
        [[nodiscard]] float windowFocusedFactor() const { return m_windowFocusedFactor; }
    protected:
        void adjustMetrics(AdjustMetricsEvent&) const override;
        void adjustPaint(AdjustPaintEvent&) override;
    private:
        static void drawMinimizeMark(DrawIconEvent&);
        static void drawMaximizeMark(DrawIconEvent&);
    private:
        static constexpr IconSize k_titleIconSize{ 12.0f };

        const float m_windowFocusedFactor;

        // Dressed as DialogTitle: a real title moves and closes the window it stands in.
        Panel& m_title{ createTopBar<Panel>(
            UiElement::DialogTitle,
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            WordWrap::No
        ) };

        StackPanel& m_titleButtons{ m_title.createRightBar<StackPanel>(
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
    };
}
