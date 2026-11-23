module;
#include <cstdio>
export module MillScene_App.Main;

import MillScene_App.Icon;
import MillScene_App.Options;

import ClaFi.App.WindowApplication;
import ClaFi.App.AppMenu;
import ClaFi.App.Settings;

import ClaFi.PathArt.Scenes;
import ClaFi.PathArt.MillScene;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.ThemeBlend;
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
import ClaFi.Controls.SplitButton;
import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.ComboBox;
import ClaFi.Controls.TextItems;
import ClaFi.Controls.Menu;

import ClaFi.Icons.DoorIcon;
import ClaFi.Icons.DotMark;

import ClaFi.Core.Context.DrawIconEvent;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;

import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.DomEngine;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Cpu.Canvas;
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
    // A tile's picture slot is wider than the figure it centers, room for a name on one line.
    constexpr IconSize k_sceneIconSize{ 96.0f, 64.0f };
    constexpr float k_sceneTilePadding{ 6.0f };
    // The units and the chart's readings are written at this share of the readout's ink.
    constexpr float k_mutedInkOpacity{ 0.67f };
    constexpr float k_pickerCaptionSpacing{ 4.0f }; // between a picker's caption and its options
    constexpr float k_pickerSpacing{ 12.0f }; // between two controls standing side by side
    // A scene taken up fades in over the one before it, as long as a theme's crossfade.
    constexpr float k_sceneFadeSeconds{ 0.555f };
    // A theme is worn through a crossfade timed on the clock, so a render waits this long and
    // lands its last step rather than writing a half-worn blend.
    constexpr std::chrono::milliseconds k_themeCrossfade{ 600 };
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

    constexpr std::array<std::wstring_view, 8> k_weatherEffectNames{
        L"Theme default", L"None", L"Rain", L"Snow", L"Falling petals", L"Blowing dust",
        L"Fireflies", L"Falling leaves"
    };
    static_assert(k_weatherEffectNames.size()
        == static_cast<std::size_t>(PathArt::WeatherEffect::FallingLeaves) + 1);

    constexpr std::size_t k_themeLaneSize{ 8 };
    constexpr float k_themeIconSize{ 24.0f };
    // The corner the swatch was drawn with, as a fraction of it rather than the 4 design pixels
    // it was: what paints the icon here is handed a rect and no scaler.
    constexpr float k_themeIconRadiusRatio{ 4.0f / k_themeIconSize };
    constexpr float k_themeLaneSpacing{ 12.0f };

    // What the metrics tooltip says draws the scene. See writeMetricsTooltip
    constexpr std::wstring_view k_drawnOnGpu{
        L"The scene is rendered on the GPU." };
    constexpr std::wstring_view k_drawnOnCpu{
        L"The scene is rendered on a single CPU thread, except the Earth scene, whose globe is "
        L"worked out in bands on several cores. CPU is selected under Graphics backend in Settings." };
    constexpr std::wstring_view k_drawnOnCpuOnly{
        L"The scene is rendered on a single CPU thread, except the Earth scene, whose globe is "
        L"worked out in bands on several cores." };

    // **What the picked scene theme is kept under**, and it is kept by name: an index moves the
    // moment the list gains a theme, and a name is what a config file read by hand should show.
    export constexpr std::wstring_view k_sceneThemeNodeName{ L"SceneTheme" };
    constexpr std::wstring_view k_weatherEffectNodeName{ L"WeatherEffect" };
    constexpr std::wstring_view k_lightningNodeName{ L"Lightning" };
    // What the picked scene is kept under, by name for the same reasons as the theme.
    export constexpr std::wstring_view k_sceneNodeName{ L"Scene" };
    // What the scenes keep their views and choices under, a section for each by its name.
    export constexpr std::wstring_view k_sceneViewsNodeName{ L"SceneViews" };

    // Enters fullscreen from a window, and leaves it for the placement the window had.
    void toggleFullscreen(FormBase&);
    // The swatch of the theme at `themeIndex` of allThemes, drawn into the icon's rect.
    void drawThemeIcon(DrawIconEvent&, std::size_t themeIndex);

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
        void nestedDoubleClick(NestedDoubleClickEvent&) override;
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
        MillSceneForm(const CreateParams&, const LaunchOptions&, Args...);
    private:
        // Draw time and frame rate over the span the chart shows
        struct WindowReadings
        {
            double drawMs{ 0.0 };
            double fps{ 0.0 };
        };
        // A picker's lanes and buttons while they stand.
        struct PickerLanes
        {
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
        // The readout's colors and the chart's, as the shown scene states them.
        void applySceneColors();
        [[nodiscard]] Color mutedReadoutInk() const;
        // Every scene that offers themes crossfades to the theme at `index` of allThemes.
        void dressScenes(std::size_t index);
        void previewSceneTheme(std::size_t index);
        // One pick for every scene that offers themes, which every theme picker shows.
        void pickSceneTheme(std::size_t index);
        [[nodiscard]] Text weatherEffectText(PathArt::WeatherEffect effect) const;
        [[nodiscard]] bool lightningEnabled() const;
        void applyWeather(const PathArt::SceneWeather&);
        void previewWeatherEffect(PathArt::WeatherEffect);
        void pickWeather(const PathArt::SceneWeather&);
        void refreshWeatherControls();
        // A picker's current item and buttons, moved from the item at `previous` to `picked`.
        void showPick(const PickerLanes&, std::size_t previous, std::size_t picked);
        // The scene the look picker serves: the first that offers looks, none where none does.
        [[nodiscard]] std::optional<std::size_t> sceneWithLooks();
        // Draws the scene that offers looks in the look at `index` of k_lookChoices.
        void previewLook(std::size_t index);
        void pickLook(std::size_t index);
        void pickOption(std::size_t sceneIndex, std::size_t choiceIndex, std::size_t option);
        void toggleSceneSwitch(PathArt::Scene&, std::size_t switchIndex);
        // Whether a scene offers an option: themes, a look, a choice, a switch.
        [[nodiscard]] bool sceneOffersOptions(std::size_t index);

        // The Scene page of the backstage: the tiles a scene is picked on.
        void buildScenePicker(AppMenuPage&);
        // The scene tiles in `column`, each with a dropdown of its options, the pointer
        // previewing the scene it rests on. Where `closesOnDoubleClick`, a double click on a
        // tile closes the menu the tiles stand in.
        void buildSceneLanes(StackPanel& column, PickerLanes&, bool closesOnDoubleClick);
        // Theme buttons in `column`, with a preview of the theme under the pointer.
        void buildThemeLanes(StackPanel& column, PickerLanes&, Menu* sceneOptions = nullptr);
        void buildWeatherControls(StackPanel& column, std::size_t sceneIndex);
        // A column captioned in the strong ink, holding what is added to it.
        [[nodiscard]] static StackPanel& addCaptionedColumn(StackPanel& column,
            std::wstring_view caption);

        void restoreScene();
        void storeScene();
        void restoreSceneTheme();
        void storeSceneTheme();
        void storeWeather();
        void restoreLook();
        // Each scene that keeps its view takes it back from its section of SceneViews.
        void restoreSceneViews();
        void storeSceneViews();
        void storeSceneView(const PathArt::Scene&);

        void traceMetrics();
        // The readings, written ahead of every frame. See writeMetricsTooltip
        void writeMetrics();
        [[nodiscard]] WindowReadings windowReadings() const;
        void writeMetricsTooltip(AdjustTooltipEvent&);

        // The ways in and out of full screen, the bars following; answers the full-screen toggle.
        [[nodiscard]] Action& connectFullscreen();
        // The two backends as radio lines of the scene's menu, with what each says of the text it
        // draws. See showSceneMenu
        void addBackendLines(Menu& list);
        // Shows the title bar and the strip under the scene in a window, and neither fullscreen.
        void followFullscreen();
        // The list of the Scene line on the scene's context menu: a scene picker.
        void buildSceneList(Menu& list);
        void showSceneOptionsMenu(SplitButton& button, std::size_t sceneIndex);
        // The list of the Theme line on the scene's context menu: a theme picker.
        void buildThemeList(Menu& list);
        void buildWeatherList(Menu& list);
        // The scenes, the themes where the scene offers them, and the full-screen toggle.
        void showSceneMenu(ContextPopupEvent&, Action& fullscreenToggle);

    private:
        // What the command line asked the form to open on; empty leaves the stored pick.
        LaunchOptions m_launch{};
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
        PathArt::SceneWeather m_weather{};
        // The look the user chose, at its index in k_lookChoices.
        std::size_t m_pickedLook{ k_defaultLook };

        // The backstage's Scene page, refilled every time that page is built: a pick repaints the
        // button it left as well as the one it took, and the stack is what the preview returns to.
        PickerLanes m_pageScenePicker{};
        PickerLanes m_menuScenePicker{}; // the one in the scene's context menu, while it is up
        std::optional<std::size_t> m_menuSceneOptionsPreview{};
        std::vector<std::pair<ComboBox*, CheckBox*>> m_weatherControls{};
        PickerLanes m_menuThemePicker{}; // the one in the scene's context menu, while it is up
        // The chart draws the benchmark above; it wears the scene's own colors, which is why it
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
        // **The readings and the chart are one block, and the block holds the tooltip.** The bar
        // around them is as tall as the panel, so a tooltip anchored to it would open at the
        // frame's foot instead of under what it explains.
        StackPanel& m_readoutBlock{ m_infoBar.add<StackPanel>(
            Orientation::Vertical,
            Events{
                [this](AdjustTooltipEvent& event) {
                    writeMetricsTooltip(event);
                }
            }
        ) };
        using MetricsReadout = WithTextLayout<Label>;
        MetricsReadout& m_metricsReadout{ m_readoutBlock.add<MetricsReadout>(
            // **The width is stated**, and it is the chart's. The reading is rewritten every frame,
            // so a readout sized by its own words asks the bar for a new width every time a number
            // gains a digit - and an align pass per frame is paid by the whole form.
            MinSize{Diagnostic::k_fpsChartWidth, 0.0f },
            MaxSize{Diagnostic::k_fpsChartWidth, k_maxFloat },
            WordWrap::No
        ) };

        Diagnostic::FpsChart& m_fpsChart{ m_readoutBlock.add<Diagnostic::FpsChart>(m_benchmark) };

        // To move the scene away from the bottom edge, so the window rounded corners remain intact.
        // RichControl rather than Spacer, because Spacer doesn't have color
        Control& m_bottomSpacer{ createBottomBar<RichControl>(
            MinSize{ 6.0f },
            UiElement::Section
        ) };
    };

    template <typename ... Args>
    MillSceneForm::MillSceneForm(const CreateParams& params, const LaunchOptions& launch,
        Args... args)
        :
        Panel{ params, std::forward<Args>(args)... },
        m_launch{ launch }
    {
        // **Before anything reads the scene.** The info bar takes its colors from the theme, so
        // the stored one has to be the scene's by now.
        restoreSceneTheme();
        restoreScene();
        m_sceneTime = sceneTimeAt(m_launch.at, sceneAt(m_shownScene).cycleDuration());
        restoreSceneViews();
        restoreLook();
        // Written as the window closes, while every scene still stands.
        form().onClose([this](FormCloseEvent&) {
            storeSceneViews();
        });


        connectAppPage(L"Scene", Icons::DotMark::draw, [this](AppMenuPage& page) {
            buildScenePicker(page);
            });
        Action& fullscreenToggle = connectFullscreen();
        m_scenePanel.onContextPopup([this, &fullscreenToggle](ContextPopupEvent& event) {
            showSceneMenu(event, fullscreenToggle);
        });
        form().setKeepsScreenOn(true);
        applySceneColors();

        // The readout has its lines before the first layout, so the first frame moves nothing.
        writeMetrics();
        m_paintTimer.start();
    }

    // What the application is called, wherever its name is shown and wherever its config is kept.
    export [[nodiscard]] ApplicationProperties millSceneApplicationProperties();

    // **What this application keeps of its own**, handed to the application beside the framework's
    // schema and merged with it: the picked scene, the theme picked for the scenes that offer
    // themes and the look picked for the scene that offers looks. Empty is no pick yet, and the
    // showcase then opens on the mill in the theme it starts with, and that scene in the look it
    // starts with.
    export [[nodiscard]] Dom::Dt::Section createMillSceneConfigSchema();

    /// @brief Builds the showcase into a form of the given application, runs it, and answers what
    /// the form answered.
    export int runMillSceneShowcase(WindowApplicationBase&);


