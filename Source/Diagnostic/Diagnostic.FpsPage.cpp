module ClaFi.Diagnostic.FpsPage;

import ClaFi.Diagnostic.Benchmark;
import ClaFi.Diagnostic.FpsChart;

import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Button;
import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Label;
import ClaFi.Controls.StackPanel;

import ClaFi.Icons.RestartIcon;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;

import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Diagnostic
{
    using namespace ::ClaFi::Controls;

    using Grids::Dt::Cell;
    using Grids::Dt::Column;
    using Grids::Dt::Columns;
    using Grids::Dt::ColumnWidthMode;
    using Grids::Dt::Header;
    using Grids::Dt::MovingText;
    using Grids::Dt::Row;
    using Grids::Dt::Rows;
    using Grids::GridLines;

    // What a reading that has measured nothing is shown as.
    constexpr std::wstring_view k_noReading{ L"-" };

    constexpr std::array<std::wstring_view, 6> k_windowRowNames{
        L"Role",
        L"Title",
        L"Root",
        L"Size",
        L"Scale",
        L"Backend"
    };

    // What each backend is called where it is named, indexed as Graphics::BackendType.
    constexpr std::array<std::wstring_view, 2> k_backendNames{
        L"CPU",
        L"Direct2D (GPU)"
    };

    // What each row of the window grid is about.
    constexpr std::array<std::wstring_view, 6> k_windowRowNotes{
        L"What kind of window it is: a Dialog, a Menu, a Tooltip or a Timer.",
        L"The window's title, as its form last stated it.",
        L"The control at the root of the window, by its diagnostic name.",
        L"The surface painted, in pixels.",
        L"The system scale the window is drawn at.",
        L"What the windows are drawn through. One answer serves the whole\n"
        L"application, so this stands whether or not a window is being measured."
    };

    // What each metric measures, indexed as FpsPage::Metric.
    constexpr std::array<std::wstring_view, 4> k_metricNotes{
        L"The measured time of a whole frame of the window - a paint covering at\n"
        L"least 80% of it. A smaller paint is a control repainting itself, an arrival\n"
        L"like any other, and states no time.",
        L"What the paint time alone would allow, as 1000 / Paint time. The gap to\n"
        L"Frame rate is everything a frame waits on that the painting is not: the\n"
        L"copy to the screen, and whatever pace the display server keeps.",
        L"Paints of the window per second, whatever each covered - the rate the\n"
        L"window is actually seen at.",
        L"Single-core utilization by this process, sampled once a second."
    };

    // What a reading of a metric is, indexed as FpsPage::Reading, for a metric measured per
    // frame and for the one sampled on its own clock.
    using ReadingNotePair = std::array<std::wstring_view, 2>;
    constexpr std::array<ReadingNotePair, 4> k_readingNotes{ {
        { L"Over the last two seconds - what the chart spans.",
          L"The mean of the last ten samples." },
        { L"The last frame.",
          L"The last sample." },
        { L"The worst since the pointer reached this window, or since the last restart.",
          L"The highest sample since the last restart." },
        { L"The best since the pointer reached this window, or since the last restart.",
          L"The lowest sample since the last restart." }
    } };

    // What a column of the metrics grid holds, for its header cell.
    constexpr std::array<std::wstring_view, 4> k_headerNotes{
        L"Over the metric's own window: the last two seconds for the frame readings,\n"
        L"which is what the chart spans, and the last ten samples for CPU core.",
        L"The last frame - for CPU core, the last sample.",
        L"The worst since the pointer reached this window, or since the last\n"
        L"restart. CPU core's since the restart alone.",
        L"The best since the pointer reached this window, or since the last\n"
        L"restart. CPU core's since the restart alone."
    };

    [[nodiscard]] std::wstring_view orNoReading(const std::wstring_view value)
    {
        return value.empty() ? k_noReading : value;
    }

    FpsPage::FpsPage(const CreateParams& params)
        :
        ScrollBoxWith{
            params,
            HostProps{},
            BodyProps{ Orientation::Vertical, Padding{ 8.0f }, Spacing{ 8.0f } }
        },
        m_readingCells{
            Cell{ Tag{ Reading::Stats }, this, &FpsPage::writeReading },
            Cell{ Tag{ Reading::Last }, this, &FpsPage::writeReading },
            Cell{ Tag{ Reading::Worst }, this, &FpsPage::writeReading },
            Cell{ Tag{ Reading::Best }, this, &FpsPage::writeReading }
        },
        m_caption{ body().add<Label>(
            Text{ TextStyleId::Section, L"Window under the pointer" },
            OnEvent{ [](GetTooltipEvent& event) {
                writeMethodTooltip(event);
            } }
        ) },
        // The Value column moves for the size alone, and the words beside it cost nothing to
        // shape with it.
        m_windowGrid{ body().add<Grid>(
            themeMetrics().page,
            UiElement::Section,
            GridLines::Horizontal,
            OnEvent{ [this](GetCellTooltipEvent& event) {
                writeWindowTooltip(event);
            } },
            Columns{
                Column{ Tag{ WindowColumn::Name } },
                Column{ Tag{ WindowColumn::Value }, ColumnWidthMode::Fill, MovingText::Yes }
            },
            Rows{
                windowRow(WindowRow::Role),
                windowRow(WindowRow::Title),
                windowRow(WindowRow::Root),
                windowRow(WindowRow::Size),
                windowRow(WindowRow::Scale),
                windowRow(WindowRow::Backend)
            }
        ) },
        // The three small readings sit against the middle of the Stats cell, which is two
        // lines tall and makes every row its height.
        m_metricsGrid{ body().add<Grid>(
            themeMetrics().page,
            UiElement::Section,
            GridLines::Horizontal,
            OnEvent{ [this](GetCellTooltipEvent& event) {
                writeReadingTooltip(event);
            } },
            Columns{
                Column{ Tag{ Reading::Stats }, Text{ L"Stats" }, TextAlign::Right,
                    ColumnWidthMode::Fixed, k_statsWidth, MovingText::Yes },
                Column{ Tag{ Reading::Last }, Text{ L"Last" }, TextAlign::Right,
                    ColumnWidthMode::Fixed, k_readingWidth, MovingText::Yes, VerticalTextAnchor::Center },
                Column{ Tag{ Reading::Worst }, Text{ L"Worst" }, TextAlign::Right,
                    ColumnWidthMode::Fixed, k_readingWidth, MovingText::Yes, VerticalTextAnchor::Center },
                Column{ Tag{ Reading::Best }, Text{ L"Best" }, TextAlign::Right,
                    ColumnWidthMode::Fixed, k_readingWidth, MovingText::Yes, VerticalTextAnchor::Center }
            },
            Header{},
            Rows{
                metricRow(Metric::PaintTime),
                metricRow(Metric::DrawRate),
                metricRow(Metric::FrameRate),
                metricRow(Metric::CpuCore)
            }
        ) },
        m_footerBar{ body().add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ 4.0f }
        ) },
        // An icon-only button's words are its tooltip.
        m_restartButton{ m_footerBar.add<ToolButton>(
            IconSize{ k_restartIconSize },
            ButtonViewMode::IconOnly,
            OnPaintIcon{ Icons::RestartIcon::paint },
            L"Start the measurement over",
            OnEvent{ [this](ClickEvent&) {
                restartMeasurement();
            } }
        ) },
        m_footer{ m_footerBar.add<Readout>(
            MinSize{ k_footerWidth, 0.0f },
            MaxSize{ k_footerWidth, k_maxFloat },
            WordWrap::No,
            VerticalAlign::Center,
            OnEvent{ [this](GetTextEvent& event) {
                writeFooter(event);
            } }
        ) },
        m_chart{ body().add<FpsChart>(
            m_benchmark,
            OnEvent{ [](GetTooltipEvent& event) {
                writeChartTooltip(event);
            } }
        ) },
        m_drive{ [this](RepeatEvent&) {
            drive();
        }, k_driveInterval },
        m_paintConnection{
            appContext().events().connect<FormPaintedEvent>(this, &FpsPage::formPainted)
        }
    {
    }

    // The page drives only while it shows. Hidden - another tab in front of it - it asks nothing
    // of anyone, and the window it was driving is left to paint at its own pace.
    void FpsPage::visibilityChanged()
    {
        ScrollBoxWith::visibilityChanged();
        if (visible())
        {
            m_drive.setIntervals(k_driveInterval, k_driveInterval);
            m_drive.start();
            return;
        }

        m_drive.stop();
        stopDriving();
    }

    void FpsPage::drive()
    {
        FormBase* target = formUnderPointer();

        // With no window under the pointer the repeater looks again after a pause rather than
        // spinning on nothing. An interval set here is the one the NEXT re-arm takes.
        const MilliSeconds interval = target ? k_driveInterval : k_lookInterval;
        m_drive.setIntervals(interval, interval);

        if (!target)
        {
            stopDriving();
            return;
        }

        // Another window is another measurement: what the readings said about the last one says
        // nothing about this one.
        if (target != m_target)
        {
            m_target = target;
            m_benchmark.restart();
            takeTargetFacts(*target);
            m_windowGrid.invalidate();
        }
        m_driving = true;
        // The root's bounds are the window's geometry. The form's own invalidate is the whole
        // surface, margins included, which is a resize frame: it lays the shadow band down and
        // no interaction ever pays for one.
        target->content().invalidate();
    }

    // The run of frames ends here. Whatever paints the window makes on its own from now on are
    // not counted, and the next driven frame states no interval - see FpsBenchmark::interrupt.
    void FpsPage::stopDriving()
    {
        if (!m_driving)
            return;

        m_driving = false;
        m_benchmark.interrupt();
    }

    void FpsPage::formPainted(FormPaintedEvent& event)
    {
        if (!m_driving || &event.sender() != m_target)
            return;

        // Every paint is an arrival; one covering the window is a whole frame - see the class note.
        const IntRect& painted = event.dirtyRect();
        const FloatRect window = event.sender().geometry();
        const double paintedArea = static_cast<double>(painted.width()) * painted.height();
        const double windowArea = static_cast<double>(window.width()) * window.height();
        const FrameExtent extent = paintedArea < windowArea * k_wholePaintShare
            ? FrameExtent::Partial
            : FrameExtent::Whole;

        m_benchmark.setDimensions(window.dimensions());
        m_benchmark.record(event.startedAt(), event.finishedAt(), extent);

        if (event.finishedAt() - m_chartShownAt >= k_chartInterval)
        {
            m_chartShownAt = event.finishedAt();
            m_chart.invalidate();
        }
        if (event.finishedAt() - m_readoutShownAt < k_readoutInterval)
            return;
        m_readoutShownAt = event.finishedAt();
        // The sender is live for the length of this call, which is what a title or a scale that
        // moved is read on.
        takeTargetFacts(event.sender());
        m_windowGrid.invalidate();
        m_metricsGrid.invalidate();
        m_footer.invalidate();
    }

    FormBase* FpsPage::formUnderPointer()
    {
        Control* hovered = Input::hoveredControl();
        if (!hovered)
            return nullptr;

        // Its own window is the one window the page must not measure: a frame of it is a frame
        // of the measurement.
        FormBase& hoveredForm = hovered->form();
        if (&hoveredForm == &form())
            return nullptr;

        return &hoveredForm;
    }

    // Read while the target is LIVE - handed in by whoever holds it as such, never reached
    // through m_target.
    void FpsPage::takeTargetFacts(FormBase& target)
    {
        m_facts.role = target.windowRole();
        m_facts.title = target.windowTitle();
        m_facts.root = target.content().diagnosticText();
        m_facts.scalePercent = target.scaler().systemPercent();
    }

    // The frame side and the CPU share both, which a change of target is not: the share is the
    // process's whatever window is under the pointer, and only the user's own restart drops it.
    void FpsPage::restartMeasurement()
    {
        m_benchmark.restart();
        m_benchmark.resetCpuUsage();
        invalidate();
    }

    Row FpsPage::windowRow(const WindowRow row)
    {
        return Row{
            Tag{ row },
            Cell{ Tag{ WindowColumn::Name }, this, &FpsPage::writeWindowName },
            Cell{ Tag{ WindowColumn::Value }, this, &FpsPage::writeWindowValue }
        };
    }

    Row FpsPage::metricRow(const Metric metric)
    {
        return Row{
            Tag{ metric },
            m_readingCells
        };
    }

    void FpsPage::writeWindowName(GetCellTextEvent& event) const
    {
        const WindowRow row = event.row().tag().get<WindowRow>();
        event.text() << InkGrade::Muted << k_windowRowNames[static_cast<std::size_t>(row)];
    }

    void FpsPage::writeWindowValue(GetCellTextEvent& event) const
    {
        Text& text = event.text();
        switch (event.row().tag().get<WindowRow>())
        {
            case WindowRow::Role:
                text << (m_facts.role
                    ? k_windowRoleNames[static_cast<std::size_t>(*m_facts.role)]
                    : k_noReading);
                break;
            case WindowRow::Title:
                text << orNoReading(m_facts.title);
                break;
            case WindowRow::Root:
                text << orNoReading(m_facts.root);
                break;
            case WindowRow::Size:
                writeSize(text);
                break;
            case WindowRow::Scale:
                if (m_facts.scalePercent > 0)
                    text << Fmt{ L"{}%", m_facts.scalePercent };
                else
                    text << k_noReading;
                break;
            case WindowRow::Backend:
                writeBackend(text);
                break;
        }
    }

    void FpsPage::writeSize(Text& text) const
    {
        const ScaledDimensions dimensions = m_benchmark.dimensions();
        if (dimensions.x <= 0.0f)
        {
            text << Fmt{ L"[code]{}", k_noReading };
            return;
        }
        text << Fmt{ L"[code]{:.0f} x {:.0f}[color muted] pix[/color]", dimensions.x, dimensions.y };
    }

    // THE APPLICATION'S ANSWER, NOT THE TARGET'S. One context builds every window of an
    // application, so they all draw through the same kind and this page's own form answers for
    // them - which is what lets the row stand while nothing is being measured.
    void FpsPage::writeBackend(Text& text) const
    {
        const Graphics::IBackend* backend = formContext().canvas().backend();
        if (!backend)
        {
            text << k_noReading;
            return;
        }
        text << k_backendNames[static_cast<std::size_t>(backend->type())];
    }

    // Every number carries its unit; a reading there is none of is a dash alone. The Stats cell
    // carries the metric's name under its value, which is what names the row. The column
    // supplies the alignment.
    void FpsPage::writeReading(GetCellTextEvent& event) const
    {
        const Metric metric = event.row().tag().get<Metric>();
        const Reading reading = event.column().tag().get<Reading>();
        const MetricSpec& spec = k_metricSpecs[static_cast<std::size_t>(metric)];
        const std::optional<double> value = readingOf(metric, reading);
        const std::wstring number = value
            ? std::format(L"{:.{}f}", *value, spec.decimals)
            : std::wstring{ k_noReading };
        const std::wstring_view unit = value ? spec.unit : std::wstring_view{};
        if (reading != Reading::Stats)
        {
            event.text() << Fmt{ L"[code]{}[color muted]{}[/color]", number, unit };
            return;
        }
        event.text() << Fmt{
            L"[code][size 18]{}[/size][color muted]{}[/color]\n"
            L"[subbody][color muted]{}[/color]",
            number, unit, spec.name
        };
    }

    void FpsPage::writeFooter(GetTextEvent& event) const
    {
        event.text << Fmt{
            L"[subbody][color muted]Measured {} frames, {} whole[/color]",
            m_benchmark.frameInterval().timesNum(),
            m_benchmark.paintTime().timesNum()
        };
    }

    // Both columns of a row say the same thing: the row is one fact about the window.
    void FpsPage::writeWindowTooltip(GetCellTooltipEvent& event) const
    {
        if (&event.row() == m_windowGrid.header())
            return;

        const WindowRow row = event.row().tag().get<WindowRow>();
        event.tooltip().placement = FormPlacement::Bottom;
        event.text() << k_windowRowNotes[static_cast<std::size_t>(row)];
    }

    // A header cell says what its column holds; a data cell names its metric, says what it
    // measures, and then which reading of it this is.
    void FpsPage::writeReadingTooltip(GetCellTooltipEvent& event) const
    {
        const Reading reading = event.column().tag().get<Reading>();
        event.tooltip().placement = FormPlacement::Bottom;
        if (&event.row() == m_metricsGrid.header())
        {
            event.text() << k_headerNotes[static_cast<std::size_t>(reading)];
            return;
        }

        const Metric metric = event.row().tag().get<Metric>();
        const std::size_t sampled = metric == Metric::CpuCore ? 1 : 0;
        event.text() << Fmt{
            L"[section]{}\n[/section]{}\n[color muted]{}[/color]",
            k_metricSpecs[static_cast<std::size_t>(metric)].name,
            k_metricNotes[static_cast<std::size_t>(metric)],
            k_readingNotes[static_cast<std::size_t>(reading)][sampled]
        };
    }

    void FpsPage::writeMethodTooltip(GetTooltipEvent& event)
    {
        event.placement = FormPlacement::Bottom;
        event.text.clear();
        event.text << Fmt{
            L"[section]How the readings are taken\n[/section]"
            L"While this page shows, the window under the pointer is asked for a whole\n"
            L"frame whenever the thread is idle, and a core is spent on that. A moving\n"
            L"pointer holds it off, and the paints it makes are then what Frame rate\n"
            L"counts. Over this page's own window, or over no framework window,\n"
            L"nothing is driven and the readings stand where they were."
        };
    }

    void FpsPage::writeChartTooltip(GetTooltipEvent& event)
    {
        event.placement = FormPlacement::Bottom;
        event.text.clear();
        event.text << Fmt{
            L"[section]The last two seconds\n[/section]"
            L"The filled area is Frame rate, the line over it Draw rate. They meet\n"
            L"when the drawing is what limits the rate, and part when something\n"
            L"after the drawing is - the copy to the screen, or the display server's\n"
            L"own pace."
        };
    }

    std::optional<double> FpsPage::readingOf(const Metric metric, const Reading reading) const
    {
        switch (metric)
        {
            case Metric::PaintTime:
                return paintTimeOf(reading);
            case Metric::DrawRate:
                return rateOf(paintTimeOf(reading));
            case Metric::FrameRate:
                return frameRateOf(reading);
            case Metric::CpuCore:
                return cpuUsageOf(reading);
        }
        return std::nullopt;
    }

    // Over the history, the whole frames in it - or with none there the last whole frame there
    // was. The rest is the aggregate.
    std::optional<double> FpsPage::paintTimeOf(const Reading reading) const
    {
        if (reading != Reading::Stats)
            return aggregateOf(m_benchmark.paintTime(), reading);

        const FpsBenchmark::Recent recent = m_benchmark.recent(FpsBenchmark::k_historySpan, Clock::now());
        if (recent.wholeFrames > 0)
            return recent.wholePaintMs;
        return aggregateOf(m_benchmark.paintTime(), Reading::Last);
    }

    // Over the history, every paint of the window in it, whatever each covered. The rest is the
    // interval's aggregate turned into a rate, so the worst interval is the worst rate.
    std::optional<double> FpsPage::frameRateOf(const Reading reading) const
    {
        if (reading != Reading::Stats)
            return rateOf(aggregateOf(m_benchmark.frameInterval(), reading));

        if (m_benchmark.frameInterval().timesNum() == 0)
            return std::nullopt;
        const FpsBenchmark::Recent recent = m_benchmark.recent(FpsBenchmark::k_historySpan, Clock::now());
        return recent.frames * 1000.0 / static_cast<double>(FpsBenchmark::k_historySpan.count());
    }

    // The share is sampled once a second, so its window is its last ten samples - see
    // CpuUsage::recentAverage.
    std::optional<double> FpsPage::cpuUsageOf(const Reading reading) const
    {
        if (reading != Reading::Stats)
            return aggregateOf(m_benchmark.cpuUsage(), reading);

        return m_benchmark.recentCpuUsage();
    }

    std::optional<double> FpsPage::aggregateOf(const BenchmarkValue& value, const Reading reading)
    {
        if (value.timesNum() == 0)
            return std::nullopt;
        switch (reading)
        {
            case Reading::Last:
                return value.lastValue();
            case Reading::Worst:
                return value.maxValue();
            case Reading::Best:
                return value.minValue();
            case Reading::Stats:
                break;
        }
        unreachable("FpsPage: Stats is read over each metric's own window, never off the aggregate");
    }

    std::optional<double> FpsPage::rateOf(const std::optional<double> milliseconds)
    {
        if (!milliseconds || *milliseconds <= 0.0)
            return std::nullopt;
        return 1000.0 / *milliseconds;
    }
}
