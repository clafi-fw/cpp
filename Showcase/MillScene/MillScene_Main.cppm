module;
#include <cstdio>
export module MillScene_App.Main;

import MillScene_App.Icon;

import ClaFi.App.Application;
import ClaFi.App.Settings;

import ClaFi.PathArt.Scenes;
import ClaFi.PathArt.MillScene;
import ClaFi.PathArt.Scene;
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

import ClaFi.Icons.DotMark;

import ClaFi.Core.Context.DrawIconEvent;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.UpdateCheck;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

// **Nothing here names a platform or a backend.** What the project file keeps is the entry point
// its operating system asks for and the two types the application is built out of; everything
// else - the form, its size, its title and what is built into it - stands here and would run over
// any platform the framework has.
namespace MillScene_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The scenes stand in two lanes, the mill at the top of the first.
    constexpr std::size_t k_sceneLaneSize{ 4 };

    // A look the picker offers, by the name it shows and the config keeps.
    struct LookChoice
    {
        PathArt::SceneLook look;
        std::wstring_view name;
    };
    constexpr std::array<LookChoice, 2> k_lookChoices{
        LookChoice{ PathArt::SceneLook::Stylized, L"Stylized" },
        LookChoice{ PathArt::SceneLook::Realistic, L"Realistic" }
    };

    // **The lanes the dropdown opened as.** Twenty-four themes in lanes of eight are the three
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

    // **What the picked scene theme is kept under**, and it is kept by name: an index moves the
    // moment the list gains a theme, and a name is what a config file read by hand should show.
    export constexpr std::wstring_view k_sceneThemeNodeName{ L"SceneTheme" };
    // What the picked scene is kept under, by name for the same reasons as the theme.
    export constexpr std::wstring_view k_sceneNodeName{ L"Scene" };
    // What the look picked for the scene that offers looks is kept under, by name as well.
    export constexpr std::wstring_view k_sceneLookNodeName{ L"SceneLook" };

    // The panel the scene covers whole; a drag and the wheel over it move the scene's view.
    class ScenePanel : public Panel
    {
    public:
        using Panel::Panel;
    public:
        // The scene drawn this frame and its bounds; another scene stops what moves the last one.
        void showScene(PathArt::Scene&, const FloatRect& bounds);
        // Carries a drag let go on and brings the wheel's notches in, over the time since the last.
        void advanceView();
    protected:
        void paintSurface(PaintEvent&) override;
        void nestedPressDown(PressDownEvent&) override;
        void nestedDrag(DragEvent&) override;
        void nestedPressUp(PressUpEvent&) override;
        void nestedMouseWheel(MouseWheelEvent&) override;
    private:
        // A move of the pointer in a drag, kept to tell how fast the drag went as it was let go.
        struct DragStep
        {
            Diagnostic::TimePoint at;
            FloatPoint by;
        };
    private:
        [[nodiscard]] bool movesView() const;
    private:
        // The moves before a drag is let go that the speed it glides on at is taken from.
        static constexpr std::chrono::milliseconds k_glideWindow{ 80 };
        // How long a glide takes to slow to about a third of its speed, in seconds.
        static constexpr float k_glideSlowing{ 0.35f };
        // The speed a glide stops at, in pixels a second.
        static constexpr float k_glideStop{ 2.0f };
        // How long the wheel's notches take to come about two thirds of the way in, in seconds.
        static constexpr float k_notchEasing{ 0.08f };
        PathArt::Scene* m_scene{ nullptr };
        FloatRect m_sceneBounds{};
        // Where the pointer stood at the last move of a drag that moves the view.
        std::optional<PointInForm> m_dragPos{};
        std::vector<DragStep> m_dragSteps{};
        FloatPoint m_glideSpeed{}; // pixels a second
        float m_notchesToCome{};
        Diagnostic::TimePoint m_lastAdvance{ Diagnostic::Clock::now() };
    };

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
        // The picker's scenes: the mill at 0, then the others in their order.
        [[nodiscard]] PathArt::Scene& sceneAt(std::size_t index);
        [[nodiscard]] std::size_t sceneCount() const;
        void previewScene(std::size_t index);
        void pickScene(std::size_t index);
        void applySceneColors() const;
        void previewSceneTheme(std::size_t index);
        // The application's own page of the backstage: the theme picker. See MillScene_Showcase
        void pickSceneTheme(std::size_t index);
        // The scene the look picker serves: the first that offers looks, none where none does.
        [[nodiscard]] std::optional<std::size_t> sceneWithLooks();
        // Draws the scene that offers looks in the look at `index` of k_lookChoices.
        void previewLook(std::size_t index);
        void pickLook(std::size_t index);

        void buildScenePage(AppMenuPage&);
        void buildScenePicker(AppMenuPage&);
        void buildThemePicker(AppMenuPage&);
        void buildLookPicker(AppMenuPage&);

        void restoreScene();
        void storeScene();
        void restoreSceneTheme();
        void storeSceneTheme();
        void restoreLook();
        void storeLook();

        void traceMetrics();
        // The readings, written ahead of every frame. See writeMetricsTooltip
        void writeMetrics();
        [[nodiscard]] WindowReadings windowReadings() const;
        void writeMetricsTooltip(AdjustTooltipEvent&);

    private:
        DialogTitle& m_titleBar{ createTopBar<DialogTitle>(
            appContext().appName())
        };

        // The scene the user chose, and the one drawn - they differ while the pointer walks the
        // picker. See previewScene
        std::size_t m_pickedScene{ 0 };
        std::size_t m_shownScene{ 0 };
        // The theme the user chose, which is not the theme the scene wears while the pointer
        // walks the picker - see previewSceneTheme.
        std::size_t m_pickedTheme{ 0 };
        // The look the user chose, at its index in k_lookChoices.
        std::size_t m_pickedLook{ 0 };

        // The backstage's Scene page, refilled every time that page is built: a pick repaints the
        // button it left as well as the one it took, and the stack is what the preview returns to.
        SingleSelectionStackView* m_sceneLanes{ nullptr };
        std::vector<Control*> m_sceneButtons{};
        // Shown while the mill is the picked scene: the themes are the mill's.
        AppMenuPage::Section* m_themeSection{ nullptr };
        SingleSelectionStackView* m_themeLanes{ nullptr };
        std::vector<Control*> m_themeButtons{};
        // Shown while the scene that offers looks is the picked scene: the looks are its own.
        AppMenuPage::Section* m_lookSection{ nullptr };
        SingleSelectionStackView* m_lookLanes{ nullptr };
        std::vector<Control*> m_lookButtons{};
        // The chart draws the benchmark above; it wears the scene's own colours, which is why it
        // is held rather than added and forgotten.
        // When the readings last went to stderr. See traceMetrics.
        Diagnostic::TimePoint m_prevTrace{ Diagnostic::Clock::now() };

        PathArt::MillScene m_millScene{};
        PathArt::SceneList m_scenes{ PathArt::createScenes() };
        Diagnostic::FpsBenchmark m_benchmark{};

        // The time the scene is drawn at.
        PathArt::SceneTime m_sceneTime{};
        // What the last frame was drawn with; the scene is prepared once two frames running agree.
        PathArt::SceneParams m_lastSceneParams{};
        Diagnostic::TimePoint m_prevRun{Diagnostic::Clock::now() };
        // The scene runs at the speed this slider names. While there is no slider it runs at 1.
        Slider* m_speedSlider{ nullptr };

        AppButton& m_appButton{ m_titleBar.createLeftBar<AppButton>(
            VerticalAlign::Center
            ,      OnEvent{ AppIcon::drawIcon }
        ) };
        ScenePanel& m_scenePanel{ createBody<ScenePanel>(
            Padding{ 8.0f },
            Events{
                [this](MouseMoveEvent&) {
                    m_millScene.animationTick();
                },
                [this](PaintEvent& event) {
                    paintScene(event);
                }
            }
        ) };
        EventRepeater m_paintTimer{ [this](RepeatEvent&) {
                writeMetrics();
                m_scenePanel.invalidate();
            }, MilliSeconds{ 0 } };

        StackPanel& m_infoBar{ m_scenePanel.createRightBar<StackPanel>(
            Orientation::Vertical
            ) };
        using MetricsReadout = WithTextLayout<Label>;
        MetricsReadout& m_metricsReadout{ m_infoBar.add<MetricsReadout>(
            // **The width is stated**, and it is the chart's. The reading is rewritten every frame,
            // so a readout sized by its own words asks the bar for a new width every time a number
            // gains a digit - and an align pass per frame is paid by the whole form.
            MinSize{Diagnostic::k_fpsChartWidth, 0.0f },
            MaxSize{Diagnostic::k_fpsChartWidth, k_maxFloat },
            WordWrap::No,
            OnEvent{ [this](AdjustTooltipEvent& event) {
                writeMetricsTooltip(event);
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
        // **Before anything reads the scene.** The info bar takes its colours from the theme, so
        // the stored one has to be the scene's by now.
        restoreSceneTheme();
        restoreScene();
        restoreLook();


        connectAppPage(L"Scene", Icons::DotMark::draw, [this](AppMenuPage& page) {
            buildScenePage(page);
            });
        applySceneColors();

        // The readout has its lines before the first layout, so the first frame moves nothing.
        writeMetrics();
        m_paintTimer.start();
    }

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] AppParams millSceneAppParams();

    // **What this application keeps of its own**, handed to the application beside the framework's
    // schema and merged with it: the picked scene, the mill's picked theme and the look picked for
    // the scene that offers looks. Empty is no pick yet, and the showcase then opens on the mill in
    // the theme it starts with, and that scene in the look it starts with.
    export [[nodiscard]] Dom::Dt::Section createMillSceneConfigSchema();

    /// @brief Builds the showcase into a form of the given application, runs it, and answers what
    /// the form answered.
    export int runMillSceneShowcase(ApplicationBase&);


//-----------------------------------------------------------------------------


void ScenePanel::showScene(PathArt::Scene& scene, const FloatRect& bounds)
{
    if (m_scene != &scene)
    {
        if (m_scene && m_dragPos)
            m_scene->holdView(false);
        m_dragPos.reset();
        m_dragSteps.clear();
        m_glideSpeed = {};
        m_notchesToCome = 0.0f;
    }
    m_scene = &scene;
    m_sceneBounds = bounds;
}

// A frame that comes late, after the window was hidden, moves the view as one of a tenth of a
// second would, so the glide does not leap.
void ScenePanel::advanceView()
{
    const Diagnostic::TimePoint now = Diagnostic::Clock::now();
    const float seconds = std::min(0.1f,
        std::chrono::duration<float>(now - m_lastAdvance).count());
    m_lastAdvance = now;
    if (!movesView())
        return;
    if (!m_dragPos && (m_glideSpeed.x != 0.0f || m_glideSpeed.y != 0.0f))
    {
        m_scene->dragView(m_glideSpeed * seconds, m_sceneBounds);
        m_glideSpeed = m_glideSpeed * std::exp(-seconds / k_glideSlowing);
        if (std::hypot(m_glideSpeed.x, m_glideSpeed.y) < k_glideStop)
            m_glideSpeed = {};
    }
    if (m_notchesToCome != 0.0f)
    {
        float notches = m_notchesToCome * (1.0f - std::exp(-seconds / k_notchEasing));
        if (std::abs(m_notchesToCome - notches) < 0.002f)
            notches = m_notchesToCome;
        m_scene->scrollView(notches);
        m_notchesToCome -= notches;
    }
}

void ScenePanel::paintSurface(PaintEvent& event)
{
    emitEvent(event);
}

// A press stops a glide, as a hand stops a spinning globe, and holds the view until let go.
void ScenePanel::nestedPressDown(PressDownEvent& event)
{
    Panel::nestedPressDown(event);
    if (!movesView())
        return;
    m_dragPos = event.clickPos();
    m_dragSteps.clear();
    m_glideSpeed = {};
    m_scene->holdView(true);
}

void ScenePanel::nestedDrag(DragEvent& event)
{
    if (!m_dragPos || !movesView())
    {
        Panel::nestedDrag(event);
        return;
    }
    const PointInForm current = event.currentPos();
    const FloatPoint by = { current.x - m_dragPos->x, current.y - m_dragPos->y };
    m_dragPos = current;
    if (by.x != 0.0f || by.y != 0.0f)
    {
        m_scene->dragView(by, m_sceneBounds);
        m_dragSteps.push_back({ .at = Diagnostic::Clock::now(), .by = by });
    }
    event.lockHoveredControl();
    event.stopPropagation();
}

// **The glide goes on at the speed of the drag's last moves**: those within k_glideWindow of the
// let-go. A drag held still before it is let go leaves none, and stops where it is.
void ScenePanel::nestedPressUp(PressUpEvent& event)
{
    Panel::nestedPressUp(event);
    if (!m_dragPos)
        return;
    m_dragPos.reset();
    m_scene->holdView(false);
    const Diagnostic::TimePoint now = Diagnostic::Clock::now();
    FloatPoint moved = {};
    std::optional<Diagnostic::TimePoint> firstAt{};
    for (const DragStep& step : m_dragSteps)
    {
        if (now - step.at > k_glideWindow)
            continue;
        moved = moved + step.by;
        if (!firstAt)
            firstAt = step.at;
    }
    m_dragSteps.clear();
    if (!firstAt)
        return;
    const float seconds = std::max(std::chrono::duration<float>(now - *firstAt).count(),
        1.0f / 120.0f);
    m_glideSpeed = moved * (1.0f / seconds);
}

void ScenePanel::nestedMouseWheel(MouseWheelEvent& event)
{
    if (!movesView())
    {
        Panel::nestedMouseWheel(event);
        return;
    }
    m_notchesToCome += event.delta;
    event.handled = true;
}

bool ScenePanel::movesView() const
{
    return m_scene && m_scene->offersView();
}

void MillSceneForm::paintScene(PaintEvent& event)
{
    const FloatRect& sceneRect = event.controlBounds();
    m_benchmark.setDimensions(sceneRect.dimensions());

    advanceCycleTime();
    m_scenePanel.showScene(sceneAt(m_shownScene), sceneRect);
    m_scenePanel.advanceView();

    // **A scene keeps its layers for the size it settles on.** While a resize changes the size
    // from one frame to the next the scene draws everything live, and the first frame drawn at an
    // unchanged size prepares it.
    const PathArt::SceneParams sceneParams = {
        .bounds = sceneRect,
        .transform = event.canvas().transform(),
        .backend = event.canvas().backend()->type()
    };
    if (sceneParams == m_lastSceneParams)
        sceneAt(m_shownScene).prepare(sceneParams);
    m_lastSceneParams = sceneParams;

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
            sceneAt(m_shownScene).draw(event.canvas(), event.controlBounds(), m_sceneTime);
            });
        traceMetrics();
    }
    else
    {
        sceneAt(m_shownScene).draw(event.canvas(), event.controlBounds(), m_sceneTime);
    }

}

