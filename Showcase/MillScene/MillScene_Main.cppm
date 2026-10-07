module;
#include <cstdio>
export module MillScene_App.Main;

import MillScene_App.Icon;

import ClaFi.App.Application;
import ClaFi.App.Settings;

import ClaFi.PathArt.MillScene;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.Types;

import ClaFi.Diagnostic.Benchmark;
import ClaFi.Diagnostic.FpsChart;

import ClaFi.Controls.Panel;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.Stack;
import ClaFi.Controls.AppButton;
import ClaFi.Controls.Label;
import ClaFi.Controls.Slider;
import ClaFi.Controls.Button;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Fmt;

import ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.UpdateCheck;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

// NOTHING HERE NAMES A PLATFORM OR A BACKEND. What the project file keeps is the entry point its
// operating system asks for and the two types the application is built out of; everything else -
// the form, its size, its title and what is built into it - stands here and would run over any
// platform the framework has.
namespace MillScene_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // THE LANES THE DROPDOWN OPENED AS. Twenty-four themes in lanes of eight are the three
    // columns the picker had in the title bar, and the page keeps them.
    constexpr std::size_t k_themeLaneSize{ 8 };
    constexpr float k_themeIconSize{ 24.0f };
    // The corner the swatch was drawn with, as a fraction of it rather than the 4 design pixels
    // it was: what paints the icon here is handed a rect and no scaler.
    constexpr float k_themeIconRadiusRatio{ 4.0f / k_themeIconSize };
    constexpr float k_themeLaneSpacing{ 12.0f };

    // What the metrics hint says draws the scene. See writeMetricsHint
    constexpr std::wstring_view k_drawnOnGpu{
        L"The scene is drawn through the graphics card - GPU acceleration is on in Settings." };
    constexpr std::wstring_view k_drawnOnCpu{
        L"The scene is rendered on a single CPU thread - GPU acceleration is off in Settings." };
    constexpr std::wstring_view k_drawnOnCpuOnly{
        L"The scene is rendered on a single CPU thread with no GPU acceleration." };

    // WHAT THE PICKED SCENE THEME IS KEPT UNDER, and it is kept by name: an index moves the
    // moment the list gains a theme, and a name is what a config file read by hand should show.
    export constexpr std::wstring_view k_sceneThemeNodeName{ L"SceneTheme" };

    export class MillSceneForm : public Panel
    {
    public:
        template<typename... Args>
        explicit MillSceneForm(const CreateParams&, Args...);
    private:
        // Draw time and frame rate over the span the chart shows
        struct WindowReadings
        {
            double drawMs{ 0.0 };
            double fps{ 0.0 };
        };
    private:
        void paintScene(PaintEvent&);
        void advanceCycleTime();
        void applySceneColors() const;
        void previewSceneTheme(std::size_t index);
        // The application's own page of the backstage: the theme picker. See MillScene_Showcase
        void pickSceneTheme(std::size_t index);

        void buildScenePage(OptionsPage&);

        void restoreSceneTheme();
        void storeSceneTheme();

        void traceMetrics();
        void writeMetrics(GetTextEvent&);
        [[nodiscard]] WindowReadings windowReadings() const;
        void writeMetricsHint(GetHintEvent&);

    private:
        DialogTitle& m_titleBar{ createTopBar<DialogTitle>(
            appContext().appName())
        };

        // The theme the user chose, which is not the theme the scene wears while the pointer
        // walks the picker - see previewSceneTheme.
        std::size_t m_pickedTheme{ 0 };

        // The backstage's Scene page, refilled every time that page is built: a pick repaints the
        // button it left as well as the one it took, and the stack is what the preview returns to.
        Stack* m_themeLanes{ nullptr };
        std::vector<Control*> m_themeButtons{};
        // The chart draws the benchmark above; it wears the scene's own colours, which is why it
        // is held rather than added and forgotten.
        // When the readings last went to stderr. See traceMetrics.
        Diagnostic::TimePoint m_prevTrace{ Diagnostic::Clock::now() };

        PathArt::MillScene m_scene{};
        Diagnostic::FpsBenchmark m_benchmark{};

        // Where the scene stands in its 40-second cycle, as a 0..1 ratio.
        float m_cycleTime{ 0.0f };
        Diagnostic::TimePoint m_prevRun{Diagnostic::Clock::now() };
        // The scene runs at the speed this slider names. While there is no slider it runs at 1.
        Slider* m_speedSlider{ nullptr };

        AppButton& m_appButton{ m_titleBar.createLeftBar<AppButton>(
            VerticalAlign::Center
            ,      OnEvent{ AppIcon::paintIcon }
        ) };
        Panel& m_scenePanel{ createBody<Panel>(
            Padding{ 8.0f },
            Events{
                [this](MouseMoveEvent&) {
                    m_scene.animationTick();
                },
                [this](PaintEvent& event) {
                    paintScene(event);
                }
            }
        ) };
        EventRepeater m_paintTimer{ [this](RepeatEvent&) {
                m_scenePanel.invalidate();
            }, MilliSeconds{ 0 } };

        Stack& m_infoBar{ m_scenePanel.createRightBar<Stack>(
            Orientation::Vertical
            ) };
        using MetricsReadout = WithTextLayout<Label>;
        MetricsReadout& m_metricsReadout{ m_infoBar.add<MetricsReadout>(
            // THE WIDTH IS STATED, and it is the chart's. The reading is rewritten every frame, so
            // a readout sized by its own words asks the bar for a new width every time a number
            // gains a digit - and an align pass per frame is paid by the whole form.
            MinSize{Diagnostic::k_fpsChartWidth, 0.0f },
            MaxSize{Diagnostic::k_fpsChartWidth, k_maxFloat },
            WordWrap::No,
            OnEvent{ [this](GetTextEvent& event) {
                writeMetrics(event);
            } },
            OnEvent{ [this](GetHintEvent& event) {
                writeMetricsHint(event);
            } }
        ) };

        Diagnostic::FpsChart& m_fpsChart{ m_infoBar.add<Diagnostic::FpsChart>(m_benchmark) };

        // To move the scene away from the bottom edge, so the window rounded corners remain intact.
        // RichControl rather than Spacer, because Spacer doesn't have color
        Control& m_bottomSpacer{ createBottomBar<RichControl>(
            MinSize{ 6.0f },
            UiElement::Section
        ) };
    };

    template <typename ... Args>
    MillSceneForm::MillSceneForm(const CreateParams& params, Args... args)
        :
        Panel{ params, std::forward<Args>(args)... }
    {
        // BEFORE ANYTHING READS THE SCENE. The info bar takes its colours from the theme, so the
        // stored one has to be the scene's by now.
        restoreSceneTheme();


        connectAppPage(L"Scene", [this](OptionsPage& page) {
            buildScenePage(page);
            });
        applySceneColors();

        m_paintTimer.start();
    }

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] AppParams millSceneAppParams();

    // WHAT THIS APPLICATION KEEPS OF ITS OWN, handed to the application beside the framework's
    // schema and merged with it. The picked scene theme is the whole of it so far; empty is no
    // pick yet, and the scene then stands on the theme it starts with.
    export [[nodiscard]] Dom::Dt::Section createMillSceneConfigSchema();

    /// @brief Builds the showcase into a form of the given application, runs it, and answers what
    /// the form answered.
    export int runMillSceneShowcase(ApplicationBase&);


