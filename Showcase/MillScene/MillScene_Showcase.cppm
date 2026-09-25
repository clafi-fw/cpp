export module ClaFi.Showcase.MillScene;

import ClaFi.PathArt.MillScene;

import ClaFi.Diagnostic.Benchmark;
import ClaFi.Diagnostic.FpsChart;

import ClaFi.Controls.Panel;
import ClaFi.Controls.Slider;
import ClaFi.Controls.StackPanel;

import ClaFi.App.Application;
import ClaFi.App.Settings;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Showcase
{
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Diagnostic;
    using namespace ::ClaFi::PathArt;

    // WHAT THE PICKED SCENE THEME IS KEPT UNDER, and it is kept by name: an index moves the
    // moment the list gains a theme, and a name is what a config file read by hand should show.
    export constexpr std::wstring_view k_sceneThemeNodeName{ L"SceneTheme" };

    // The path painter demo: a mill scene painted on the CPU or through the graphics card as
    // Settings chooses, a theme picker in the backstage, and a metrics panel charting the last two
    // seconds of frame times.
    //
    // Every handler the constructor plants reads this object, so it has to outlive the form it
    // builds into.
    export class MillSceneShowcase
    {
    public:
        MillSceneShowcase(ApplicationBase&, Panel& content);
    private:
        // Draw time and frame rate over the span the chart shows
        struct WindowReadings
        {
            double drawMs{ 0.0 };
            double fps{ 0.0 };
        };
    private:
        void buildTitleBar(Panel& content);
        void buildScenePanel(Panel& content);
        void buildInfoBar();
        // The application's own page of the backstage: the theme picker. See MillScene_Showcase
        void buildScenePage(OptionsPage&);
        void pickSceneTheme(std::size_t index);
        void previewSceneTheme(std::size_t index);
        //
        void advanceCycleTime();
        void applySceneColors();
        void restoreSceneTheme();
        void storeSceneTheme();
        void paintScene(PaintEvent&);
        void traceMetrics();
        void writeMetrics(GetTextEvent&);
        [[nodiscard]] WindowReadings windowReadings() const;
        void writeMetricsTooltip(GetTooltipEvent&);
    private:
        // The config the picked scene theme is read out of and written back into.
        ApplicationBase& m_application;
        // The theme the user chose, which is not the theme the scene wears while the pointer
        // walks the picker - see previewSceneTheme.
        std::size_t m_pickedTheme{ 0 };
        // The backstage's Scene page, refilled every time that page is built: a pick repaints the
        // button it left as well as the one it took, and the stack is what the preview returns to.
        Controls::StackPanel* m_themeLanes{ nullptr };
        std::vector<Control*> m_themeButtons{};
        MillScene m_scene{};
        FpsBenchmark m_benchmark{};
        // Where the scene stands in its 40-second cycle, as a 0..1 ratio.
        float m_cycleTime{ 0.0f };
        TimePoint m_prevRun{ Clock::now() };
        // The scene runs at the speed this slider names. While there is no slider it runs at 1.
        Slider* m_speedSlider{ nullptr };
        Panel* m_scenePanel{ nullptr };
        // The chart draws the benchmark above; it wears the scene's own colours, which is why it
        // is held rather than added and forgotten.
        FpsChart* m_fpsChart{ nullptr };
        // When the readings last went to stderr. See traceMetrics.
        TimePoint m_prevTrace{ Clock::now() };
        EventRepeater m_paintTimer;
    };
}
