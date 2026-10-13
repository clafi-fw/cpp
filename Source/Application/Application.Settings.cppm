module;
#include "../Y-Core/System/Switches.h"

export module ClaFi.App.Settings;

import ClaFi.App.ThemePick;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Slider;
import ClaFi.Controls.Stack;
import ClaFi.Controls.Divider;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    // A page of captioned option groups - the shape the backstage's pages share. See Application
    export class OptionsPage : public Stack
    {
    public:
        // A caption and the column of options it heads, as one control.
        using Section = ExpanderWith<Stack>;
    public:
        explicit OptionsPage(const CreateParams&);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"OptionsPage"; }
        // A caption with a column of options under it, added down the page.
        [[nodiscard]] Stack& addGroup(std::wstring_view caption)
            { return addSection(caption).body(); }
        // The same, as the whole section - for a page that shows or hides one.
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
        void nestedPressDown(PressDownEvent&) override;
        // A press on the thumb, which the slot never sees - see SliderBase::Thumb::nestedPressDown.
        void thumbPressDown() override;
        // The pointer lets go, and the form is drawn at the size it has named from here.
        void nestedPressUp(PressUpEvent&) override;
    };

    // The backstage's page of the options every application has. See Application
    export class SettingsPage : public Controls::Panel
    {
    public:
        explicit SettingsPage(const CreateParams&);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"SettingsPage"; }
    private:
        // A percent as it stands, in a box of its own.
        using PercentReadout = WithTextLayout<Label>;
        // What a row of the interface section is called, in the box the rows share.
        class RowCaption : public Label
        {
        public:
            RowCaption(const CreateParams&, std::wstring_view caption);
        };
    private:
        void allowStorage(); // asks, and makes the folder where the answer allows it
        void withdrawStorage(); // asks, and takes the folder away where the answer allows it
        void writeScaleReadout(); // writes the percent the application is drawn at into the readout
#if CLAFI_TEXT_MOVING
        // Writes the Z animation amount the application moves at into the readout.
        void writeZAnimationReadout();
#endif
    private:
        // **Every option the page holds, on one box that scrolls.** The sections are expanders on
        // this column and nothing inside it scrolls on its own, so a group opened past the foot
        // of the menu is reached by scrolling the page rather than through a bar of its own.
        ScrollBoxWith<OptionsPage>& m_optionsBox;
        OptionsPage& m_options;
        Stack& m_interfaceGroup;
        // The theme and the mode it is worn in, as one row.
        Stack& m_themeRow;
        RowCaption& m_themeCaption;
        ThemePick& m_themePick;
        ToolButton& m_autoButton;
        ToolButton& m_darkButton;
        ToolButton& m_lightButton;
        // The size everything is drawn at, as one row: what it is, what it stands at, and the
        // slot that moves it.
        Stack& m_scaleRow;
        RowCaption& m_scaleCaption;
        // Stated, and the same on both rows: what each row names stands at one left edge.
        PercentReadout& m_scaleReadout;
        ScaleSlider& m_scaleSlider;
#if CLAFI_TEXT_MOVING
        // How far controls move in depth, as a row laid out the way the scale's is.
        Stack& m_zAnimationRow;
        RowCaption& m_zAnimationCaption;
        PercentReadout& m_zAnimationReadout;
        Slider& m_zAnimationSlider;
#endif
        CheckBox& m_alwaysOnTop;
        // Held whole: all of it goes where the application named no GPU backend.
        OptionsPage::Section& m_performanceSection;
        Stack& m_performanceGroup;
        CheckBox& m_gpuAcceleration;
        // The foot of the page, outside the box: the answer here is about the page as a whole,
        // so it stands where it can be seen rather than travelling with the options.
        Stack& m_footer;
        Controls::Divider& m_footerDivider;
        CheckBox& m_keepSettings;
    };

    // What an application puts on a page of the backstage, its own or Settings. See Application
    export using AppPageBuilder = std::function<void(OptionsPage&)>;

    // A page an application asked for, as it stated it.
    export struct AppPage
    {
        std::wstring caption;
        PaintIconFunc paintIcon;
        AppPageBuilder build;
    };

    // The pages applications have asked for, in the order they asked.
    export [[nodiscard]] const std::vector<AppPage>& appPages();

    // Adds a page of the application's own beside Settings, once per application. See Application
    export void connectAppPage(std::wstring_view caption, PaintIconFunc paintIcon,
        AppPageBuilder);

    // Adds the application's own groups to Settings, under the common ones. See Application
    export void connectAppSettings(AppPageBuilder);


    //-------------------------------------------------------------------------


    template <typename... Args>
    ScaleSlider::ScaleSlider(const CreateParams& params, Args&&... args)
        :
        // Whole percents, and a finer slider would be drawn at the scale it moves.
        Slider{ params, FineAdjust::No, std::forward<Args>(args)... }
    {
    }
}
