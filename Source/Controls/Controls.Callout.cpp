module ClaFi.Controls.Callout;

import ClaFi.Icons.MessageIcons;

import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Label;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        [[nodiscard]] MessageIcon iconOf(const CalloutKind kind)
        {
            return kind == CalloutKind::Warning ? MessageIcon::Warning : MessageIcon::Information;
        }

        [[nodiscard]] std::wstring_view nameOf(const CalloutKind kind)
        {
            return kind == CalloutKind::Warning ? L"Warning" : L"Info";
        }
    }

    // Callout

    // The blocks stand the callout's spacing down apart, unless the stack was given its own.
    void Callout::adjustChildMetrics(AdjustMetricsEvent& event) const
    {
        PanelBase::adjustChildMetrics(event);
        if (&event.control == &m_blocks && !m_blocks.statedMetrics().has(MetricsField::Spacing))
            event.metrics.spacing.y = spacing().y;
    }

    // A mark in a bar stands at the top, beside the first line of the blocks.
    void Callout::addMark()
    {
        const DrawIconFunc drawMark = Icons::iconPainterOf(iconOf(kind()));
        switch (shape())
        {
            case CalloutShape::Marked:
                createLeftBar<Label>(
                    ButtonViewMode::IconOnly,
                    IconSize{ k_markSize },
                    OnDrawIcon{ drawMark },
                    VerticalAlign::Top
                );
                break;
            case CalloutShape::Banner:
                createTopBar<Label>(
                    Text{ BoldText{ nameOf(kind()) } },
                    ButtonViewMode::LeftIcon,
                    IconSize{ k_markSize },
                    OnDrawIcon{ drawMark }
                );
                break;
            case CalloutShape::Plain:
                break;
        }
    }
}