//-----------------------------------------------------------------------------


void MillSceneForm::paintScene(PaintEvent& event)
{
    const FloatRect& sceneRect = event.controlBounds();
    m_benchmark.setDimensions(sceneRect.dimensions());

    advanceCycleTime();

    // A paint covering at least 80% of the scene is a full one, and only a full one is
    // measured: a repaint of the combo box or the slider covers a fraction of the scene and
    // would read as a spike. The 80% leaves room for the off-screen clipping a maximized
    // window brings.
    const IntRect& clipRect = event.canvas().clipBox().toInt();
    const double clipArea = static_cast<double>(clipRect.width()) * clipRect.height();
    const double sceneArea = static_cast<double>(sceneRect.width()) * sceneRect.height();
    const bool isSignificantPaint = (clipArea / sceneArea) > 0.8;

    if (isSignificantPaint)
    {
        m_benchmark.run([this, &event]() {
            m_scene.paint(event.canvas(), event.controlBounds(), m_cycleTime);
            });
        traceMetrics();
    }
    else
    {
        m_scene.paint(event.canvas(), event.controlBounds(), m_cycleTime);
    }

}

void MillSceneForm::advanceCycleTime()
{
    if (m_benchmark.buckets().empty())
        return;

    using namespace std::chrono;
    constexpr seconds k_cycleDuration{ 40 };

    const Diagnostic::TimePoint newRun = Diagnostic::Clock::now();
    const Diagnostic::Clock::duration sincePreviousRun = newRun - m_prevRun;
    m_prevRun = newRun;
    // The step is a ratio of the 40 seconds a full cycle takes.
    const float timeSincePreviousEvent = static_cast<float>(sincePreviousRun.count())
        / duration_cast<Diagnostic::Clock::duration>(k_cycleDuration).count();

    float speed = 1.0f;
    if (m_speedSlider)
    {
        speed = m_speedSlider->relativePosition() - 0.5f;
        bool negative = speed < 0.0f;
        speed *= 8;
        speed *= speed;
        speed = (negative ? -speed : speed);
    }
    m_cycleTime += timeSincePreviousEvent * speed;

    if (m_cycleTime > 1.0f)
        m_cycleTime = 0.0f;

}