void MillSceneForm::advanceCycleTime()
{
    if (m_benchmark.buckets().empty())
        return;

    using namespace std::chrono;
    const seconds cycleDuration = sceneAt(m_shownScene).cycleDuration();

    const Diagnostic::TimePoint newRun = Diagnostic::Clock::now();
    const Diagnostic::Clock::duration sincePreviousRun = newRun - m_prevRun;
    m_prevRun = newRun;
    // The step is a ratio of the time a full cycle of the scene takes.
    const float timeSincePreviousEvent = static_cast<float>(sincePreviousRun.count())
        / duration_cast<Diagnostic::Clock::duration>(cycleDuration).count();

    float speed = 1.0f;
    if (m_speedSlider)
    {
        speed = m_speedSlider->relativePosition() - 0.5f;
        bool negative = speed < 0.0f;
        speed *= 8;
        speed *= speed;
        speed = (negative ? -speed : speed);
    }
    m_sceneTime.cycleTime += timeSincePreviousEvent * speed;
    const float wholeCycles = std::floor(m_sceneTime.cycleTime);
    m_sceneTime.cyclesPassed += static_cast<std::int64_t>(wholeCycles);
    m_sceneTime.cycleTime -= wholeCycles;

}

PathArt::Scene& MillSceneForm::sceneAt(const std::size_t index)
{
    if (index == 0)
        return m_millScene;
    return *m_scenes[index - 1];
}

