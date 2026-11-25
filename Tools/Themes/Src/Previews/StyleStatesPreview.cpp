module Themes_App.StyleStatesPreview;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Styles;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace
    {
        struct SampleState
        {
            std::wstring_view caption{};
            VisualState visual{};
            bool editing{};
            bool windowFocused{ true };
        };

        constexpr SampleState k_states[]{
            { L"Normal" },
            { L"Hovered", { .hot = true } },
            { L"Pressed", { .hot = true, .pressed = true } },
            { L"Focused", { .focused = true, .current = true } },
            { L"Selected", { .selected = true } },
            { L"Current", { .current = true } },
            { L"Disabled", { .enabled = false } },
            { .caption = L"Editing", .editing = true },
            { .caption = L"Inactive window", .windowFocused = false }
        };

        class StateSample : public RichControl
        {
        public:
            StateSample(const CreateParams&, UiElement, const SampleState&);
        public:
            [[nodiscard]] const VisualState& sampleState() const { return m_state.visual; }
            [[nodiscard]] float editFactor() const override { return m_state.editing ? 1.0f : 0.0f; }
        protected:
            void getControlState(GetStateEvent&) const override;
            void adjustPaint(AdjustPaintEvent&) override;
            void paintFrame(PaintEvent&) override;
        private:
            const SampleState& m_state;
        };

        StateSample::StateSample(const CreateParams& params, const UiElement element,
            const SampleState& state)
            :
            RichControl{ params, element, Text{ state.caption }, HorizontalAlign::Fill },
            m_state{ state }
        {
        }

        void StateSample::getControlState(GetStateEvent& event) const
        {
            event.state.enabled = m_state.visual.enabled;
            event.state.selected = m_state.visual.selected;
            event.stopPropagation();
        }

        void StateSample::adjustPaint(AdjustPaintEvent& event)
        {
            RichControl::adjustPaint(event);
            event.setWindowFocusedFactor(m_state.windowFocused ? 1.0f : 0.0f);
        }

        void StateSample::paintFrame(PaintEvent& event)
        {
            RichControl::paintFrame(event);
            event.paintBand(RectSide::Left, event.bandRgb());
        }
    }

    StyleStatesPreview::StyleStatesPreview(const CreateParams& params)
        :
        PreviewPageBase{ params }
    {
        const UiElement element = appElement(params.appContext().appElements().size());
        for (const SampleState& state : k_states)
        {
            add<StateSample>(element, state).invalidateState(AnimationMode::Off);
        }
    }

    void StyleStatesPreview::showStyle(const AppTheme& theme, const std::wstring_view name,
        const ColorMode mode)
    {
        const ControlStyle* style = theme.styles.of(name);
        const std::wstring_view background = style
            ? std::wstring_view{ style->standsOn } : std::wstring_view{};
        m_background = restingColors(theme, background, mode);
        const ControlMetrics metrics = theme.styles.resolvedMetrics(name);
        if (metrics != m_sampleMetrics)
        {
            m_sampleMetrics = metrics;
            invalidateFormAlign();
        }
    }

    void StyleStatesPreview::adjustNestedControlVisualState(const Control& control,
        VisualState& state) const
    {
        if (control.parent() == this)
        {
            state = static_cast<const StateSample&>(control).sampleState();
        }
    }

    void StyleStatesPreview::adjustChildMetrics(AdjustMetricsEvent& event) const
    {
        event.metrics = m_sampleMetrics;
    }

    void StyleStatesPreview::adjustPaint(AdjustPaintEvent& event)
    {
        PreviewPageBase::adjustPaint(event);
        event.setSurfaceHsl(m_background.surface);
        event.setTextHsl(m_background.ink, m_background.inkHueNamed);
    }

    void StyleStatesPreview::paintSurface(PaintEvent& event)
    {
        event.canvas().fillRectangle(event.surfaceBounds(), m_background.surface.toColor());
    }
}