//-----------------------------------------------------------------------------


void toggleFullscreen(FormBase& form)
{
    form.setFullscreen(!form.isFullscreen());
}

void drawThemeIcon(DrawIconEvent& event, const std::size_t themeIndex)
{
    const PathArt::SceneTheme& theme = PathArt::Themes::allThemes[themeIndex];
    const float radius = event.iconWidth() * k_themeIconRadiusRatio;
    theme.drawIcon(event.canvas(), event.iconRect(), radius, 1.0f);
}

template<typename Pick>
void pickAndKeepMenuOpen(Menu& menu, ClickEventBase& event, const Pick& pick)
{
    event.stopPropagation();
    pick();
    menu.body().invalidateChildrenStates();
}

template<typename Pick>
void pickOnRightClick(Menu& menu, Control& item, Pick pick)
{
    item.onContextPopup([&menu, pick = std::move(pick)](ContextPopupEvent& event) {
        if (!event.mousePos)
        {
            return;
        }
        pickAndKeepMenuOpen(menu, event, pick);
    });
}

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

// **A double click on the panel where a drag moves the view is the scene's first**, after its
// first click. The press it ends goes on as a press, so it holds the view and is let go as any
// press is. One the scene leaves, and one on the readings over the scene, enters fullscreen or
// leaves it, and the press ends with it: the layout it would go on into is another by then.
void ScenePanel::nestedDoubleClick(NestedDoubleClickEvent& event)
{
    Panel::nestedDoubleClick(event);
    const bool onPanel = &event.clickedControl == this;
    if (onPanel && dragMovesViewFrom(event.clickPos()) && m_scene->doubleClickView(m_sceneBounds))
        return;
    toggleFullscreen(form());
    event.stopPropagation();
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

    showPick(m_pageScenePicker, previous, index);
    showPick(m_menuScenePicker, previous, index);
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
        {
            scene.setTheme(index);
        }
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
    refreshWeatherControls();

    showPick(m_menuThemePicker, previous, index);
}

