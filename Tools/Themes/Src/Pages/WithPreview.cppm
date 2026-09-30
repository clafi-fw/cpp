export module ThisApp.WithPreview;

import ThisApp.Consts;
import ThisApp.PreviewPanel;

import ClaFi.Application.ThemesManager;
import ClaFi.Application.ThemesManager_Elements;
import ClaFi.App.Themes;

import ClaFi.Documents.BasePage;

import ClaFi.Browser.Control;
import ClaFi.Browser.Settings;

import ClaFi.Controls.Button;
import ClaFi.Controls.Panel;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Divider;

import ClaFi.Icons.MoonIcon;
import ClaFi.Icons.SideBar;
import ClaFi.Icons.SunIcon;

import ClaFi.Core.Foundation;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.DomEngine;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    export template<typename T>
        concept IsDocumentsPage = std::derived_from<T, Documents::DocumentsBasePage>;

    // The preview choices, application wide - outside the template, where a static is per page.
    class PreviewState
    {
    public:
        inline static bool s_inApp{ false };
        inline static ColorMode s_colorMode{ ColorMode::Dark };
    };

    // A page with the preview beside it: its buttons on the right of the bar, and its panel.
    export template<IsDocumentsPage PageBase>
    class WithPreview : public PageBase
    {
    public:
        template<typename... Args>
        explicit WithPreview(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] ThemesManager& themesManager() const { return appThemes(); }
        void restoreViewState() override;
    protected:
        // The mode every preview shows a theme in. It belongs to no theme, so moving it edits none.
        [[nodiscard]] static ColorMode previewColorMode() { return PreviewState::s_colorMode; }
        // What a page draws from the preview mode on top of the preview itself.
        virtual void previewColorModeChanged() {}
        void invalidatePreview();
        virtual const AppTheme* selectedTheme() { return nullptr; }
        void visibilityChanged() override;
    private:
        StackPanel& m_rightToolBar{ this->topPanel().template createRightBar<StackPanel>(
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
            Control::OnPaintIcon{ Icons::MoonIcon::paint },
            HintText{ L"Preview in dark mode" },
            Tag{ ColorMode::Dark }
        ) };

        ToolButton& m_previewLightButton{ m_rightToolBar.add<ToolButton>(
            ShowSelectionOnSurface::Yes,
            IconSize{ 20.0f },
            ButtonViewMode::IconOnly,
            Control::OnPaintIcon{ Icons::SunIcon::paint },
            HintText{ L"Preview in light mode" },
            Tag{ ColorMode::Light }
        ) };

        Divider& m_previewModeDivider{ m_rightToolBar.add<Divider>(
            Padding{ 4.0f }
        ) };
        ToolButton& m_previewInAppButton{ m_rightToolBar.add<ToolButton>(
            ShowSelectionOnSurface::Yes,
            IconSize{ 20.0f },
            ButtonViewMode::IconOnly,
            Control::OnPaintIcon{ Icons::SideBar::paintPanelIcon },
            HintText{ L"Preview in application" }
        ) };

        ToolButton& m_previewInSidebarButton{ m_rightToolBar.add<ToolButton>(
            ShowSelectionOnSurface::Yes,
            IconSize{ 20.0f },
            ButtonViewMode::IconOnly,
            Control::OnPaintIcon{ Icons::SideBar::paintRightBarIcon },
            HintText{ L"Preview in sidebar" }
        ) };

        PreviewPanel& m_previewPanel{ this->template createRightBar<PreviewPanel>(
            this->themeMetrics().page,
            UiElement::Page
        ) };
    };


    //-------------------------------------------------------------------------


    template<IsDocumentsPage PageBase>
    template<typename... Args>
    WithPreview<PageBase>::WithPreview(const CreateParams& params, Args&&... args)
        :
        PageBase{ params, std::forward<Args>(args)... }
    {
        m_previewInAppButton.onClick([this](ClickEvent& event) {
            PreviewState::s_inApp = !PreviewState::s_inApp;
            (this->settings().appConfig() / k_previewInAppAttrName).set(PreviewState::s_inApp);
            invalidatePreview();
            if (!PreviewState::s_inApp)
                applyStoredTheme(this->appContext());
            event.control->invalidateState();
        });
        m_previewInAppButton.onGetState([](GetStateEvent& event) {
            event.state.selected = PreviewState::s_inApp;
        });

        m_previewInSidebarButton.onClick([this](ClickEvent& event) {
            m_previewPanel.toggleVisible();
            (this->tabConfig() / L"Preview").set(m_previewPanel.visible());
            event.control->invalidateState();
        });
        m_previewInSidebarButton.onGetState([this](GetStateEvent& event) {
            event.state.selected = m_previewPanel.visible();
        });

        for (ToolButton* button : { &m_previewDarkButton, &m_previewLightButton })
        {
            button->onClick([this](ClickEvent& event) {
                PreviewState::s_colorMode = event.control->tag<ColorMode>();
                (this->settings().appConfig() / k_previewColorModeAttrName)
                    .set(PreviewState::s_colorMode);
                m_previewDarkButton.invalidateState();
                m_previewLightButton.invalidateState();
                invalidatePreview();
                previewColorModeChanged();
            });
            button->onGetState([](GetStateEvent& event) {
                event.state.selected =
                    event.control.tag<ColorMode>() == PreviewState::s_colorMode;
            });
        }

        m_previewPanel.onAdjustPaint([this](AdjustPaintEvent& event) {
            if (m_previewTheme)
                event.resetTheme(*m_previewTheme, m_previewColors);
        });
    }

    template<IsDocumentsPage PageBase>
    void WithPreview<PageBase>::restoreViewState()
    {
        PageBase::restoreViewState();
        PreviewState::s_inApp = (this->settings().appConfig() / k_previewInAppAttrName)
            .template get<bool>();
        PreviewState::s_colorMode = (this->settings().appConfig() / k_previewColorModeAttrName)
            .template get<ColorMode>();
        const bool previewVisible = (this->tabConfig() / L"Preview").template get<bool>();
        m_previewPanel.setVisible(previewVisible);
    }

    // EVERY POINT AT WHICH WHAT THE PREVIEW SHOWS HAS MOVED. The theme is taken once here, so the
    // panel and the application answer the same question at the same moment; asking the list again
    // while painting put the panel a preview delay ahead of the window.
    template<IsDocumentsPage PageBase>
    void WithPreview<PageBase>::invalidatePreview()
    {
        m_previewTheme = selectedTheme();
        // Held rather than baked where it is read: the panel paints from a reference, and a set
        // baked into the call would be gone before the paint reached it.
        if (m_previewTheme)
            m_previewColors = bake(m_previewTheme->colors, PreviewState::s_colorMode);

        // The tab carries the same theme in its icon.
        this->tab().invalidate();

        if (PreviewState::s_inApp)
        {
            if (m_previewTheme)
                applyTheme(this->appContext(), *m_previewTheme, PreviewState::s_colorMode);
            this->form().invalidate();
        }
        else if (m_previewPanel.visible())
            m_previewPanel.invalidate();
    }

    // A page coming into view is a point at which the preview has moved: the tab before it stood
    // for another theme. The panel reads what invalidatePreview takes, so it is asked for whether
    // or not the application is previewing too - otherwise the panel keeps the theme the page
    // before it left there until the next preview settles.
    template<IsDocumentsPage PageBase>
    void WithPreview<PageBase>::visibilityChanged()
    {
        if (!this->visible())
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

        if (!PreviewState::s_inApp)
            applyStoredTheme(this->appContext());
        invalidatePreview();
    }
}
