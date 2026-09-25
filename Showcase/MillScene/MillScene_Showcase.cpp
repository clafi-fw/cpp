module;
#include <cstdio>
module ClaFi.Showcase.MillScene;

import ClaFi.Showcase.MillScene.Icon;

import ClaFi.PathArt.MillScene;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.Types;

import ClaFi.Diagnostic.Benchmark;
import ClaFi.Diagnostic.FpsChart;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.FormTitle;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Slider;
import ClaFi.Controls.StackPanel;

import ClaFi.App.Application;
import ClaFi.App.Settings;

import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Showcase
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Graphics;
    using namespace ::ClaFi::PathArt;
    using namespace ::ClaFi::Diagnostic;

    // The readout holds its OWN layout. Its numbers are rewritten on every frame, and TextEngine's
    // cache is addressed by what a text says: a reading that changes misses on every paint, takes a
    // fresh entry for every value it passes through, and evicts the static layouts the rest of the
    // form draws from.
    using MetricsReadout = WithTextLayout<Label>;

    // THE LANES THE DROPDOWN OPENED AS. Twenty-four themes in lanes of eight are the three
    // columns the picker had in the title bar, and the page keeps them.
    constexpr std::size_t k_themeLaneSize{ 8 };
    constexpr float k_themeIconSize{ 24.0f };
    // The corner the swatch was drawn with, as a fraction of it rather than the 4 design pixels
    // it was: what paints the icon here is handed a rect and no scaler.
    constexpr float k_themeIconRadiusRatio{ 4.0f / k_themeIconSize };
    constexpr float k_themeLaneSpacing{ 12.0f };

    // What the metrics tooltip says draws the scene. See writeMetricsTooltip
    constexpr std::wstring_view k_drawnOnGpu{
        L"The scene is drawn through the graphics card - GPU acceleration is on in Settings." };
    constexpr std::wstring_view k_drawnOnCpu{
        L"The scene is rendered on a single CPU thread - GPU acceleration is off in Settings." };
    constexpr std::wstring_view k_drawnOnCpuOnly{
        L"The scene is rendered on a single CPU thread with no GPU acceleration." };

    // MillSceneShowcase

    MillSceneShowcase::MillSceneShowcase(ApplicationBase& application, Panel& content)
        :
        m_application{ application },
        m_paintTimer{ [this](RepeatEvent&){
            m_scenePanel->invalidate();
        }, MilliSeconds{ 0 } }
    {
        // BEFORE ANYTHING READS THE SCENE. The info bar takes its colours from the theme, so the
        // stored one has to be the scene's by now.
        restoreSceneTheme();
        // THE PICKER LIVES IN THE BACKSTAGE. Stated once, before the first menu opens; the page
        // itself is built afresh every time one does.
        connectAppPage(L"Scene", [this](OptionsPage& page) {
            buildScenePage(page);
        });
        // THE INFO BAR IS BUILT FIRST. The theme picker names a theme as it is given its index,
        // and what answers that reaches the chart - so the chart has to be standing by then.
        buildScenePanel(content);
        buildInfoBar();
        buildTitleBar(content);
        m_paintTimer.start();
    }

    void MillSceneShowcase::buildTitleBar(Panel& content)
    {
        FormTitle& titleBar = content.createTopBar<FormTitle>(m_application.name());

        StackPanel& leftBar = titleBar.createLeftBar<StackPanel>(
            Orientation::Horizontal,
            Padding{ 4.0f }
        );
        // First in the bar, so the mark sits in the corner of the window the way it does in
        // every other application.
        leftBar.add<AppButton>(
            VerticalAlign::Center,
            AppButton::OnPaintIcon{ AppIcon::paintIcon }
        );
    }

    void MillSceneShowcase::buildScenePanel(Panel& content)
    {
        m_scenePanel = &content.createBody<Panel>(
            Padding{ 8.0f },
            OnEvent{ [this](MouseMoveEvent&){
                m_scene.animationTick();
            } }
        );

        m_scenePanel->connectEvent<PaintEvent>(
            [this](PaintEvent& event){
                paintScene(event);
            });
    }

    void MillSceneShowcase::buildInfoBar()
    {
        StackPanel& infoBar = m_scenePanel->createRightBar<StackPanel>(Orientation::Vertical);

        infoBar.add<MetricsReadout>(
            // THE WIDTH IS STATED, and it is the chart's. The reading is rewritten every frame, so
            // a readout sized by its own words asks the bar for a new width every time a number
            // gains a digit - and an align pass per frame is paid by the whole form.
            MinSize{ k_fpsChartWidth, 0.0f },
            MaxSize{ k_fpsChartWidth, k_maxFloat },
            WordWrap::No,
            OnEvent{ [this](GetTextEvent& event){
                writeMetrics(event);
            } },
            OnEvent{ [this](GetTooltipEvent& event){
                writeMetricsTooltip(event);
            } }
        );

        m_fpsChart = &infoBar.add<FpsChart>(m_benchmark);
        applySceneColors();
    }

    // THE PICKER THE TITLE BAR USED TO CARRY, in the backstage beside Settings. The buttons are
    // the dropdown's items: the swatch, the name beside it, and the lanes it opened as.
    void MillSceneShowcase::buildScenePage(OptionsPage& page)
    {
        StackPanel& group = page.addGroup(L"Scene Theme");
        // THE LANES ARE STATED, NOT WRAPPED. A wrapping stack's MINIMUM is one lane - wrapping is
        // its promise to fit whatever width it is given - so nothing measuring this page could
        // learn that it wants three, and the third lane was laid out past the popup's edge. A row
        // of columns has the width of its columns for a minimum, which is the truth.
        //
        // THE ROW OWNS THE BUTTONS, not the columns: a container answers canFocusItem only where
        // it follows the user (Interactivity::ActiveContainer, see StackPanelBase::followsUser),
        // and the walk from the pointer returns the first control it says yes to - the button,
        // through the column standing between them. That is what StackPanel::nestedControlHovered
        // means by a stack holding its items in groups.
        StackPanel& lanes = group.add<StackPanel>(
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
        m_themeButtons.reserve(Themes::allThemes.size());
        StackPanel* lane = nullptr;
        std::size_t index = 0;
        for (const SceneTheme& theme : Themes::allThemes)
        {
            // A column every k_themeLaneSize themes, so the count follows the list rather than
            // being stated beside it.
            if (index % k_themeLaneSize == 0)
                lane = &lanes.add<StackPanel>(Orientation::Vertical);

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
                const SceneTheme& painted = Themes::allThemes[event.tag().value];
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

    // Both buttons are repainted rather than the one pressed: the other has just stopped being
    // the answer, and nothing else tells it so.
    void MillSceneShowcase::pickSceneTheme(const std::size_t index)
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

    // The theme under the pointer, which the user has not chosen: the scene wears it while the
    // pointer rests there and goes back to the pick when it leaves. Nothing is stored, and no
    // button changes its mark - that is what makes it a look rather than a choice.
    void MillSceneShowcase::previewSceneTheme(const std::size_t index)
    {
        m_scene.setTheme(index);
        m_benchmark.reset();
        applySceneColors();
    }

    void MillSceneShowcase::advanceCycleTime()
    {
        if (m_benchmark.buckets().empty())
            return;

        using namespace std::chrono;
        constexpr seconds k_cycleDuration{ 40 };

        TimePoint newRun = Clock::now();
        Clock::duration sincePreviousRun = newRun - m_prevRun;
        m_prevRun = newRun;
        // The step is a ratio of the 40 seconds a full cycle takes.
        float timeSincePreviousEvent = static_cast<float>(sincePreviousRun.count())
            / duration_cast<Clock::duration>(k_cycleDuration).count();

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
    void MillSceneShowcase::applySceneColors()
    {
        const SceneTheme& theme = m_scene.pickedTheme();
        m_fpsChart->setColors(theme.riverBottom, theme.riverWave);
    }

    // A NAME THE LIST NO LONGER HAS LEAVES THE SCENE ON THE ONE IT STARTED WITH, which is what a
    // theme renamed or dropped between two runs should do - the picker then stores the new pick.
    void MillSceneShowcase::restoreSceneTheme()
    {
        const std::wstring name =
            (m_application.config() / k_sceneThemeNodeName).get<std::wstring>();
        if (name.empty())
            return;

        std::size_t index = 0;
        for (const SceneTheme& theme : Themes::allThemes)
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

    // Written on a pick, not on a preview: hovering the list walks the scene through every theme
    // in it, and none of those is what the user chose.
    void MillSceneShowcase::storeSceneTheme()
    {
        (m_application.config() / k_sceneThemeNodeName).set(
            std::wstring{ m_scene.pickedTheme().name });
    }

    void MillSceneShowcase::paintScene(PaintEvent& event)
    {
        const FloatRect& sceneRect = event.controlBounds();
        m_benchmark.setDimensions(sceneRect.dimensions());

        advanceCycleTime();

        // A paint covering at least 80% of the scene is a full one, and only a full one is
        // measured: a repaint of the combobox or the slider covers a fraction of the scene and
        // would read as a spike. The 80% leaves room for the off-screen clipping a maximized
        // window brings.
        const IntRect& clipRect = event.canvas().clipBox().toInt();
        double clipArea = static_cast<double>(clipRect.width()) * clipRect.height();
        double sceneArea = static_cast<double>(sceneRect.width()) * sceneRect.height();
        bool isSignificantPaint = (clipArea / sceneArea) > 0.8;

        if (isSignificantPaint)
        {
            m_benchmark.run([this, &event](){
                m_scene.paint(event.canvas(), event.controlBounds(), m_cycleTime);
            });
            traceMetrics();
        }
        else
        {
            m_scene.paint(event.canvas(), event.controlBounds(), m_cycleTime);
        }
    }

    // A READING WHERE THERE IS NO TEXT. The Linux text layout measures and does not draw, so the
    // panel beside the scene is laid out and empty there - and a benchmark whose whole subject is
    // a number is worth nothing that way. Under CLAFI_FPS_TRACE the same readings go to stderr,
    // once a second, on every platform.
    void MillSceneShowcase::traceMetrics()
    {
#pragma warning(push)
#pragma warning(disable : 4996) // 'getenv': the CRT's alternative is Windows-only
        static const bool s_enabled = std::getenv("CLAFI_FPS_TRACE") != nullptr;
#pragma warning(pop)
        if (!s_enabled)
            return;

        using namespace std::chrono;
        const TimePoint now = Clock::now();
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

    void MillSceneShowcase::writeMetrics(GetTextEvent& event)
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
    MillSceneShowcase::WindowReadings MillSceneShowcase::windowReadings() const
    {
        const FpsBenchmark::Recent recent =
            m_benchmark.recent(FpsBenchmark::k_historySpan, Clock::now());
        WindowReadings result{};
        result.drawMs = recent.wholePaintMs;
        result.fps = recent.frames * 1000.0 / static_cast<double>(FpsBenchmark::k_historySpan.count());
        return result;
    }

    void MillSceneShowcase::writeMetricsTooltip(GetTooltipEvent& event)
    {
        event.placement = FormPlacement::Bottom;
        auto paintBullet = [](PaintIconEvent& event){
            float radius = event.scaleF(3.0f);
            event.canvas().fillCircle(event.iconRect().center(), radius, event.textRgb(InkGrade::Strongest));
        };
        // An application that named no GPU backend offers no such setting.
        std::wstring_view drawnOn = k_drawnOnCpuOnly;
        if (m_application.context().usingGpu())
            drawnOn = k_drawnOnGpu;
        else if (m_application.context().gpuAvailable())
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
}
