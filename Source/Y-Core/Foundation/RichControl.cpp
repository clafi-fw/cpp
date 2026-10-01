module ClaFi.Core.Foundation;

import :Control;
import :PaintEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.StdLib;

namespace ClaFi
{
    // RichControl

    // None where the format is empty, so the cache keys an unformatted text the one way.
    const TextFormat* RichControl::textFormat() const
    {
        return m_textFormat.empty() ? nullptr : &m_textFormat;
    }

    void RichControl::adjustMetrics(AdjustMetricsEvent& event) const
    {
        event.metrics = m_metrics;
    }

    void RichControl::adjustPaint(AdjustPaintEvent& event)
    {
        if (m_colorRules)
            event.setColorRules(*m_colorRules);
        Control::adjustPaint(event);
    }

    void RichControl::getText(GetTextEvent& event) const
    {
        // Named rather than written in: a control's own text is usually the whole answer, and
        // what a gather costs is what it copies - see GetTextEventBase::contribute.
        event.contribute(m_text);
        Control::getText(event);
    }

    void RichControl::nestedGetHint(GetHintEvent& event)
    {
        event.text << m_hintText;
        // The base emits the event and, if nothing produced text, adds the control's own
        // text when it is trimmed.
        Control::nestedGetHint(event);
    }

}
