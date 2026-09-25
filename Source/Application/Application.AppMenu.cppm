export module ClaFi.App.AppMenu;

import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TabStrip;

import ClaFi.Core.Foundation;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    // What the user asked of the application menu, answered once the menu has closed.
    export enum class AppMenuCommand
    {
        None,
        ShowDiagnostic,
        Exit
    };

    // The application's menu: its pages down the left, its commands under them. See Application
    export class AppMenu : public Controls::TabbedBox
    {
    public:
        explicit AppMenu(const CreateParams&);
        std::wstring_view diagnosticText() const override { return L"AppMenu"; }
        [[nodiscard]] AppMenuCommand command() const { return m_command; }
        // The input that chose the command, which is what carrying it out is authorised by.
        [[nodiscard]] InputStamp commandStamp() const { return m_commandStamp; }
        // The page the menu was left on, which is the page the next one opens. See Application
        [[nodiscard]] std::wstring_view pageCaption() const { return m_pageCaption; }
        // Keeps the open page for the next menu, while this one still stands. See Application
        void storePage();
    public:
        // The Settings page's caption, and what a menu with nothing stored opens on.
        static constexpr std::wstring_view k_settingsPageCaption{ L"Settings" };
        // What the config keeps the last opened page under, by caption. See Application
        static constexpr std::wstring_view k_pageNodeName{ L"AppMenuPage" };
    private:
        // Takes the command and closes the menu; whoever ran the menu carries it out.
        void run(AppMenuCommand, ClickEvent&);
        // A tab for a page, kept so that the stored page can be opened by its caption.
        void addPageTab(std::wstring_view caption, RichControl& page, const PaintIconFunc& paintIcon);
        // Opens the page under that caption, and answers whether the strip has one.
        bool openPage(std::wstring_view caption);
        void openStoredPage();
    private:
        // A FLOOR. The page control measures every page - see PageSizing::WidestPage - so an
        // application's page that wants more than this widens the menu on its own.
        static constexpr float k_width{ 480.0f };
        // Tall enough for the Settings page with every section open. See Application
        static constexpr float k_height{ 400.0f };
    private:
        AppMenuCommand m_command{ AppMenuCommand::None };
        InputStamp m_commandStamp{};
        // Every caption here outlives the menu: a literal, or a page an application stated once.
        std::wstring_view m_pageCaption{ k_settingsPageCaption };
        std::vector<std::pair<std::wstring_view, Controls::Tab*>> m_tabs{};
    };

    // What the backstage keeps of itself: the page it was last left on. See Application
    export [[nodiscard]] Dom::Dt::Section createAppMenuConfigSchema();

    // Puts the menu under every AppButton, once per application. See Application
    export void connectAppMenu();
}
