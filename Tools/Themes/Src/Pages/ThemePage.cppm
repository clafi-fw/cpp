export module ThisApp.ThemePage;

import ThisApp.WithPreview;
import ThisApp.CodeOptions;
import ThisApp.Consts;
import ThisApp.DesignPage;
import ThisApp.ElementPage;
import ThisApp.FloorSlider;
import ThisApp.HueRuleControl;
import ThisApp.RuleSlider;
import ThisApp.ThemeToCppCode;
import ThisApp.Utils;
import ThisApp.ValueRuleControl;
import ThisApp.Palette.Controls;

// Icons
import ClaFi.Icons.SaveIcon;
import ClaFi.Icons.HueIcon;
import ClaFi.Icons.SaturationIcon;
import ClaFi.Icons.LuminosityIcon;

import ClaFi.Documents.Page;

import ClaFi.Browser.Control;

import ClaFi.Controls.Base.StackPanelBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.ColorSlider;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Label;
import ClaFi.Controls.PageControl;
import ClaFi.Controls.Panel;
import ClaFi.Controls.RadioButton;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Divider;
import ClaFi.Controls.Slider;
import ClaFi.Controls.Spacer;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TabStrip;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.TextItems;
import ClaFi.Controls.Base.SliderBase;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Core.DomEngine;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine_Document;

import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;

