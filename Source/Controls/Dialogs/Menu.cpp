module;
#include "../../Y-Core/System/Switches.h"

module ClaFi.Controls.Menu;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.TextEngine;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        constexpr float k_trailingColumnGap = 12.0f;
        constexpr float k_shortcutIconGap = 4.0f;
    }

    void MenuItem::adjustText(AdjustTextEvent& event) const
    {
        RichControl::adjustText(event);
    }

    void MenuItem::adjustTextRect(AdjustTextRectEvent& event) const
    {
        MenuItemBase::adjustTextRect(event);
        event.textBounds.right -= trailingColumnInset(event.padding.x);
    }

    void MenuItem::paintOwnText(PaintEvent& event)
    {
        MenuItemBase::paintOwnText(event);
        if (!showsCaption() || m_trailingWidth == 0.0f)
            return;

        FloatRect content = event.controlBounds();
        content.inflate(-event.padding());
        if (m_menuTrailingColumn)
            content.right = event.right() - m_menuTrailingColumn->rightPadding;
        content.left = std::max(content.left, content.right - m_trailingWidth);
        if (content.empty())
            return;

        event.canvas().pushClip(content);
        float iconRight = content.right;
        if (!m_shortcutText.empty())
        {
            event.controlContext().currentMatch = {};
            textEngine().drawText(
                event.controlContext(),
                content,
                m_shortcutText,
                { verticalTextAnchor(), HorizontalTextAnchor::Right },
                nullptr,
#if CLAFI_TEXT_MOVING
                textRenderMode(),
#endif
                false,
                textFormat()
            );
            iconRight -= m_shortcutSize.x + event.scale(k_shortcutIconGap);
        }
        if (const float iconSize = event.scaleF(endIconSize()); iconSize > 0.0f)
        {
            const FloatRect iconBounds = FloatRect::FromDimensions(
                { iconRight - iconSize, content.center().y - iconSize / 2.0f },
                { iconSize, iconSize });
            DrawIconEvent iconEvent{
                event.controlContext(),
                iconBounds,
                tag(),
                event.disabledAmount()
            };
            drawEndIcon(iconEvent, endIcon());
        }
        event.canvas().popClip();
    }

    DrawTextResult MenuItem::drawText(PaintEvent& event, const FloatRect& textBounds, const Text& text)
    {
        if (trailingColumnInset(event.padding().x) == 0.0f)
            return MenuItemBase::drawText(event, textBounds, text);

        // Glyph overhang also stays on the caption side of the column boundary.
        FloatRect clip = event.controlBounds();
        clip.right = textBounds.right;
        event.canvas().pushClip(clip);
        const DrawTextResult result = MenuItemBase::drawText(event, textBounds, text);
        event.canvas().popClip();
        return result;
    }

    ScaledSize MenuItem::measureText(AlignEvent& event, ScaledSize maxDimensions, const Text& text)
    {
        if (wordWrap())
        {
            const float inset = trailingColumnInset(event.scaledPaddingX());
            maxDimensions.x = std::max(maxDimensions.x - inset, 1.0f);
        }
        return MenuItemBase::measureText(event, maxDimensions, text);
    }

    void MenuItem::calculateChildren(FormBase& form)
    {
        if (!m_menuTrailingColumn)
            measureTrailingContent(form.context());
        MenuItemBase::calculateChildren(form);
    }

    ScaledSize MenuItem::contentAroundText(AlignEvent& event, ScaledSize captionSize) const
    {
        ScaledSize result = MenuItemBase::contentAroundText(event, captionSize);
        if (showsCaption())
        {
            result.x += trailingColumnInset(event.scaledPaddingX());
            const float iconHeight = event.scaleF(endIconSize());
            result.y = std::max({ result.y, m_shortcutSize.y, iconHeight });
        }
        return result;
    }

    void MenuItem::measureTrailingContent(const FormContext& context)
    {
        m_shortcutText = {};
        m_shortcutSize = {};
        m_trailingWidth = 0.0f;
        if (!showsCaption())
            return;

        const float iconSize = context.scaleF(endIconSize());
        GetShortcutEvent event{ *this };
        emitEvent(event);
        const std::wstring shortcut = event.shortcut.text();
        if (!shortcut.empty())
        {
            m_shortcutText << ColoredText{ InkGrade::Muted, shortcut };
            m_shortcutSize = textEngine().measureText(
                context, m_shortcutText, { k_maxFloat, k_maxFloat }, false, false, textFormat());
            m_shortcutSize.x = std::ceil(m_shortcutSize.x);
        }

        m_trailingWidth = m_shortcutSize.x + iconSize;
        if (iconSize && m_shortcutSize.x)
            m_trailingWidth += context.scale(k_shortcutIconGap);
    }

    float MenuItem::trailingColumnInset(float rowPadding) const
    {
        const float width = m_menuTrailingColumn ? m_menuTrailingColumn->width : m_trailingWidth;
        if (width == 0.0f)
            return 0.0f;

        const float rightPadding = m_menuTrailingColumn ? m_menuTrailingColumn->rightPadding : rowPadding;
        return width + formContext().scale(k_trailingColumnGap) + rightPadding - rowPadding;
    }

    void Menu::calculateChildren(FormBase& form)
    {
        MenuItem::TrailingColumn trailingColumn;
        for (const ControlPtr& control : body().controls())
        {
            if (!control->visible())
                continue;

            MenuItem::GetMenuItemEvent event;
            control->emitEvent(event);
            if (MenuItem* item = event.item)
            {
                item->m_menuTrailingColumn = &m_trailingColumn;
                item->measureTrailingContent(form.context());
                trailingColumn.width = std::max(trailingColumn.width, item->m_trailingWidth);
                trailingColumn.rightPadding = std::max(trailingColumn.rightPadding, item->scaledPadding().x);
            }
        }
        m_trailingColumn = trailingColumn;
        BaseMenuForm::calculateChildren(form);
    }
}
