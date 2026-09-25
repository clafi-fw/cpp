export module ThisApp.ThemePage;

import ThisApp.BasePage;
import ThisApp.CodeOptions;
import ThisApp.Consts;
import ThisApp.FloorSlider;
import ThisApp.HueRuleControl;
import ThisApp.RuleSlider;
import ThisApp.Utils;
import ThisApp.Palette.Controls;

// Icons
import ClaFi.Icons.SaveIcon;
import ClaFi.Icons.HueIcon;
import ClaFi.Icons.SaturationIcon;
import ClaFi.Icons.LuminosityIcon;

import ClaFi.Controls.Base.MessageBoxBase;
import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Checkbox;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.ColorSlider;
import ClaFi.Controls.ComboBox;
import ClaFi.Controls.Expander;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.RadioButton;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Divider;
import ClaFi.Controls.PromptDialog;
import ClaFi.Controls.Slider;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.SplitButton;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TabStrip;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.TextItems;
import ClaFi.Controls.Base.SliderBase;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.StdActions;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Document;

import ClaFi.Core.Foundation;

import ClaFi.Core.Syntax.Languages;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Controls::Grids::Dt;

    // One code page: the document on the right, and on the left the choice of how much of it to
    // state. A page needing more than that choice adds to options().
    export class CodePage : public Panel
    {
    public:
        template<typename... Args>
        explicit CodePage(const CreateParams&, Args&&...);
        [[nodiscard]] StackPanel& options() { return m_options; }
        [[nodiscard]] StackPanel& contentGroup() { return m_contentGroup; }
        [[nodiscard]] CodeBox& box() { return m_box; }
    private:
        StackPanel& m_options{ createLeftBar<ScrollBox>(
            ScrollBars::Vertical,
            UiElement::Section
        ).createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 8.0f, 4.0f },
            Spacing{ 4.0f }
        ) };

        Label& m_contentLabel{ m_options.add<Label>(
            Text{ TextStyleId::Section, L"Content" }
        ) };

        // One container per set of choices: a click repaints its siblings, so the group is what
        // bounds the buttons a choice excludes.
        StackPanel& m_contentGroup{ m_options.add<StackPanel>(
            Orientation::Vertical
        ) };

        ScrollBox& m_scrollBox{ createBody<ScrollBox>(
            ScrollBars::Both
        ) };

        // The document the page shows, read in the language the page names.
        CodeBox& m_box{ m_scrollBox.createBody<CodeBox>(
            Orientation::Vertical,
            Padding{ 12.0f }
        ) };
    };

    export class ThemePage : public BasePage
    {
    public:
        template<typename... Args>
        explicit ThemePage(const CreateParams&, Args&&...);
    public:
        void restoreViewState() override;
        [[nodiscard]] bool hasUnsavedEdits() const override;
        [[nodiscard]] bool canSaveEdits() const override { return !isBuiltIn(); }
        [[nodiscard]] bool saveEdits(Control& initiator) const override;
        const AppTheme* tabTheme() const override { return &m_editTheme; }
    protected:
        const AppTheme* selectedTheme() override { return &m_editTheme; }
        void visibilityChanged() override;
        // The grid reads every element in the preview's mode.
        void previewColorModeChanged() override;
    private:
        // A fold of the design page: the name the tab stores it under, and how to read and set it.
        struct Fold
        {
            std::wstring name;
            std::function<bool()> expanded;
            std::function<void(bool)> setExpanded;
            Control* head{}; // what opens and closes it
            Control* stop{}; // where the focus goes when the fold closes over it
            [[nodiscard]] bool holds(const Control*) const; // in its head or its body
        };
        using FoldCollection = std::vector<Fold>;
        using FoldNames = std::vector<std::wstring>;
    private:
        ThemeColors& editColors() { return m_editTheme.colors; }
        //
        void anchorChanged();
        void harmonyChanged();
        void paletteMapChanged();
        void mapHuesToColors();
        // The icon hues of the harmony the theme names, written as marks after the row's
        // caption, in slot order. They are derived from the anchor and the harmony kind alone, so
        // nothing here reads or writes the palette.
        void iconHuesText(GetTextEvent&);
        void darkModeFloorChanged();
        void darkModeFloorText(GetTextEvent&);
        //
        void storeViewState() const;
        // Remembers a fold, and stores the tab's folds whenever it turns.
        void addFold(Fold&&);
        // An expander's fold, named by its title.
        void addFold(ExpanderHeader&);
        // A group's fold, named by its element's token - the name the theme file knows it by.
        void addFold(UiElement, Grids::RowGroupSpan&);
        // What a section of the grid registers its fold through, once its title is written.
        [[nodiscard]] Init<Grids::Rt::RowExpander> sectionFold();
        void storeFolds() const;
        void restoreFolds();
        void connectFoldMenu(); // the design page's menu, which turns its folds at once
        void showFoldMenu(ContextPopupEvent&);
        [[nodiscard]] bool anyFold(bool expanded) const;
        [[nodiscard]] bool canCollapseOtherFolds() const;
        void setAllFolds(bool expanded);
        void collapseOtherFolds(); // every fold the menu's target does not stand in
        void followFolds(const Control* owner); // keeps the focus and the menu's target in sight
        // The outermost closed fold whose body holds the control, or null where none does.
        [[nodiscard]] const Fold* hidingFold(const Control*) const;
        void saveTheme() const;
        // Writes this page's work to a name the user gives, and takes the tab there. The original
        // is left on the disk as it stands, which is what tells this from Save.
        void saveThemeAs(Control& initiator);
        // Asks for a name and writes the work to it, and does no more than that. Answers the file
        // name it wrote, or nothing where the user would not give one.
        [[nodiscard]] std::wstring saveThemeAsFile(Control& initiator) const;
        // Puts the name the user typed back to them where a theme already stands under it, and
        // answers whether they said to write over it. Raised from inside the naming question and
        // owned by the answer that asked, so it stands on top of it - see AcceptEditEvent.
        [[nodiscard]] bool confirmReplacingTheme(Control& initiator, std::wstring_view stem) const;
        void writeThemeTo(const std::filesystem::path& fileName) const;
        // The file a user theme of this name stands in. Save's own file is named by the page and
        // not through here: a built-in's page name already carries its own extension.
        [[nodiscard]] std::filesystem::path themeFileName(std::wstring_view stem) const;
        // Whether this page stands for one of the built-ins. They are answered from the
        // compiled-in colours rather than from a file, so Save has nothing to write over and
        // falls back to Save as.
        [[nodiscard]] bool isBuiltIn() const;
        //
        // Blueprint fragments built by function, so one definition serves both
        // column pairs - the pattern Dom uses for makeNetworkSection().
        CellSet operationCells(ColumnTag actionKey, ColumnTag valueKey, ComboboxTarget target);
        // A rule typed over an Operation cell - +0.02, =0.66 - sets its operation and value at once.
        void acceptOperationText(ComboboxAcceptTextEvent&);
        // An element the descriptor gives one state or none: one row, its name spanning the
        // Name and State columns.
        Row staticRow(UiElement) const;
        Row elementRow(UiElement, UiElementState) const;
        // An element the descriptor gives two states or more: one group, the header naming the
        // element and one row per state it names. Not const, because the header's Flip control
        // takes handlers that edit the theme - and a blueprint built in a member initializer is
        // built on a page that is not const either.
        Group elementGroup(UiElement);
        // One cell text provider per cell kind, instead of one switch over all of them.
        // Same signature as a row or grid level handler, so any of these could be
        // connected at either level unchanged.
        // Whether the row states a colour nothing stands under. A window root opens a window
        // of its own and what is behind it is not the theme's to know, so its Surface rule names
        // its hue outright and sets its saturation and elevation: the hue cell refuses Clear and
        // the two operation cells are not the user's to move. Its other rows derive as any
        // element's do.
        [[nodiscard]] static bool statesAbsoluteSurface(const Grids::Rt::RowBase&);
        void elementNameCell(Grids::GetCellTextEvent&) const;
        void elementStateCell(Grids::GetCellTextEvent&) const;
        // The mode an element stands in, accumulated down that nesting: the preview's, flipped
        // once for every element on the way that states a flip. Every rule an element carries is
        // read in it, so a ramp is drawn the way the painter will walk it.
        [[nodiscard]] ColorMode elementColorMode(OptionalUiElement);
        [[nodiscard]] const ColorRule& rowRule(UiElement, UiElementState);
        [[nodiscard]] Hsl elementColor(OptionalUiElement);
        // The ink an element's text starts from: the same nesting as elementColor, walked
        // through text rules instead of surface ones.
        [[nodiscard]] Hsl elementTextColor(OptionalUiElement);
        // The ink of the colour mode before any rule has touched it.
        [[nodiscard]] Hsl bareInk();
        [[nodiscard]] RuleBase rowRuleBase(const Grids::Rt::RowBase&, RuleChannel);
        //
        ColorRule& rowColorRule(Grids::Row&);
        ControlColorRules& rowColorRules(Grids::Rt::Row&);
        // The rules a control standing in a cell edits. A cell blueprint names a column and knows
        // nothing of the row, so a control asks through its own parent, which is the row it landed
        // in - the group's span, for a control the span declares.
        ControlColorRules& rowColorRulesOf(const Control&);
        // The Flip cell, which stands once per group rather than once per state: the element is
        // either carried across the theme or it is not, and its states are all on the side it
        // ends up on.
        void elementFlipState(GetStateEvent&);
        void elementFlipClicked(ClickEvent&);
        void updateGridControls();
        void updateRowControls(ComboBox&, RuleSlider&, ColorRuleValue&);
        void updateHueControl(Grids::Rt::RowContainer&);
        void updateSaturationSliderAndCombobox(Grids::Rt::RowContainer&);
        void updateElevationSliderAndCombobox(Grids::Rt::RowContainer&);
        void hueRuleChanged();
        void generateCode();
        // The theme as one document, written into a box in the format's own spelling.
        void writeThemeTo(const Dom::FileFormatBase&, TextBox&);
        // The document a box shows, replacing the one it holds.
        void showCode(const std::wstring& code, TextBox&);
        // What a theme states: the tree the framework would have built is made beside the
        // theme's own, and what differs is what gets written. A file states the same, so a
        // theme carrying nothing of its own comes back as whatever the framework defaults to.
        [[nodiscard]] std::unique_ptr<Dom::Section> statedTheme(const Dom::Section&) const;
        // A pick was made on this page. The action has already moved the answer and refreshed
        // every presenter of it; what is left is the document this page is showing.
        void codeOptionPicked(ClickEvent&);
    private:
        static constexpr TagValue k_harmonyTag = 2ull;
        // The floor slider's end. Useful floors sit far below it; at 1 a surface has no room.
        static constexpr float k_maxDarkModeFloor = 0.5f;
        // Holds "No change" in SubBody on the common sans-serif faces. DejaVu Sans, the widest,
        // needs 95 with the combobox's strip and padding.
        static constexpr float k_operationColumnWidth = 96.0f;
        // Save on the face, Save as behind the strip. Both are StdActions, so the page answers
        // for them once and the keys reach the same answer the button does.
        SplitButton& m_saveButton{ toolBar().add<SplitButton>(
            StdActions::save,
            ButtonViewMode::LeftIcon,
            IconSize{ 18.0f }
        ) };
    private:
        OnSelectPaletteMap m_onSelectPaletteMap;
        OnHueRuleChanged m_onHueRuleChanged;

        Controls::Divider& m_sep1{ toolBar().add<Controls::Divider>(
            Padding{ 4.0f }
        ) };

        AppTheme m_editTheme{};
        // Set while restoreViewState brings the controls up on the theme it has just read. What it
        // calls to do that are the same handlers a user edit calls, and each of them ends by
        // writing the theme back - see storeViewState.
        bool m_restoring{ false };
        // Set by Save as, which has just written this page's work under another name. What stands
        // in the view state differs from this page's own file and always will - and nothing is at
        // risk by it: the work is on the disk, and the tab is on its way to where it went.
        bool m_savedUnderAnotherName{ false };
        // Declared ahead of the grid, whose sections and groups add theirs as it is built.
        FoldCollection m_folds;
        Control* m_foldMenuTarget{}; // what the design page's menu was raised on, while it is up

        ColorHarmonySelector m_harmonySelector{ editColors().anchorHue };

        TabbedBox& m_tabbedBox{ createBody<TabbedBox>(
            TabsOrientation::HorizontalBottom
        ) };

        ScrollBox& m_designScrollBox{ m_tabbedBox.pageControl().add<ScrollBox>(
            ScrollBars::Vertical
        ) };

        CodePage& m_cppCodePage{ m_tabbedBox.pageControl().add<CodePage>() };
        CodePage& m_claFiPage{ m_tabbedBox.pageControl().add<CodePage>() };
        CodePage& m_xmlPage{ m_tabbedBox.pageControl().add<CodePage>() };
        CodePage& m_jsonPage{ m_tabbedBox.pageControl().add<CodePage>() };

        StackPanel& m_designView{ m_designScrollBox.createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 12.0f },
            Spacing{ 8.0f }
        ) };

        // The scope is the C++ page's own question, so it is added to that page's bar under the
        // choice every code page carries.
        Controls::Divider& m_cppOptionsDivider{ m_cppCodePage.options().add<Controls::Divider>(
            Thickness::Heavy,
            Padding{ 0.0f, 4.0f }
        ) };

        Label& m_cppScopeLabel{ m_cppCodePage.options().add<Label>(
            Text{ TextStyleId::Section, L"Scope" }
        ) };

        StackPanel& m_cppScopeGroup{ m_cppCodePage.options().add<StackPanel>(
            Orientation::Vertical
        ) };

        Tab& m_designTab{ m_tabbedBox.strip().addTab(
            L"Design",
            Page{m_designScrollBox}
        )
            .select()
        };

        Tab& m_cppCodeTab{ m_tabbedBox.strip().addTab(
            Text{ TextStyleId::Code, L"C++" },
            Page{ m_cppCodePage }
        ) };

        Tab& m_claFiTab{ m_tabbedBox.strip().addTab(
            Text{ TextStyleId::Code, L"ClaFi" },
            Page{ m_claFiPage }
        ) };

        Tab& m_xmlTab{ m_tabbedBox.strip().addTab(
            Text{ TextStyleId::Code, L"XML" },
            Page{ m_xmlPage }
        ) };

        Tab& m_jsonTab{ m_tabbedBox.strip().addTab(
            Text{ TextStyleId::Code, L"JSON" },
            Page{ m_jsonPage }
        ) };

        Controls::Expander& m_paletteExpander{ m_designView.add<Controls::Expander>(
            VerticalAlign::Top,
            HeaderText{ TextStyleId::Section, L"Palette"}
        ) };

        StackPanel& m_paleteExpanderBody = m_paletteExpander.createBody<StackPanel>(
            Orientation::HorizontalWrap,
            Padding{ 8.0f, 8.0f }
        );

        StackPanel& m_paletteColumn1{ m_paleteExpanderBody.add<StackPanel>(
            Orientation::Vertical
        ) };

        MinSize m_laneSize{ 0.0f, 40.04 };

        Panel& m_paletteAnchorLane = m_paletteColumn1.add<Panel>(
            m_laneSize
        );

        TabTo m_labelsMargin{ 70.0f };

        const Text k_labelPush{};
        const Text k_labelPop{};

        // just because the slider (it's panel base) does not respect VerticalTextAnchor yet.
        // When it'll do, this label can be eliminated and replaced with uncommenting *** below
        Label& m_anchorLabel{ m_paletteAnchorLane.createLeftBar<Label>(
            VerticalTextAnchor::Center,
            Text{ k_labelPush, L"Anchor:", m_labelsMargin, k_labelPop })
        };

        ColorSlider& m_anchorSlider{ m_paletteAnchorLane.createBody<ColorSlider>(
            // *** Text{ k_subHeaderFont, L"Anchor color:" },
            // *** TextPlacement::Left,
            // *** VerticalTextAnchor::Center,
            ColorAttribute::Hue,
            ScrollButtons::No,
            Spacing{ 0.0f, 4.0f },
            //MaxSize{ 600.0f, k_maxFloat },
            //MinSize{ 290.0f, 0.0f },
            HorizontalAlign::Fill
        ) };

        Panel& m_harmonyPanel{ m_paletteColumn1.add<Panel>(
            Text{ k_labelPush, L"Harmony:", m_labelsMargin, k_labelPop },
            TextPlacement::Top,
            VerticalAlign::Top,
            Padding{ 0.0f, 4.0f },
            Spacing{ 8.0f }
        ) };

        StackPanel& m_harmonyStack{ m_harmonyPanel.createBody<StackPanel>(
            Orientation::HorizontalWrap,
            Interactivity::ActiveContainer,
            Padding{ 12.0f, 4.0f },
            Spacing{ 4.0f },
            Events{
                [](CanFocusItemEvent& event) {
                    event.canFocus = event.item.tag().value == k_harmonyTag;
                }
            }
        ) };

        // The four hues the selected harmony offers an icon, shown and nothing more: nothing
        // names one of them yet, and the label carries the whole row - the marks are written into
        // its text beside the caption rather than standing as controls of their own.
        Label& m_iconHuesLabel{ m_paletteColumn1.add<Label>(
            VerticalTextAnchor::Center,
            Padding{ 0.0f, 4.0f },
            Text{ k_labelPush, L"Icon hues:", m_labelsMargin, k_labelPop }
        ) };

        //Controls::Separator& m_paletteSep1{ m_paleteExpanderBody.add<Controls::Separator>() };

        Spacer& m_spacer1{ m_paleteExpanderBody.add<Spacer>(8.0f) };

        Controls::Expander& m_transformExpander{ m_designView.add<Controls::Expander>(
            VerticalAlign::Top,
            HeaderText{ TextStyleId::Section, L"Transform" }
        ) };

        StackPanel& m_transformExpanderBody{ m_transformExpander.createBody<StackPanel>(
            Orientation::Vertical,
            Padding{ 8.0f, 8.0f }
        ) };

        Panel& m_darkModeFloorLane{ m_transformExpanderBody.add<Panel>(
            m_laneSize,
            Spacing{ 8.0f, 0.0f }
        ) };

        Label& m_darkModeFloorLabel{ m_darkModeFloorLane.createLeftBar<Label>(
            VerticalTextAnchor::Center,
            Text{ k_labelPush, L"Dark mode floor:", k_labelPop }
        ) };

        FloorSlider& m_darkModeFloorSlider{ m_darkModeFloorLane.createBody<FloorSlider>(
            ScrollButtons::No,
            Spacing{ 0.0f, 4.0f },
            HorizontalAlign::Fill
        ) };

        ThemeRule m_satColumnColor{ UiElement::Section, &BakedElement::surface };

        // ---- reusable cell bundles -------------------------------------------
        // Declared before the grid, because the grid's argument list names them. Rows
        // take them by reference, so they are shared, never copied.

        const CellSet m_saturationCells{
            operationCells(ColumnTag::SaturationAction, ColumnTag::SaturationAmount, ComboboxTarget::Saturation) };

        const CellSet m_elevationCells{
            operationCells(ColumnTag::ElevationOperation, ColumnTag::ElevationAmount, ComboboxTarget::Elevation) };

        const CellSet m_elementCells{
            CellWith<HueRuleControl>{ Tag{ ColumnTag::Hue },
                themeMetrics().listItem.padding,
                VerticalAlign::Fill
            },
            m_saturationCells,
            m_elevationCells };

        // ---- the whole grid, start to finish ---------------------------------
        // The design is the grid's argument list, not a second object applied to it.
        // A column is named by its Tag, and those Tags have to be unique within one
        // grid. Cells resolve against them as the grid is built, and columnByTag
        // answers for them afterwards.

        Grid& m_grid{ m_designView.add<Grid>(
            themeMetrics().page,
            UiElement::Section,
            Grids::GridLines::Horizontal,

            Columns{
                Column{ Tag{ ColumnTag::StaticName },
                    Text{ k_labelPush, L"", k_labelPop },
                    ShowInHeader::Yes,
                    m_satColumnColor,
                    Column{ Tag{ ColumnTag::Name },
                        Text{ k_labelPush, L"Element", k_labelPop },
                        ShowInHeader::No,
                        m_satColumnColor },
                    Column{ Tag{ ColumnTag::State },
                        Text{ k_labelPush, L"State", k_labelPop },
                        ShowInHeader::No,
                        m_satColumnColor }
                },
                Column{ Tag{ ColumnTag::Hue },
                    Text{ k_labelPush, InTextIcon{ 16.0f, 16.0f, Icons::HueIcon::paint }, L" Hue", k_labelPop },
                    CellHighlightMode::Control
                },
                Column{ Tag{ ColumnTag::SaturationGroup },
                    Text{ k_labelPush, InTextIcon{ 16.0f, 16.0f, Icons::SaturationIcon::paint }, L" Saturation", k_labelPop },
                    ColumnWidthMode::Fill,
                    m_satColumnColor,
                    Column{ Tag{ ColumnTag::SaturationAction },
                        Text{ k_labelPush, L"Operation", k_labelPop },
                        ShowInHeader::No,
                        ColumnWidthMode::Fixed, k_operationColumnWidth,
                        m_satColumnColor },
                    Column{ Tag{ ColumnTag::SaturationAmount },
                        Text{ k_labelPush, L"Value", k_labelPop },
                        ColumnWidthMode::Fill,
                        ShowInHeader::No,
                        CellHighlightMode::Control,
                        m_satColumnColor }
                },
                Column{ Tag{ ColumnTag::ElevationGroup },
                    Text{ k_labelPush, InTextIcon{ 16.0f, 16.0f, Icons::LuminosityIcon::paint }, L" Elevation", k_labelPop },
                    ColumnWidthMode::Fill,
                    Column{ Tag{ ColumnTag::ElevationOperation },
                        Text{ k_labelPush, L"Operation", k_labelPop },
                        ColumnWidthMode::Fixed, k_operationColumnWidth,
                        ShowInHeader::No },
                    Column{ Tag{ ColumnTag::ElevationAmount },
                        Text{ k_labelPush, L"Value", k_labelPop },
                        ColumnWidthMode::Fill,
                        ShowInHeader::No }
                },
                Column{ Tag{ ColumnTag::ElevationFlip },
                    Text{ k_labelPush, InTextIcon{ 16.0f, 16.0f, Icons::LuminosityIcon::paint }, L" Flip", k_labelPop },
                    TooltipText{ L"This element stands on the far side of the theme, so everything drawn from it rises the other way" },
                    // Grid mode, which is the default: the cell's own frame carries the highlight.
                    // Control mode writes the cell's hovered, selected and focused factors onto the
                    // hosted control, and a check mark IS the selected factor - the mark would then
                    // say which cell the grid is on rather than what the theme states.
                    }
            },

            Header{},

            // ONE KIND OF THING PER SECTION, AND INSIDE A SECTION THE NESTING ORDER. An element
            // is listed after the one it is painted on, so reading down a section follows the
            // chain a rule is applied along. Every name cell is indented by the same one step:
            // the order carries the nesting, and a column stepped in by depth as well pushes the
            // names too far apart to read against each other.
            //
            // The sections are the only thing stated here. Which rows an element gets, and in
            // what order, is k_uiElements' answer - so a rule the element does not paint cannot
            // reach the grid by being listed at this call site.
            Rows{
                // THE THREE WINDOW ROOTS. Each opens a window of its own and states the pair
                // that window establishes: the surface it is filled with and the ink everything
                // inside it starts from. What stands behind such a window is not the theme's to
                // know, so each names its Surface hue outright and sets its saturation and
                // elevation - see statesAbsoluteSurface. FormBase::initialMetrics draws the same
                // line: the form takes primaryWindow, the other two take secondaryWindow.
                Grids::Dt::Expander{
                    Header{ Text{ TextStyleId::Section, L"Window roots" } },
                    sectionFold(),
                    elementGroup(UiElement::Form),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::Menu),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::Tooltip)
                },
                // What is painted inside a form, and the line each surface carries. A page on the
                // form with the line its tabs stand on, a section on the page, a header on the
                // section with the divider that parts one band of it from the next, a bar along an
                // edge of the window, the title strip across the top of the form itself, and the
                // grid a lattice is drawn inside, with its rows and the lattice's line under it.
                Grids::Dt::Expander{
                    Header{ Text{ TextStyleId::Section, L"Surfaces" } },
                    sectionFold(),
                    elementGroup(UiElement::Page),
                    Grids::Dt::Divider{},
                    staticRow(UiElement::TabLine),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::Section),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::Header),
                    Grids::Dt::Divider{},
                    staticRow(UiElement::Divider),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::Bar),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::FormTitle),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::Grid),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::GridRow),
                    Grids::Dt::Divider{},
                    staticRow(UiElement::GridLine)
                },
                // What answers the pointer.
                Grids::Dt::Expander{
                    Header{ Text{ TextStyleId::Section, L"Controls" } },
                    sectionFold(),
                    elementGroup(UiElement::Button),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::ScrollButton),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::ScrollThumb)
                },
                // Where the user is and what the user has picked. Accent is the one ink for the
                // focus ring and the on state of every mark, and Spot is the emphasis that stands
                // apart from the interface rather than answering to it. The two sets under them
                // are what a mark and a run of selected text wear, and both read the focus.
                Grids::Dt::Expander{
                    Header{ Text{ TextStyleId::Section, L"Focus and Selection" } },
                    sectionFold(),
                    staticRow(UiElement::Accent),
                    Grids::Dt::Divider{},
                    staticRow(UiElement::Spot),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::InactiveIndicator),
                    Grids::Dt::Divider{},
                    elementGroup(UiElement::SelectedText)
                }
            }
        ) };
    };


    //----------------------------------------------------------------------------


    template<typename ...Args>
    CodePage::CodePage(const CreateParams& params, Args&& ...args)
        :
        Panel{ params, Spacing{ 8.0f, 0.0f }, std::forward<Args>(args)... }
    {
    }

    template<typename ...Args>
    ThemePage::ThemePage(const CreateParams& params, Args&& ...args)
        :
        BasePage{ params, std::forward<Args>(args)... },
        m_onSelectPaletteMap{ [this]() { paletteMapChanged(); } },
        m_onHueRuleChanged{ [this]() { hueRuleChanged(); } }
    {
        // The second half of the button IS Save as. A menu of one was a stop on the way to it,
        // naming what the arrow already means and asking for a second press to get there; behind
        // the strip the command names itself in the strip's tooltip and runs off one press.
        m_saveButton.dropdownAction(StdActions::saveAs);

        onGetActionState([this](GetActionStateEvent& event) {
            // Claiming says this page is what the command acts on. Neither is ever refused: a page
            // with no file of its own still has somewhere to put the work, and Save asks where
            // rather than saying no.
            if (&event.action == &StdActions::save || &event.action == &StdActions::saveAs)
                event.claim({});
            });

        onActionClick([this](ActionClickEvent& event) {
            const bool isSave = &event.action == &StdActions::save;
            if (!isSave && &event.action != &StdActions::saveAs)
                return;

            // Under whatever showed the command. The strip is not a thing of its own for this -
            // it is half of the button, and a question hanging off half of a button reads as
            // being about that half - so the whole button carries it, which is also where the
            // shortcut, shown by nothing, puts it. A message about what the command did stands
            // there for the same reason.
            Control& shownBy = event.presenter && !m_saveButton.containsNested(event.presenter)
                ? *event.presenter
                : m_saveButton;

            // SAVE ON A PAGE WITH NO FILE OF ITS OWN IS SAVE AS. The work has somewhere to go and
            // the press said to put it there, so the command that can reach the disk is the one
            // that runs. This is the toolbar's half of what canLeavePage already does with the
            // Save it offers.
            if (isSave && canSaveEdits())
            {
                saveTheme();
                // A write leaves nothing on screen to say it happened - the page looks exactly as
                // it did - so the command says so itself, beside the button that ran it.
                ContextMessage::show(shownBy, messageText(MessageIcon::Ok, L"Theme saved"));
                return;
            }

            // SAVE AS SAYS NOTHING HERE. It takes the tab to the file it wrote, and the page it
            // arrives at is the answer: a message hung off this button would be raised about a
            // control the rebuild is about to destroy - see saveThemeAs.
            saveThemeAs(shownBy);
            });

        // The palette's colours are the most saturated thing on the page, and judging a theme's
        // quieter ones beside them is what the warning is about.
        m_paletteExpander.header().button().connectEvent([this](GetTooltipEvent& event) {
            if (!m_paletteExpander.header().expanded())
            {
                event.text << L"Show palette";
                return;
            }
            event.text << L"Hide palette, to ensure vibrant colors won't skew your perception";
            });

        addFold(m_paletteExpander.header());
        addFold(m_transformExpander.header());
        connectFoldMenu();

        for (std::size_t i = 0ull; i != m_harmonySelector.count(); ++i)
            m_harmonyStack.add<ColorHarmonyItem>(
                ColorHarmonyItemInitializer{ m_harmonySelector.harmony(i), m_onSelectPaletteMap },
                Tag{ k_harmonyTag }
            );

        // Bound here rather than through a constructor property: the slider takes a bare hue, and
        // an argument list has no way to say which of the theme's floats a float* is.
        m_anchorSlider.setEditedHue(editColors().anchorHue);

        m_anchorSlider.connectEvent([this](SliderChangeEvent&) {
            anchorChanged();
            });

        m_darkModeFloorSlider.setMaxPosition(k_maxDarkModeFloor);
        // The page's hue and saturation: the page is the surface at elevation 0.
        m_darkModeFloorSlider.bind([this]() {
            return elementColor(UiElement::Page);
            });
        m_darkModeFloorSlider.connectEvent([this](SliderChangeEvent&) {
            darkModeFloorChanged();
            });
        m_darkModeFloorLabel.connectEvent(this, &ThemePage::darkModeFloorText);

        m_cppCodePage.box().setLanguage(Syntax::Languages::cpp);
        m_claFiPage.box().setLanguage(Syntax::Languages::claFi);
        m_xmlPage.box().setLanguage(Syntax::Languages::xml);
        m_jsonPage.box().setLanguage(Syntax::Languages::json);

        // Every page presents the same two commands. A presenter takes the action as a property
        // and takes its words, its mark and its click from it, so nothing here says what a button
        // reads or which choice it stands for.
        for (CodePage* page : { &m_cppCodePage, &m_claFiPage, &m_xmlPage, &m_jsonPage })
            for (Action* action : { &CodeOptions::onlyDifferences, &CodeOptions::full })
            {
                RadioButton& button = page->contentGroup().add<RadioButton>(*action);
                button.connectEvent(this, &ThemePage::codeOptionPicked);
            }

        for (Action* action : {
            &CodeOptions::asClassDeclarations,
            &CodeOptions::asClassMethod,
            &CodeOptions::asOutsideClass })
        {
            RadioButton& button = m_cppScopeGroup.add<RadioButton>(*action);
            button.connectEvent(this, &ThemePage::codeOptionPicked);
        }

        m_harmonyPanel.connectEvent([this](GetTextEvent& event) {
            event.text << L" "
                << (m_harmonyStack.currentItem()
                    ? static_cast<ColorHarmonyItem*>(m_harmonyStack.currentItem())->harmony().name()
                    : L"Not selected" // add textInk(InkGrade::Muted) and subBody
                    );
            });

        m_iconHuesLabel.connectEvent(this, &ThemePage::iconHuesText);

        m_harmonyStack.connectEvent([this](CurrentItemChangeEvent&) {
                harmonyChanged();
            });

        m_tabbedBox.pageControl().connectEvent([this](CurrentItemChangeEvent&)
            {
                generateCode();
            });
    }

}