Text MillSceneForm::weatherEffectText(const PathArt::WeatherEffect effect) const
{
    Text text{ k_weatherEffectNames[static_cast<std::size_t>(effect)] };
    if (effect == PathArt::WeatherEffect::ThemeDefault)
    {
        const PathArt::SceneTheme& theme = PathArt::Themes::allThemes[m_pickedTheme];
        const PathArt::WeatherEffect themeEffect = PathArt::activeWeatherEffect(theme);
        text << L" (" << k_weatherEffectNames[static_cast<std::size_t>(themeEffect)] << L")";
    }
    return text;
}

bool MillSceneForm::lightningEnabled() const
{
    return m_weather.lightning.value_or(PathArt::Themes::allThemes[m_pickedTheme].hasLightning > 0.0f);
}

void MillSceneForm::applyWeather(const PathArt::SceneWeather& weather)
{
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        PathArt::Scene& scene = sceneAt(index);
        if (scene.offersThemes())
        {
            scene.setWeather(weather);
        }
    }
    m_benchmark.reset();
}

void MillSceneForm::previewWeatherEffect(const PathArt::WeatherEffect effect)
{
    PathArt::SceneWeather weather = m_weather;
    weather.effect = effect;
    applyWeather(weather);
}

void MillSceneForm::pickWeather(const PathArt::SceneWeather& weather)
{
    applyWeather(weather);
    if (m_weather == weather)
    {
        return;
    }
    m_weather = weather;
    storeWeather();
    refreshWeatherControls();
}

