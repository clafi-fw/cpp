export module ClaFi.App.Settings;

import ClaFi.App.ThemePick;

import ClaFi.Controls.Button;
import ClaFi.Controls.Checkbox;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Slider;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.Divider;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    // A page of captioned option groups - the shape the backstage's pages share. See Application
    export class OptionsPage : public StackPanel
    {
    public:
        // A caption and the column of options it heads, as one control. See Application
        using Section = ExpanderWith<StackPanel>;
    public:
        explicit OptionsPage(const CreateParams&);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"OptionsPage"; }
        // A caption with a column of options under it, added down the page.
        [[nodiscard]] StackPanel& addGroup(std::wstring_view caption)
            { return addSection(caption).body(); }
        // The same, as the whole section - for a page that shows or hides one. See Application
        [[nodiscard]] Section& addSection(std::wstring_view caption);
    };

    // A slider that holds its form's size while the pointer moves it. See Application
    export class ScaleSlider : public Slider
    {
    public:
        template <typename... Args>
        explicit ScaleSlider(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"ScaleSlider"; }
    protected:
        // A press on the slot, which is where the hold starts for a click on it.
        void pressDown(PressDownEvent&) override;
        // A press on the thumb, which the slot never sees - see SliderBase::Thumb::pressDown.
        void thumbPressDown() override;
        // The pointer lets go, and the form is drawn at the size it has named from here.
        void pressUp(PressUpEvent&) override;
    };

    // The backstage's own page: the options every application has, on a box that scrolls under a
    // foot that does not. See Application
    export class SettingsPage : public Controls::Panel
    {
    public:
        explicit SettingsPage(const CreateParams&);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"SettingsPage"; }
    private:
        // The percent as it stands, in a box of its own.
        using ScaleReadout = WithTextLayout<Label>;
    private:
        // Asks, and makes the folder where the answer allows it. See Application
        void allowStorage();
        // Asks, and takes the folder away where the answer allows it. See Application
        void withdrawStorage();
        // Writes the percent the application is drawn at into the readout. See Application
        void writeScaleReadout();
    private:
        // EVERY OPTION THE PAGE HOLDS, ON ONE BOX THAT SCROLLS. The sections are expanders on
        // this column and nothing inside it scrolls on its own, so a group opened past the foot
        // of the menu is reached by scrolling the page rather than through a bar of its own.
        ScrollBoxWith<OptionsPage>& m_optionsBox;
        OptionsPage& m_options;
        StackPanel& m_appearanceGroup;
        // The theme and the mode it is worn in, as one row.
        StackPanel& m_themeRow;
        Label& m_themeCaption;
        ThemePick& m_themePick;
        ToolButton& m_autoButton;
        ToolButton& m_darkButton;
        ToolButton& m_lightButton;
        // The size everything is drawn at, as one row: what it is, what it stands at, and the
        // slot that moves it.
        StackPanel& m_scaleRow;
        Label& m_scaleCaption;
        // Stated, and the same on both rows: what each row names stands at one left edge.
        ScaleReadout& m_scaleReadout;
        ScaleSlider& m_scaleSlider;
        // Held whole: the whole of it goes where the platform has no keep-above to ask for.
        OptionsPage::Section& m_windowSection;
        StackPanel& m_windowGroup;
        Checkbox& m_alwaysOnTop;
        // Held whole for the same reason the window's is: all of it goes where the application
        // named no GPU backend.
        OptionsPage::Section& m_graphicsSection;
        StackPanel& m_graphicsGroup;
        Checkbox& m_gpuAcceleration;
        // The foot of the page, outside the box: the answer here is about the page as a whole,
        // so it stands where it can be seen rather than travelling with the options.
        StackPanel& m_footer;
        Controls::Divider& m_footerDivider;
        Checkbox& m_keepSettings;
    };

    // What an application puts on a page of its own in the backstage. See Application
    export using AppPageBuilder = std::function<void(OptionsPage&)>;

    // A page an application asked for, as it stated it.
    export struct AppPage
    {
        std::wstring caption;
        AppPageBuilder build;
    };

    // The pages applications have asked for, in the order they asked. See Application
    export [[nodiscard]] const std::vector<AppPage>& appPages();

    // Adds a page of the application's own beside Settings, once per application. See Application
    export void connectAppPage(std::wstring_view caption, AppPageBuilder);


    //-------------------------------------------------------------------------


    template <typename... Args>
    ScaleSlider::ScaleSlider(const CreateParams& params, Args&&... args)
        :
        // Whole percents, and a finer slider would be drawn at the scale it moves.
        Slider{ params, FineAdjust::No, std::forward<Args>(args)... }
    {
    }
}
