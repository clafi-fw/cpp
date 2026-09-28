module ThisApp.ThemePage;

import ThisApp.BasePage;
import ThisApp.CodeOptions;
import ThisApp.Consts;
import ThisApp.HueRuleControl;
import ThisApp.RuleSlider;
import ThisApp.Palette.Controls;
import ThisApp.ThemeToCppCode;
import ThisApp.Utils;
import ThisApp.ValueRuleControl;

import ClaFi.Application.ThemesManager;
import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.PromptDialog;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Slider;
import ClaFi.Controls.TextBox;

import ClaFi.Dom;
import ClaFi.Dom.Formats.ClaFi;
import ClaFi.Dom.Formats.Json;
import ClaFi.Dom.Formats.Xml;

import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Url;
import ClaFi.Browser.Control;
import ClaFi.Browser.Consts;
import ClaFi.Browser.Settings;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ThisApp
{
    namespace
    {
        // Holds a flag for as long as it is in scope. Set and cleared by two statements instead,
        // anything that threw between them would leave the flag standing - and a page that cannot
        // store is silent about it: every later edit would reach the screen and go no further.
        class ScopedFlag
        {
        public:
            explicit ScopedFlag(bool& flag)
                :
                m_flag{ flag }
            {
                m_flag = true;
            }
            // A guard whose whole business is holding a scope must not be copied out of it: the
            // first destructor would clear the flag and the second would write through a
            // reference to a scope that has ended.
            ScopedFlag(const ScopedFlag&) = delete;
            ScopedFlag& operator=(const ScopedFlag&) = delete;
            ~ScopedFlag()
            {
                m_flag = false;
            }
        private:
            bool& m_flag;
        };

        // What a pigment the theme states itself is used for, the second line of its name.
        [[nodiscard]] std::wstring_view pigmentUse(const UiElement element)
        {
            switch (element)
            {
                case UiElement::Accent:
                    return L"Focus, emphasis, on state";
                case UiElement::Spot:
                    return L"Brand, highlights, tooltips";
                default:
                    break;
            }
            unreachable("a pigment row names an element that is not a pigment");
        }
    }

    void ThemePage::restoreViewState()
    {
        const ScopedFlag restoring{ m_restoring };
        BasePage::restoreViewState();
        // Application wide, so they are read from the root section rather than from the tab's.
        CodeOptions::restore(settings().appConfig());
        auto& themeNode = (tabConfig() / k_themeDataAttrName).as<Dom::Value<AppTheme>>();
        UserTheme::loadTheme(themeNode, m_editTheme);

        // we're passing propagateChanges = false to trackingValueChanged,
        // so it doesn't trigger the thumb tooltip unnecessary
        m_anchorSlider.trackingValueChanged(false);

        m_darkModeFloorSlider.setPosition(editColors().darkModeFloor, false);

        // THE ANCHOR FIRST, THEN THE MAP, THEN THE PALETTE. Each step below is what the next one
        // reads, so the order is the whole of it and anchorChanged() cannot stand in for the pair:
        // it derives the palette, which is the last step, not the first.
        //
        // The anchor decides every colour a harmony is built from - ColorHarmony::anchorChanged
        // fills m_colors out of it - and the search below matches on those colours.
        m_harmonySelector.anchorChanged();
        m_harmonyStack.invalidate();

        for (ColorHarmonyItem& item : m_harmonyStack.controlsAs<ColorHarmonyItem>())
        {
            ColorHarmony& harmony = item.harmony();
            if (harmony.kind() == editColors().harmonyKind)
            {
                PaletteMap* map = harmony.findMapByHueDegrees({
                    hueDegreeOf(editColors().paletteHues[0]),
                    hueDegreeOf(editColors().paletteHues[1]),
                    hueDegreeOf(editColors().paletteHues[2])
                    });
                if (map)
                    harmony.selectMap(*map);
                m_harmonyStack.setCurrentItem(item);
                break;
            }
        }

        // THE PALETTE IS WHAT THE THEME STATES, so nothing derives it here. mapHuesToColors fills
        // the hues in from the selected map, and at restore that is one of two things: the map the
        // search above just matched, in which case it writes back the hues it was given, or the
        // harmony's first map where nothing matched, in which case it writes a palette nobody
        // chose over the one the file states. Never useful, and half the time a silent edit.
        //
        // What is wanted from it here is the invalidation at its end.
        //
        // TODO: a theme whose hues match no map of its harmony leaves the map selector standing on
        // one that does not describe them - which is every stock theme, the defaults included:
        // Monochromatic carries the single map {0,0,0} against defaults of 210/210/260, and the
        // built-in Dark and Light state 207 against an anchor of 210. Should the selector show no
        // map at all in that case, or should the theme carry the map it was built from rather than
        // the hues that came out of it?
        invalidatePreview();
        m_darkModeFloorSlider.invalidate();
        bindPigmentEditors();
        m_designPage.rebuildRules();
        restoreView();
    }

    bool ThemePage::hasUnsavedEdits() const
    {
        if (m_savedUnderAnotherName)
            return false;

        // THE SAVED THEME IS THE OTHER SIDE OF THE COMPARISON, not a flag kept as edits arrive.
        // The tab's view state is where an edit lands and where it stays until Save, so a tab
        // restored from settings comes back holding edits the saved theme has never seen - and a
        // flag set as they were made would have been left behind with the session that made them.
        //
        // A user theme is read off the disk rather than asked of the manager, because the manager
        // is not what Save wrote to: it answers with whatever the last directory read found, which
        // is the theme as it stood before a Save the watcher has not reported yet, and that would
        // leave a page reading unsaved immediately after saving it.
        //
        // A BUILT-IN HAS NO FILE OF ITS OWN, so the disk is the wrong side for one. The path it
        // would name is never written and never read, the seed below would stand as the saved
        // theme, and a built-in whose colours differ from that seed would read as edited the
        // moment it was opened. The compiled-in colours are what it was opened from, and the
        // manager answers a built-in name with those whatever stands on the disk - which is the
        // one case where the manager is exactly what is wanted.
        //
        // Seeded with the defaults and loaded over, exactly as UserTheme reads a file: the file
        // states only what it changes, so the two sides are whole themes only if this side is
        // filled in the same way.
        Dom::Value<AppTheme> savedNode{ nullptr, AppTheme{} };
        if (isBuiltIn())
        {
            themesManager().saveTheme(pageData().name, savedNode);
        }
        else
        {
            const Dom::FileFormat::ClaFi ff;
            ff.loadSectionFromFile(savedNode, themesManager().directory() / pageData().name);
        }
        return !Dom::sameValue((tabConfig() / k_themeDataAttrName).as<Dom::Section>(), savedNode);
    }

    bool ThemePage::saveEdits(Control& initiator) const
    {
        if (canSaveEdits())
        {
            saveTheme();
            return true;
        }
        // A BUILT-IN HAS NO FILE OF ITS OWN, SO SAVE HERE MEANS SAVE AS. The work still has to
        // land somewhere and the user is being asked before it is lost, so the command that can
        // keep it is the one to offer. It writes the file and no more: the tab is already on its
        // way somewhere else, and taking it to the copy instead would answer a question nobody
        // asked.
        return !saveThemeAsFile(initiator).empty();
    }

    void ThemePage::visibilityChanged()
    {
        BasePage::visibilityChanged();
        // The answer is the application's, so another theme tab may have moved it while this page
        // was off screen, and the document this page holds would state the choice it was left
        // with. The buttons need nothing: the actions refreshed them where they stand.
        if (visible())
            generateCode();
    }

    void ThemePage::previewColorModeChanged()
    {
        m_pigmentGrid.invalidate();
    }

    void ThemePage::anchorChanged()
    {
        m_harmonySelector.anchorChanged();
        m_harmonyStack.invalidate();
        m_otherPigmentsLabel.invalidate();
        mapHuesToColors();

        storeViewState();
    }

    void ThemePage::harmonyChanged()
    {
        if (m_harmonyStack.currentItem())
            editColors().harmonyKind = static_cast<ColorHarmonyItem&>(*m_harmonyStack.currentItem()).harmony().kind();
        else
            editColors().harmonyKind = k_defaultHarmonyKind;
        m_harmonyPanel.invalidate();
        m_otherPigmentsLabel.invalidate();
        mapHuesToColors();

        storeViewState();
    }

    void ThemePage::paletteMapChanged()
    {
        mapHuesToColors();
        storeViewState();
    }

    void ThemePage::mapHuesToColors()
    {
        ColorHarmony& harmony = m_harmonySelector.harmony(static_cast<std::size_t>(editColors().harmonyKind));
        PaletteMap& map = *harmony.selectedMap();
        for (std::size_t i = 0; i != 3; ++i)
            editColors().paletteHues[i] = harmony.color(map[i]).hue();

        invalidatePreview();
        m_pigmentGrid.invalidate();
        m_darkModeFloorSlider.invalidate();
    }

    void ThemePage::otherPigmentsText(GetTextEvent& event)
    {
        // The selector holds every harmony built against the current anchor, so the one the theme
        // names answers for itself. Which map is selected does not reach these hues: a map orders
        // the colours a palette takes, and a pigment is not one of them.
        const ColorHarmony& harmony = m_harmonySelector.harmony(static_cast<std::size_t>(editColors().harmonyKind));
        for (const PaletteColor& color : harmony.pigmentColors())
            paintColorDot(event.text, color.rgb());
    }

    Row ThemePage::pigmentRow(const UiElement element) const
    {
        const Padding padding = themeMetrics().listItem.padding;
        return Row{
            Tag{ element },
            Cell{ Tag{ PigmentColumn::Name }, this, &ThemePage::pigmentNameCell },
            CellWith<HueRuleControl>{ Tag{ PigmentColumn::Hue }, padding, VerticalAlign::Fill },
            CellWith<ValueRuleControl>{ Tag{ PigmentColumn::Saturation }, padding,
                VerticalAlign::Fill },
            CellWith<ValueRuleControl>{ Tag{ PigmentColumn::Elevation }, padding,
                VerticalAlign::Fill }
        };
    }

    // Set the way an element page sets the channel a rule writes, with the use one grade down.
    void ThemePage::pigmentNameCell(Grids::GetCellTextEvent& event) const
    {
        const UiElement element = event.row().tag<UiElement>();
        event.text() << TextStyleId::SubHeading
            << uiElementOf(element).name
            << PopTextStyle{}
            << L'\n'
            << InkGrade::Muted
            << TextStyleId::SubBody
            << pigmentUse(element)
            << PopTextStyle{}
            << PopColor{};
    }

    // Bound again after a restore, which writes the theme over in place: the rules keep their
    // addresses, and a bind is what redraws the faces. A pigment is a colour of its own rather
    // than a change to one, so its hue is never cleared and its values are stated ones only.
    void ThemePage::bindPigmentEditors()
    {
        m_pigmentGrid.forEachRow([this](Grids::Rt::RowContainer& row) {
            const UiElement element = row.tag<UiElement>();
            ColorEffect& rule = editColors().*uiElementOf(element).effect;
            row.controlAtColumnAs<HueRuleControl>(pigmentColumn(PigmentColumn::Hue))
                .bind(rule.hue, editColors(), m_onPigmentChanged, false);
            row.controlAtColumnAs<ValueRuleControl>(pigmentColumn(PigmentColumn::Saturation)).bind(
                rule.saturation,
                RuleChannel::Saturation,
                [this, element]() {
                    return pigmentRuleBase(element, RuleChannel::Saturation);
                },
                m_onPigmentChanged,
                ValueRuleOperations::SetOnly
            );
            row.controlAtColumnAs<ValueRuleControl>(pigmentColumn(PigmentColumn::Elevation)).bind(
                rule.elevation,
                RuleChannel::Elevation,
                [this, element]() {
                    return pigmentRuleBase(element, RuleChannel::Elevation);
                },
                m_onPigmentChanged,
                ValueRuleOperations::SetOnly
            );
        });
    }

    Grids::Column& ThemePage::pigmentColumn(const PigmentColumn tag)
    {
        return m_pigmentGrid.columnByTag(Tag{ tag });
    }

    // On the surface the pigment's element stands on.
    RuleBase ThemePage::pigmentRuleBase(const UiElement element, const RuleChannel channel)
    {
        const UiElementDescriptor& descriptor = uiElementOf(element);
        return ruleBaseOn(
            elementColor(descriptor.base),
            editColors().*descriptor.effect,
            channel,
            previewColorMode(),
            editColors().darkModeFloor
        );
    }

    // Each ramp reads its rule's other channels, and the preview reads the pigment.
    void ThemePage::pigmentChanged()
    {
        m_pigmentGrid.invalidate();
        invalidatePreview();
        storeViewState();
    }

    void ThemePage::darkModeFloorChanged()
    {
        editColors().darkModeFloor = m_darkModeFloorSlider.position();
        // Every swatch and every ramp that stands on a surface moves with the floor.
        m_pigmentGrid.invalidate();
        invalidatePreview();
        storeViewState();
    }

    void ThemePage::storeViewState() const
    {
        // A RESTORE IS NOT AN EDIT. restoreViewState brings the controls up by calling the same
        // handlers a user edit calls, and mapHuesToColors along the way fills the palette hues in
        // from the harmony rather than reading them. Stored, that normalisation would stand in the
        // view state as a change nobody made, and the page would read as edited the moment it was
        // opened - which is exactly what a theme whose file does not carry those hues yet does.
        if (m_restoring)
            return;

        themesManager().saveTheme(m_editTheme, (tabConfig() / k_themeDataAttrName).as<Dom::Value<AppTheme>>());
    }

    void ThemePage::storeView() const
    {
        const Control* page = m_tabbedBox.pageControl().currentItem();
        for (std::size_t i = 0ull; i != m_viewTabs.size(); ++i)
            if (m_viewTabs[i]->page() == page)
                (tabConfig() / k_viewAttrName).set(static_cast<ThemeView>(i));
    }

    void ThemePage::restoreView()
    {
        const ThemeView view = (tabConfig() / k_viewAttrName).get<ThemeView>();
        m_viewTabs[static_cast<std::size_t>(view)]->select();
    }

    // The page the tree picked shows once the browser has taken the tab there - see showAnchor.
    // A step made by a key replaces the entry, so a walk down the tree leaves one entry behind.
    void ThemePage::designPagePicked()
    {
        if (m_showingAnchor)
            return;

        const Browser::HistoryEntry entry = Input::device() == InputDevice::Keyboard
            ? Browser::HistoryEntry::Replace
            : Browser::HistoryEntry::Push;
        tab().browserControl().goTo(tab().url().withAnchor(m_designPage.pickedPage()), entry);
    }

    void ThemePage::showAnchor(const Browser::ShowAnchorEvent& event)
    {
        const ScopedFlag showing{ m_showingAnchor };
        m_designPage.pickPage(event.url.anchor());
    }

    void ThemePage::rulesChanged()
    {
        // A pigment's ramps stand on a surface the rules colour.
        m_pigmentGrid.invalidate();
        invalidatePreview();
        storeViewState();
    }

    void ThemePage::saveTheme() const
    {
        // Named by the page rather than through themeFileName: a built-in's page name already
        // carries its own extension.
        writeThemeTo(themesManager().directory() / pageData().name);
    }

    void ThemePage::saveThemeAs(Control& initiator)
    {
        const std::wstring fileName = saveThemeAsFile(initiator);
        if (fileName.empty())
            return;

        // Said before the tab moves, and read by the question canLeavePage puts. The work is on
        // the disk under the name that was just given, so there is nothing left to ask about -
        // what this page's own file holds is no longer anybody's business.
        m_savedUnderAnotherName = true;

        // ONLY THE GOING WAITS. The naming question stood where it was asked - on top of the
        // strip's menu when that is where Save as was chosen - but taking the tab to what it wrote
        // rebuilds this page, and the frames above the press go on reading the button that goes
        // down with it. The browser holds that wait, because it outlives what the going destroys.
        // The anchor goes along, so the copy opens on the design page this one shows.
        const std::wstring path = std::wstring{ Browser::ConfigNames::homePath }.append(fileName);
        tab().browserControl().goToLater(Url{ path, tab().anchor() });
    }

    std::wstring ThemePage::saveThemeAsFile(Control& initiator) const
    {
        // The prompt opens on the name this page already stands under - the name the user is
        // working from, and the one they are about to vary. It is offered to be edited rather
        // than taken: accepted as it stands the handler below refuses it, because Save as never
        // writes over a theme that is already there and a built-in's name is reserved.
        const std::wstring currentStem = std::filesystem::path{ pageData().name }.stem().wstring();

        // Under what asked, which is this page's own Save as button when that is what was
        // pressed, and whatever raised the leaving question when this is its fallback. A tab
        // closed from the strip while another one is showing has no visible button of its own.
        PromptDialog dialog{ initiator, L"Save theme as", currentStem, MessageIcon::Question };
        dialog.onAccept([this](AcceptEditEvent& event) {
            const std::wstring stem{ event.text.plainText() };
            checkThemeNameShape(event, stem);
            if (event.refused())
                return;

            // A NAME ALREADY TAKEN IS A QUESTION, NOT A REFUSAL. Writing over a theme is a thing
            // the user may well have meant, and the one who typed the name is the only one who
            // can say whether they did - so the name is put back to them rather than turned down.
            // A built-in never reaches here: checkThemeNameShape has already refused its name,
            // and there is no file of its own to write over.
            std::error_code errorCode;
            if (!std::filesystem::exists(themeFileName(stem), errorCode))
                return;
            if (!confirmReplacingTheme(event.askedBy, stem))
                event.refuse();
            });
        if (!dialog.execute())
            return {};

        std::wstring fileName = std::wstring{ dialog.text() }.append(k_themeFileExtension);
        writeThemeTo(themesManager().directory() / fileName);
        return fileName;
    }

    bool ThemePage::confirmReplacingTheme(Control& initiator, const std::wstring_view stem) const
    {
        // Owned by the answer that asked for the name to be taken, so it stands on top of the
        // naming question with the name the user typed still on screen behind it.
        //
        // A warning, not a question: the triangle is what says the answer cannot be taken back,
        // and what stands under this name now is about to stop existing.
        Text message{};
        message << themeInQuestionText(stem) << L" already exists. Replace it?";

        MessageDialog dialog{
            initiator,
            L"Replace theme",
            message,
            MessageIcon::Warning
        };
        dialog.add(DialogButton::Yes);
        dialog.add(DialogButton::No);
        // Dismissed without an answer is no, which is what makes Escape the safe way out of a
        // question about something that cannot be brought back.
        return dialog.execute() == DialogButton::Yes;
    }

    void ThemePage::writeThemeTo(const std::filesystem::path& fileName) const
    {
        const Dom::FileFormat::ClaFi ff;
        const Dom::Section& themeNode = (tabConfig() / k_themeDataAttrName).as<Dom::Section>();
        ff.saveSectionToFile(*statedTheme(themeNode), fileName);
    }

    std::filesystem::path ThemePage::themeFileName(const std::wstring_view stem) const
    {
        return themesManager().directory() / std::wstring{ stem }.append(k_themeFileExtension);
    }

    bool ThemePage::isBuiltIn() const
    {
        // Asked of the name, against the names the built-ins hold - the same test that keeps a
        // user theme from taking one. The extension cannot answer it: a file the user put in the
        // directory called Foo.theme carries the built-ins' extension and is not one of them.
        return isReservedThemeName(std::filesystem::path{ pageData().name }.stem().wstring());
    }

    // The colour an element resolves to, walked down from the bare surface through everything
    // it sits on. Only resting rules are applied: nothing below the element is in a state.
    Hsl ThemePage::elementColor(OptionalUiElement element)
    {
        if (!element)
            return editColors().rootSurface(previewColorMode());

        const UiElementDescriptor& descriptor = uiElementOf(*element);
        Hsl result = elementColor(descriptor.base);
        if (descriptor.effect)
        {
            (editColors().*descriptor.effect).applyTo(result, 1.0f, editColors(),
                previewColorMode());
        }
        applyRestingRules(result, *element, PaintChannel::Surface);
        return result;
    }

    // The ink an element's text starts from, walked down the same nesting the surfaces use. Only
    // text rules are applied on the way: what an element is painted on is a separate chain and
    // does not reach the ink. No element is the bare ink of the colour mode, with nothing
    // applied - a window root's own text rule is the first thing on the chain, exactly as its
    // surface rule is the first thing on the other one. The one thing the surface lends the ink
    // is its hue, until a text rule on the way names one - the seed PaintEvent takes.
    Hsl ThemePage::elementTextColor(OptionalUiElement element, bool* hueNamed)
    {
        if (!element)
            return bareInk();

        const UiElementDescriptor& descriptor = uiElementOf(*element);
        bool named = false;
        Hsl result = elementTextColor(descriptor.base, &named);
        if (!named)
            result.hue = elementColor(element).hue;
        if (applyRestingRules(result, *element, PaintChannel::Text))
            named = true;
        if (hueNamed)
            *hueNamed = named;
        return result;
    }

    // The element's own list, the window's where the element is worn by a form's root control,
    // then the shared list.
    bool ThemePage::applyRestingRules(Hsl& color, const UiElement element,
        const PaintChannel channel)
    {
        const ColorMode mode = previewColorMode();
        const ThemeRules& rules = editColors().rules;
        const ColorRules* windowRules = uiElementOf(element).isWindowRoot
            ? &rules.anyWindow
            : nullptr;
        bool namesHue = false;
        for (const ColorRules* list : { &rules.of(element), windowRules, &rules.shared })
        {
            if (!list)
                continue;
            for (const ColorRule& rule : *list)
            {
                if (!rule.atRest() or rule.output != channel)
                    continue;
                rule.effect.applyTo(color, 1.0f, editColors(), mode);
                if (rule.effect.hue.operation() != ColorRuleHueOp::NoChange)
                    namesHue = true;
            }
        }
        return namesHue;
    }

    // The ink of the preview's colour mode before any rule: white at the dark end, black at the
    // light one, in the hue of the bare surface - the theme's anchor.
    Hsl ThemePage::bareInk()
    {
        const ColorMode mode = previewColorMode();
        return {
            editColors().rootSurface(mode).hue,
            0.0f,
            mode == ColorMode::Dark ? 1.0f : 0.0f
        };
    }

    // The seed BakedColors::windowShadow takes: a stroke starts at the element's surface and its
    // resting stroke rules move it, and a shadow rule naming no hue keeps the stroke's.
    Hsl ThemePage::bareShadow(OptionalUiElement element)
    {
        Hsl stroke = elementColor(element);
        if (element)
            applyRestingRules(stroke, *element, PaintChannel::Stroke);
        return { stroke.hue, 0.0f, 0.0f };
    }

    // What the element carries at rest, on the channel the rule writes.
    RuleBase ThemePage::elementRuleBase(const OptionalUiElement element, const ColorRule& rule,
        const RuleChannel channel)
    {
        const bool shadow = rule.output == PaintChannel::Shadow;
        const ColorMode mode = shadow ? ColorMode::Dark : previewColorMode();
        Hsl result{};
        if (shadow)
            result = bareShadow(element);
        else if (rule.output == PaintChannel::Text)
            result = elementTextColor(element);
        else
            result = elementColor(element);

        const float luminosityFloor = shadow ? k_noFloor : editColors().darkModeFloor;
        return ruleBaseOn(result, rule.effect, channel, mode, luminosityFloor);
    }

    RuleBase ThemePage::ruleBaseOn(Hsl base, const ColorEffect& rule, const RuleChannel channel,
        const ColorMode mode, const float luminosityFloor)
    {
        base.hue = rule.hue.actualHue(editColors(), base.hue);
        if (channel != RuleChannel::Saturation)
            rule.saturation.applyTo(base.saturation, 1.0f, ColorMode::Dark, k_noFloor);
        if (channel != RuleChannel::Elevation)
            rule.elevation.applyTo(base.luminosity, 1.0f, mode, luminosityFloor);
        return { base, mode, luminosityFloor };
    }

    void ThemePage::generateCode()
    {
        const Control* page = m_tabbedBox.pageControl().currentItem();
        if (page == &m_cppCodePage)
        {
            showCode(themeToCppCode(editColors(), CodeOptions::content(), CodeOptions::scope()),
                m_cppCodePage.box());
            return;
        }
        if (page == &m_claFiPage)
        {
            writeThemeTo(Dom::FileFormat::ClaFi{}, m_claFiPage.box());
            return;
        }
        if (page == &m_xmlPage)
        {
            writeThemeTo(Dom::FileFormat::Xml{}, m_xmlPage.box());
            return;
        }
        if (page == &m_jsonPage)
            writeThemeTo(Dom::FileFormat::Json{}, m_jsonPage.box());
    }

    void ThemePage::writeThemeTo(const Dom::FileFormatBase& format, TextBox& box)
    {
        Dom::Value<AppTheme> themeNode{ nullptr, m_editTheme };
        themesManager().saveTheme(m_editTheme, themeNode);
        std::wostringstream stream;
        // A format that has no header of its own writes nothing here.
        format.writeHeader(stream);
        if (CodeOptions::content() == CodeContent::Full)
        {
            format.saveSectionToStream(themeNode, stream);
        }
        else
        {
            format.saveSectionToStream(*statedTheme(themeNode), stream);
        }
        showCode(stream.str(), box);
    }

    void ThemePage::showCode(const std::wstring& code, TextBox& box)
    {
        box.text().clear();
        // Leading indentation and the C++ block's marker column only line up in a monospaced
        // style.
        box.text() << TextStyleId::Code << code;
        // The document is what the box measures, so a new one is a new size for the scroll
        // box around it.
        box.invalidateFormAlign();
    }

    std::unique_ptr<Dom::Section> ThemePage::statedTheme(const Dom::Section& themeNode) const
    {
        const Dom::Value<AppTheme> defaults{ nullptr, AppTheme{} };
        return Dom::withoutDefaults(themeNode, defaults);
    }

    void ThemePage::codeOptionPicked(ClickEvent&)
    {
        // The action ran first - it is connected when the button takes it as a property, before
        // this handler was added - so the answer has already moved and every presenter of it has
        // already been asked again. What is left is the document this page is showing.
        generateCode();
    }
}