void MillSceneForm::refreshWeatherControls()
{
    const std::size_t index = static_cast<std::size_t>(m_weather.effect);
    for (const auto& [picker, lightning] : m_weatherControls)
    {
        picker->setItemIndex(index);
        picker->invalidateFormAlign();
        picker->invalidate();
        lightning->invalidateState();
    }
}

void MillSceneForm::showPick(const PickerLanes& picker, const std::size_t previous,
    const std::size_t picked)
{
    if (picker.lanes)
        picker.lanes->setCurrentItem(*picker.buttons[picked]);
    if (!picker.buttons.empty())
    {
        picker.buttons[previous]->invalidateState();
        picker.buttons[picked]->invalidateState();
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
    const std::optional<std::size_t> sceneIndex = sceneWithLooks();
    if (!sceneIndex)
    {
        return;
    }
    m_pickedLook = index;
    PathArt::Scene& scene = sceneAt(*sceneIndex);
    scene.setLook(k_lookChoices[index].look);
    m_benchmark.reset();
    storeSceneView(scene);
}

void MillSceneForm::pickOption(const std::size_t sceneIndex, const std::size_t choiceIndex,
    const std::size_t option)
{
    sceneAt(sceneIndex).chooseOption(choiceIndex, option);
}

void MillSceneForm::toggleSceneSwitch(PathArt::Scene& scene, const std::size_t switchIndex)
{
    scene.setSwitch(switchIndex, !scene.isSwitchOn(switchIndex));
    m_benchmark.reset();
}

bool MillSceneForm::sceneOffersOptions(const std::size_t index)
{
    return sceneAt(index).offersThemes()
        || sceneWithLooks() == index
        || !sceneAt(index).choices().empty()
        || !sceneAt(index).switches().empty();
}

void MillSceneForm::buildScenePicker(AppMenuPage& page)
{
    StackPanel& group = page.addGroup(L"Scene");
    buildSceneLanes(group, m_pageScenePicker, true);
}

// Built the way the theme picker is: the pointer previews, a pick is the current item.
void MillSceneForm::buildSceneLanes(StackPanel& column, PickerLanes& scenePicker,
    const bool closesOnDoubleClick)
{
    SingleSelectionStackView& lanes = column.add<SingleSelectionStackView>(
        Orientation::HorizontalWrap,
        LaneSize{ k_sceneTilesPerRow },
        ItemSizing::Equal,
        Spacing{ 0.0f },
        VerticalAlign::Top
    );

    scenePicker.lanes = &lanes;
    lanes.setPreviewMode(PreviewMode::Hover);
    lanes.onPreview([this](PreviewEvent& event) {
        previewScene(m_menuSceneOptionsPreview.value_or(event.item.tag().value));
        });
    lanes.onDestroy([this, &scenePicker](DestroyEvent&) {
        previewScene(m_pickedScene);
        scenePicker.lanes = nullptr;
        scenePicker.buttons.clear();
        });

    scenePicker.buttons.clear();
    scenePicker.buttons.reserve(sceneCount());
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        const auto addTile = [&]<typename ButtonType>() -> ButtonType& {
            return lanes.add<ButtonType>(
                Text{ sceneAt(index).name() },
                Tag{ index },
                MouseFocus::Self,
                ButtonViewMode::TopCenterIcon,
                k_sceneIconSize,
                HorizontalTextAnchor::Center,
                TextFormat{ TextAlign::Center },
                Padding{ k_sceneTilePadding },
                HorizontalAlign::Fill,
                ShowSelectionOnSurface::Yes
            );
        };
        Control* item = nullptr;
        if (sceneOffersOptions(index))
        {
            SplitButton& button = addTile.operator()<SplitButton>();
            button.onDropdown([this, index](DropdownEvent& event) {
                showSceneOptionsMenu(event.button(), index);
            });
            item = &button;
        }
        else
        {
            item = &addTile.operator()<ToolButton>();
        }
        // The scene is read off the button's own tag, so one painter answers for all of them.
        item->onDrawIcon([this](DrawIconEvent& event) {
            sceneAt(event.tag().value).drawIcon(event.canvas(), event.iconRect());
        });
        item->onGetState([this, index](GetStateEvent& event) {
            event.state.selected = m_pickedScene == index;
        });
        item->onClick([this, index](ClickEvent&) {
            pickScene(index);
        });
        if (closesOnDoubleClick)
        {
            // The first click has already picked the scene.
            item->onDoubleClick([](DoubleClickEvent& event) {
                event.closeForm();
            });
        }
        scenePicker.buttons.push_back(item);
    }
    lanes.setCurrentItem(*scenePicker.buttons[m_pickedScene]);
}