// THE PICKED THEME'S COLOURS, NOT THE BLEND'S. This runs the moment a theme is picked, when
// the blend still wears the one before it - and at startup, when it wears nothing at all.
void MillSceneForm::applySceneColors() const
{
    const PathArt::SceneTheme& theme = m_scene.pickedTheme();
    m_fpsChart.setColors(theme.riverBottom, theme.riverWave);
}

void MillSceneForm::previewSceneTheme(std::size_t index)
{
    m_scene.setTheme(index);
    m_benchmark.reset();
    applySceneColors();
}

void MillSceneForm::pickSceneTheme(std::size_t index)
{
    const std::size_t previous = m_pickedTheme;
    m_pickedTheme = index;
    previewSceneTheme(index);
    storeSceneTheme();

    if (m_themeLanes)
        m_themeLanes->setCurrentItem(*m_themeButtons[index]);
    m_themeButtons[previous]->invalidateState();
    m_themeButtons[index]->invalidateState();
}

void MillSceneForm::buildScenePage(OptionsPage& page)
{
    Stack& group = page.addGroup(L"Scene Theme");
    // THE LANES ARE STATED, NOT WRAPPED. A wrapping stack's MINIMUM is one lane - wrapping is
    // its promise to fit whatever width it is given - so nothing measuring this page could
    // learn that it wants three, and the third lane was laid out past the popup's edge. A row
    // of columns has the width of its columns for a minimum, which is the truth.
    //
    // THE ROW OWNS THE BUTTONS, not the columns: a container answers canFocusItem only where
    // it follows the user (Interactivity::ActiveContainer, see StackBase::followsUser),
    // and the walk from the pointer returns the first control it says yes to - the button,
    // through the column standing between them. That is what Stack::nestedControlHovered
    // means by a stack holding its items in groups.
    Stack& lanes = group.add<Stack>(
        Orientation::Horizontal,
        Spacing{ k_themeLaneSpacing, 0.0f },
        VerticalAlign::Top,
        Interactivity::ActiveContainer
    );

    m_themeLanes = &lanes;
    // WHAT THE POINTER IS ON, WHICH IS NOT WHAT THE USER HAS CHOSEN. A stack previewing what
    // it is hovered over goes back to its CURRENT ITEM when the pointer leaves it, so the
    // pick is made that item below and the scene finds its own way home - see PreviewEvent.
    lanes.setPreviewMode(PreviewMode::Hover);
    lanes.onPreview([this](PreviewEvent& event) {
        previewSceneTheme(event.item.tag().value);
        });
    // The popup takes the page with it when it closes, and the pointer may never have left
    // the stack - so the scene is put back here rather than waiting for a leave that a closed
    // window will not send.
    lanes.onDestroy([this](DestroyEvent&) {
        previewSceneTheme(m_pickedTheme);
        m_themeLanes = nullptr;
        m_themeButtons.clear();
        });

    m_themeButtons.clear();
    m_themeButtons.reserve(PathArt::Themes::allThemes.size());
    Stack* lane = nullptr;
    std::size_t index = 0;
    for (const PathArt::SceneTheme& theme : PathArt::Themes::allThemes)
    {
        // A column every k_themeLaneSize themes, so the count follows the list rather than
        // being stated beside it.
        if (index % k_themeLaneSize == 0)
            lane = &lanes.add<Stack>(Orientation::Vertical);

        ToolButton& item = lane->add<ToolButton>(
            Text{ theme.name },
            Tag{ index },
            ButtonViewMode::LeftIcon,
            IconSize{ k_themeIconSize, k_themeIconSize },
            // The pick wears the surface, the way the app button wears the open backstage.
            ShowSelectionOnSurface::Yes
        );
        // The theme is read off the button's own tag, so one painter answers for all of them.
        item.onPaintIcon([](PaintIconEvent& event) {
            const PathArt::SceneTheme& painted = PathArt::Themes::allThemes[event.tag().value];
            const float radius = event.iconWidth() * k_themeIconRadiusRatio;
            painted.paintIcon(event.canvas(), event.iconRect(), radius, 1.0f);
            });
        item.onGetState([this, index](GetStateEvent& event) {
            event.state.selected = m_pickedTheme == index;
            });
        item.onClick([this, index](ClickEvent&) {
            pickSceneTheme(index);
            });
        m_themeButtons.push_back(&item);
        ++index;
    }
    // What a stack stands on is its current item, and that is where a preview returns.
    lanes.setCurrentItem(*m_themeButtons[m_pickedTheme]);

}

