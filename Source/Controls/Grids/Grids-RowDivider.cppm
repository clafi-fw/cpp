export module ClaFi.Controls.Grids :RowDivider;

import :RowBase;
import :Columns;
import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    // A line drawn across a grid where a row would stand.
    export class RowDivider : public RowBase
    {
    public:
        using RowBase::RowBase;
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"RowDivider"; }
    protected:
        bool hasContent(const Column&) const override { return false; }
        void adjustMetrics(AdjustMetricsEvent& event) const override { event.metrics.minSize.y = 4.0f; }
        void adjustPaint(AdjustPaintEvent& event) override { event.setColorRules(UiElement::Divider); }
        void paintSurface(PaintEvent&) override;
    };
}