std::size_t MillSceneForm::sceneCount() const
{
    return m_scenes.size() + 1;
}

// A different scene is a different load, so the readings start again.
void MillSceneForm::previewScene(const std::size_t index)
{
    if (m_shownScene == index)
        return;
    m_shownScene = index;
    m_benchmark.reset();
}

void MillSceneForm::pickScene(const std::size_t index)
{
    const std::size_t previous = m_pickedScene;
    m_pickedScene = index;
    previewScene(index);
    storeScene();

    if (m_sceneLanes)
        m_sceneLanes->setCurrentItem(*m_sceneButtons[index]);
    if (!m_sceneButtons.empty())
    {
        m_sceneButtons[previous]->invalidateState();
        m_sceneButtons[index]->invalidateState();
    }
    if (m_themeSection)
        m_themeSection->setVisible(index == 0);
    if (m_lookSection)
        m_lookSection->setVisible(sceneWithLooks() == index);
}

// **The picked theme's colours, not the blend's.** This runs the moment a theme is picked, when
// the blend still wears the one before it - and at startup, when it wears nothing at all.
void MillSceneForm::applySceneColors() const
{
    const PathArt::SceneTheme& theme = m_millScene.pickedTheme();
    m_fpsChart.setColors(theme.riverBottom, theme.riverWave);
}

