export module ClaFi.Diagnostic.FpsPage;

import ClaFi.Diagnostic.FpsChart;
import ClaFi.Diagnostic.Benchmark;

import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Button;
import ClaFi.Controls.Label;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.Foundation;

import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;
import ClaFi.Controls.ScrollBox;

namespace ClaFi::Diagnostic
{
    using namespace Controls;

    // What the framework window under the pointer costs to paint. See Diagnostic
    export class FpsPage : public ScrollBoxWith<StackPanel>
    {
    public:
        explicit FpsPage(const CreateParams&);
    public:
        std::wstring_view diagnosticText() const override { return L"FpsPage"; }
    protected:
        void visibilityChanged() override;
    private:
        // The footer holds its OWN layout - see WithTextLayout. Its count moves with every
        // frame, and a layout taken from the cache for each value would evict what the rest of
        // the window draws from.
        using Readout = WithTextLayout<Label>;
        using Grid = Controls::Grids::Dt::Grid;
        using CellSet = Controls::Grids::Dt::CellSet;
        using GetCellTextEvent = Controls::Grids::GetCellTextEvent;
        using GetCellTooltipEvent = Controls::Grids::GetCellTooltipEvent;
        // The columns of the window grid, as their Tags.
        enum class WindowColumn
        {
            Name,
            Value
        };
        // The rows of the window grid.
        enum class WindowRow
        {
            Role,
            Title,
            Root,
            Size,
            Scale,
            Backend
        };
        // The rows of the metrics grid.
        enum class Metric
        {
            PaintTime,
            DrawRate,
            FrameRate,
            CpuCore
        };
        // The readings of a metric, as the columns of the metrics grid. Stats is the reading
        // over the metric's own window: the benchmark's history for the frames, the last ten
        // samples for the CPU.
        enum class Reading
        {
            Stats,
            Last,
            Worst,
            Best
        };
        // What the window grid says about the target. Copied while the target is live - see
        // the class note.
        struct TargetFacts
        {
            std::optional<WindowRole> role{};
            std::wstring title{};
            std::wstring root{};
            int scalePercent{ 0 };
        };
        // How a metric is named and written. The unit is spelled as it follows the number,
        // gap included: a percent sign closes on its number, a word stands off it.
        struct MetricSpec
        {
            std::wstring_view name;
            std::wstring_view unit;
            int decimals;
        };
    private:
        // One tick of the drive: finds the window under the pointer and asks it for a frame.
        void drive();
        void stopDriving();
        void formPainted(FormPaintedEvent&);
        // The form under the pointer, unless it is this page's own. nullptr for none.
        [[nodiscard]] FormBase* formUnderPointer();
        void takeTargetFacts(FormBase&);
        // Starts the measurement over: the readings, the chart and the CPU share.
        void restartMeasurement();
        // The rows of the two grids, built once by the constructor.
        [[nodiscard]] Controls::Grids::Dt::Row windowRow(WindowRow);
        [[nodiscard]] Controls::Grids::Dt::Row metricRow(Metric);
        // What the cells say. A reading cell knows its metric from the row's Tag and its reading
        // from the column's.
        void writeWindowName(GetCellTextEvent&) const;
        void writeWindowValue(GetCellTextEvent&) const;
        void writeSize(Text&) const;
        void writeBackend(Text&) const;
        void writeReading(GetCellTextEvent&) const;
        void writeFooter(GetTextEvent&) const;
        // The hints: one per cell of each grid, the method on the caption, the curves on the
        // chart.
        void writeWindowTooltip(GetCellTooltipEvent&) const;
        void writeReadingTooltip(GetCellTooltipEvent&) const;
        static void writeMethodTooltip(GetTooltipEvent&);
        static void writeChartTooltip(GetTooltipEvent&);
        // The readings themselves. A metric that has measured nothing answers none.
        [[nodiscard]] std::optional<double> readingOf(Metric, Reading) const;
        [[nodiscard]] std::optional<double> paintTimeOf(Reading) const;
        [[nodiscard]] std::optional<double> frameRateOf(Reading) const;
        [[nodiscard]] std::optional<double> cpuUsageOf(Reading) const;
        // Last, Worst and Best of an aggregate. Stats is each metric's own and never asked here.
        [[nodiscard]] static std::optional<double> aggregateOf(const BenchmarkValue&, Reading);
        [[nodiscard]] static std::optional<double> rateOf(std::optional<double> milliseconds);
    private:
        // The share of the window a paint has to cover to be a whole frame of it.
        static constexpr double k_wholePaintShare{ 0.8 };
        // Between ticks with a window under the pointer, and between looks without one.
        static constexpr MilliSeconds k_driveInterval{ 0 };
        static constexpr MilliSeconds k_lookInterval{ 100 };
        // How often the readings are repainted, and how often the chart is. Each repaint is a
        // paint of this window inside an interval it reports, so the readings - two grids of
        // text - wait; the chart is the one picture of time here, a frame it skipped is a frame
        // it does not show, and its plot is small enough to follow every one.
        static constexpr std::chrono::milliseconds k_readoutInterval{ 100 };
        static constexpr std::chrono::milliseconds k_chartInterval{ 0 };
        // THE COLUMNS ARE FIXED. A column fitted to its digits would ask for a new width
        // whenever a value gained one, and an align pass is paid by the whole window. A
        // five-digit rate and its unit in the mono face, plus the cell padding, is what the
        // reading width holds; the Stats column holds the same at size 18 over the metric's
        // name.
        static constexpr float k_statsWidth{ 108.0f };
        static constexpr float k_readingWidth{ 88.0f };
        static constexpr float k_footerWidth{ 232.0f };
        static constexpr float k_restartIconSize{ 16.0f };
        static constexpr std::array<std::wstring_view, 4> k_windowRoleNames{
            L"Dialog",
            L"Menu",
            L"Tooltip",
            L"Timer"
        };
        static constexpr std::array<MetricSpec, 4> k_metricSpecs{ {
            { L"Paint time", L" ms", 2 },
            { L"Draw rate", L" FPS", 0 },
            { L"Frame rate", L" FPS", 0 },
            { L"CPU core", L"%", 2 }
        } };
    private:
        FpsBenchmark m_benchmark{};
        // The window the readings are of, FOR IDENTITY ONLY - see the class note.
        const FormBase* m_target{ nullptr };
        // Whether the last tick found a window to drive.
        bool m_driving{ false };
        TimePoint m_readoutShownAt{};
        TimePoint m_chartShownAt{};
        TargetFacts m_facts{};
        // The four reading cells every metric row shares, built ahead of the grid that
        // references them.
        CellSet m_readingCells;
        // The controls, built in the constructor in this order.
        Label& m_caption;
        Grid& m_windowGrid;
        Grid& m_metricsGrid;
        // The restart button and the count of what it would discard, on one line.
        StackPanel& m_footerBar;
        ToolButton& m_restartButton;
        Readout& m_footer;
        FpsChart& m_chart;
        EventRepeater m_drive;
        // On the application's dispatcher, which outlives this page - see FormPaintedEvent.
        ScopedEventConnection m_paintConnection;
    };
}
