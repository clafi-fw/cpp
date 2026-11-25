export module Themes_App.StyleStatesPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ClaFi;

    export class StyleStatesPreview : public PreviewPageBase
    {
    public:
        explicit StyleStatesPreview(const CreateParams&);
    public:
        [[nodiscard]] std::wstring_view whatItShows() const override { return L"Control states"; }
        void showStyle(const AppTheme&, std::wstring_view name, ColorMode);
    protected:
        void adjustNestedControlVisualState(const Control&, VisualState&) const override;
        void adjustChildMetrics(AdjustMetricsEvent&) const override;
        void adjustPaint(AdjustPaintEvent&) override;
        void paintSurface(PaintEvent&) override;
    private:
        ControlMetrics m_sampleMetrics{};
        RestingColors m_background{};
    };
}