void MillSceneForm::previewSceneTheme(std::size_t index)
{
    m_millScene.setTheme(index);
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

std::optional<std::size_t> MillSceneForm::sceneWithLooks()
{
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        if (sceneAt(index).offersLooks())
            return index;
    }
    return std::nullopt;
}

// A different look is a different load, so the readings start again.
void MillSceneForm::previewLook(const std::size_t index)
{
    const std::optional<std::size_t> scene = sceneWithLooks();
    if (!scene)
        return;
    sceneAt(*scene).setLook(k_lookChoices[index].look);
    m_benchmark.reset();
}

void MillSceneForm::pickLook(const std::size_t index)
{
    const std::size_t previous = m_pickedLook;
    m_pickedLook = index;
    previewLook(index);
    storeLook();

    if (m_lookLanes)
        m_lookLanes->setCurrentItem(*m_lookButtons[index]);
    m_lookButtons[previous]->invalidateState();
    m_lookButtons[index]->invalidateState();
}

void MillSceneForm::buildScenePage(AppMenuPage& page)
{
    buildScenePicker(page);
    buildThemePicker(page);
    buildLookPicker(page);
}

// Built the way the theme picker is: the pointer previews, a pick is the current item.
void MillSceneForm::buildScenePicker(AppMenuPage& page)
{
    StackPanel& group = page.addGroup(L"Scene");
    SingleSelectionStackView& lanes = group.add<SingleSelectionStackView>(
        Orientation::VerticalWrap,
        LaneSize{ k_sceneLaneSize },
        Spacing{ k_themeLaneSpacing, 0.0f },
        VerticalAlign::Top
    );

    m_sceneLanes = &lanes;
    lanes.setPreviewMode(PreviewMode::Hover);
    lanes.onPreview([this](PreviewEvent& event) {
        previewScene(event.item.tag().value);
        });
    lanes.onDestroy([this](DestroyEvent&) {
        previewScene(m_pickedScene);
        m_sceneLanes = nullptr;
        m_sceneButtons.clear();
        });

    m_sceneButtons.clear();
    m_sceneButtons.reserve(sceneCount());
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        ToolButton& item = lanes.add<ToolButton>(
            Text{ sceneAt(index).name() },
            Tag{ index },
            MouseFocus::Self, // an item: the press that focuses it makes it current
            ButtonViewMode::TextLabel,
            ShowSelectionOnSurface::Yes
        );
        item.onGetState([this, index](GetStateEvent& event) {
            event.state.selected = m_pickedScene == index;
            });
        item.onClick([this, index](ClickEvent&) {
            pickScene(index);
            });
        m_sceneButtons.push_back(&item);
    }
    lanes.setCurrentItem(*m_sceneButtons[m_pickedScene]);
}

