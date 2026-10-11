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

    static void openAppMenu(ActionClickEvent&);

    // A command an application asked for, as it stated it.
    struct AppCommand
    {
        std::wstring caption;
        PaintIconFunc paintIcon;
        EndIcon endIcon{ EndIcon::None };
        AppCommandHandler run;
    };

    // The commands applications have asked for, in the order they asked.
    static std::vector<AppCommand>& appCommands();

    // A command in the strip, doing whatever its handler does with the click that chose it.
    static void addCommand(TabStripBase& strip, const Text& caption,
        const PaintIconFunc& paintIcon, EndIcon, const AppCommandHandler& run);
    // Closes the menu, then asks the application's window to close as its close button does.
    static void closeApplication(ClickEvent&);

    constexpr Padding k_itemPadding{ 24.0f, 12.0f };
    // Twice what a button draws its icon at. The strip's items are the backstage's own headings,
    // and an icon at the toolbar's size reads as a bullet beside one.
    constexpr IconSize k_iconSize{ 32.0f };
    // **The icon is what sets an item's height**, being taller than the caption, and a button
    // anchors its text at the top of the room it is given. Every item here says Center, so the
    // caption stands against the middle of its icon rather than against the icon's first rows.
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

    // connectAppCommand

    std::vector<AppCommand>& appCommands()
    {
        static std::vector<AppCommand> commands{};
        return commands;
    }

    void connectAppCommand(const std::wstring_view caption, PaintIconFunc paintIcon,
        const EndIcon endIcon, AppCommandHandler run)
    {
        appCommands().push_back(
            AppCommand{ std::wstring{ caption }, std::move(paintIcon), endIcon, std::move(run) });
    }

    void openAppMenu(ActionClickEvent& event)
    {
        Form menu{ event.form.createPopup<AppMenu>(event.presenter) };
        // Under the button that opened it; a shortcut has none, so the menu takes the middle.
        menu.setPlacement(event.presenter ? FormPlacement::BottomLeft : FormPlacement::Default);
        menu.execute();
        menu.storePage();
        // **A theme tried on goes with the menu.** The theme picker wears a theme the pointer is
        // over without stating it, so what the config names is what the application is left in -
        // and where nothing was tried on this is the theme it already has, which crosses to
        // itself and shows nothing. See ThemePick
        applyStoredTheme(event.form.appContext());
    }

    // addCommand

    void addCommand(TabStripBase& strip, const Text& caption, const PaintIconFunc& paintIcon,
        const EndIcon endIcon, const AppCommandHandler& run)
    {
        strip.add<ToolButton>(
            caption,
            k_itemPadding,
            endIcon,
            ButtonViewMode::LeftIcon,
            k_iconSize,
            k_textAnchor,
            ToolButton::OnPaintIcon{ paintIcon },
            ToolButton::OnClick{ run }
        );
    }

    // closeApplication

    void closeApplication(ClickEvent& event)
    {
        event.closeForm();
        event.form.rootForm().requestClose();
    }

    // AppMenu

    AppMenu::AppMenu(const CreateParams& params)
        :
        // **A floor, not a size**: a stated preference stands **in place of** the measurement - see
        // Control::calculate - and the menu has to be measured, or the page control's work below
        // reaches nothing.
        TabbedBox{
            params,
            params.themeMetrics().secondaryWindow,
            params.themeMetrics().secondaryWindowShadow,
            UiElement::Menu,
            TabsOrientation::VerticalLeft,
            // TabLineThickness{ Thickness::Bold },
            StripPadding{ 8.0f, 12.0f },
            StripSpacing{ 4.0f, 0.0f },
            MinSize{ k_width, k_height },
            Padding{ -4.0f } // reducing borders clutter
        }
    {
        // **As wide as the widest page**, not as the page that happens to be open: an application's
        // page is the one likely to want the room, and it is not the page the menu opens on.
        pageControl().setPageSizing(PageSizing::WidestPage);

        // **The strip reads top down**: an application's own pages, the commands, and at the foot
        // the framework's two pages. The application is named where the eye goes first, one click
        // reaches any of its pages, and a page is free to be a list where the Settings page is a
        // column of small answers.
        for (const AppPage& page : appPages())
        {
            OptionsPage& appPage = pageControl().add<OptionsPage>();
            page.build(appPage);
            addPageTab(page.caption, appPage, page.paintIcon);
        }

        // The application's own commands lead the framework's.
        for (const AppCommand& command : appCommands())
        {
            addCommand(strip(), Text{ TextStyleId::SubHeading, command.caption },
                command.paintIcon, command.endIcon, command.run);
        }

        if constexpr (Diagnostic::Options::enabled)
        {
            addCommand(strip(), Text{ TextStyleId::SubHeading, L"Show diagnostic" },
                Icons::GaugeIcon::paint, EndIcon::OpensWindow,
                [](ClickEvent& event) {
                    event.closeForm();
                    showDiagnosticWindow(event.stamp);
                });
        }
        addCommand(strip(),
            Text{ TextStyleId::SubHeading, L"Exit ", params.appContext().appName() },
            Icons::DoorIcon::paint, EndIcon::None, closeApplication);

        // The settings sit at the foot of the strip, whatever the commands above them come to.
        strip().add<FlexSpacer>();
        // strip().add<Divider>(Thickness::Heavy, Padding{ 8.0f, 4.0f });

        // The strip takes a button beside its tabs the way a title bar's does.
        addPageTab(L"Information", pageControl().add<InformationPage>(), Icons::InformationIcon::paint);
        addPageTab(k_settingsPageCaption, pageControl().add<SettingsPage>(), Icons::GearIcon::paint);

        openStoredPage();
    }

    // **Asked of the menu**, because appContext() is a control's and openAppMenu is not one.
    // Written while the menu is still standing, which is the only time it can be asked what is
    // open.
    void AppMenu::storePage()
    {
        (appContext().config() / k_pageNodeName).set(std::wstring{ m_pageCaption });
    }

    // **The caption is what is stored**, not the tab's place: a page an application adds moves
    // every index after it, and a config read by hand should show a name.
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
