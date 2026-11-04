module Themes_App.WindowsPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.Menu;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.ShadowPainter;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    // The margins are wide enough for the shadows, which reach past each mockup.
    WindowsPreview::WindowsPreview(const CreateParams& params)
        :
        PreviewPageBase{ params, Padding{ 20.0f }, Spacing{ 24.0f } }
    {
        // As DialogTitle paints itself.
        m_dialogTitle.onAdjustPaint([](AdjustPaintEvent& event) {
            event.setWindowSelectedAmount(1.0f);
        });
    }

    // While the panel paints, the form holds the edited theme, so the shadows are drawn in it.
    void WindowsPreview::paintChildren(PaintEvent& event)
    {
        const ThemeMetrics& metrics = themeMetrics();
        drawWindowShadow(event, m_dialog, UiElement::Dialog, metrics.primaryWindowShadow);
        drawWindowShadow(event, m_menu, UiElement::Menu, metrics.secondaryWindowShadow);
        drawWindowShadow(event, m_tooltip, UiElement::Tooltip, metrics.secondaryWindowShadow);
        PreviewPageBase::paintChildren(event);
    }

    void WindowsPreview::drawWindowShadow(PaintEvent& event, const RichControl& mockup,
        const UiElement element, const WindowShadow& shadow)
    {
        FormBase& owner = form();
        const Graphics::ShadowPainter& painter = owner.shadowPainter(
            owner.shadowDesign(element, shadow, mockup.metrics().radius));
        const FloatRect rect = FloatRect::FromDimensions(
            event.contentPosition() + mockup.topLeft(), mockup.scaledSize());
        painter.draw(event.canvas(), rect);
    }

    MenuItem& WindowsPreview::addMenuLine(const std::wstring_view caption)
    {
        return m_menu.add<MenuItem>(
            caption,
            OnEvent{ [](ClickEvent& event) {
                event.stopPropagation();
            } }
        );
    }

    void WindowsPreview::drawMinimizeMark(DrawIconEvent& event)
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

    void WindowsPreview::drawMaximizeMark(DrawIconEvent& event)
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