void MillSceneForm::buildThemePicker(AppMenuPage& page)
{
    AppMenuPage::Section& section = page.addSection(L"Scene Theme");
    m_themeSection = &section;
    section.setVisible(m_pickedScene == 0);
    StackPanel& group = section.body();
    SingleSelectionStackView& lanes = group.add<SingleSelectionStackView>(
        Orientation::VerticalWrap,
        LaneSize{ k_themeLaneSize },
        Spacing{ k_themeLaneSpacing, 0.0f },
        VerticalAlign::Top
    );

    m_themeLanes = &lanes;
    // **What the pointer is on, which is not what the user has chosen.** A stack previewing what
    // it is hovered over goes back to its **current item** when the pointer leaves it, so the
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
        m_themeSection = nullptr;
        m_themeLanes = nullptr;
        m_themeButtons.clear();
        });

    m_themeButtons.clear();
    m_themeButtons.reserve(PathArt::Themes::allThemes.size());
    std::size_t index = 0;
    for (const PathArt::SceneTheme& theme : PathArt::Themes::allThemes)
    {
        ToolButton& item = lanes.add<ToolButton>(
            Text{ theme.name },
            Tag{ index },
            MouseFocus::Self, // an item: the press that focuses it makes it current
            ButtonViewMode::LeftIcon,
            IconSize{ k_themeIconSize, k_themeIconSize },
            // The pick wears the surface, the way the app button wears the open backstage.
            ShowSelectionOnSurface::Yes
        );
        // The theme is read off the button's own tag, so one painter answers for all of them.
        item.onDrawIcon([](DrawIconEvent& event) {
            const PathArt::SceneTheme& painted = PathArt::Themes::allThemes[event.tag().value];
            const float radius = event.iconWidth() * k_themeIconRadiusRatio;
            painted.drawIcon(event.canvas(), event.iconRect(), radius, 1.0f);
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

// **The section is captioned with its scene's name**, being shown only while that scene is
// picked, in the room the mill's themes leave. Built the way the theme picker is.
void MillSceneForm::buildLookPicker(AppMenuPage& page)
{
    const std::optional<std::size_t> scene = sceneWithLooks();
    if (!scene)
        return;
    AppMenuPage::Section& section =
        page.addSection(std::format(L"{} Look", sceneAt(*scene).name()));
    m_lookSection = &section;
    section.setVisible(m_pickedScene == *scene);
    SingleSelectionStackView& lanes = section.body().add<SingleSelectionStackView>(
        Orientation::VerticalWrap,
        LaneSize{ k_sceneLaneSize },
        Spacing{ k_themeLaneSpacing, 0.0f },
        VerticalAlign::Top
    );

    m_lookLanes = &lanes;
    lanes.setPreviewMode(PreviewMode::Hover);
    lanes.onPreview([this](PreviewEvent& event) {
        previewLook(event.item.tag().value);
    });
    lanes.onDestroy([this](DestroyEvent&) {
        previewLook(m_pickedLook);
        m_lookSection = nullptr;
        m_lookLanes = nullptr;
        m_lookButtons.clear();
    });

    m_lookButtons.clear();
    m_lookButtons.reserve(k_lookChoices.size());
    for (std::size_t index = 0; index != k_lookChoices.size(); ++index)
    {
        ToolButton& item = lanes.add<ToolButton>(
            Text{ k_lookChoices[index].name },
            Tag{ index },
            MouseFocus::Self, // an item: the press that focuses it makes it current
            ButtonViewMode::TextLabel,
            ShowSelectionOnSurface::Yes
        );
        item.onGetState([this, index](GetStateEvent& event) {
            event.state.selected = m_pickedLook == index;
        });
        item.onClick([this, index](ClickEvent&) {
            pickLook(index);
        });
        m_lookButtons.push_back(&item);
    }
    lanes.setCurrentItem(*m_lookButtons[m_pickedLook]);
}

void MillSceneForm::restoreScene()
{
    const std::wstring name =
        (appContext().config() / k_sceneNodeName).get<std::wstring>();
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        if (sceneAt(index).name() == name)
        {
            m_pickedScene = index;
            m_shownScene = index;
            return;
        }
    }
}

void MillSceneForm::storeScene()
{
    (appContext().config() / k_sceneNodeName).set(
        std::wstring{ sceneAt(m_pickedScene).name() });
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
            m_millScene.setTheme(index);
            m_pickedTheme = index;
            return;
        }
        ++index;
    }
}