import ClaFi.Core.Syntax.Languages;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
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

    // One theme, edited in place: the design page, the four code views and the edit history.
    export class ThemePage : public WithPreview<Documents::DocumentPage>, private IEditHistory
    {
    public:
        template<typename... Args>
        explicit ThemePage(const CreateParams&, Args&&...);
    public:
        void restoreViewState() override;
        [[nodiscard]] bool canSaveEdits() const override { return !isBuiltIn(); }
        // The theme this page's tab stands for - what its icon is drawn from, edits and all.
        [[nodiscard]] const AppTheme& tabTheme() const { return m_editTheme; }
    protected:
        [[nodiscard]] bool readSavedDocument(Dom::DomNodeBase& into) const override;
        const AppTheme* selectedTheme() override { return &m_editTheme; }
        void visibilityChanged() override;
        // The pigment grid reads every element in the preview's mode.
        void previewColorModeChanged() override;
    private:
        using Base = WithPreview<Documents::DocumentPage>;
        using ViewTabs = std::array<Tab*, static_cast<std::size_t>(ThemeView::Count)>;
        using ThemeHistory = StateHistory<AppTheme, DesignPlace>;
        // The pigment grid's columns, as their tags name them.
        enum class PigmentColumn : TagValue
        {
            Name,
            Hue,
            Saturation,
            Elevation
        };
    private:
        ThemeColors& editColors() { return m_editTheme.colors; }
        //
        void anchorChanged(EditPhase);
        void harmonyChanged();
        void paletteMapChanged();
        void mapHuesToColors();
        // The pigments of the harmony the theme names, written as marks after the row's caption, in
        // pigment order. They are derived from the anchor and the harmony kind alone, so nothing
        // here reads or writes the palette.
        void otherPigmentsText(GetTextEvent&);
        // A row for a pigment the theme states itself: its name and an element page's editors.
        [[nodiscard]] Row pigmentRow(UiElement) const;
        // The name, and under it in gray what the pigment is used for.
        void pigmentNameCell(Grids::GetCellTextEvent&) const;
        void bindPigmentEditors(); // onto the rules they edit, which live in m_editTheme
        // What a change to one thing of the palette is called.
        [[nodiscard]] static Text changeStepName(std::wstring_view what);
        // What a change to one column of a pigment is called.
        [[nodiscard]] static Text pigmentStepName(UiElement, std::wstring_view column);
        [[nodiscard]] Grids::Column& pigmentColumn(PigmentColumn);
        // What a pigment's value is about to change: the surface its element stands on.
        [[nodiscard]] RuleBase pigmentRuleBase(UiElement, RuleChannel);
        void pigmentChanged(EditPhase, const Text& what);
        void darkModeFloorChanged(EditPhase);
        //
        // Brings every control up on m_editTheme as it stands - what a restore and an undo share.
        void showEditTheme();
        // Every edit ends here: the tab node takes the theme, and the history takes the step and
        // where it was made - the picked page, and for a grid edit the cells it was made on.
        void edited(EditPhase, const Text& what, const RuleSelection& at = {});
        // The page's history is the theme's: what the page's Undo and Redo walk.
        [[nodiscard]] std::size_t undoDepth() const override { return m_history.undoDepth(); }
        [[nodiscard]] std::size_t redoDepth() const override { return m_history.redoDepth(); }
        void writeUndoStep(std::size_t i, Text&) const override;
        void writeRedoStep(std::size_t i, Text&) const override;
        void undo(std::size_t steps) override;
        void redo(std::size_t steps) override;
        // Puts a state of the history on as the theme - on screen and in the tab node - and the
        // selection back where the step was made.
        void showHistoryState(const ThemeHistory::Landing&);
        void storeViewState() const;
        void storeView() const;
        void restoreView();
        void designPagePicked(); // sends the pick to the browser as this tab's anchor
        void showAnchor(const Browser::ShowAnchorEvent&);
        // A rule was added on the Design page, moved or taken away.
        void rulesChanged(EditPhase, const Text& what, const RuleSelection& at);
        // Asks whether the user is the one this button is for, and answers whether they said so.
        [[nodiscard]] bool confirmWritingToSource(Control& initiator) const;
        // Whether this page stands for one of the built-ins. They are answered from the
        // compiled-in colours rather than from a file, so Save has nothing to write over and
        // falls back to Save as.
        [[nodiscard]] bool isBuiltIn() const;
        //
        [[nodiscard]] Hsl elementColor(OptionalUiElement);
        // The ink an element's text starts from: the same nesting as elementColor, walked
        // through text rules instead of surface ones.
        [[nodiscard]] Hsl elementTextColor(OptionalUiElement, bool* hueNamed = nullptr);
        // The resting rules on one channel of an element; true where one of them names a hue.
        bool applyRestingRules(Hsl&, UiElement, PaintChannel);
        // The ink of the colour mode before any rule has touched it.
        [[nodiscard]] Hsl bareInk();
        // Black in the hue of an element's resting stroke, where its shadow starts.
        [[nodiscard]] Hsl bareShadow(OptionalUiElement);
        // What a value of a rule is about to change.
        [[nodiscard]] RuleBase elementRuleBase(OptionalUiElement, const ColorRule&, RuleChannel);
        // A rule on a base, applied on every channel but the one its value ramp draws.
        [[nodiscard]] RuleBase ruleBaseOn(Hsl base, const ColorEffect&, RuleChannel, ColorMode,
            float luminosityFloor);
        //
        void generateCode();
        // The theme as one document, written into a box in the format's own spelling.
        void writeThemeTo(const Dom::FileFormatBase&, TextBox&);
        // The document a box shows, replacing the one it holds.
        void showCode(const std::wstring& code, TextBox&);
        // A pick was made on this page. The action has already moved the answer and refreshed
        // every presenter of it; what is left is the document this page is showing.
        void codeOptionPicked(ClickEvent&);
        // Writes this theme as the framework's own once the user has said they are a ClaFi
        // developer, and says beside the button what came of it.
        void writeToSource(ClickEvent&);
    private:
        static constexpr TagValue k_harmonyTag = 2ull;
        // The floor slider's end. Useful floors sit far below it; at 1 a surface has no room.
        static constexpr float k_maxDarkModeFloor = 0.5f;
    private:
        OnSelectPaletteMap m_onSelectPaletteMap;
        OnHueRuleChanged m_onPigmentChanged; // what the pigment editors call, held by address

        AppTheme m_editTheme{};
        ThemeHistory m_history{}; // the states m_editTheme has been through
        // What the palette's edits are called in the history, held rather than spelled per edit.
        const Text m_anchorStep{ changeStepName(L"Anchor hue") };
        const Text m_floorStep{ changeStepName(L"Dark mode floor") };
        const Text m_paletteMapStep{ changeStepName(L"Palette map") };
        // Set while showEditTheme brings the controls up on a theme that was read or undone to.
        // What it calls to do that are the same handlers a user edit calls, and each of them ends
        // by writing the theme back and recording a step - see edited.
        bool m_restoring{ false };
        // Set while showAnchor picks a design page, a pick the tab already stands on.
        bool m_showingAnchor{ false };
        // Where Write to source writes - empty where the tree it was compiled from is not there.
        const std::filesystem::path m_builtInColorsFile{ builtInColorsFile() };

        ColorHarmonySelector m_harmonySelector{ editColors().anchorHue };

        TabbedBox& m_tabbedBox{ createBody<TabbedBox>(
            TabsOrientation::HorizontalBottom
        ) };

        DesignPage& m_designPage{ m_tabbedBox.pageControl().add<DesignPage>() };

        CodePage& m_cppCodePage{ m_tabbedBox.pageControl().add<CodePage>() };
        CodePage& m_claFiPage{ m_tabbedBox.pageControl().add<CodePage>() };
        CodePage& m_xmlPage{ m_tabbedBox.pageControl().add<CodePage>() };
        CodePage& m_jsonPage{ m_tabbedBox.pageControl().add<CodePage>() };

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

        Controls::Divider& m_writeSourceDivider{ m_cppCodePage.options().add<Controls::Divider>(
            Thickness::Heavy,
            Padding{ 0.0f, 4.0f }
        ) };

        Button& m_writeSourceButton{ m_cppCodePage.options().add<Button>(
            L"Write to source"
        ) };

        Tab& m_designTab{ m_tabbedBox.strip().addTab(
            L"Design",
            Page{ m_designPage }
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

        // One tab per ThemeView, in its order.
        const ViewTabs m_viewTabs{
            &m_designTab,
            &m_cppCodeTab,
            &m_claFiTab,
            &m_xmlTab,
            &m_jsonTab
        };

        StackPanel& m_paletteBody{ m_designPage.paletteView().add<StackPanel>(
            Orientation::HorizontalWrap,
            Padding{ 8.0f, 8.0f }
        ) };

        StackPanel& m_paletteColumn1{ m_paletteBody.add<StackPanel>(
            Orientation::Vertical
        ) };

        MinSize m_laneSize{ 0.0f, 40.04 };

        Panel& m_paletteAnchorLane = m_paletteColumn1.add<Panel>(
            m_laneSize
        );

        TabTo m_labelsMargin{ 120.0f };

        const Text k_labelPush{};
        const Text k_labelPop{};

        // just because the slider (it's panel base) does not respect VerticalTextAnchor yet.
        // When it'll do, this label can be eliminated and replaced with uncommenting *** below
        Label& m_anchorLabel{ m_paletteAnchorLane.createLeftBar<Label>(
            VerticalTextAnchor::Center,
            Text{ k_labelPush, L"Anchor hue:", m_labelsMargin, k_labelPop })
        };

        ColorSlider& m_anchorSlider{ m_paletteAnchorLane.createBody<ColorSlider>(
            // *** Text{ k_subHeaderFont, L"Anchor hue:" },
            // *** TextPlacement::Left,
            // *** VerticalTextAnchor::Center,
            HslChannel::Hue,
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

        Spacer& m_pigmentsSpacer{ m_paletteColumn1.add<Spacer>(16.0f) };

        // Accent and Spot: the pigments a theme states itself rather than takes from its harmony.
        Grid& m_pigmentGrid{ m_paletteColumn1.add<Grid>(
            themeMetrics().page,
            UiElement::Section,
            Grids::GridLines::Horizontal,
            HorizontalAlign::Fill,
            Columns{
                Column{ Tag{ PigmentColumn::Name },
                    Text{ L"Pigment" },
                    ColumnWidthMode::Fill
                },
                Column{ Tag{ PigmentColumn::Hue },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::HueIcon::paint }, L" Hue" }
                },
                Column{ Tag{ PigmentColumn::Saturation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::SaturationIcon::paint }, L" Saturation" }
                },
                Column{ Tag{ PigmentColumn::Elevation },
                    Text{ InTextIcon{ 16.0f, 16.0f, Icons::LuminosityIcon::paint }, L" Elevation" }
                }
            },
            Header{},
            pigmentRow(UiElement::Accent),
            pigmentRow(UiElement::Spot)
        ) };

        // The selected harmony's four pigments, drawn as marks in the label's own text.
        Label& m_otherPigmentsLabel{ m_paletteColumn1.add<Label>(
            VerticalTextAnchor::Center,
            Padding{ 0.0f, 4.0f },
            Text{ k_labelPush, L"Other pigments:", m_labelsMargin, k_labelPop },
            HintText{ L"Pigments safe to use in small doses" }
        ) };

        Spacer& m_floorSpacer{ m_paletteColumn1.add<Spacer>(16.0f) };

        Panel& m_darkModeFloorLane{ m_paletteColumn1.add<Panel>(
            m_laneSize
        ) };

        Label& m_darkModeFloorLabel{ m_darkModeFloorLane.createLeftBar<Label>(
            VerticalTextAnchor::Center,
            Text{ k_labelPush, L"Dark mode floor:", m_labelsMargin, k_labelPop }
        ) };

        FloorSlider& m_darkModeFloorSlider{ m_darkModeFloorLane.createBody<FloorSlider>(
            ScrollButtons::No,
            Spacing{ 0.0f, 4.0f },
            HorizontalAlign::Fill
        ) };

        //Controls::Separator& m_paletteSep1{ m_paletteBody.add<Controls::Separator>() };

        Spacer& m_spacer1{ m_paletteBody.add<Spacer>(8.0f) };
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
        Base{ params, std::forward<Args>(args)... },
        m_onSelectPaletteMap{ [this]() { paletteMapChanged(); } },
        m_onPigmentChanged{ [this](const EditPhase phase, const Text& what) {
            pigmentChanged(phase, what);
        } }
    {
        // The page's Undo and Redo walk the theme's history, wherever the focus stands.
        setEditHistory(*this);

        for (std::size_t i = 0ull; i != m_harmonySelector.count(); ++i)
            m_harmonyStack.add<ColorHarmonyItem>(
                ColorHarmonyItemInitializer{ m_harmonySelector.harmony(i), m_onSelectPaletteMap },
                Tag{ k_harmonyTag }
            );

        // Bound here rather than through a constructor property: the slider takes a bare hue, and
        // an argument list has no way to say which of the theme's floats a float* is.
        m_anchorSlider.setEditedHue(editColors().anchorHue);

        m_anchorSlider.connectEvent([this](SliderChangeEvent& event) {
            anchorChanged(event.slider.editPhase());
            });
        m_anchorSlider.onSettle([this](SliderSettleEvent&) {
            edited(EditPhase::Settled, m_anchorStep);
            });

        m_darkModeFloorSlider.setMaxPosition(k_maxDarkModeFloor);
        // The page's hue and saturation: the page is the surface at elevation 0.
        m_darkModeFloorSlider.bind([this]() {
            return elementColor(UiElement::Page);
            });
        m_darkModeFloorSlider.connectEvent([this](SliderChangeEvent& event) {
            darkModeFloorChanged(event.slider.editPhase());
            });
        m_darkModeFloorSlider.onSettle([this](SliderSettleEvent&) {
            edited(EditPhase::Settled, m_floorStep);
            });

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

        for (Action* action : { &CodeOptions::asClassMethod, &CodeOptions::asOutsideClass })
        {
            RadioButton& button = m_cppScopeGroup.add<RadioButton>(*action);
            button.connectEvent(this, &ThemePage::codeOptionPicked);
        }

        m_writeSourceButton.onGetState([this](GetStateEvent& event) {
            event.state.enabled = !m_builtInColorsFile.empty();
            });
        m_writeSourceButton.connectEvent(this, &ThemePage::writeToSource);

        m_harmonyPanel.connectEvent([this](GetTextEvent& event) {
            event.text << L" "
                << (m_harmonyStack.currentItem()
                    ? static_cast<ColorHarmonyItem*>(m_harmonyStack.currentItem())->harmony().name()
                    : L"Not selected" // add textInk(InkGrade::Muted) and subBody
                    );
            });

        m_otherPigmentsLabel.connectEvent(this, &ThemePage::otherPigmentsText);

        m_harmonyStack.connectEvent([this](CurrentItemChangeEvent&) {
                harmonyChanged();
            });

        m_tabbedBox.pageControl().connectEvent([this](CurrentItemChangeEvent&)
            {
                generateCode();
                storeView();
            });

        m_designPage.onPagePick([this](CurrentItemChangeEvent&) {
            designPagePicked();
        });

        onShowAnchor([this](Browser::ShowAnchorEvent& event) {
            showAnchor(event);
        });

        m_designPage.bind(editColors(),
            [this](const OptionalUiElement element, const ColorRule& rule,
                const RuleChannel channel) {
                return elementRuleBase(element, rule, channel);
            },
            [this](const EditPhase phase, const Text& what, const RuleSelection& at) {
                rulesChanged(phase, what, at);
            }
        );
        bindPigmentEditors();
    }

}
