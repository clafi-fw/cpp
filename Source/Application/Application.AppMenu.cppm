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
    // What an application's own command does with the click that chose it. See Application
    export using AppCommandHandler = std::function<void(ClickEvent&)>;

    // The application's menu: its pages down the left, its commands under them. See Application
    export class AppMenu : public Controls::TabbedBox
    {
    public:
        explicit AppMenu(const CreateParams&);
        std::wstring_view diagnosticText() const override { return L"AppMenu"; }
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
        // A tab for a page, kept so that the stored page can be opened by its caption.
        void addPageTab(std::wstring_view caption, RichControl& page, const PaintIconFunc& paintIcon);
        // Opens the page under that caption, and answers whether the strip has one.
        bool openPage(std::wstring_view caption);
        void openStoredPage();
    private:
        // **A floor.** The page control measures every page - see PageSizing::WidestPage - so an
        // application's page that wants more than this widens the menu on its own.
        static constexpr float k_width{ 480.0f };
        // Tall enough for the Settings page with every section open. See Application
        static constexpr float k_height{ 400.0f };
    private:
        // Every caption here outlives the menu: a literal, or a page an application stated once.
        std::wstring_view m_pageCaption{ k_settingsPageCaption };
        std::vector<std::pair<std::wstring_view, Controls::Tab*>> m_tabs{};
    };

    // What the backstage keeps of itself: the page it was last left on. See Application
    export [[nodiscard]] Dom::Dt::Section createAppMenuConfigSchema();

    // Puts the menu under every AppButton, once per application. See Application
    export void connectAppMenu();

    // Adds a command of the application's own below its pages. See Application
    export void connectAppCommand(std::wstring_view caption, PaintIconFunc paintIcon, EndIcon,
        AppCommandHandler);

    // Opens the menu over the form on the page under that caption. See Application
    export void openAppMenuPage(FormBase&, std::wstring_view caption, InputStamp);
}