void MillSceneForm::buildThemeLanes(StackPanel& column, PickerLanes& themePicker, Menu* sceneOptions)
{
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
    lanes.onDestroy([this, &themePicker](DestroyEvent&) {
        previewSceneTheme(m_pickedTheme);
        themePicker.lanes = nullptr;
        themePicker.buttons.clear();
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
            drawThemeIcon(event, event.tag().value);
            });
        item.onGetState([this, index](GetStateEvent& event) {
            event.state.selected = m_pickedTheme == index;
            });
        const auto pick = [this, &themePicker, index, sceneOptions]() {
            const std::size_t previous = m_pickedTheme;
            pickSceneTheme(index);
            if (sceneOptions)
            {
                showPick(themePicker, previous, index);
            }
        };
        item.onClick([this, index, pick](ClickEvent& event) {
            if (m_pickedTheme == index)
            {
                event.closeForm();
                return;
            }
            pick();
        });
        if (sceneOptions)
        {
            pickOnRightClick(*sceneOptions, item, pick);
        }
        else
        {
            item.onDoubleClick([](DoubleClickEvent& event) {
                event.closeForm();
            });
        }
        themePicker.buttons.push_back(&item);
        ++index;
    }
    // What a stack stands on is its current item, and that is where a preview returns.
    lanes.setCurrentItem(*themePicker.buttons[m_pickedTheme]);

}

void MillSceneForm::buildWeatherControls(StackPanel& column, const std::size_t sceneIndex)
{
    const auto pickWeatherScene = [this, sceneIndex]() {
        if (m_pickedScene != sceneIndex)
        {
            pickScene(sceneIndex);
        }
    };
    StackPanel& row = column.add<StackPanel>(
        Orientation::Horizontal,
        HorizontalAlign::Left,
        Spacing{ k_pickerSpacing }
    );
    TextItems items{};
    for (const std::wstring_view name : k_weatherEffectNames)
    {
        items.emplace_back(Text{ name });
    }
    ComboBox& picker = row.add<ComboBox>(std::move(items));
    picker.setItemIndex(static_cast<std::size_t>(m_weather.effect));
    picker.onAdjustItemText([this](AdjustItemTextEvent& event) {
        if (&event.item() == &event.comboBox().items().front())
        {
            event.text() = weatherEffectText(PathArt::WeatherEffect::ThemeDefault);
        }
    });
    picker.onPreviewItem([this](PreviewItemEvent& event) {
        previewWeatherEffect(static_cast<PathArt::WeatherEffect>(event.itemIndex()));
    });
    picker.onPick([this, pickWeatherScene](ComboBoxPickEvent& event) {
        PathArt::SceneWeather weather = m_weather;
        weather.effect = static_cast<PathArt::WeatherEffect>(event.itemIndex());
        pickWeather(weather);
        pickWeatherScene();
    });
    CheckBox& lightning = row.add<CheckBox>(Text{ L"Lightning" });
    lightning.onGetState([this](GetStateEvent& event) {
        event.state.selected = lightningEnabled();
    });
    lightning.onClick([this, pickWeatherScene](ClickEvent&) {
        PathArt::SceneWeather weather = m_weather;
        weather.lightning = !lightningEnabled();
        pickWeather(weather);
        pickWeatherScene();
    });
    m_weatherControls.emplace_back(&picker, &lightning);
    row.onDestroy([this, picker = &picker](DestroyEvent&) {
        std::erase_if(m_weatherControls, [picker](const auto& controls) {
            return controls.first == picker;
        });
    });
}

StackPanel& MillSceneForm::addCaptionedColumn(StackPanel& column,
    const std::wstring_view caption)
{
    StackPanel& captionedColumn = column.add<StackPanel>(
        Orientation::Vertical,
        Spacing{ k_pickerCaptionSpacing }
    );
    captionedColumn.add<Label>(
        Text{ caption },
        TextFormat{ InkGrade::Strong }
    );
    return captionedColumn;
}

void MillSceneForm::restoreScene()
{
    // **A name on the command line wins over the kept pick**, and is not kept itself: the launch
    // is a look at a scene, not a change to where the showcase opens next.
    std::wstring name = (appContext().config() / k_sceneNodeName).get<std::wstring>();
    if (!m_launch.scene.empty())
        name = m_launch.scene;
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
    const std::wstring effect = (appContext().config() / k_weatherEffectNodeName).get<std::wstring>();
    const std::wstring lightning = (appContext().config() / k_lightningNodeName).get<std::wstring>();
    const auto effectName = std::ranges::find(k_weatherEffectNames, effect);
    if (effectName != k_weatherEffectNames.end())
    {
        m_weather.effect = static_cast<PathArt::WeatherEffect>(effectName - k_weatherEffectNames.begin());
    }
    if (lightning == L"On" || lightning == L"Off")
    {
        m_weather.lightning = lightning == L"On";
    }

    const std::wstring name = m_launch.theme.empty()
        ? (appContext().config() / k_sceneThemeNodeName).get<std::wstring>() : m_launch.theme;
    if (const std::optional<std::size_t> theme = themeNamed(name))
    {
        m_pickedTheme = *theme;
    }
    dressScenes(m_pickedTheme);
    applyWeather(m_weather);
}

void MillSceneForm::storeSceneTheme()
{
    (appContext().config() / k_sceneThemeNodeName).set(
        std::wstring{ PathArt::Themes::allThemes[m_pickedTheme].name });
}

void MillSceneForm::storeWeather()
{
    const std::size_t index = static_cast<std::size_t>(m_weather.effect);
    (appContext().config() / k_weatherEffectNodeName).set(std::wstring{ k_weatherEffectNames[index] });
    const wchar_t* lightning = !m_weather.lightning ? L"" : *m_weather.lightning ? L"On" : L"Off";
    (appContext().config() / k_lightningNodeName).set(std::wstring{ lightning });
}