void MillSceneForm::storeSceneTheme()
{
    (appContext().config() / k_sceneThemeNodeName).set(
        std::wstring{ m_millScene.pickedTheme().name });
}

void MillSceneForm::restoreLook()
{
    const std::wstring name =
        (appContext().config() / k_sceneLookNodeName).get<std::wstring>();
    for (std::size_t index = 0; index != k_lookChoices.size(); ++index)
    {
        if (k_lookChoices[index].name == name)
        {
            m_pickedLook = index;
            previewLook(index);
            return;
        }
    }
}

void MillSceneForm::storeLook()
{
    (appContext().config() / k_sceneLookNodeName).set(
        std::wstring{ k_lookChoices[m_pickedLook].name });
}

void MillSceneForm::traceMetrics()
{
#pragma warning(push)
#pragma warning(disable : 4996) // 'getenv': the **crt's** alternative is Windows-only
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

void MillSceneForm::writeMetrics()
{
    double cpuVal = m_benchmark.cpuUsage().lastValue();
    if (cpuVal > 10000.0)
        cpuVal = 0.0;

    const WindowReadings readings = windowReadings();

    // TODO: the text disappears under some conditions - which ones?
    Text lines;
    lines << std::format(L"{:.0f}", m_benchmark.dimensions().x);
    lines << ColoredText{ InkGrade::Muted, L"x" };
    lines << std::format(L"{:.0f} ", m_benchmark.dimensions().y);
    lines << ColoredText{ InkGrade::Muted, L"pix\u202F", Space{ 0.0f } } << k_endLine;
    lines << std::format(L"{:.2f} ", cpuVal);
    lines << ColoredText{ InkGrade::Muted, L"CPU\u202F", Space{ 0.0f } } << k_endLine;
    lines << std::format(L"{:.2f} ", readings.drawMs);
    lines << ColoredText{ InkGrade::Muted, L"ms\u202F\u202F", Space{ 0.0f } } << k_endLine;
    lines << std::format(L"{:.0f} ", readings.drawMs > 0.0 ? 1000.0 / readings.drawMs : 0.0);
    lines << ColoredText{ InkGrade::Muted, L"draw" } << k_endLine;
    lines << std::format(L"{:.0f} ", readings.fps);
    lines << ColoredText{ InkGrade::Muted, L"FPS\u202F", Space{ 0.0f } } << k_endLine;
    m_metricsReadout.text() = Text{ TextAlign::Right, StyledText{ TextStyles::Code, lines } };
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

void MillSceneForm::writeMetricsTooltip(AdjustTooltipEvent& event)
{
    event.placement = FormPlacement::BottomLeft;
    auto drawBullet = [](DrawIconEvent& event) {
        float radius = event.scaleF(3.0f);
        event.canvas().fillCircle(event.iconRect().center(), radius, event.textRgb(InkGrade::Strongest));
        };
    // An application that named no GPU backend offers no such setting.
    std::wstring_view drawnOn = k_drawnOnCpuOnly;
    if (appContext().usingGpu())
        drawnOn = k_drawnOnGpu;
    else if (appContext().gpuAvailable())
        drawnOn = k_drawnOnCpu;
    const InTextIcon bullet{ 18.0f, 18.0f, drawBullet };
    event.text.clear();
    event.text << StyledText{ TextStyles::Section, L"Performance Metrics\n" };
    event.text << Space{ 16.0f } << bullet << L"Resolution (pix): ";
    event.text << ColoredText{ InkGrade::Muted, L"Dimensions of the current scene.\n" };
    event.text << Space{ 16.0f } << bullet << L"CPU usage (%): ";
    event.text << ColoredText{ InkGrade::Muted, L"Single-core utilization by this specific process.\n" };
    event.text << Space{ 16.0f } << bullet << L"Time (ms): ";
    event.text << ColoredText{ InkGrade::Muted, L"Measured time to render the scene to a bitmap.\n" };
    event.text << Space{ 16.0f } << bullet << L"draw: ";
    event.text << ColoredText{ InkGrade::Muted, L"What that time alone would allow, as 1000/Time.\n" };
    event.text << Space{ 16.0f } << bullet << L"FPS: ";
    event.text << ColoredText{
        InkGrade::Muted,
        L"The rate frames actually arrived at, measured from one to the next"
    };
    event.text << L"." << VSpace{ 18.0f } << k_endLine;
    event.text << StyledText{ TextStyles::Section, L"Note:\n" };
    event.text << Space{ 18.0f } << drawnOn << k_endLine;
    event.text << Space{ 18.0f } << L"The gap between " << SizedText{ 20.0f, L"draw" };
    event.text << L" and " << SizedText{ 20.0f, L"FPS" } << L" is everything a frame waits on\n";
    event.text << Space{ 18.0f } << L"that the scene is not: the rest of the form, the copy to the screen,\n";
    event.text << Space{ 18.0f } << L"and whatever pace the display server keeps.\n";
    event.text << Space{ 18.0f } << L"The chart shows both - the filled area is FPS, the line over it draw.\n";
    event.text << Space{ 18.0f } << L"Time, draw and FPS are read over the two seconds the chart spans.";
}


//-----------------------------------------------------------------------------


    AppParams millSceneAppParams()
    {
        return {
            .name = Text{ L"MillScene" },
            .publisher = L"ClaFi Framework",
            .description = Text{
                L"Shows the ClaFi path painter at work: "
                L"a watermill landscape in a choice of themes and seven more scenes, "
                L"animated on a single CPU thread or through the graphics card, "
                L"with their frame times charted live."
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
        return Dom::Dt::Section{
            Dom::Dt::Value{ k_sceneNodeName, L"" },
            Dom::Dt::Value{ k_sceneThemeNodeName, L"" },
            Dom::Dt::Value{ k_sceneLookNodeName, L"" }
        };
    }

    constexpr float k_minWindowWidth{ 640.0f };
    constexpr float k_minWindowHeight{ 480.0f };

    int runMillSceneShowcase(ApplicationBase& application)
    {
        const auto form = application.createDialog<MillSceneForm>(
            UiElement::Dialog,
            MinSize{ k_minWindowWidth / 2.0f, k_minWindowHeight / 2.0f },
            PreferredSize{ k_minWindowWidth, k_minWindowHeight }
        );
        form->setConfigName(AppContext::k_mainFormName);
        return form->execute();
    }
}
