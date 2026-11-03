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
        // What a banner calls the kind. A callout naming no kind is information.
        [[nodiscard]] std::wstring_view nameOf(const MessageIcon kind)
        {
            switch (kind)
            {
                case MessageIcon::Question:
                    return L"Question";
                case MessageIcon::Warning:
                    return L"Warning";
                case MessageIcon::Error:
                    return L"Error";
                case MessageIcon::Ok:
                    return L"Ok";
                case MessageIcon::None:
                case MessageIcon::Information:
                    break;
            }
            return L"Info";
        }

        [[nodiscard]] MessageIcon markOf(const MessageIcon kind)
        {
            return kind == MessageIcon::None ? MessageIcon::Information : kind;
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

    // The band runs down the left edge and round the corners there in its own colour, which the
    // kind's element states.
    void Callout::paintFrame(PaintEvent& event)
    {
        PanelBase::paintFrame(event);
        event.paintBand(RectSide::Left, event.bandRgb());
    }

    // A mark in a bar stands at the top, beside the first line of the blocks.
    void Callout::addMark()
    {
        const DrawIconFunc drawMark = Icons::iconPainterOf(markOf(kind()));
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
