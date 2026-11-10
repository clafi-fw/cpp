module;
#include <cstdio>
export module MillScene_App.Main;

import MillScene_App.Icon;

import ClaFi.App.Application;
import ClaFi.App.Settings;

import ClaFi.PathArt.Scenes;
import ClaFi.PathArt.MillScene;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.Themes;
import ClaFi.PathArt.Types;

import ClaFi.Diagnostic.Benchmark;
import ClaFi.Diagnostic.FpsChart;

import ClaFi.Controls.Panel;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.Stack;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.AppButton;
import ClaFi.Controls.Label;
import ClaFi.Controls.Slider;
import ClaFi.Controls.Button;
import ClaFi.Controls.RadioButton;
import ClaFi.Controls.CheckBox;

import ClaFi.Icons.DotMark;

import ClaFi.Core.Context.DrawIconEvent;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.UpdateCheck;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.DomEngine;

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

    // The scenes stand as tiles in rows of four, the mill first.
    constexpr std::size_t k_sceneTilesPerRow{ 4 };
    // A tile's picture slot is wider than the figure it centres, room for a name on one line.
    constexpr IconSize k_sceneIconSize{ 96.0f, 64.0f };
    constexpr float k_sceneTilePadding{ 6.0f };
    constexpr float k_sceneTileSpacing{ 8.0f };
    // The units and the chart's readings are written at this share of the readout's ink.
    constexpr float k_mutedInkOpacity{ 0.67f };
    // A scene's own options stand in lanes of four.
    constexpr std::size_t k_optionLaneSize{ 4 };
    constexpr float k_pickerCaptionSpacing{ 4.0f }; // between a picker's caption and its options
    constexpr float k_pickerSpacing{ 12.0f }; // between the pickers on one scene's page of options
    // A scene taken up fades in over the one before it, as long as a theme's crossfade.
    constexpr float k_sceneFadeSeconds{ 0.555f };
    constexpr int k_outgoingSceneImage{ 0 }; // the kept image the scene being left fades out of

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
    // The look, at its index in k_lookChoices, a scene is drawn in until the user picks one.
    constexpr std::size_t k_defaultLook{ 1 };
    static_assert(k_lookChoices[k_defaultLook].look == PathArt::SceneLook::Realistic);

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
    // What the scenes keep their views and choices under, a section for each by its name.
    export constexpr std::wstring_view k_sceneViewsNodeName{ L"SceneViews" };

    // The panel the scene covers whole; it hands the wheel and a drag on to the scene's view.
    class ScenePanel : public Panel
    {
    public:
        using Panel::Panel;
    public:
        // The scene drawn this frame and its bounds; another scene stops what moves the last one.
        void showScene(PathArt::Scene&, const FloatRect& bounds);
        // Carries a drag let go on and brings the wheel's notches in, over the time since the last.
        void advanceView();
        [[nodiscard]] CursorShape cursor() const override;
    protected:
        void paintSurface(PaintEvent&) override;
        void mouseMove(MouseMoveEvent&) override;
        void nestedPressDown(PressDownEvent&) override;
        void nestedDrag(DragEvent&) override;
        void nestedPressUp(PressUpEvent&) override;
        void doubleClick(DoubleClickEvent&) override;
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
        [[nodiscard]] bool dragMovesViewFrom(PointInForm) const;
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
        bool m_pointerShowsHand{}; // where a drag from the pointer would move the view
        // Where the pointer stood at the last move of a drag that moves the view.
        std::optional<PointInForm> m_dragPos{};
        bool m_pressBecameDrag{}; // it left the slack a click is allowed
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
        // A choice a scene offers, as its picker on the scene's page of options shows it.
        struct ChoicePicker
        {
            std::size_t sceneIndex{};
            std::size_t choiceIndex{}; // among the scene's choices
            std::size_t pickedOption{}; // at its index among the choice's options
            SingleSelectionStackView* lanes{ nullptr };
            std::vector<Control*> buttons{};
        };
        // A theme picker on the page of a scene that offers themes; all of them show one pick.
        struct ThemePicker
        {
            std::size_t sceneIndex{};
            SingleSelectionStackView* lanes{ nullptr };
            std::vector<Control*> buttons{};
        };
    private:
        void paintScene(PaintEvent&);
        void keepShownScene(PaintEvent&);
        void advanceSceneFade();
        void advanceCycleTime();
        // The picker's scenes: the mill at 0, then the others in their order.
        [[nodiscard]] PathArt::Scene& sceneAt(std::size_t index);
        [[nodiscard]] std::size_t sceneCount() const;
        void previewScene(std::size_t index);
        void pickScene(std::size_t index);
        // The readout's colours and the chart's, as the shown scene states them.
        void applySceneColors();
        [[nodiscard]] Color mutedReadoutInk() const;
        // Every scene that offers themes crossfades to the theme at `index` of allThemes.
        void dressScenes(std::size_t index);
        void previewSceneTheme(std::size_t index);
        // One pick for every scene that offers themes, which the picker on each one's page shows.
        void pickSceneTheme(std::size_t index);
        // The scene the look picker serves: the first that offers looks, none where none does.
        [[nodiscard]] std::optional<std::size_t> sceneWithLooks();
        // Draws the scene that offers looks in the look at `index` of k_lookChoices.
        void previewLook(std::size_t index);
        void pickLook(std::size_t index);
        // Takes an option in the choice a picker of m_choicePickers shows, while it is hovered.
        void previewOption(std::size_t pickerIndex, std::size_t option);
        void pickOption(std::size_t pickerIndex, std::size_t option);
        // Whether a scene's page holds an option: themes, a look, a choice, a switch.
        [[nodiscard]] bool sceneOffersOptions(std::size_t index);

        void buildScenePage(AppMenuPage&);
        void buildScenePicker(AppMenuPage&);
        // A page of options for every scene in one section, the picked scene's page showing.
        void buildSceneOptions(AppMenuPage&);
        // What that section is captioned with while the scene at `index` is the picked one.
        [[nodiscard]] std::wstring sceneOptionsCaption(std::size_t index);
        // A theme picker on the page of each scene that offers themes.
        void buildThemePickers();
        void buildThemePicker(std::size_t pickerIndex);
        void buildLookPicker();
        void buildChoicePickers();
        void buildChoicePicker(std::size_t pickerIndex);
        // The switches the scene at `index` offers, a check box each, under its pickers.
        void buildSwitchBoxes(std::size_t index);
        // The gestures the scene at `index` answers, captioned Controls, under its options.
        void buildGestureHints(std::size_t index);
        // Adds a column to a scene's page of options: the caption, with room under it.
        [[nodiscard]] static StackPanel& addCaptionedColumn(StackPanel& optionsPage,
            std::wstring_view caption);

        void restoreScene();
        void storeScene();
        void restoreSceneTheme();
        void storeSceneTheme();
        void restoreLook();
        void storeLook();
        // Each scene that keeps its view takes it back from its section of SceneViews.
        void restoreSceneViews();
        void storeSceneViews();

        void traceMetrics();
        // The readings, written ahead of every frame. See writeMetricsTooltip
        void writeMetrics();
        [[nodiscard]] WindowReadings windowReadings() const;
        void writeMetricsTooltip(AdjustTooltipEvent&);

    private:
        DialogTitle& m_titleBar{ createTopBar<DialogTitle>(
            appContext().appName())
        };

        std::size_t m_pickedScene{ 0 }; // the scene the user chose
        // The scene to show: the picked one, or the one the pointer rests on in the picker.
        std::size_t m_sceneToShow{ 0 };
        // The scene the frames draw; it takes up m_sceneToShow on the next full paint.
        std::size_t m_shownScene{ 0 };
        PathArt::SceneReadoutColors m_readoutColors{}; // the shown scene's
        // The theme the user chose, which is not the theme the scene wears while the pointer
        // walks the picker - see previewSceneTheme.
        std::size_t m_pickedTheme{ 0 };
        // The look the user chose, at its index in k_lookChoices.
        std::size_t m_pickedLook{ k_defaultLook };

        // The backstage's Scene page, refilled every time that page is built: a pick repaints the
        // button it left as well as the one it took, and the stack is what the preview returns to.
        SingleSelectionStackView* m_sceneLanes{ nullptr };
        std::vector<Control*> m_sceneButtons{};
        AppMenuPage::Section* m_sceneOptionsSection{ nullptr }; // named after the picked scene
        // Holds a page of options for each scene and shows the picked scene's.
        PageControl* m_sceneOptionsControl{ nullptr };
        std::vector<StackPanel*> m_sceneOptionPages{}; // at each scene's index
        std::vector<ThemePicker> m_themePickers{};
        SingleSelectionStackView* m_lookLanes{ nullptr };
        std::vector<Control*> m_lookButtons{};
        std::vector<ChoicePicker> m_choicePickers{};
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
        // How much of the frame is the shown scene; the kept picture of the one before is the rest.
        float m_shownSceneShare{ 1.0f };
        // When the shown scene began to fade in; empty once it covers the frame.
        std::optional<Diagnostic::TimePoint> m_sceneFadeStart{};
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
                    sceneAt(m_shownScene).animationTick();
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
        restoreSceneViews();
        // Written as the window closes, while every scene still stands.
        form().onClose([this](FormCloseEvent&) {
            storeSceneViews();
        });


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
    // schema and merged with it: the picked scene, the theme picked for the scenes that offer
    // themes and the look picked for the scene that offers looks. Empty is no pick yet, and the
    // showcase then opens on the mill in the theme it starts with, and that scene in the look it
    // starts with.
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
            m_scene->letGoView({ 0.0f, 0.0f }, m_sceneBounds);
        m_pointerShowsHand = false;
        m_dragPos.reset();
        m_pressBecameDrag = false;
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

CursorShape ScenePanel::cursor() const
{
    return m_pointerShowsHand ? CursorShape::Hand : Panel::cursor();
}

void ScenePanel::paintSurface(PaintEvent& event)
{
    emitEvent(event);
}

void ScenePanel::mouseMove(MouseMoveEvent& event)
{
    Panel::mouseMove(event);
    m_pointerShowsHand = dragMovesViewFrom(event.posOnForm);
}

// **A press where a drag moves the view takes hold of it at once**: it stops a glide, as a hand
// stops a spinning globe, and holds the view until let go. A press anywhere else leaves the scene
// moving as it was, and one on a control over the scene is that control's.
void ScenePanel::nestedPressDown(PressDownEvent& event)
{
    Panel::nestedPressDown(event);
    const PointInForm pressPos = event.clickPos();
    if (event.control != this || !dragMovesViewFrom(pressPos))
        return;
    m_dragPos = pressPos;
    m_pressBecameDrag = false;
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
    if (event.pastThreshold())
        m_pressBecameDrag = true;
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

// **A press let go inside the click's slack is a click**, and the scene is told so, however the
// pointer trembled. A drag glides on at the speed of its last moves, those within k_glideWindow of
// the let-go; one held still before it is let go leaves none, and the scene is let go at 0.
void ScenePanel::nestedPressUp(PressUpEvent& event)
{
    Panel::nestedPressUp(event);
    if (!m_dragPos)
        return;
    m_dragPos.reset();
    if (!m_pressBecameDrag)
    {
        m_dragSteps.clear();
        m_scene->clickView(m_sceneBounds);
        return;
    }
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
    if (firstAt)
    {
        const float seconds = std::max(std::chrono::duration<float>(now - *firstAt).count(),
            1.0f / 120.0f);
        m_glideSpeed = moved * (1.0f / seconds);
    }
    if (std::hypot(m_glideSpeed.x, m_glideSpeed.y) < k_glideStop)
        m_glideSpeed = {};
    m_scene->letGoView(m_glideSpeed, m_sceneBounds);
}

// **A double click where a drag moves the view is the scene's too**, after its first click. The
// press it ends goes on as a press, so it holds the view and is let go as any press is.
void ScenePanel::doubleClick(DoubleClickEvent& event)
{
    Panel::doubleClick(event);
    if (dragMovesViewFrom(event.clickPos()))
        m_scene->doubleClickView(m_sceneBounds);
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

bool ScenePanel::dragMovesViewFrom(const PointInForm point) const
{
    return movesView() && m_scene->dragMovesViewFrom(point, m_sceneBounds);
}

void MillSceneForm::paintScene(PaintEvent& event)
{
    Graphics::Canvas& canvas = event.canvas();
    const FloatRect& sceneRect = event.controlBounds();
    m_benchmark.setDimensions(sceneRect.dimensions());

    // A paint covering at least 80% of the scene is a full one, and only a full one is
    // measured: a repaint of the combo box or the slider covers a fraction of the scene and
    // would read as a spike. The 80% leaves room for the off-screen clipping a maximized
    // window brings.
    const IntRect& clipRect = canvas.clipBox().toInt();
    const double clipArea = static_cast<double>(clipRect.width()) * clipRect.height();
    const double sceneArea = static_cast<double>(sceneRect.width()) * sceneRect.height();
    const bool isSignificantPaint = (clipArea / sceneArea) > 0.8;

    // **A scene is taken up on a full paint only.** The picture it fades in over is drawn in that
    // paint, and a picture of a part of the panel would leave the rest to change at once.
    const bool switchesScene = isSignificantPaint && m_shownScene != m_sceneToShow;
    if (switchesScene)
    {
        keepShownScene(event);
        m_shownScene = m_sceneToShow;
        applySceneColors();
    }
    advanceSceneFade();

    advanceCycleTime();
    m_scenePanel.showScene(sceneAt(m_shownScene), sceneRect);
    m_scenePanel.advanceView();

    // **A scene keeps its layers for the size it settles on.** While a resize changes the size
    // from one frame to the next the scene draws everything live, and the first frame drawn at an
    // unchanged size prepares it.
    const PathArt::SceneParams sceneParams = {
        .bounds = sceneRect,
        .transform = canvas.transform(),
        .backend = canvas.backend()->type()
    };
    if (sceneParams == m_lastSceneParams)
        sceneAt(m_shownScene).prepare(sceneParams);
    m_lastSceneParams = sceneParams;

    // **The fade starts once this frame is drawn.** The frame is the kept picture whole, so the
    // scene taken up is not drawn under it, and the time the frame takes - the picture and the
    // new scene's layers - is not taken out of the fade.
    if (switchesScene)
    {
        canvas.drawImage(k_outgoingSceneImage, 1.0f);
        m_shownSceneShare = 0.0f;
        m_sceneFadeStart = Diagnostic::Clock::now();
        return;
    }

    if (isSignificantPaint)
    {
        m_benchmark.run([this, &event]() {
            sceneAt(m_shownScene).draw(event.canvas(), event.controlBounds(), m_sceneTime);
            });
        traceMetrics();
    }
    else
    {
        sceneAt(m_shownScene).draw(canvas, sceneRect, m_sceneTime);
    }
    if (m_shownSceneShare < 1.0f)
        canvas.drawImage(k_outgoingSceneImage, 1.0f - m_shownSceneShare);
}

// **The picture kept is the frame the panel shows**, drawn again at the time that frame was drawn
// at. The form's surface goes under it: the mill leaves parts of its frame open and draws on that
// surface. Mid-fade the frame is a blend, so the shown scene is laid over the picture kept before
// at the share it has reached - or whole, where a resize has emptied the kept images.
void MillSceneForm::keepShownScene(PaintEvent& event)
{
    Graphics::Canvas& canvas = event.canvas();
    const float shownSceneShare =
        canvas.hasImage(k_outgoingSceneImage) ? m_shownSceneShare : 1.0f;
    const Graphics::ScopedCanvasImage picture{ canvas, k_outgoingSceneImage };
    const Graphics::ScopedCanvasOpacity layer{ canvas, shownSceneShare };
    canvas.fillRectangle(event.controlBounds(), event.parentEvent()->surfaceRgb());
    sceneAt(m_shownScene).draw(canvas, event.controlBounds(), m_sceneTime);
}

void MillSceneForm::advanceSceneFade()
{
    if (!m_sceneFadeStart)
        return;
    const float seconds =
        std::chrono::duration<float>(Diagnostic::Clock::now() - *m_sceneFadeStart).count();
    m_shownSceneShare = PathArt::smoothStep(0.0f, k_sceneFadeSeconds, seconds);
    if (m_shownSceneShare >= 1.0f)
        m_sceneFadeStart.reset();
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
    if (m_sceneToShow == index)
        return;
    m_sceneToShow = index;
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
    if (m_sceneOptionsSection)
    {
        // In the heading style AppMenuPage::addSection gives a section's caption.
        m_sceneOptionsSection->setHeaderText(
            Text{ StyledText{ TextStyles::Heading, sceneOptionsCaption(index) } });
    }
    if (m_sceneOptionsControl)
        m_sceneOptionsControl->setCurrentItem(*m_sceneOptionPages[index]);
}

// Asked again when another scene is shown, and when a theme is picked or tried on.
void MillSceneForm::applySceneColors()
{
    m_readoutColors = sceneAt(m_shownScene).readoutColors();
    m_fpsChart.setColors(m_readoutColors.field, m_readoutColors.series, mutedReadoutInk());
}

Color MillSceneForm::mutedReadoutInk() const
{
    return m_readoutColors.ink.withOpacity(k_mutedInkOpacity);
}

void MillSceneForm::dressScenes(const std::size_t index)
{
    for (std::size_t sceneIndex = 0; sceneIndex != sceneCount(); ++sceneIndex)
    {
        PathArt::Scene& scene = sceneAt(sceneIndex);
        if (scene.offersThemes())
            scene.setTheme(index);
    }
}

void MillSceneForm::previewSceneTheme(const std::size_t index)
{
    dressScenes(index);
    m_benchmark.reset();
    applySceneColors();
}

void MillSceneForm::pickSceneTheme(const std::size_t index)
{
    const std::size_t previous = m_pickedTheme;
    m_pickedTheme = index;
    previewSceneTheme(index);
    storeSceneTheme();

    for (const ThemePicker& themePicker : m_themePickers)
    {
        if (themePicker.lanes)
            themePicker.lanes->setCurrentItem(*themePicker.buttons[index]);
        if (!themePicker.buttons.empty())
        {
            themePicker.buttons[previous]->invalidateState();
            themePicker.buttons[index]->invalidateState();
        }
    }
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

void MillSceneForm::previewOption(const std::size_t pickerIndex, const std::size_t option)
{
    const ChoicePicker& choicePicker = m_choicePickers[pickerIndex];
    sceneAt(choicePicker.sceneIndex).chooseOption(choicePicker.choiceIndex, option);
}

void MillSceneForm::pickOption(const std::size_t pickerIndex, const std::size_t option)
{
    ChoicePicker& choicePicker = m_choicePickers[pickerIndex];
    const std::size_t previous = choicePicker.pickedOption;
    choicePicker.pickedOption = option;
    previewOption(pickerIndex, option);

    if (choicePicker.lanes)
        choicePicker.lanes->setCurrentItem(*choicePicker.buttons[option]);
    choicePicker.buttons[previous]->invalidateState();
    choicePicker.buttons[option]->invalidateState();
}

bool MillSceneForm::sceneOffersOptions(const std::size_t index)
{
    return sceneAt(index).offersThemes()
        || sceneWithLooks() == index
        || !sceneAt(index).choices().empty()
        || !sceneAt(index).switches().empty();
}

void MillSceneForm::buildScenePage(AppMenuPage& page)
{
    buildScenePicker(page);
    buildSceneOptions(page);
}

// Built the way the theme picker is: the pointer previews, a pick is the current item.
void MillSceneForm::buildScenePicker(AppMenuPage& page)
{
    StackPanel& group = page.addGroup(L"Scene");
    SingleSelectionStackView& lanes = group.add<SingleSelectionStackView>(
        Orientation::HorizontalWrap,
        LaneSize{ k_sceneTilesPerRow },
        ItemSizing::Equal,
        Spacing{ k_sceneTileSpacing },
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
            ButtonViewMode::TopCenterIcon,
            k_sceneIconSize,
            HorizontalTextAnchor::Center,
            TextFormat{ TextAlign::Center },
            Padding{ k_sceneTilePadding },
            // Every tile takes the whole share of its row, so all come out the same width.
            HorizontalAlign::Fill,
            ShowSelectionOnSurface::Yes
        );
        // The scene is read off the button's own tag, so one painter answers for all of them.
        item.onDrawIcon([this](DrawIconEvent& event) {
            sceneAt(event.tag().value).drawIcon(event.canvas(), event.iconRect());
            });
        item.onGetState([this, index](GetStateEvent& event) {
            event.state.selected = m_pickedScene == index;
            });
        item.onClick([this, index](ClickEvent&) {
            pickScene(index);
            });
        // **A pick and done.** The double click's first click has made the pick.
        item.onDoubleClick([](DoubleClickEvent& event) {
            event.closeForm();
            });
        m_sceneButtons.push_back(&item);
    }
    lanes.setCurrentItem(*m_sceneButtons[m_pickedScene]);
}

// **As large as the largest page**, whichever page shows - see PageSizing::WidestPage. The menu
// keeps the window it was placed in as it opened, so a page larger than the one shown then would
// be cut to that window.
void MillSceneForm::buildSceneOptions(AppMenuPage& page)
{
    AppMenuPage::Section& section = page.addSection(sceneOptionsCaption(m_pickedScene));
    m_sceneOptionsSection = &section;
    section.onDestroy([this](DestroyEvent&) {
        m_sceneOptionsSection = nullptr;
        m_sceneOptionsControl = nullptr;
        m_sceneOptionPages.clear();
    });
    PageControl& optionsControl = section.body().add<PageControl>();
    optionsControl.setPageSizing(PageSizing::WidestPage);
    m_sceneOptionsControl = &optionsControl;

    m_sceneOptionPages.clear();
    m_sceneOptionPages.reserve(sceneCount());
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        StackPanel& optionsPage = optionsControl.add<StackPanel>(
            Orientation::Vertical,
            Spacing{ k_pickerSpacing }
        );
        m_sceneOptionPages.push_back(&optionsPage);
    }

    buildThemePickers();
    buildLookPicker();
    buildChoicePickers();
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        buildSwitchBoxes(index);
        if (!sceneOffersOptions(index))
        {
            m_sceneOptionPages[index]->add<Label>(
                Text{ L"This scene has no options" },
                TextFormat{ InkGrade::Muted }
            );
        }
        buildGestureHints(index);
    }
    optionsControl.setCurrentItem(*m_sceneOptionPages[m_pickedScene]);
}

std::wstring MillSceneForm::sceneOptionsCaption(const std::size_t index)
{
    return std::format(L"{} Options", sceneAt(index).name());
}

// The pickers are listed whole before any is built, as the choice pickers are.
void MillSceneForm::buildThemePickers()
{
    m_themePickers.clear();
    for (std::size_t sceneIndex = 0; sceneIndex != sceneCount(); ++sceneIndex)
    {
        if (sceneAt(sceneIndex).offersThemes())
            m_themePickers.push_back(ThemePicker{ .sceneIndex = sceneIndex });
    }
    for (std::size_t pickerIndex = 0; pickerIndex != m_themePickers.size(); ++pickerIndex)
        buildThemePicker(pickerIndex);
}

void MillSceneForm::buildThemePicker(const std::size_t pickerIndex)
{
    ThemePicker& themePicker = m_themePickers[pickerIndex];
    StackPanel& column = addCaptionedColumn(*m_sceneOptionPages[themePicker.sceneIndex], L"Theme");
    SingleSelectionStackView& lanes = column.add<SingleSelectionStackView>(
        Orientation::VerticalWrap,
        LaneSize{ k_themeLaneSize },
        Spacing{ k_themeLaneSpacing, 0.0f },
        VerticalAlign::Top
    );

    themePicker.lanes = &lanes;
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
    lanes.onDestroy([this, pickerIndex](DestroyEvent&) {
        previewSceneTheme(m_pickedTheme);
        ThemePicker& destroyedPicker = m_themePickers[pickerIndex];
        destroyedPicker.lanes = nullptr;
        destroyedPicker.buttons.clear();
        });

    themePicker.buttons.reserve(PathArt::Themes::allThemes.size());
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
        // A pick and done, as on a scene tile.
        item.onDoubleClick([](DoubleClickEvent& event) {
            event.closeForm();
            });
        themePicker.buttons.push_back(&item);
        ++index;
    }
    // What a stack stands on is its current item, and that is where a preview returns.
    lanes.setCurrentItem(*themePicker.buttons[m_pickedTheme]);

}