void MillSceneForm::restoreSceneTheme()
{
    const std::wstring name =
        (appContext().config() / k_sceneThemeNodeName).get<std::wstring>();
    if (name.empty())
        return;

    std::size_t index = 0;
    for (const PathArt::SceneTheme& theme : PathArt::Themes::allThemes)
    {
        if (theme.name == name)
        {
            m_scene.setTheme(index);
            m_pickedTheme = index;
            return;
        }
        ++index;
    }
}

void MillSceneForm::storeSceneTheme()
{
    (appContext().config() / k_sceneThemeNodeName).set(
        std::wstring{ m_scene.pickedTheme().name });
}

void MillSceneForm::traceMetrics()
{
#pragma warning(push)
#pragma warning(disable : 4996) // 'getenv': the CRT's alternative is Windows-only
    static const bool s_enabled = std::getenv("CLAFI_FPS_TRACE") != nullptr;
#pragma warning(pop)
    if (!s_enabled)
        return;

    using namespace std::chrono;
    const Diagnostic::TimePoint now = Diagnostic::Clock::now();
    if (now - m_prevTrace < seconds{ 1 })
        return;

    m_prevTrace = now;
    const WindowReadings readings = windowReadings();
    std::fprintf(stderr,
        "[fps] %.0fx%.0f  draw %6.2f ms (%5.1f fps)  frame %6.2f ms (%5.1f fps)  cpu %5.1f%%\n",
        m_benchmark.dimensions().x, m_benchmark.dimensions().y,
        readings.drawMs, readings.drawMs > 0.0 ? 1000.0 / readings.drawMs : 0.0,
        readings.fps > 0.0 ? 1000.0 / readings.fps : 0.0, readings.fps,
        m_benchmark.cpuUsage().lastValue());
}

void MillSceneForm::writeMetrics(GetTextEvent& event)
{
    double cpuVal = m_benchmark.cpuUsage().lastValue();
    if (cpuVal > 10000.0)
        cpuVal = 0.0;

    const WindowReadings readings = windowReadings();

    // TODO: the text disappears under some conditions - which ones?
    event.text << Fmt{
        L"[code][right]"
        L"[nobr]{:.0f}[color muted]x[/color]{:.0f} [color muted]pix\u202F[space 0][/color]\n"
        L"{:.2f} [color muted]CPU\u202F[space 0][/color]\n"
        L"{:.2f} [color muted]ms\u202F\u202F[space 0][/color]\n"
        L"{:.0f} [color muted]draw[/color]\n"
        L"{:.0f} [color muted]FPS\u202F[space 0][/color]\n[/nobr]"
        ,
        m_benchmark.dimensions().x, m_benchmark.dimensions().y,
        cpuVal,
        readings.drawMs,
        readings.drawMs > 0.0 ? 1000.0 / readings.drawMs : 0.0,
        readings.fps
    };
}

