module ClaFi.App.AppMenu;

import ClaFi.App.Information;
import ClaFi.App.Settings;
import ClaFi.App.Themes;

import ClaFi.Diagnostic.Log;
import ClaFi.Diagnostic.Options;

import ClaFi.Controls.TabbedBox;
// PageSizing is named here, and TabbedBox does not export the module that declares it.
import ClaFi.Controls.PageControl;
import ClaFi.Controls.TabStrip;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.Button;

import ClaFi.Icons.GaugeIcon;
import ClaFi.Icons.DoorIcon;
import ClaFi.Icons.GearIcon;
import ClaFi.Icons.InformationIcon;
import ClaFi.Icons.DotMark;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine_Dt;
// Dom::Value<std::wstring> picks its specialization off the scalar serializer declared here.
import ClaFi.Core.Dom_StdSerializers;

import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

namespace ClaFi
{
    using namespace Controls;

    void openAppMenu(ActionClickEvent&);

    constexpr Padding k_itemPadding{ 24.0f, 12.0f };
    // Twice what a button draws its icon at. The strip's items are the backstage's own headings,
    // and an icon at the toolbar's size reads as a bullet beside one.
    constexpr IconSize k_iconSize{ 32.0f };
    // THE ICON IS WHAT SETS AN ITEM'S HEIGHT, being taller than the caption, and a button anchors
    // its text at the top of the room it is given. Every item here says Center, so the caption
    // stands against the middle of its icon rather than against the icon's first rows.
    constexpr VerticalTextAnchor k_textAnchor{ VerticalTextAnchor::Center };

    Dom::Dt::Section createAppMenuConfigSchema()
    {
        return Dom::Dt::Section{ Dom::Dt::Value{ AppMenu::k_pageNodeName, L"" } };
    }

    // connectAppMenu

    void connectAppMenu()
    {
        appMenuAction().onClick([](ActionClickEvent& event) {
            openAppMenu(event);
        });
    }

    // THE MENU IS RUN TO ITS CLOSE, AND WHAT IT ANSWERED IS CARRIED OUT AFTER. Exit closes the
    // window the menu stands on, and doing that from inside the menu's own loop would take the
    // loop down under the click that asked.
    void openAppMenu(ActionClickEvent& event)
    {
        Form<AppMenu> menu{ event.form.createPopup<AppMenu>(event.presenter) };
        // Under the button that opened it; a shortcut has none, so the menu takes the middle.
        menu.setPlacement(event.presenter ? FormPlacement::Bottom : FormPlacement::Default);
        menu.execute();
        menu.storePage();
        // A THEME TRIED ON GOES WITH THE MENU. The theme picker wears a theme the pointer is
        // over without stating it, so what the config names is what the application is left in -
        // and where nothing was tried on this is the theme it already has, which crosses to
        // itself and shows nothing. See ThemePick
        applyStoredTheme(event.form.appContext());

        switch (menu.command())
        {
        case AppMenuCommand::ShowDiagnostic:
            showDiagnosticWindow(menu.commandStamp());
            break;
        case AppMenuCommand::Exit:
            event.form.rootForm().close();
            break;
        case AppMenuCommand::None:
            break;
        }
    }

    // AppMenu

