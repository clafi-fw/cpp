module ThisApp.ThemePage;

import ThisApp.WithPreview;
import ThisApp.CodeOptions;
import ThisApp.Consts;
import ThisApp.DesignPage;
import ThisApp.ElementPage;
import ThisApp.HueRuleControl;
import ThisApp.RuleSlider;
import ThisApp.Palette.Controls;
import ThisApp.ThemeToCppCode;
import ThisApp.Utils;
import ThisApp.ValueRuleControl;

import ClaFi.Application.ThemesManager;
import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Documents.Page;

import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.Slider;
import ClaFi.Controls.TextBox;

import ClaFi.Dom;
import ClaFi.Dom.Formats.ClaFi;
import ClaFi.Dom.Formats.Json;
import ClaFi.Dom.Formats.Xml;

import ClaFi.StdActions;

import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Browser.Control;
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
        Base::restoreViewState();
        // Application wide, so they are read from the root section rather than from the tab's.
        CodeOptions::restore(settings().appConfig());
        auto& themeNode = documentNode().as<Dom::Value<AppTheme>>();
        UserTheme::loadTheme(themeNode, m_editTheme);
        showEditTheme();
        // The theme as the controls now show it, palette hues included, is where undo bottoms out.
        m_history.reset(m_editTheme);
        restoreView();
    }

    // A BUILT-IN HAS NO FILE OF ITS OWN, so the disk is the wrong side for one: the path it would
    // name is never written and never read. The compiled-in colours are what it was opened from,
    // and the manager answers a built-in name with those whatever stands on the disk.
    bool ThemePage::readSavedDocument(Dom::DomNodeBase& into) const
    {
        if (!isBuiltIn())
            return Base::readSavedDocument(into);
        themesManager().saveTheme(pageData().name, into.as<Dom::Value<AppTheme>>());
        return true;
    }

    void ThemePage::visibilityChanged()
    {
        Base::visibilityChanged();
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

    void ThemePage::anchorChanged(const EditPhase phase)
    {
        m_harmonySelector.anchorChanged();
        m_harmonyStack.invalidate();
        m_otherPigmentsLabel.invalidate();
        mapHuesToColors();

        edited(phase, m_anchorStep);
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

        const ColorHarmony& harmony = m_harmonySelector.harmony(
            static_cast<std::size_t>(editColors().harmonyKind));
        Text what{};
        what << InkWell::accentInk() << L"Harmony" << PopColor{} << L" change to "
            << InkWell::accentInk() << harmony.name() << PopColor{};
        edited(EditPhase::Settled, what);
    }

    void ThemePage::paletteMapChanged()
    {
        mapHuesToColors();
        edited(EditPhase::Settled, m_paletteMapStep);
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
                .bind(rule.hue, editColors(), m_onPigmentChanged, false,
                    pigmentStepName(element, L"Hue"));
            row.controlAtColumnAs<ValueRuleControl>(pigmentColumn(PigmentColumn::Saturation)).bind(
                rule.saturation,
                RuleChannel::Saturation,
                [this, element]() {
                    return pigmentRuleBase(element, RuleChannel::Saturation);
                },
                m_onPigmentChanged,
                ValueRuleOperations::SetOnly,
                pigmentStepName(element, L"Saturation")
            );
            row.controlAtColumnAs<ValueRuleControl>(pigmentColumn(PigmentColumn::Elevation)).bind(
                rule.elevation,
                RuleChannel::Elevation,
                [this, element]() {
                    return pigmentRuleBase(element, RuleChannel::Elevation);
                },
                m_onPigmentChanged,
                ValueRuleOperations::SetOnly,
                pigmentStepName(element, L"Elevation")
            );
        });
    }

    Text ThemePage::changeStepName(const std::wstring_view what)
    {
        Text result{};
        result << InkWell::accentInk() << what << PopColor{} << L" change";
        return result;
    }

    Text ThemePage::pigmentStepName(const UiElement element, const std::wstring_view column)
    {
        Text result{};
        result << InkWell::accentInk() << column << PopColor{} << L" change in "
            << InkWell::accentInk() << uiElementOf(element).name << PopColor{};
        return result;
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
    void ThemePage::pigmentChanged(const EditPhase phase, const Text& what)
    {
        m_pigmentGrid.invalidate();
        invalidatePreview();
        edited(phase, what);
    }

    void ThemePage::darkModeFloorChanged(const EditPhase phase)
    {
        editColors().darkModeFloor = m_darkModeFloorSlider.position();
        // Every swatch and every ramp that stands on a surface moves with the floor.
        m_pigmentGrid.invalidate();
        invalidatePreview();
        edited(phase, m_floorStep);
    }

    // Called with m_restoring held: the handlers this goes through end in edited, and none of
    // what they store or record is an edit.
    void ThemePage::showEditTheme()
    {
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
        m_otherPigmentsLabel.invalidate();

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
    }

    void ThemePage::edited(const EditPhase phase, const Text& what, const RuleSelection& at)
    {
        // A RESTORE IS NOT AN EDIT. showEditTheme brings the controls up by calling the same
        // handlers a user edit calls, and mapHuesToColors along the way fills the palette hues in
        // from the harmony rather than reading them. Stored, that normalisation would stand in the
        // view state as a change nobody made, and the page would read as edited the moment it was
        // opened - which is exactly what a theme whose file does not carry those hues yet does.
        // Recorded, it would stand in the history as a step the user never took.
        if (m_restoring)
            return;

        storeViewState();
        m_history.record(m_editTheme, phase, what,
            DesignPlace{ .page = std::wstring{ m_designPage.pickedPage() }, .selection = at });
        // A presenter keeps the answer it was last given until it is told to ask again.
        StdActions::undo.invalidateState();
        StdActions::redo.invalidateState();
    }

    void ThemePage::writeUndoStep(const std::size_t i, Text& text) const
    {
        text << m_history.undoStep(i);
    }

    void ThemePage::writeRedoStep(const std::size_t i, Text& text) const
    {
        text << m_history.redoStep(i);
    }

    void ThemePage::undo(const std::size_t steps)
    {
        if (const ThemeHistory::Landing landing = m_history.undo(steps))
            showHistoryState(landing);
    }

    void ThemePage::redo(const std::size_t steps)
    {
        if (const ThemeHistory::Landing landing = m_history.redo(steps))
            showHistoryState(landing);
    }

    void ThemePage::showHistoryState(const ThemeHistory::Landing& landing)
    {
        {
            const ScopedFlag restoring{ m_restoring };
            m_editTheme = *landing.state;
            showEditTheme();
        }
        // After the sync, which has rebuilt the rows the selection names. The tree's pick reaches
        // the browser as the tab's anchor the way a click on it does - see designPagePicked.
        m_designPage.showPlace(*landing.place);
        storeViewState();
        // A code page states the theme, and one of them may be the page showing.
        generateCode();
        StdActions::undo.invalidateState();
        StdActions::redo.invalidateState();
    }

    void ThemePage::storeViewState() const
    {
        themesManager().saveTheme(m_editTheme, documentNode().as<Dom::Value<AppTheme>>());
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

        const Browser::HistoryEntry entry = Input::mouse().active()
            ? Browser::HistoryEntry::Push
            : Browser::HistoryEntry::Replace;
        tab().browserControl().goTo(tab().url().withAnchor(m_designPage.pickedPage()), entry);
    }

    void ThemePage::showAnchor(const Browser::ShowAnchorEvent& event)
    {
        const ScopedFlag showing{ m_showingAnchor };
        m_designPage.pickPage(event.url.anchor());
    }

    void ThemePage::rulesChanged(const EditPhase phase, const Text& what,
        const RuleSelection& at)
    {
        // A pigment's ramps stand on a surface the rules colour.
        m_pigmentGrid.invalidate();
        invalidatePreview();
        edited(phase, what, at);
    }

    bool ThemePage::confirmWritingToSource(Control& initiator) const
    {
        Text message{};
        message << L"This rewrites " << m_builtInColorsFile.filename().wstring()
            << L" in the framework's source tree. Are you a ClaFi developer?";

        MessageDialog dialog{
            initiator,
            L"Write to source",
            message,
            MessageIcon::Question
        };
        dialog.add(DialogAnswer::Yes);
        // Opens on No: Return declines, Escape dismisses, and only a deliberate move reaches Yes.
        dialog.add(DialogAnswer::No).setFocus();
        return dialog.execute() == DialogAnswer::Yes;
    }

    bool ThemePage::isBuiltIn() const
    {
        // Asked of the name, against the names the built-ins hold - the same test that keeps a
        // user theme from taking one. The extension cannot answer it: a file the user put in the
        // directory called Foo.theme carries the built-ins' extension and is not one of them.
        const std::wstring stem = std::filesystem::path{ pageData().name }.stem().wstring();
        return ThemesManager::isReservedName(stem);
    }

    // The colour an element resolves to, walked down from the bare surface through everything
    // it sits on. Only resting rules are applied: nothing below the element is in a state.
    Hsl ThemePage::elementColor(OptionalUiElement element)
    {
        if (!element)
            return editColors().rootSurface(previewColorMode());

        const UiElementDescriptor& descriptor = uiElementOf(*element);
        Hsl result = elementColor(descriptor.base);
        // The hue ControlPaintContext::foundTextRgb starts the band from.
        if (const std::optional<Pigment> pigment = seedPigmentOf(*element))
            result.hue = editColors().harmony().pigmentColor(*pigment).hsl().hue;
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
            format.saveSectionToStream(*ThemesManager::statedTheme(themeNode), stream);
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

    void ThemePage::codeOptionPicked(ClickEvent&)
    {
        // The action ran first - it is connected when the button takes it as a property, before
        // this handler was added - so the answer has already moved and every presenter of it has
        // already been asked again. What is left is the document this page is showing.
        generateCode();
    }

    void ThemePage::writeToSource(ClickEvent& event)
    {
        Control& button = *event.control;
        if (!confirmWritingToSource(button))
            return;

        switch (writeBuiltInColors(m_builtInColorsFile, editColors()))
        {
        case SourceWrite::Written:
            ContextMessage::show(
                button,
                messageText(MessageIcon::Ok, L"Written. The built-in theme from the next build")
            );
            break;
        case SourceWrite::Unchanged:
            ContextMessage::show(
                button,
                messageText(MessageIcon::Information, L"The source already states this theme")
            );
            break;
        case SourceWrite::Failed:
            ContextMessage::show(
                button,
                messageText(MessageIcon::Error, L"The source could not be written")
            );
            break;
        }
    }
}
