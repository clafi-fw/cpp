export module ClaFi.Diagnostic.FpsChart;

import ClaFi.Diagnostic.Benchmark;

import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;

import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Diagnostic
{
    // The chart's proportions before scale. The axis column holds the readings; the plot is what
    // is left of the width.
    constexpr float k_axisAreaWidth{ 64.0f };
    constexpr float k_plotWidth{ 96.0f };

    /// @brief What a chart asks for across, before scale. Whatever stands in the same column
    /// STATES THIS as its own width: a neighbour left to be sized by its words re-measures the
    /// column every time a reading gains a digit.
    export constexpr float k_fpsChartWidth{ k_axisAreaWidth + k_plotWidth };

    /// @brief The colour the field under the curve is drawn from.
    export struct ChartFieldColor
    {
        Color value{};
    };

    /// @brief The colour the curve itself is drawn from.
    export struct ChartSeriesColor
    {
        Color value{};
    };

    // The frame rate over the last two seconds, drawn against an axis that names it. See Diagnostic
    export class FpsChart : public Control
    {
    public:
        template <typename... Args>
        FpsChart(const CreateParams&, const FpsBenchmark&, Args&&...);
    public:
        void setColors(Color field, Color series);
    protected:
        void paintSurface(PaintEvent&) override;
        ScaledDimensions calculateContent(AlignEvent&) override;
    private:
        // A gathered series: one rate per bucket, in the order the buckets stand.
        using FloatValues = std::vector<float>;
    private:
        // The rectangle the curve is drawn in: the control's bounds less the axis column, and less
        // the half reading the top and the bottom lines each stand outside their line.
        [[nodiscard]] FloatRect plotRect(const PaintEvent&) const;
        // Reads the buckets into the point buffers and answers the highest rate among them.
        [[nodiscard]] float gatherPoints(const FloatRect& plot);
        // Lays one of the gathered series into m_path. Closed to the baseline it is an area to be
        // filled; open it is a line.
        void buildSeriesPath(const FloatRect& plot, const FloatValues& fps, bool closeToBaseline);
        // Eases the axis top towards the rate the buckets ask for.
        void advanceTopLimit(float maxFps);
        // How far apart the guide lines stand, in frames per second.
        [[nodiscard]] float guideStep() const;
        // The colours this paint draws in: what was stated, or the theme's - see the class note.
        [[nodiscard]] Color fieldColor(const PaintEvent&) const;
        [[nodiscard]] Color seriesColor(const PaintEvent&) const;
        // Where a rate falls inside the plot.
        [[nodiscard]] float yOf(const FloatRect& plot, float fps) const;
        //
        void paintField(PaintEvent&, const FloatRect& plot);
        void paintGuides(PaintEvent&, const FloatRect& plot, float topLineValue);
        void paintSeries(PaintEvent&, const FloatRect& plot);
        void paintAxis(PaintEvent&, const FloatRect& plot, float topLineValue);
    private:
        static constexpr float k_plotHeight{ 48.0f };
        static constexpr float k_axisFontSize{ 10.0f };
        // The clear space a reading keeps from the one above it and from the bottom reading.
        static constexpr float k_axisLabelSpacing{ 4.0f };
        // The space a reading keeps from the plot it stands beside.
        static constexpr float k_axisLabelMargin{ 4.0f };
        // How much of the past the chart shows.
        static constexpr float k_timeWindowMs{ 2000.0f };
        // The axis states no less than this, so a rate under it is read against a steady scale
        // rather than against itself.
        static constexpr float k_baseTopLimit{ 60.0f };
        // The headroom the axis keeps over the highest rate in the window.
        static constexpr float k_topLimitHeadroom{ 1.15f };
        // The axis rises towards a rate it cannot show quickly and falls back slowly, so a spike
        // is on screen the frame it happens and the scale does not shake once it has passed.
        static constexpr float k_topLimitRiseRate{ 0.1f };
        static constexpr float k_topLimitFallRate{ 0.01f };
        // What the two stated colours are drawn at.
        static constexpr ColorByte k_fieldAlpha{ 100 };
        static constexpr ColorByte k_seriesFillAlpha{ 128 };
        static constexpr ColorByte k_seriesStrokeAlpha{ 230 };
        // The drawing-rate line, in the guides' white rather than the series colour: it is a
        // ceiling over the rate rather than another reading of it.
        static constexpr ColorByte k_drawLineAlpha{ 120 };
        // The guides are white rather than an ink, because what they lie on is a stated colour and
        // not a surface the theme knows about.
        static constexpr ColorAsUint k_guideColor{ 0xffffFFff };
        static constexpr ColorByte k_guideAlpha{ 40 };
    private:
        const FpsBenchmark& m_benchmark;
        // The colours stated, or none - see the class note.
        std::optional<ChartFieldColor> m_fieldColor;
        std::optional<ChartSeriesColor> m_seriesColor;
        // The top of the axis, easing towards the value the current buckets ask for. Zero means no
        // frame has been painted yet, and the first one snaps the axis to its value instead of
        // easing up from nothing.
        float m_topLimit{ 0.0f };
        // The points, and the path they are drawn as, held across frames so a repaint reuses the
        // capacity rather than taking it from the allocator again.
        FloatValues m_pointX{};
        // The rate frames arrived at, and the rate the drawing alone would allow.
        FloatValues m_pointFps{};
        FloatValues m_pointDrawFps{};
        Graphics::PixelPath m_path{};
    };


//-----------------------------------------------------------------------------


    template <typename... Args>
    FpsChart::FpsChart(const CreateParams& params, const FpsBenchmark& benchmark, Args&&... args)
        :
        Control{ params, std::forward<Args>(args)... },
        m_benchmark{ benchmark },
        m_fieldColor{ Props::find<ChartFieldColor>(std::forward<Args>(args)...) },
        m_seriesColor{ Props::find<ChartSeriesColor>(std::forward<Args>(args)...) }
    {
    }

    void FpsChart::setColors(Color field, Color series)
    {
        m_fieldColor = ChartFieldColor{ field };
        m_seriesColor = ChartSeriesColor{ series };
        invalidate();
    }

    void FpsChart::paintSurface(PaintEvent& event)
    {
        Control::paintSurface(event);

        const FloatRect plot = plotRect(event);
        paintField(event, plot);

        if (m_benchmark.buckets().empty())
            return;

        const float maxFps = gatherPoints(plot);
        advanceTopLimit(maxFps);

        // The top line is the highest whole step the axis can show.
        const float topLineValue = std::floor(m_topLimit / guideStep()) * guideStep();
        paintGuides(event, plot, topLineValue);
        paintSeries(event, plot);
        paintAxis(event, plot, topLineValue);
    }

    ScaledDimensions FpsChart::calculateContent(AlignEvent& event)
    {
        // The height is the plot plus one reading: half of the top reading stands above the plot
        // and half of the bottom one below it.
        return {
            event.scale(k_fpsChartWidth),
            event.scale(k_plotHeight + k_axisFontSize)
        };
    }

    FloatRect FpsChart::plotRect(const PaintEvent& event) const
    {
        const float halfLabel = event.scale(k_axisFontSize) / 2.0f;
        FloatRect plot = event.controlBounds();
        plot.left += event.scale(k_axisAreaWidth);
        plot.top += halfLabel;
        plot.bottom -= halfLabel;
        return plot;
    }

    float FpsChart::gatherPoints(const FloatRect& plot)
    {
        using namespace std::chrono;

        const FpsBuckets& buckets = m_benchmark.buckets();
        const TimePoint newest = buckets.back().startedAt;
        float maxFps = k_baseTopLimit;

        m_pointX.reserve(buckets.size());
        m_pointFps.reserve(buckets.size());
        m_pointDrawFps.reserve(buckets.size());
        m_pointX.clear();
        m_pointFps.clear();
        m_pointDrawFps.clear();

        // THE DRAWING RATE IS STATED BY WHOLE FRAMES ALONE - see FrameExtent. A slice holding
        // none keeps the rate last stated, and the slices before the first statement take that
        // first one: the line is a ceiling, and a ceiling holds until a whole frame measures it
        // again. Negative marks a slice still waiting for the first statement.
        float carriedDrawFps = -1.0f;
        std::size_t unstated = 0;

        for (const FpsBucket& bucket : buckets)
        {
            const float intervalMs = bucket.avgFrameIntervalMs();
            const float fps = intervalMs > 0.0f ? 1000.0f / intervalMs : 0.0f;
            const float drawMs = bucket.avgDurationInMilliSeconds();
            if (drawMs > 0.0f)
            {
                carriedDrawFps = 1000.0f / drawMs;
                for (std::size_t i = 0; i < unstated; ++i)
                    m_pointDrawFps[i] = carriedDrawFps;
                unstated = 0;
            }
            else if (carriedDrawFps < 0.0f)
            {
                ++unstated;
            }

            // THE AXIS IS SET BY THE HIGHER OF THE TWO, which is the drawing rate whenever the
            // two have parted. Set by the arrival rate alone, the line above it would sit off the
            // top of the chart in exactly the case the chart exists to show.
            maxFps = std::max({ maxFps, fps, carriedDrawFps });

            const float agoMs =
                static_cast<float>(duration_cast<microseconds>(newest - bucket.startedAt).count())
                / 1000.0f;
            m_pointX.push_back(plot.right - (agoMs / k_timeWindowMs) * plot.width());
            m_pointFps.push_back(fps);
            m_pointDrawFps.push_back(carriedDrawFps);
        }

        // No whole frame in the window at all: the ceiling lies on the baseline.
        for (std::size_t i = 0; i < unstated; ++i)
            m_pointDrawFps[i] = 0.0f;
        return maxFps;
    }

    // WHERE THE CURVE STARTS AND ENDS IS THE WHOLE DIFFERENCE between the two series. An area
    // runs down to the baseline at both ends and closes; a line begins and ends on its own first
    // and last readings.
    void FpsChart::buildSeriesPath(const FloatRect& plot, const FloatValues& fps,
        bool closeToBaseline)
    {
        m_path.clear();

        bool started = false;
        float prevX = 0.0f;
        float prevY = 0.0f;
        float lastX = 0.0f;
        float lastY = 0.0f;

        for (std::size_t i = 0; i < m_pointX.size(); ++i)
        {
            const float x = m_pointX[i];
            const float y = yOf(plot, fps[i]);

            // A point older than the window stands off the left edge and is not drawn. The first
            // one inside the window carries the crossing, so the curve starts on the edge rather
            // than at the first bucket that happens to be young enough.
            if (x < plot.left)
            {
                prevX = x;
                prevY = y;
                continue;
            }

            // Clamped, so a rate the axis has not risen to yet runs along the top of the plot
            // instead of over whatever stands above it.
            const float clampedY = std::clamp(y, plot.top, plot.bottom);

            if (!started)
            {
                if (i > 0 && prevX < plot.left)
                {
                    const float t = (plot.left - prevX) / (x - prevX);
                    const float crossingY =
                        std::clamp(prevY + t * (y - prevY), plot.top, plot.bottom);
                    if (closeToBaseline)
                        m_path.moveTo(plot.left, plot.bottom);
                    else
                        m_path.moveTo(plot.left, crossingY);
                    m_path.lineTo(plot.left, crossingY);
                }
                else if (closeToBaseline)
                {
                    m_path.moveTo(x, plot.bottom);
                }
                else
                {
                    m_path.moveTo(x, clampedY);
                }
                started = true;
            }
            m_path.lineTo(x, clampedY);

            prevX = x;
            prevY = y;
            lastX = x;
            lastY = clampedY;
        }

        if (!started)
        {
            m_path.clear();
            return;
        }

        if (closeToBaseline)
        {
            m_path.lineTo(plot.right, plot.bottom);
            m_path.close();
            return;
        }

        // The line runs on to the right edge at the height it ended, rather than stopping short
        // of the area beneath it.
        if (lastX < plot.right)
            m_path.lineTo(plot.right, lastY);
    }

    void FpsChart::advanceTopLimit(float maxFps)
    {
        // Snapped to a whole 50, so the axis states round numbers whatever the rate is.
        const int steps = static_cast<int>(maxFps * k_topLimitHeadroom / 50.0f) + 1;
        const float target = steps * 50.0f;

        if (m_topLimit == 0.0f)
            m_topLimit = target;
        else if (target > m_topLimit)
            m_topLimit += (target - m_topLimit) * k_topLimitRiseRate;
        else
            m_topLimit += (target - m_topLimit) * k_topLimitFallRate;
    }

    float FpsChart::guideStep() const
    {
        if (m_topLimit >= 2000.0f)
            return 1000.0f;
        if (m_topLimit >= 1000.0f)
            return 500.0f;
        if (m_topLimit >= 500.0f)
            return 200.0f;
        if (m_topLimit >= 200.0f)
            return 100.0f;

        return 50.0f;
    }

    Color FpsChart::fieldColor(const PaintEvent& event) const
    {
        if (m_fieldColor)
            return m_fieldColor->value;

        return event.spotRgb(InkGrade::Strongest);
    }

    Color FpsChart::seriesColor(const PaintEvent& event) const
    {
        if (m_seriesColor)
            return m_seriesColor->value;

        return event.accentRgb(InkGrade::Strongest);
    }

    float FpsChart::yOf(const FloatRect& plot, float fps) const
    {
        return plot.bottom - plot.height() * fps / m_topLimit;
    }

    void FpsChart::paintField(PaintEvent& event, const FloatRect& plot)
    {
        Color field = fieldColor(event);
        field.alpha = k_fieldAlpha;
        event.canvas().fillRectangle(plot, field);
    }

    void FpsChart::paintGuides(PaintEvent& event, const FloatRect& plot, float topLineValue)
    {
        m_path.clear();
        for (float value = guideStep(); value <= topLineValue; value += guideStep())
        {
            // The half pixel is what makes a one pixel line crisp.
            const float y = std::floor(yOf(plot, value)) + 0.5f;
            m_path.moveTo(plot.left, y);
            m_path.lineTo(plot.right, y);
        }

        Color guide{ k_guideColor };
        guide.alpha = k_guideAlpha;
        event.canvas().drawPath(m_path, guide, event.scale(1.0f));
    }

    void FpsChart::paintSeries(PaintEvent& event, const FloatRect& plot)
    {
        buildSeriesPath(plot, m_pointFps, true);
        if (!m_path.isEmpty())
        {
            const Color series = seriesColor(event);
            Color fill = series;
            fill.alpha = k_seriesFillAlpha;
            Color stroke = series;
            stroke.alpha = k_seriesStrokeAlpha;
            event.canvas().drawPath(
                m_path,
                {
                    Graphics::PathDrawLayer::fill(fill),
                    Graphics::PathDrawLayer::stroke(stroke, event.scale(1.0f))
                }
            );
        }

        // DRAWN OVER THE AREA AND NEVER FILLED. It is the rate the drawing alone would allow -
        // a ceiling the arrivals stand under, not a second reading to be compared by area.
        buildSeriesPath(plot, m_pointDrawFps, false);
        if (m_path.isEmpty())
            return;

        Color drawLine{ k_guideColor };
        drawLine.alpha = k_drawLineAlpha;
        event.canvas().drawPath(m_path, drawLine, event.scale(1.0f));
    }

    void FpsChart::paintAxis(PaintEvent& event, const FloatRect& plot, float topLineValue)
    {
        const float labelHeight = event.scale(k_axisFontSize);
        const float minSpacing = labelHeight + event.scale(k_axisLabelSpacing);
        const float bottomY = plot.bottom - labelHeight / 2.0f;

        auto drawReading = [&](float value, float topY){
            const FloatRect bounds{
                event.controlBounds().left,
                topY,
                plot.left - event.scale(k_axisLabelMargin),
                topY + labelHeight + event.scale(k_axisLabelSpacing)
            };
            Control::textEngine().drawText(
                event.controlContext(),
                bounds,
                Text{
                    TextAlign::Right,
                    TextStyleId::Code,
                    PushFontSize{ static_cast<int>(k_axisFontSize) },
                    static_cast<int>(value)
                },
                { VerticalTextAnchor::Top, HorizontalTextAnchor::Left }
            );
        };

        // The top reading stands whatever the spacing says.
        const float topY = yOf(plot, topLineValue) - labelHeight / 2.0f;
        drawReading(topLineValue, topY);

        // The readings between are taken from the top downwards, and one needs room both from the
        // reading above it and from the bottom one.
        float lastPlacedY = topY;
        for (float value = topLineValue - guideStep(); value > 0.0f; value -= guideStep())
        {
            const float y = yOf(plot, value) - labelHeight / 2.0f;
            if (y - lastPlacedY < minSpacing || bottomY - y < minSpacing)
                continue;

            drawReading(value, y);
            lastPlacedY = y;
        }

        // The bottom reading - no frames at all - stands whatever the spacing says.
        drawReading(0.0f, bottomY);
    }
}
