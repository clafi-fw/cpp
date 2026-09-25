module ClaFi.App.Settings;

import ClaFi.App.ThemePick;
import ClaFi.App.Themes;

import ClaFi.Icons.MoonIcon;
import ClaFi.Icons.SunIcon;
import ClaFi.Icons.SunMoonIcon;

import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.Checkbox;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Slider;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Divider;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.AppContext;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;
// Dom::Value<ColorModeSetting> is complete only with the enum serializer and the names it reads.
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.System.InkWell;
// HostProps and BodyProps are declared here - see WithBody, which the options box is one of.
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    using namespace Controls;

    // The pages applications have asked for, as they last stated them. A function-local static
    // rather than a member of anything: the backstage is built afresh every time it opens, and
    // what an application put in it outlives each of those.
    static std::vector<AppPage>& mutableAppPages();

    // The two ways between a percent and a point along the scale slot. See the definitions
    [[nodiscard]] static int percentAt(float position);
    [[nodiscard]] static float positionOf(int percent);

    namespace
    {
        enum class FolderDisplayPurpose
        {
            ToDisplay,    // a hint, which takes no click
            ToCreate,     // the folder is still to be made, and only the system's part stands
            ToDelete      // the folder is asking to be deleted
        };
    }

    // The config folder as text: the system's part muted, the application's in spot ink.
    [[nodiscard]] static Text folderText(const AppContext&, FolderDisplayPurpose);
    // Opens a folder a question names in the system's file manager.
    static void openLinkedFolder(LinkClickEvent&);

    // What a page of options is drawn to: the room around the column, the gap between one
    // section and the next, and the gap between two options inside one.
    constexpr float k_pagePadding{ 12.0f };
    constexpr float k_pageSpacing{ 10.0f };
    constexpr float k_captionSpacing{ 3.0f };
    // THE GAP DOWN A GROUP, from one row to the next. The gap ACROSS a row - a caption and what
    // it names - is the one above, and the two are different questions: a row reads as one thing
    // by standing close, and two rows read as two by standing apart.
    constexpr float k_rowSpacing{ 6.0f };
    // The room a section holds its options in, inside the surface it draws.
    constexpr Padding k_groupPadding{ 12.0f };

    // THE WIDEST THE THEMES MAY MAKE THE PAGE, whatever the list asks for. The backstage is
    // measured from its pages - see AppMenu - so a page that widens widens it, and the themes
    // are the one thing here that grows with what the user has made.
    constexpr float k_themesWidth{ 360.0f };

    // The tallest the options may make the page - past it they scroll. See Application
    constexpr float k_optionsHeight{ 440.0f };

    // WHAT EACH ROW OF THE APPEARANCE SECTION IS CALLED, in a box both rows share, so what they
    // name starts at one left edge whichever word is the longer.
    constexpr float k_rowCaptionWidth{ 52.0f };
    // The box the percent is written in - wide enough for the widest of them, so the slot's left
    // edge holds still while the number under the pointer changes.
    constexpr float k_scaleReadoutWidth{ 44.0f };
    // THE SLOT IS STATED, NOT FILLED. A lane hands every item the width it measured and only a
    // FlexSpacer takes what is over - see Control::fillsLane - so this is what the row comes to,
    // and it is stated to leave the row about as wide as the themes above it are capped at.
    constexpr float k_scaleSliderWidth{ 250.0f };

    // SettingsPage

    // ONE UNIT OF THE SLOT IS ONE PERCENT, so the slot runs from the floor to the ceiling and a
    // step of the slider - an arrow key, a click on an end button - is a step of one.
    int percentAt(const float position)
    {
        return AppContext::k_minScalePercent + static_cast<int>(std::round(position));
    }

    float positionOf(const int percent)
    {
        return static_cast<float>(percent - AppContext::k_minScalePercent);
    }

    // THE SYSTEM'S PART IS THE PLATFORM'S APPLICATION DATA ROOT, the floor deleteConfigFolder
    // stops at. Each ink is pushed inside the link, which keeps the link's own ink off it.
    Text folderText(const AppContext& context, const FolderDisplayPurpose purpose)
    {
        const std::wstring folder = context.configFolder().wstring();
        const std::wstring root = Platform::appDataPath();
        const std::size_t split = folder.starts_with(root) ? root.size() : 0;
        const std::wstring_view systemPart = std::wstring_view{ folder }.substr(0, split);
        const std::wstring_view appPart = std::wstring_view{ folder }.substr(split);

        const bool toCreate = purpose == FolderDisplayPurpose::ToCreate;
        const bool toDelete = purpose == FolderDisplayPurpose::ToDelete;

        Text result{};
        result << VSpace{ 8.0f } << L"\n";
        result << SetIndent{ 12.0f };
        
        if (toDelete)
            result << PushLink{ folder };
        
        // ClaFi part
        result << PushThemeColor{ InkWell::spotInk() };
        result << appPart;
        result << PopColor{};

        // System part
        if (toCreate)
            result << PushLink{ std::wstring{ systemPart } };
        result << InkGrade::Muted;
        result << (toDelete ? L"\nfrom " : L"\nin ");
        result << systemPart;
        result << PopColor{};
        if (toCreate)
            result << PopLink{};
        
        if (toDelete)
            result << PopLink{};
        
        return result;
    }

    void openLinkedFolder(LinkClickEvent& event)
    {
        event.stopPropagation();
        Platform::shellExecute(nullptr, event.target);
    }

    // connectAppPage

    std::vector<AppPage>& mutableAppPages()
    {
        static std::vector<AppPage> pages{};
        return pages;
    }

    const std::vector<AppPage>& appPages()
    {
        return mutableAppPages();
    }

    void connectAppPage(const std::wstring_view caption, AppPageBuilder build)
    {
        mutableAppPages().push_back(AppPage{ std::wstring{ caption }, std::move(build) });
    }

    // OptionsPage

    // THE PAGE IS THE STACK. One column down the page, a group at a time, each item handed the
    // page's width by the lane it stands in.
    OptionsPage::OptionsPage(const CreateParams& params)
        :
        StackPanel{
            params,
            Orientation::Vertical,
            Padding{ k_pagePadding },
            Spacing{ k_pageSpacing }
        }
    {
    }

    // A GROUP IS A SECTION THAT CAN BE PUT AWAY. The caption heads the column its options stand
    // in and that column is its body, so the two are one control and collapsing the caption takes
    // the options with it. A section draws itself, which is what tells it from the groups a page
    // holds INSIDE one - see ThemesList, whose own two wear the divider look.
    //
    // VerticalAlign::Top IS WHAT LETS A SECTION GROW, the same as it is there: a wrapping panel
    // deeper in measures one lane and wraps into as many as it needs once the width arrives, and
    // every group between it and the page has to follow.
    OptionsPage::Section& OptionsPage::addSection(const std::wstring_view caption)
    {
        return add<Section>(
            HostProps{
                VerticalAlign::Top,
                ExpanderViewMode::Section,
                HeaderText{ InkGrade::Strong, caption }
            },
            BodyProps{
                Orientation::Vertical,
                k_groupPadding,
                Spacing{ k_rowSpacing }
            }
        );
    }

    // ScaleSlider

    // BEFORE THE BASE, which sends the position to the point under the pointer and raises the
    // change that writes the config. See Application
    void ScaleSlider::pressDown(PressDownEvent& event)
    {
        form().holdScale();
        Slider::pressDown(event);
    }

    // The same hold, taken again: a press on the thumb never reaches the slot. See Application
    void ScaleSlider::thumbPressDown()
    {
        form().holdScale();
        Slider::thumbPressDown();
    }

    // AFTER THE BASE, which is what clears the pointer's claim on the position.
    void ScaleSlider::pressUp(PressUpEvent& event)
    {
        Slider::pressUp(event);
        form().followScale();
    }

    SettingsPage::SettingsPage(const CreateParams& params)
        :
        // The room around the options is the column's own, and the divider is what parts it from
        // the foot, so the panel between them adds nothing of its own.
        Panel{ params, Padding{ 0.0f }, Spacing{ 0.0f } },
        // The bar always stands, so a section opening or closing moves nothing. See Application
        m_optionsBox{ createBody<ScrollBoxWith<OptionsPage>>(
            HostProps{
                ScrollBars::Vertical,
                MaxSize{ k_maxFloat, k_optionsHeight }
            },
            BodyProps<>{}
        ) },
        m_options{ m_optionsBox.body() },
        m_appearanceGroup{ m_options.addGroup(L"Appearance") },
        m_themeRow{ m_appearanceGroup.add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ k_captionSpacing }
        ) },
        m_themeCaption{ m_themeRow.add<Label>(
            Text{ InkGrade::Strong, L"Theme:" },
            MinSize{ k_rowCaptionWidth, 0.0f },
            MaxSize{ k_rowCaptionWidth, k_maxFloat },
            WordWrap::No,
            VerticalTextAnchor::Center
        ) },
        m_themePick{ m_themeRow.add<ThemePick>(
            MaxSize{ k_themesWidth, k_maxFloat },
            VerticalTextAnchor::Center
        ) },
        m_autoButton{ m_themeRow.add<ToolButton>(
            L"Auto",
            TooltipText{ L"Follow system" },
            ButtonViewMode::IconOnly,
            k_themeIconSize,
            OnPaintIcon{ Icons::SunMoonIcon::paint },
            ShowSelectionOnSurface::Yes,
            VerticalAlign::Center,
            Tag{ ColorModeSetting::Auto }
        ) },
        m_darkButton{ m_themeRow.add<ToolButton>(
            L"Dark",
            ButtonViewMode::IconOnly,
            k_themeIconSize,
            OnPaintIcon{ Icons::MoonIcon::paint },
            ShowSelectionOnSurface::Yes,
            VerticalAlign::Center,
            Tag{ ColorModeSetting::Dark }
        ) },
        m_lightButton{ m_themeRow.add<ToolButton>(
            L"Light",
            ButtonViewMode::IconOnly,
            k_themeIconSize,
            OnPaintIcon{ Icons::SunIcon::paint },
            ShowSelectionOnSurface::Yes,
            VerticalAlign::Center,
            Tag{ ColorModeSetting::Light }
        ) },
        m_scaleRow{ m_appearanceGroup.add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ k_captionSpacing }
        ) },
        m_scaleCaption{ m_scaleRow.add<Label>(
            Text{ InkGrade::Strong, L"Scale:" },
            MinSize{ k_rowCaptionWidth, 0.0f },
            MaxSize{ k_rowCaptionWidth, k_maxFloat },
            WordWrap::No,
            VerticalTextAnchor::Center
        ) },
        m_scaleReadout{ m_scaleRow.add<ScaleReadout>(
            MinSize{ k_scaleReadoutWidth, 0.0f },
            MaxSize{ k_scaleReadoutWidth, k_maxFloat },
            WordWrap::No,
            HorizontalTextAnchor::Right,
            VerticalTextAnchor::Center
        ) },
        m_scaleSlider{ m_scaleRow.add<ScaleSlider>(
            MinSize{ k_scaleSliderWidth, 0.0f },
            MaxSize{ k_scaleSliderWidth, k_maxFloat },
            ScrollButtons::No,
            TooltipText{ L"Scales the UI on top of the system scale" }
        ) },
        m_windowSection{ m_options.addSection(L"Main Window") },
        m_windowGroup{ m_windowSection.body() },
        // NO ROOM AT THE SIDES, so the box stands where a label's text stands. A checkbox is
        // built on the theme's listItem metrics and a Label states none, so the indicator is
        // inset by that padding and the two start at different edges down one group. The room
        // above and below is the theme's, which is what holds the row's height.
        m_alwaysOnTop{ m_windowGroup.add<Checkbox>(
            L"Always on top",
            TooltipText{ L"Keep above others" },
            Padding{ 0.0f, params.themeMetrics().listItem.padding.y }
        ) },
        m_graphicsSection{ m_options.addSection(L"Graphics") },
        m_graphicsGroup{ m_graphicsSection.body() },
        m_gpuAcceleration{ m_graphicsGroup.add<Checkbox>(
            L"GPU acceleration",
            Padding{ 0.0f, params.themeMetrics().listItem.padding.y }
        ) },
        m_footer{ createBottomBar<StackPanel>(
            Orientation::Vertical,
            Padding{ k_pagePadding },
            Spacing{ k_pageSpacing }
        ) },
        m_footerDivider{ m_footer.add<Divider>() },
        m_keepSettings{ m_footer.add<Checkbox>(L"Keep settings on this PC") }
    {
        // THE MODE IS STATED, NOT TRIED ON. A click writes the config like a pick from the list,
        // and the node's change carries it to the theme - see connectAppThemes.
        for (ToolButton* button : { &m_autoButton, &m_darkButton, &m_lightButton })
        {
            button->onGetState([this](GetStateEvent& event) {
                event.state.selected =
                    event.control.tag<ColorModeSetting>() == appContext().colorMode().get();
            });
            button->onClick([this](ClickEvent& event) {
                applyColorMode(appContext(), event.control->tag<ColorModeSetting>());
                m_autoButton.invalidateState();
                m_darkButton.invalidateState();
                m_lightButton.invalidateState();
            });
        }

        m_scaleSlider.setMaxPosition(positionOf(AppContext::k_maxScalePercent));
        m_scaleSlider.setPosition(positionOf(appContext().scalePercent()), false);
        writeScaleReadout();
        // EVERY WINDOW TAKES THE NEW PERCENT AS IT IS WRITTEN - see AppContext::stateScale. The
        // menu is one of them, except while the pointer is moving the thumb: the slider holds
        // this form's size for the length of that gesture and lets it follow again at the
        // release - see ScaleSlider.
        m_scaleSlider.onChange([this](SliderChangeEvent& event) {
            appContext().scale().set(percentAt(event.newPosition));
            writeScaleReadout();
        });
        // The window the menu stands on, not the menu: a popup is held above its owner already,
        // and what the user means by this is where the application sits among other applications.
        m_alwaysOnTop.onGetState([this](GetStateEvent& event) {
            event.state.selected = form().rootForm().isAlwaysOnTop();
        });
        m_alwaysOnTop.onClick([this](ClickEvent&) {
            FormBase& root = form().rootForm();
            root.setAlwaysOnTop(!root.isAlwaysOnTop());
            m_alwaysOnTop.invalidateState();
        });
        // THE SECTION GOES, NOT THE CHECK. Hidden where the platform has no keep-above a client
        // may ask for - see IPlatformWindow::canSetAlwaysOnTop - and a check that could be turned
        // on and would not hold says something untrue about the window. The section is what draws
        // the caption and the surface under it, so hiding the one answer inside it would leave an
        // Main Window card holding nothing.
        if (!form().rootForm().canSetAlwaysOnTop())
            m_windowSection.setVisible(false);

        // THE ANSWER IS WRITTEN, NOT ACTED ON HERE. The node carries it to the context, which
        // states the backend and tells every window - see AppContext::stateBackend - the same
        // path a colour mode travels.
        m_gpuAcceleration.onGetState([this](GetStateEvent& event) {
            event.state.selected = appContext().usingGpu();
        });
        m_gpuAcceleration.onClick([this](ClickEvent&) {
            appContext().gpuAcceleration()->set(!appContext().usingGpu());
            m_gpuAcceleration.invalidateState();
        });
        m_gpuAcceleration.onGetTooltip([](GetTooltipEvent& event) {
            event.text << Fmt{
                L"[b]On[/b][tabto 26] The windows are drawn through the graphics card\n"
                L"[b]Off[/b][tabto 26] They are drawn on the CPU instead"
            };
        });
        // THE SECTION GOES, NOT THE CHECK, the same as the window's above: an application that
        // named no GPU backend has nothing to turn on, and the section is what draws the caption
        // and the surface the check stands on.
        if (!appContext().gpuAvailable())
            m_graphicsSection.setVisible(false);

        // THE FOLDER IS THE PERMISSION. Nothing is stored until one exists, so the check reads
        // the folder rather than a value of its own - see AppContext::configFolderExists.
        m_keepSettings.onGetState([this](GetStateEvent& event) {
            event.state.selected = appContext().configFolderExists();
        });
        m_keepSettings.onClick([this](ClickEvent&) {
            if (appContext().configFolderExists())
                withdrawStorage();
            else
                allowStorage();
            m_keepSettings.invalidateState();
        });
        // Unticked, what the answer is for. Ticked, where the settings are kept.
        m_keepSettings.onGetTooltip([this](GetTooltipEvent& event) {
            if (appContext().configFolderExists())
            {
                event.text << L"Settings are stored in";
                event.text << TextOp::EndLine;
                event.text << folderText(appContext(), FolderDisplayPurpose::ToDisplay);
            }
            else
            {
                event.text << appContext().appName();
                event.text << L" will remember windows placement, settings and other";
                event.text << TextOp::EndLine;
                event.text << L"things between sessions.";
            }
        });
    }

    // ASKED BEFORE THE FOLDER IS MADE, because the folder is what is being allowed: the path is
    // in the question so that what appears on the disk is what was read there.
    void SettingsPage::allowStorage()
    {
        // The name stands as it is spelled.
        Text message{};
        message << appContext().appName();
        message << L" will make a folder to store its settings:";
        message << TextOp::EndLine;
        message << folderText(appContext(), FolderDisplayPurpose::ToCreate);

        MessageDialog dialog{
            m_keepSettings,
            L"Setup settings storage",
            message,
            MessageIcon::Question
        };
        dialog.onLinkClick(openLinkedFolder);
        dialog.add(DialogButton::Ok);
        dialog.add(DialogButton::Cancel);
        if (dialog.execute() != DialogButton::Ok)
            return;

        appContext().ensureConfigFolder();
        // Written at once rather than at the application's exit: the answer was about keeping
        // settings on this computer, and an empty directory keeps nothing.
        appContext().config().save();
    }

    // The whole folder goes, which is what the question names. The publisher's folder above it
    // stands: it is shared with that publisher's other applications, and nothing was asked
    // about it.
    void SettingsPage::withdrawStorage()
    {
        Text message{};
        message << L"Are you sure you want to remove ";
        message << appContext().appName();
        message << L" settings?";
        message << TextOp::EndLine;
        message << L"This folder will now be deleted:";
        message << TextOp::EndLine;
        message << folderText(appContext(), FolderDisplayPurpose::ToDelete);

        MessageDialog dialog{
            m_keepSettings,
            L"Delete stored settings",
            message,
            MessageIcon::Warning
        };
        dialog.onLinkClick(openLinkedFolder);
        dialog.add(DialogButton::Yes);
        dialog.add(DialogButton::Cancel);
        if (dialog.execute() != DialogButton::Yes)
            return;

        appContext().deleteConfigFolder();
    }

    // WHAT THE APPLICATION IS DRAWN AT, not what the thumb names: a percent outside the band is
    // brought inside it - see AppContext::stateScale - and the readout states the answer that was
    // taken.
    void SettingsPage::writeScaleReadout()
    {
        Text reading{};
        reading << Fmt{ L"{}%", appContext().scalePercent() };
        m_scaleReadout.text() = std::move(reading);
        m_scaleReadout.invalidate();
    }
}