// Built the way the theme picker is.
void MillSceneForm::buildLookPicker()
{
    const std::optional<std::size_t> scene = sceneWithLooks();
    if (!scene)
        return;
    StackPanel& column = addCaptionedColumn(*m_sceneOptionPages[*scene], L"Look");
    SingleSelectionStackView& lanes = column.add<SingleSelectionStackView>(
        Orientation::VerticalWrap,
        LaneSize{ k_optionLaneSize },
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
        m_lookLanes = nullptr;
        m_lookButtons.clear();
    });

    m_lookButtons.clear();
    m_lookButtons.reserve(k_lookChoices.size());
    for (std::size_t index = 0; index != k_lookChoices.size(); ++index)
    {
        RadioButton& item = lanes.add<RadioButton>(
            Text{ k_lookChoices[index].name },
            Tag{ index },
            Padding{ 0.0f, styleMetrics(UiElement::ListItem).padding.y }
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

// The pickers are listed whole before any is built, so the handlers' indices stay valid.
void MillSceneForm::buildChoicePickers()
{
    m_choicePickers.clear();
    for (std::size_t sceneIndex = 0; sceneIndex != sceneCount(); ++sceneIndex)
    {
        const PathArt::Scene& scene = sceneAt(sceneIndex);
        for (std::size_t choiceIndex = 0; choiceIndex != scene.choices().size(); ++choiceIndex)
        {
            m_choicePickers.push_back(ChoicePicker{
                .sceneIndex = sceneIndex,
                .choiceIndex = choiceIndex,
                .pickedOption = scene.chosenOption(choiceIndex),
                .lanes = nullptr,
                .buttons = {}
            });
        }
    }
    for (std::size_t pickerIndex = 0; pickerIndex != m_choicePickers.size(); ++pickerIndex)
        buildChoicePicker(pickerIndex);
}

// **Captioned with the choice's name**, on its scene's page. Built the way the look picker is.
void MillSceneForm::buildChoicePicker(const std::size_t pickerIndex)
{
    ChoicePicker& choicePicker = m_choicePickers[pickerIndex];
    const PathArt::Scene& scene = sceneAt(choicePicker.sceneIndex);
    const PathArt::SceneChoice& choice = scene.choices()[choicePicker.choiceIndex];
    StackPanel& column =
        addCaptionedColumn(*m_sceneOptionPages[choicePicker.sceneIndex], choice.name);
    SingleSelectionStackView& lanes = column.add<SingleSelectionStackView>(
        Orientation::VerticalWrap,
        LaneSize{ k_optionLaneSize },
        Spacing{ k_themeLaneSpacing, 0.0f },
        VerticalAlign::Top
    );

    choicePicker.lanes = &lanes;
    lanes.setPreviewMode(PreviewMode::Hover);
    lanes.onPreview([this, pickerIndex](PreviewEvent& event) {
        previewOption(pickerIndex, event.item.tag().value);
    });
    lanes.onDestroy([this, pickerIndex](DestroyEvent&) {
        ChoicePicker& destroyedPicker = m_choicePickers[pickerIndex];
        previewOption(pickerIndex, destroyedPicker.pickedOption);
        destroyedPicker.lanes = nullptr;
        destroyedPicker.buttons.clear();
    });

    choicePicker.buttons.reserve(choice.options.size());
    for (std::size_t option = 0; option != choice.options.size(); ++option)
    {
        RadioButton& item = lanes.add<RadioButton>(
            Text{ choice.options[option] },
            Tag{ option },
            Padding{ 0.0f, styleMetrics(UiElement::ListItem).padding.y }
        );
        item.onGetState([this, pickerIndex, option](GetStateEvent& event) {
            event.state.selected = m_choicePickers[pickerIndex].pickedOption == option;
        });
        item.onClick([this, pickerIndex, option](ClickEvent&) {
            pickOption(pickerIndex, option);
        });
        choicePicker.buttons.push_back(&item);
    }
    lanes.setCurrentItem(*choicePicker.buttons[choicePicker.pickedOption]);
}

// A switch is read from the scene and turned on a click; the scene keeps it with its view.
void MillSceneForm::buildSwitchBoxes(const std::size_t index)
{
    PathArt::Scene& scene = sceneAt(index);
    const std::span<const std::wstring_view> switchNames = scene.switches();
    for (std::size_t switchIndex = 0; switchIndex != switchNames.size(); ++switchIndex)
    {
        CheckBox& box = m_sceneOptionPages[index]->add<CheckBox>(
            Text{ switchNames[switchIndex] },
            Padding{ 0.0f, styleMetrics(UiElement::ListItem).padding.y }
        );
        box.onGetState([&scene, switchIndex](GetStateEvent& event) {
            event.state.selected = scene.isSwitchOn(switchIndex);
        });
        // A different picture is a different load, so the readings start again.
        box.onClick([this, &scene, switchIndex](ClickEvent& event) {
            scene.setSwitch(switchIndex, !scene.isSwitchOn(switchIndex));
            m_benchmark.reset();
            event.control->invalidateState();
        });
    }
}

void MillSceneForm::buildGestureHints(const std::size_t index)
{
    const std::span<const PathArt::SceneGesture> gestures = sceneAt(index).gestures();
    if (gestures.empty())
        return;
    Text hints = {};
    for (std::size_t line = 0; line != gestures.size(); ++line)
    {
        if (line != 0)
            hints << L'\n';
        hints << BoldText{ gestures[line].name } << L' ' << gestures[line].effect;
    }
    addCaptionedColumn(*m_sceneOptionPages[index], L"Controls").add<Label>(std::move(hints));
}

StackPanel& MillSceneForm::addCaptionedColumn(StackPanel& optionsPage,
    const std::wstring_view caption)
{
    StackPanel& column = optionsPage.add<StackPanel>(
        Orientation::Vertical,
        Spacing{ k_pickerCaptionSpacing }
    );
    column.add<Label>(
        Text{ caption },
        TextFormat{ InkGrade::Strong }
    );
    return column;
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
            m_sceneToShow = index;
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
            dressScenes(index);
            m_pickedTheme = index;
            return;
        }
        ++index;
    }
}

