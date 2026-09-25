export module ThisApp.BasePage;

import ThisApp.Consts;
import ThisApp.Utils;
import ThisApp.PreviewPanel;

import ClaFi.Application.ThemesManager;
import ClaFi.Application.ThemesManager_Elements;
import ClaFi.Dom;

import ClaFi.App.Application;
import ClaFi.App.Themes;

import ClaFi.Browser;

import ClaFi.Controls.StackView;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Button;
import ClaFi.Controls.Panel;

import ClaFi.Icons.MoonIcon;
import ClaFi.Icons.SideBar;
import ClaFi.Icons.SunIcon;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    export class BasePage : public Browser::BrowserPage
    {
    public:
        template<typename... Args>
        explicit BasePage(const CreateParams&, Args&&...);
    public:
        ThemesManager& themesManager() const { return appThemes(); }
        virtual void restoreViewState();
        // Whether this page holds work the file behind it has never seen, and how to put it there.
        // A page with nothing to save answers no and does nothing, which is every page but a theme.
        [[nodiscard]] virtual bool hasUnsavedEdits() const { return false; }
        // Whether saveEdits writes without asking anything. A page whose work has no place of its
        // own to go answers no - it still saves, it asks where first, and its toolbar Save is the
        // command that has nothing to do rather than the one that would.
        [[nodiscard]] virtual bool canSaveEdits() const { return true; }
        // Puts this page's work on the disk. ANSWERS WHETHER IT GOT THERE: a page that had to ask
        // where is refused by the user saying no, and leaving then would lose the work Save was
        // pressed to keep.
        //
        // The initiator is what asked - the crumb, the Up button, the tab being closed. A page
        // with a question of its own puts it under the same control, so the second one stands
        // where the first did rather than over whatever happens to be on screen.
        [[nodiscard]] virtual bool saveEdits(Control&) const { return true; }
        // The theme this page's tab stands for, or nullptr where the page is not one theme - which
        // is what decides whether the tab carries a palette tile at all.
        virtual const AppTheme* tabTheme() const { return nullptr; }
    protected:
        StackPanel& toolBar() const { return m_toolBar; }
        // The mode every preview shows a theme in. It belongs to no theme, so moving it edits none.
        [[nodiscard]] static ColorMode previewColorMode() { return s_previewColorMode; }
        // What a page draws from the preview mode on top of the preview itself.
        virtual void previewColorModeChanged() {}
        void invalidatePreview();
        virtual const AppTheme* selectedTheme() { return nullptr; }
        void visibilityChanged() override;
    private:
        // Application wide, and persisted in the root config section, so opening a second tab
        // finds the same setting the first one left.
        inline static bool s_previewInApp{};
        inline static ColorMode s_previewColorMode{ ColorMode::Dark };
        StackPanel& m_topStack{ createTopBar<StackPanel>(
            Orientation::Vertical,
            UiElement::Bar
        ) };
        Panel& m_topPanel{ m_topStack.add<Panel>(
            Padding{ 4.0f },
            Spacing{ 0.0f, 4.0f },
            UiElement::Section
        ) };
        Controls::Divider& m_s0{ m_topStack.add<Controls::Divider>(
            Padding{ 0.0f, 0.0f }
        ) };

        StackPanel& m_toolBar{ m_topPanel.createBody<StackPanel>(
            Orientation::Horizontal,
            Interactivity::ActiveContainer
        ) };
        StackPanel& m_rightToolBar{ m_topPanel.createRightBar<StackPanel>(
            Orientation::Horizontal,
            Interactivity::ActiveContainer
        ) };

        // WHAT THE PREVIEW IS SHOWING - taken by invalidatePreview, and read by both halves of it
        // so that the panel and the application show one theme. Not asked of the list per paint: a
        // list answers with the tile it is ON, and between a tile being reached and the preview
        // delay elapsing that is a tile ahead of the one the preview has been asked for. See
        // StackPanel::previewItem
        const AppTheme* m_previewTheme{ nullptr };
        // What that theme is painted from, kept beside it because the panel paints from a
        // reference the adjustment hands on.
        BakedColors m_previewColors{};

        ToolButton& m_previewDarkButton{ m_rightToolBar.add<ToolButton>(
            ShowSelectionOnSurface::Yes,
            IconSize{ 20.0f },
            ButtonViewMode::IconOnly,
            OnPaintIcon{ Icons::MoonIcon::paint },
            TooltipText{ L"Preview in dark mode" },
            Tag{ ColorMode::Dark }
        ) };

        ToolButton& m_previewLightButton{ m_rightToolBar.add<ToolButton>(
            ShowSelectionOnSurface::Yes,
            IconSize{ 20.0f },
            ButtonViewMode::IconOnly,
            OnPaintIcon{ Icons::SunIcon::paint },
            TooltipText{ L"Preview in light mode" },
            Tag{ ColorMode::Light }
        ) };

        Divider& m_previewModeDivider{ m_rightToolBar.add<Controls::Divider>(
            Padding{ 4.0f }
        ) };
        ToolButton& m_previewInAppButton{ m_rightToolBar.add<ToolButton>(
            ShowSelectionOnSurface::Yes,
            IconSize{ 20.0f },
            ButtonViewMode::IconOnly,
            OnPaintIcon{ Icons::SideBar::paintPanelIcon },
            TooltipText{ L"Preview in application" }
        ) };

        ToolButton& m_previewInSidebarButton{ m_rightToolBar.add<ToolButton>(
            ShowSelectionOnSurface::Yes,
            IconSize{ 20.0f },
            ButtonViewMode::IconOnly,
            OnPaintIcon{ Icons::SideBar::paintRightBarIcon },
            TooltipText{ L"Preview in sidebar" }
        ) };

        PreviewPanel& m_previewPanel{ createRightBar<PreviewPanel>(
            themeMetrics().page,
            UiElement::Page
        ) };
    };

    template<typename... Args>
    BasePage::BasePage(const CreateParams& params, Args&&... args)
        :
        BrowserPage{
            params,
            Padding{ 0.0f },
            Spacing{ 0.0f },
            UiElement::Page,
            std::forward<Args>(args)...
        }
    {
        m_previewInAppButton.onClick([this](ClickEvent& event) {
            s_previewInApp = !s_previewInApp;
            (settings().appConfig() / k_previewInAppAttrName).set(s_previewInApp);
            invalidatePreview();
            if (!s_previewInApp)
                applyStoredTheme(appContext());
            event.control->invalidateState();
            });
        m_previewInAppButton.onGetState([this](GetStateEvent& event) {
            event.state.selected = s_previewInApp;
            });

        m_previewInSidebarButton.onClick([this](ClickEvent& event) {
            m_previewPanel.toggleVisible();
            (tabConfig() / L"Preview").set(m_previewPanel.visible());
            event.control->invalidateState();
            });
        m_previewInSidebarButton.onGetState([this](GetStateEvent& event) {
            event.state.selected = m_previewPanel.visible();
            });

        for (ToolButton* button : { &m_previewDarkButton, &m_previewLightButton })
        {
            button->onClick([this](ClickEvent& event) {
                s_previewColorMode = event.control->tag<ColorMode>();
                (settings().appConfig() / k_previewColorModeAttrName).set(s_previewColorMode);
                m_previewDarkButton.invalidateState();
                m_previewLightButton.invalidateState();
                invalidatePreview();
                previewColorModeChanged();
                });
            button->onGetState([](GetStateEvent& event) {
                event.state.selected = event.control.tag<ColorMode>() == s_previewColorMode;
                });
        }

        m_previewPanel.onAdjustPaint([this](AdjustPaintEvent& event) {
            if (m_previewTheme)
                event.resetTheme(*m_previewTheme, m_previewColors);
            });
    }

    void BasePage::restoreViewState()
    {
        s_previewInApp = (settings().appConfig() / k_previewInAppAttrName).get<bool>();
        s_previewColorMode = (settings().appConfig() / k_previewColorModeAttrName).get<ColorMode>();
        bool previewVisible = (tabConfig() / L"Preview").get<bool>();
        m_previewPanel.setVisible(previewVisible);
    }

    // EVERY POINT AT WHICH WHAT THE PREVIEW SHOWS HAS MOVED. The theme is taken once here, so the
    // panel and the application answer the same question at the same moment; asking the list again
    // while painting put the panel a preview delay ahead of the window.
    void BasePage::invalidatePreview()
    {
        m_previewTheme = selectedTheme();
        // Held rather than baked where it is read: the panel paints from a reference, and a set
        // baked into the call would be gone before the paint reached it.
        if (m_previewTheme)
            m_previewColors = bake(m_previewTheme->colors, s_previewColorMode);

        // The tab carries the same theme in its icon.
        tab().invalidate();

        if (s_previewInApp)
        {
            if (m_previewTheme)
                applyTheme(appContext(), *m_previewTheme, s_previewColorMode);
            form().invalidate();
        }
        else if (m_previewPanel.visible())
            m_previewPanel.invalidate();
    }

    // A page coming into view is a point at which the preview has moved: the tab before it stood
    // for another theme. The panel reads what invalidatePreview takes, so it is asked for whether
    // or not the application is previewing too - otherwise the panel keeps the theme the page
    // before it left there until the next preview settles.
    void BasePage::visibilityChanged()
    {
        if (!visible())
            return;

        // The preview settings are the application's, so another tab may have moved them while
        // this one was off screen, and a button's state is only asked again when it is told to.
        for (ToolButton* button : {
            &m_previewInAppButton,
            &m_previewDarkButton,
            &m_previewLightButton })
        {
            button->invalidateState(AnimationMode::Off);
        }

        if (!s_previewInApp)
            applyStoredTheme(appContext());
        invalidatePreview();
    }
}