void MillSceneForm::restoreLook()
{
    const std::optional<std::size_t> sceneIndex = sceneWithLooks();
    if (!sceneIndex)
    {
        return;
    }
    const PathArt::SceneLook look = sceneAt(*sceneIndex).look();
    for (std::size_t index = 0; index != k_lookChoices.size(); ++index)
    {
        if (k_lookChoices[index].look == look)
        {
            m_pickedLook = index;
            return;
        }
    }
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
    // Closing can interrupt a hover preview.
    if (const std::optional<std::size_t> sceneIndex = sceneWithLooks())
    {
        sceneAt(*sceneIndex).setLook(k_lookChoices[m_pickedLook].look);
    }
    for (std::size_t index = 0; index != sceneCount(); ++index)
    {
        storeSceneView(sceneAt(index));
    }
}

void MillSceneForm::storeSceneView(const PathArt::Scene& scene)
{
    Dom::Section& views = appContext().config().childSection(k_sceneViewsNodeName);
    if (views.child(scene.name()) != nullptr)
    {
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

// **The bars follow the state and not the request**: a compositor may put the window fullscreen
// itself, or refuse to. Escape's action is enabled in fullscreen alone, and the key goes on
// to whatever else answers it in a window.
Action& MillSceneForm::connectFullscreen()
{
    Actions& formActions = form().actions();
    Action& fullscreenToggle = formActions.add(
        Text{ L"Full screen" },
        Shortcut{ Keys::F11 },
        Action::OnDrawIcon{ drawFullscreenIcon },
        Action::OnGetState{ [this](GetActionStateEvent& event) {
            event.claim({ .selected = form().isFullscreen() });
        } },
        // The form is named here: run from a menu line, the event's form is the menu.
        Action::OnClick{ [this](ActionClickEvent&) {
            toggleFullscreen(form());
        } }
    );
    form().onFullscreenChange([this, &fullscreenToggle](FormFullscreenChangeEvent&) {
        followFullscreen();
        fullscreenToggle.invalidateState();
    });
    formActions.add(
        Text{ L"Exit full screen" },
        Shortcut{ Keys::Escape },
        Action::OnGetState{ [this](GetActionStateEvent& event) {
            event.claim({ .enabled = form().isFullscreen() });
        } },
        // The form is named here: run from a menu line, the event's form is the menu.
        Action::OnClick{ [this](ActionClickEvent&) {
            form().setFullscreen(false);
        } }
    );

    connectAppCommand(L"Enter full screen", Icons::DotMark::draw, EndIcon::None,
        [](ClickEvent& event) {
            event.closeForm();
            event.form.rootForm().setFullscreen(true);
        });
    return fullscreenToggle;
}

void MillSceneForm::followFullscreen()
{
    const bool inWindow = !form().isFullscreen();
    m_titleBar.setVisible(inWindow);
    m_bottomSpacer.setVisible(inWindow);
}

// **The two backends as radio lines**, each saying what it draws and what that does to text. The
// answer is written to the same node the Settings page writes, so the two ways of saying it are
// one, and a window already open moves over on the next paint - see AppContext::stateBackend.
void MillSceneForm::addBackendLines(Menu& list)
{
    const auto addLine = [this, &list](const std::wstring_view caption, const bool useGpu) {
        const auto pick = [this, useGpu]() {
            appContext().backendChoice()->set(useGpu ? BackendChoice::Gpu : BackendChoice::Cpu);
        };
        MenuItem& item = list.add(
            Text{ caption },
            // The line wears a radio mark, as the Settings page's choice does.
            IndicatorVisibility::Always,
            IndicatorStyle::Radio,
            ShowSelectionOnSurface::Yes,
            Events{
                [this, useGpu](GetStateEvent& event) {
                    event.state.selected = appContext().usingGpu() == useGpu;
                },
                [this, &list, useGpu, pick](ClickEvent& event) {
                    if (appContext().usingGpu() == useGpu)
                    {
                        event.closeForm();
                        return;
                    }
                    pickAndKeepMenuOpen(list, event, pick);
                },
                [&list, useGpu](AdjustTooltipEvent& event) {
                    constexpr float k_tooltipGap{ 6.0f };
                    const FloatRect menuBounds = list.boundsInForm();
                    const float gap = event.scale(k_tooltipGap);
                    event.anchorRect.left = menuBounds.left - gap;
                    event.anchorRect.right = menuBounds.right + gap;
                    event.placement = TooltipPlacement::Right;
                    if (useGpu)
                    {
                        event.text << L"Render on the GPU." << k_endLine;
                        event.text << L"Subpixel text antialiasing is available.";
                    }
                    else
                    {
                        event.text << L"Render on the CPU." << k_endLine;
                        event.text << L"Subpixel text antialiasing is not supported.";
                    }
                }
            }
        );
        pickOnRightClick(list, item, pick);
    };
    addLine(L"GPU", true);
    addLine(L"CPU", false);
}

void MillSceneForm::buildSceneList(Menu& list)
{
    StackPanel& column = list.addControl<StackPanel>(Orientation::Vertical);
    buildSceneLanes(column, m_menuScenePicker, false);
}

void MillSceneForm::showSceneOptionsMenu(SplitButton& button, const std::size_t sceneIndex)
{
    // **The options open on the scene they are about**, and opening them picks it, as pressing
    // the tile's face does.
    pickScene(sceneIndex);
    PickerLanes themePicker = {};
    Menu options{ button };
    PathArt::Scene& scene = sceneAt(sceneIndex);
    const auto pickSceneInMenu = [this, sceneIndex](ClickEvent& event) {
        event.stopPropagation();
        pickScene(sceneIndex);
    };
    const auto addOption = [this, &options, sceneIndex](Text text, const IndicatorStyle indicatorStyle,
        const auto& getState, const auto& pick) -> MenuItem& {
        MenuItem& item = options.add(
            std::move(text),
            IndicatorVisibility::Always,
            indicatorStyle,
            MenuItem::OnGetState{ getState },
            MenuItem::OnClick{ [this, &options, sceneIndex, indicatorStyle, pick](ClickEvent& event) {
                if (indicatorStyle == IndicatorStyle::Radio && event.control->state().selected)
                {
                    event.closeForm();
                    return;
                }
                pickAndKeepMenuOpen(options, event, [&]() {
                    pick();
                    pickScene(sceneIndex);
                });
            } }
        );
        pickOnRightClick(options, item, pick);
        return item;
    };

    if (scene.offersThemes())
    {
        StackPanel& column = options.addControl<StackPanel>(Orientation::Vertical);
        buildThemeLanes(column, themePicker, &options);
        for (Control* themeButton : themePicker.buttons)
        {
            themeButton->onClick(pickSceneInMenu);
        }
        buildWeatherControls(addCaptionedColumn(column, L"Weather effects"), sceneIndex);
        options.addSeparator();
    }
    if (sceneWithLooks() == sceneIndex)
    {
        options.onClose([this](FormCloseEvent&) {
            previewLook(m_pickedLook);
        });
        options.body().onHoverLeave([this](HoverLeaveEvent&) {
            previewLook(m_pickedLook);
        });
        for (std::size_t lookIndex = 0; lookIndex != k_lookChoices.size(); ++lookIndex)
        {
            MenuItem& item = addOption(
                Text{ k_lookChoices[lookIndex].name },
                IndicatorStyle::Radio,
                [this, lookIndex](GetStateEvent& event) {
                    event.state.selected = m_pickedLook == lookIndex;
                },
                [this, lookIndex]() {
                    pickLook(lookIndex);
                }
            );
            item.onHoverEnter([this, lookIndex](HoverEnterEvent&) {
                previewLook(lookIndex);
            });
        }
        options.addSeparator();
    }
    const std::span<const PathArt::SceneChoice> choices = scene.choices();
    for (std::size_t choiceIndex = 0; choiceIndex != choices.size(); ++choiceIndex)
    {
        const PathArt::SceneChoice& choice = choices[choiceIndex];
        options.addControl<Label>(Text{ choice.name }, TextFormat{ InkGrade::Strong });
        for (std::size_t option = 0; option != choice.options.size(); ++option)
        {
            addOption(
                Text{ choice.options[option] },
                IndicatorStyle::Radio,
                [&scene, choiceIndex, option](GetStateEvent& event) {
                    event.state.selected = scene.chosenOption(choiceIndex) == option;
                },
                [this, sceneIndex, choiceIndex, option]() {
                    pickOption(sceneIndex, choiceIndex, option);
                }
            );
        }
        options.addSeparator();
    }
    const std::span<const std::wstring_view> switchNames = scene.switches();
    for (std::size_t switchIndex = 0; switchIndex != switchNames.size(); ++switchIndex)
    {
        addOption(
            Text{ switchNames[switchIndex] },
            IndicatorStyle::Check,
            [&scene, switchIndex](GetStateEvent& event) {
                event.state.selected = scene.isSwitchOn(switchIndex);
            },
            [this, &scene, switchIndex]() {
                toggleSceneSwitch(scene, switchIndex);
            }
        );
    }

    m_menuSceneOptionsPreview = sceneIndex;
    previewScene(sceneIndex);
    options.executeUnder(button);
    m_menuSceneOptionsPreview.reset();
    previewScene(m_pickedScene);
}

// A new theme pick closes the scene menu stack.
void MillSceneForm::buildThemeList(Menu& list)
{
    StackPanel& column = list.addControl<StackPanel>(Orientation::Vertical);
    buildThemeLanes(column, m_menuThemePicker);
    for (Control* button : m_menuThemePicker.buttons)
    {
        button->onClick([&list](ClickEvent& event) {
            list.closeOnCommand(event);
        });
        pickOnRightClick(list, *button, [this, index = button->tag().value]() {
            pickSceneTheme(index);
        });
    }
}

void MillSceneForm::buildWeatherList(Menu& list)
{
    list.onClose([this](FormCloseEvent&) {
        applyWeather(m_weather);
    });
    list.body().onHoverLeave([this](HoverLeaveEvent&) {
        applyWeather(m_weather);
    });
    const auto addChoice = [&list](Text text, const IndicatorStyle indicatorStyle,
        const auto& getState, const auto& pick) -> MenuItem& {
        MenuItem& item = list.add(
            std::move(text),
            IndicatorVisibility::Always,
            indicatorStyle,
            MenuItem::OnGetState{ getState },
            MenuItem::OnClick{ [&list, indicatorStyle, pick](ClickEvent& event) {
                if (indicatorStyle == IndicatorStyle::Radio && event.control->state().selected)
                {
                    event.closeForm();
                    return;
                }
                pickAndKeepMenuOpen(list, event, pick);
            } }
        );
        pickOnRightClick(list, item, pick);
        return item;
    };
    for (std::size_t index = 0; index != k_weatherEffectNames.size(); ++index)
    {
        const PathArt::WeatherEffect effect = static_cast<PathArt::WeatherEffect>(index);
        MenuItem& item = addChoice(
            weatherEffectText(effect),
            IndicatorStyle::Radio,
            [this, effect](GetStateEvent& event) {
                event.state.selected = m_weather.effect == effect;
            },
            [this, effect]() {
                PathArt::SceneWeather weather = m_weather;
                weather.effect = effect;
                pickWeather(weather);
            }
        );
        item.onHoverEnter([this, effect](HoverEnterEvent&) {
            previewWeatherEffect(effect);
        });
    }
    list.addSeparator();
    MenuItem& lightning = addChoice(
        Text{ L"Lightning" },
        IndicatorStyle::Check,
        [this](GetStateEvent& event) {
            event.state.selected = lightningEnabled();
        },
        [this]() {
            PathArt::SceneWeather weather = m_weather;
            weather.lightning = !lightningEnabled();
            pickWeather(weather);
        }
    );
    lightning.onHoverEnter([this](HoverEnterEvent&) {
        applyWeather(m_weather);
    });
}

// **A fullscreen window shows no bar to leave it by**, so the menu holds the full-screen toggle
// and the application's Exit. It holds both in a window too, where the toggle shows the way in.
void MillSceneForm::showSceneMenu(ContextPopupEvent& event, Action& fullscreenToggle)
{
    event.stopPropagation();
    Menu menu{ m_scenePanel };
    menu.addGroup(
        [this](Menu& list) {
            buildSceneList(list);
        },
        Text{ L"Scene" },
        OnEvent{ [this](DrawIconEvent& iconEvent) {
            sceneAt(m_pickedScene).drawIcon(iconEvent.canvas(), iconEvent.iconRect());
        } }
    );
    if (sceneAt(m_shownScene).offersThemes())
    {
        menu.addGroup(
            [this](Menu& list) {
                buildThemeList(list);
            },
            Text{ L"Theme" },
            OnEvent{ [this](DrawIconEvent& iconEvent) {
                drawThemeIcon(iconEvent, m_pickedTheme);
            } }
        );
        menu.addGroup(
            [this](Menu& list) {
                buildWeatherList(list);
            },
            Text{ L"Weather effects" }
        );
    }
    // The same switch the Settings page offers, for a window already open.
    if (appContext().gpuAvailable())
    {
        menu.addGroup(
            [this](Menu& list) {
                addBackendLines(list);
            },
            Text{ L"Graphics backend" }
        );
    }
    menu.addSeparator();
    menu.add(
        fullscreenToggle,
        IndicatorVisibility::Always,
        IndicatorStyle::Check,
        ShowSelectionOnSurface::No
    );
    menu.addSeparator();
    menu.add(
        Text{ L"Exit ", appContext().appName() },
        MenuItem::OnDrawIcon{ Icons::DoorIcon::draw },
        // The menu closes ahead of the window, as the app menu's Exit does.
        MenuItem::OnClick{ [this](ClickEvent& event) {
            event.closeForm();
            form().requestClose();
        } }
    );
    menu.execute();
}


    ApplicationProperties millSceneApplicationProperties()
    {
        return millSceneProperties(L"MillScene");
    }

    Dom::Dt::Section createMillSceneConfigSchema()
    {
        return Dom::Dt::Section{
            Dom::Dt::Value{ k_sceneNodeName, L"" },
            Dom::Dt::Value{ k_sceneThemeNodeName, L"" },
            Dom::Dt::Value{ k_weatherEffectNodeName, L"" },
            Dom::Dt::Value{ k_lightningNodeName, L"" },
            Dom::Dt::Section{ k_sceneViewsNodeName, PathArt::createSceneViewsSchema() }
        };
    }

    // Said of the Earth scene, so it is not taken for a map.
    constexpr std::wstring_view k_earthSceneIsArt{
        L"The Earth scene is a piece of art and does not claim to be accurate."
    };
    // Natural Earth's own credit, for the Earth scene's coastlines and sea depths.
    constexpr std::wstring_view k_naturalEarthCredit{
        L"Made with Natural Earth. Free vector and raster map data @ https://naturalearthdata.com."
    };

    int runMillSceneShowcase(WindowApplicationBase& application)
    {
        const CommandLine& commandLine = application.commandLine();
        if (const std::optional<int> answer = answerCommandLine(commandLine))
            return *answer;

        std::wstring error{};
        const std::optional<LaunchOptions> options = launchOptionsOf(commandLine, error);
        if (!options)
        {
            writeErrorToConsole(error);
            writeErrorToConsole(L"\n");
            writeErrorToConsole(commandLine.usage());
            return ExitCode::Misused;
        }

        application.context().addInformation(k_earthSceneIsArt);
        application.context().addInformation(k_naturalEarthCredit);
        const auto form = application.createDialog<MillSceneForm>(
            *options,
            UiElement::Dialog,
            MinSize{ 320.0f, 240.0f },
            PreferredSize{ 840.0f, 680.0f }
        );
        form->setConfigName(AppContext::k_mainFormName);
        return form->execute();
    }
}