void MillSceneForm::storeSceneTheme()
{
    (appContext().config() / k_sceneThemeNodeName).set(
        std::wstring{ PathArt::Themes::allThemes[m_pickedTheme].name });
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
    // With no pick kept, the scene takes the default look, its own being Stylized.
    previewLook(m_pickedLook);
}

void MillSceneForm::storeLook()
{
    (appContext().config() / k_sceneLookNodeName).set(
        std::wstring{ k_lookChoices[m_pickedLook].name });
}

// A scene with no section under its name keeps no view.
void MillSceneForm::restoreSceneViews()
{
    const Dom::Section& views = appContext().config().childSection(k_sceneViewsNodeName);
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        PathArt::Scene& scene = sceneAt(index);
        if (views.child(scene.name()) != nullptr)
            scene.restoreView(views.childSection(scene.name()));
    }
}

void MillSceneForm::storeSceneViews()
{
    Dom::Section& views = appContext().config().childSection(k_sceneViewsNodeName);
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        const PathArt::Scene& scene = sceneAt(index);
        if (views.child(scene.name()) != nullptr)
            scene.storeView(views.childSection(scene.name()));
    }
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
    const Color ink = m_readoutColors.ink;
    const Color muted = mutedReadoutInk();
    Text lines;
    lines << ColoredText{ ink, std::format(L"{:.0f}", m_benchmark.dimensions().x) };
    lines << ColoredText{ muted, L"x" };
    lines << ColoredText{ ink, std::format(L"{:.0f} ", m_benchmark.dimensions().y) };
    lines << ColoredText{ muted, L"pix\u202F", Space{ 0.0f } } << k_endLine;
    lines << ColoredText{ ink, std::format(L"{:.2f} ", cpuVal) };
    lines << ColoredText{ muted, L"CPU\u202F", Space{ 0.0f } } << k_endLine;
    lines << ColoredText{ ink, std::format(L"{:.2f} ", readings.drawMs) };
    lines << ColoredText{ muted, L"ms\u202F\u202F", Space{ 0.0f } } << k_endLine;
    lines << ColoredText{ ink,
        std::format(L"{:.0f} ", readings.drawMs > 0.0 ? 1000.0 / readings.drawMs : 0.0) };
    lines << ColoredText{ muted, L"draw" } << k_endLine;
    lines << ColoredText{ ink, std::format(L"{:.0f} ", readings.fps) };
    lines << ColoredText{ muted, L"FPS\u202F", Space{ 0.0f } } << k_endLine;
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
            Dom::Dt::Value{ k_sceneLookNodeName, L"" },
            Dom::Dt::Section{ k_sceneViewsNodeName, PathArt::createSceneViewsSchema() }
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
