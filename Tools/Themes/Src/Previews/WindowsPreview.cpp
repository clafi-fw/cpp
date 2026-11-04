module Themes_App.WindowsPreview;

import Themes_App.DialogMockup;
import Themes_App.PreviewPageBase;

import ClaFi.Controls.Menu;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
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
        // A popup reads the focus of the window it opens over - here the focused dialog.
        const auto readOwnerFocus = [this](AdjustPaintEvent& event) {
            event.setWindowFocusedFactor(m_focusedDialog.windowFocusedFactor());
        };
        m_menu.onAdjustPaint(readOwnerFocus);
        m_tooltip.onAdjustPaint(readOwnerFocus);
    }

    // While the panel paints, the form holds the edited theme, so the shadows are drawn in it.
    void WindowsPreview::paintChildren(PaintEvent& event)
    {
        const ThemeMetrics& metrics = themeMetrics();
        drawWindowShadow(event, m_focusedDialog, UiElement::Dialog, metrics.primaryWindowShadow,
            m_focusedDialog.windowFocusedFactor());
        drawWindowShadow(event, m_unfocusedDialog, UiElement::Dialog, metrics.primaryWindowShadow,
            m_unfocusedDialog.windowFocusedFactor());
        drawWindowShadow(event, m_menu, UiElement::Menu, metrics.secondaryWindowShadow,
            m_focusedDialog.windowFocusedFactor());
        drawWindowShadow(event, m_tooltip, UiElement::Tooltip, metrics.secondaryWindowShadow,
            m_focusedDialog.windowFocusedFactor());
        PreviewPageBase::paintChildren(event);
    }

    void WindowsPreview::drawWindowShadow(PaintEvent& event, const RichControl& mockup,
        const UiElement element, const WindowShadow& shadow, const float windowFocusedFactor)
    {
        FormBase& owner = form();
        const Graphics::ShadowPainter& painter = owner.shadowPainter(
            owner.shadowDesign(element, shadow, mockup.metrics().radius, windowFocusedFactor));
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
}