    AppMenu::AppMenu(const CreateParams& params)
        :
        // A FLOOR, NOT A SIZE: a stated preference stands IN PLACE OF the measurement - see
        // Control::calculate - and the menu has to be measured, or the page control's work below
        // reaches nothing.
        TabbedBox{
            params,
            params.themeMetrics().secondaryWindow,
            params.themeMetrics().secondaryWindowShadow,
            UiElement::Menu,
            TabsOrientation::VerticalLeft,
            TabLineThickness{ Thickness::Bold },
            MinSize{ k_width, k_height }
        }
    {
        // AS WIDE AS THE WIDEST PAGE, not as the page that happens to be open: an application's
        // page is the one likely to want the room, and it is not the page the menu opens on.
        pageControl().setPageSizing(PageSizing::WidestPage);

        if constexpr (Diagnostic::Options::enabled)
        {
            //strip().setPadding(8.0f);
            strip().add<ToolButton>(
                Text{ TextStyleId::SubHeading, L"Show diagnostic" },
                k_itemPadding,
                OpensWindow::Yes,
                ButtonViewMode::LeftIcon,
                k_iconSize,
                k_textAnchor,
                ToolButton::OnPaintIcon{ Icons::GaugeIcon::paint },
                ToolButton::OnClick{ [this](ClickEvent& event) {
                    run(AppMenuCommand::ShowDiagnostic, event);
                } }
            );
        }
        strip().add<ToolButton>(
            Text{ TextStyleId::SubHeading, L"Exit ", params.appContext().appName() },
            k_itemPadding,
            ButtonViewMode::LeftIcon,
            k_iconSize,
            k_textAnchor,
            ToolButton::OnPaintIcon{ Icons::DoorIcon::paint },
            ToolButton::OnClick{ [this](ClickEvent& event) {
                run(AppMenuCommand::Exit, event);
            } }
        );

        // The settings sit at the foot of the strip, whatever the commands above them come to.
        strip().add<FlexSpacer>();
        // strip().add<Divider>(Thickness::Heavy, Padding{ 8.0f, 4.0f });

        // The pages first, the commands under them. The strip takes a button beside its tabs
        // the way a title bar's does.
        addPageTab(L"Information", pageControl().add<InformationPage>(), Icons::InformationIcon::paint);
        addPageTab(k_settingsPageCaption, pageControl().add<SettingsPage>(), Icons::GearIcon::paint);

        // AN APPLICATION'S OWN PAGES STAND BESIDE SETTINGS, in the strip that is already there.
        // One click reaches any of them, the application is named where the eye goes first, and
        // a page is free to be a list where the Settings page is a column of small answers.
        for (const AppPage& page : appPages())
        {
            OptionsPage& appPage = pageControl().add<OptionsPage>();
            page.build(appPage);
            // THE DOT STANDS IN FOR AN ICON THE PAGE DOES NOT STATE. A tab with an empty slot
            // starts its caption where the others start their icon, so the strip reads as two
            // columns that do not line up.
            addPageTab(page.caption, appPage, Icons::DotMark::paint);
        }

        openStoredPage();
    }

    // ASKED OF THE MENU, because appContext() is a control's and openAppMenu is not one. Written
    // while the menu is still standing, which is the only time it can be asked what is open.
    void AppMenu::storePage()
    {
        (appContext().config() / k_pageNodeName).set(std::wstring{ m_pageCaption });
    }

    void AppMenu::run(const AppMenuCommand command, ClickEvent& event)
    {
        m_command = command;
        m_commandStamp = event.stamp;
        event.closeForm();
    }

    // THE CAPTION IS WHAT IS STORED, not the tab's place: a page an application adds moves every
    // index after it, and a config read by hand should show a name.
    void AppMenu::addPageTab(const std::wstring_view caption, RichControl& page, const PaintIconFunc& paintIcon)
    {
        Controls::Tab& tab = strip().addTab(
            Text{ TextStyleId::Heading, caption },
            k_itemPadding,
            ButtonViewMode::LeftIcon,
            k_iconSize,
            k_textAnchor,
            Controls::Tab::OnPaintIcon{ paintIcon },
            Page{ page }
        );
        // A tab is what opens a page, so the press that opens one is where the answer changes.
        tab.onClick([this, caption](ClickEvent&) {
            m_pageCaption = caption;
        });
        m_tabs.emplace_back(caption, &tab);
    }

    // The caption kept is the tab's own, which outlives the menu where the one asked with may not.
    bool AppMenu::openPage(const std::wstring_view caption)
    {
        for (const auto& [tabCaption, tab] : m_tabs)
        {
            if (tabCaption == caption)
            {
                tab->select();
                m_pageCaption = tabCaption;
                return true;
            }
        }
        return false;
    }

    // Settings until a page has been left open, and Settings again where the stored page belonged
    // to an application that no longer states it.
    void AppMenu::openStoredPage()
    {
        const std::wstring stored =
            (appContext().config() / k_pageNodeName).get<std::wstring>();

        if (!openPage(stored))
            openPage(k_settingsPageCaption);
    }
}
