module Themes_App.DialogMockup;

import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    DialogMockup::DialogMockup(const CreateParams& params, const std::wstring_view title,
        const WindowFocus focus)
        :
        WithBody{
            params,
            HostProps{
                UiElement::Dialog,
                MetricsSet::PrimaryWindow
            },
            BodyProps{
                Orientation::Vertical,
                Padding{ 14.0f, 12.0f },
                Spacing{ 14.0f }
            }
        },
        m_windowFocusedFactor{ focus == WindowFocus::Focused ? 1.0f : 0.0f }
    {
        m_title.text() = Text{ title };
        // As DialogTitle paints itself.
        m_title.onAdjustPaint([](AdjustPaintEvent& event) {
            event.setWindowSelectedAmount(1.0f);
        });
    }

    void DialogMockup::adjustPaint(AdjustPaintEvent& event)
    {
        Panel::adjustPaint(event);
        event.setWindowFocusedFactor(m_windowFocusedFactor);
    }

    void DialogMockup::drawMinimizeMark(DrawIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const float centerY = iconRect.centerY();
        event.canvas().drawLine(
            { iconRect.left, centerY },
            { iconRect.right, centerY },
            event.textRgb(InkGrade::Strongest),
            event.scaleF(1.0f)
        );
    }

    void DialogMockup::drawMaximizeMark(DrawIconEvent& event)
    {
        const float radius = event.scale(2.0f);
        event.canvas().drawRoundedRectangle(
            event.iconRect(),
            radius,
            radius,
            event.textRgb(InkGrade::Strongest),
            event.scale(1.0f)
        );
    }
}