// The same window and the same sums as the FPS page - see FpsPage::paintTimeOf and
// FpsPage::frameRateOf. A window with no frame in it reads zero.
MillSceneForm::WindowReadings MillSceneForm::windowReadings() const
{
    const Diagnostic::FpsBenchmark::Recent recent =
        m_benchmark.recent(Diagnostic::FpsBenchmark::k_historySpan, Diagnostic::Clock::now());
    WindowReadings result;
    result.drawMs = recent.wholePaintMs;
    result.fps = recent.frames * 1000.0 / static_cast<double>(Diagnostic::FpsBenchmark::k_historySpan.count());
    return result;
}

void MillSceneForm::writeMetricsHint(GetHintEvent& event)
{
    event.placement = FormPlacement::Bottom;
    auto paintBullet = [](PaintIconEvent& event) {
        float radius = event.scaleF(3.0f);
        event.canvas().fillCircle(event.iconRect().center(), radius, event.textRgb(InkGrade::Strongest));
        };
    // An application that named no GPU backend offers no such setting.
    std::wstring_view drawnOn = k_drawnOnCpuOnly;
    if (appContext().usingGpu())
        drawnOn = k_drawnOnGpu;
    else if (appContext().gpuAvailable())
        drawnOn = k_drawnOnCpu;
    event.text.clear();
    event.text << Fmt{
        L"[section]Performance Metrics\n[/section]"
        L"[space 16]{icon 18, 18}Resolution (pix): [color muted]Dimensions of the current scene.\n"
        L"[color strongest][space 16]{icon 18, 18}CPU usage (%): [color muted]Single-core utilization by this specific process.\n"
        L"[color strongest][space 16]{icon 18, 18}Time (ms): [color muted]Measured time to render the scene to a bitmap.\n"
        L"[color strongest][space 16]{icon 18, 18}draw: [color muted]What that time alone would allow, as 1000/Time.\n"
        L"[color strongest][space 16]{icon 18, 18}FPS: [color muted]The rate frames actually arrived at, measured from one to the next[color strongest]."
        L"[vspace 18]\n"
        L"[section]Note:\n[/section][color strongest]"
        L"[space 18]{}\n"
        L"[space 18]The gap between [size 20]draw[/size] and [size 20]FPS[/size] is everything a frame waits on\n"
        L"[space 18]that the scene is not: the rest of the form, the copy to the screen,\n"
        L"[space 18]and whatever pace the display server keeps.\n"
        L"[space 18]The chart shows both - the filled area is FPS, the line over it draw.\n"
        L"[space 18]Time, draw and FPS are read over the two seconds the chart spans.",
        paintBullet, paintBullet, paintBullet, paintBullet, paintBullet,
        drawnOn
    };
}


//-----------------------------------------------------------------------------


    AppParams millSceneAppParams()
    {
        return {
            .name = Text{ L"MillScene" },
            .publisher = L"ClaFi Framework",
            .description = Text{
                L"Shows the ClaFi path painter at work: "
                L"a watermill landscape animated on a single CPU thread or through the graphics card, "
                L"in a choice of scene themes, with its frame times charted live."
            },
            .version = CLAFI_APP_VERSION,
            .updates = UpdateSource{
                .repository = L"clafi-fw/cpp",
                .tagPrefix = L"MillScene-v"
            }
        };
    }

    Dom::Dt::Section createMillSceneConfigSchema()
    {
        return Dom::Dt::Section{ Dom::Dt::Value{ k_sceneThemeNodeName, L"" } };
    }

    constexpr float k_minWindowWidth{ 640.0f };
    constexpr float k_minWindowHeight{ 480.0f };

    int runMillSceneShowcase(ApplicationBase& application)
    {
        const auto form = application.createDialog<MillSceneForm>(
            application.metrics().primaryWindow,
            application.metrics().primaryWindowShadow,
            MinSize{ k_minWindowWidth / 2.0f, k_minWindowHeight / 2.0f },
            PreferredSize{ k_minWindowWidth, k_minWindowHeight }
        );
        form->setConfigName(AppContext::k_mainFormName);
        return form->execute();
    }
}
