module ThisApp.ThemePage;

import ThisApp.BasePage;
import ThisApp.CodeOptions;
import ThisApp.Consts;
import ThisApp.HueRuleControl;
import ThisApp.RuleSlider;
import ThisApp.Palette.Controls;
import ThisApp.ThemeToCppCode;
import ThisApp.Utils;

import ClaFi.Application.ThemesManager;
import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Controls.Base.ExpanderBase;
import ClaFi.Controls.Base.SliderBase;
import ClaFi.Controls.Checkbox;
import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Menu;
import ClaFi.Controls.MessageDialog;
import ClaFi.Controls.PromptDialog;
import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;
import ClaFi.Controls.ComboBox;
import ClaFi.Controls.Slider;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.TextItems;

import ClaFi.Dom;
import ClaFi.Dom.Formats.ClaFi;
import ClaFi.Dom.Formats.Json;
import ClaFi.Dom.Formats.Xml;

import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.Fmt;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
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

        // The design page's menu - the page it was raised on answers each, for its own folds.
        namespace FoldActions
        {
            Action expandAll{ Text{ L"Expand all" } };
            Action collapseAll{ Text{ L"Collapse all" } };
            Action collapseOthers{ Text{ L"Collapse others" } };
        }

        // The operation a mark names: the marks the list shows, and what a keyboard has in their
        // place.
        [[nodiscard]] std::optional<ColorRuleOp> operationOfMark(const wchar_t mark)
        {
            switch (mark)
            {
                case L'+':
                case L'-':
                    return ColorRuleOp::Offset;
                case L'\u00D7':
                case L'*':
                case L'x':
                case L'X':
                    return ColorRuleOp::Scale;
                case L'=':
                    return ColorRuleOp::Set;
            }
            return std::nullopt;
        }

        // A number as typed, with a point or a comma before the fraction.
        [[nodiscard]] std::optional<float> typedNumber(const std::wstring_view text)
        {
            std::string narrow{};
            for (const wchar_t character : text)
            {
                if (character > L'\x7F')
                    return std::nullopt;
                narrow.push_back(character == L',' ? '.' : static_cast<char>(character));
            }
            // from_chars reads a minus sign and no plus.
            if (!narrow.empty() && narrow.front() == '+')
                narrow.erase(0, 1);

            float value = 0.0f;
            const char* last = narrow.data() + narrow.size();
            const std::from_chars_result result = std::from_chars(narrow.data(), last, value);
            if (result.ec != std::errc{} || result.ptr != last)
                return std::nullopt;
            return value;
        }

        // The values an operation reaches: the slider's two ends, read through the rule's own
        // mapping.
        struct OperationRange
        {
            float low{};
            float high{};
        };

        [[nodiscard]] OperationRange operationRange(const ColorRuleOp operation)
        {
            ColorRuleValue probe{};
            probe.setNormalizedValue(0.0f);
            const float low = probe.value(operation);
            probe.setNormalizedValue(1.0f);
            return { low, probe.value(operation) };
        }

        // The rule a text typed over an Operation cell states: a mark and a value, the mark alone,
        // or a value for the operation the cell stands on. Nothing where the text states none of
        // these - the list looks it up then - and nothing where the rule is refused.
        [[nodiscard]] std::optional<ColorRuleValue> typedRule(AcceptEditEvent& event,
            const ColorRuleValue& current)
        {
            std::wstring_view text = event.text.plainText();
            trimLeft(text);
            trimRight(text);
            if (text.empty())
                return std::nullopt;

            const std::optional<ColorRuleOp> marked = operationOfMark(text.front());
            const bool startsNumber = std::iswdigit(text.front())
                || text.front() == L'.'
                || text.front() == L',';
            if (!marked.has_value() && !startsNumber)
                return std::nullopt;

            ColorRuleOp operation = current.operation();
            bool negative = false;
            if (marked.has_value())
            {
                operation = marked.value();
                negative = text.front() == L'-';
                text.remove_prefix(1);
                trimLeft(text);
                // The mark alone is a pick from the list, and the value stays where it is.
                if (text.empty())
                {
                    ColorRuleValue result = current;
                    result.setOperation(operation);
                    return result;
                }
            }
            if (operation == ColorRuleOp::NoChange)
            {
                event.refuse(L"Type a mark before the value, such as +0.02 or =0.66.");
                return std::nullopt;
            }

            const std::optional<float> number = typedNumber(text);
            if (!number.has_value())
            {
                event.refuse(L"Type a number after the mark, such as +0.02 or =0.66.");
                return std::nullopt;
            }
            const float value = negative ? -number.value() : number.value();
            const OperationRange range = operationRange(operation);
            // Written so that a NaN is refused as well.
            if (!(value >= range.low && value <= range.high))
            {
                event.refuse(std::format(L"Type a value from {:.2f} to {:.2f}.",
                    range.low, range.high));
                return std::nullopt;
            }
            return ColorRuleValue{ operation, value };
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
        m_darkModeFloorLabel.invalidate();

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
        m_grid.invalidate();
        m_darkModeFloorSlider.invalidate();
        updateGridControls();
        restoreFolds();
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
        m_grid.invalidate();
    }

    bool ThemePage::Fold::holds(const Control* control) const
    {
        return head->parent()->containsNested(control);
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
        m_grid.invalidate();
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

    void ThemePage::darkModeFloorChanged()
    {
        editColors().darkModeFloor = m_darkModeFloorSlider.position();
        m_darkModeFloorLabel.invalidate();
        // Every swatch and every ramp that stands on a surface moves with the floor.
        m_grid.invalidate();
        invalidatePreview();
        storeViewState();
    }

    void ThemePage::darkModeFloorText(GetTextEvent& event)
    {
        event.text << L" " << TextStyleId::Code
            << Fmt{ L"{:.2f}", editColors().darkModeFloor };
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

    void ThemePage::addFold(Fold&& fold)
    {
        fold.head->connectEvent([this](ToggleExpandedEvent&) {
            storeFolds();
        });
        m_folds.push_back(std::move(fold));
    }

    void ThemePage::addFold(ExpanderHeader& header)
    {
        addFold(Fold{
            .name = header.text().plainText(),
            .expanded = [&header]() {
                return header.expanded();
            },
            .setExpanded = [&header](bool value) {
                header.setExpanded(value);
            },
            .head = &header,
            .stop = &header.button(),
        });
    }

    void ThemePage::addFold(UiElement element, Grids::RowGroupSpan& span)
    {
        addFold(Fold{
            .name = std::wstring{ uiElementOf(element).token },
            .expanded = [&span]() {
                return span.expanded();
            },
            .setExpanded = [&span](bool value) {
                span.setExpanded(value);
            },
            .head = &span,
            .stop = &span,
        });
    }

    Init<Grids::Rt::RowExpander> ThemePage::sectionFold()
    {
        return Init<Grids::Rt::RowExpander>{ [this](Grids::Rt::RowExpander& section) {
            addFold(section.header());
        } };
    }

    void ThemePage::storeFolds() const
    {
        // A restore turns the folds one at a time to match the list it read, and a list stored
        // after each turn would be one it had not finished reading.
        if (m_restoring)
            return;
        FoldNames collapsed;
        for (const Fold& fold : m_folds)
            if (!fold.expanded())
                collapsed.push_back(fold.name);
        (tabConfig() / k_collapsedAttrName).set(collapsed);
    }

    void ThemePage::restoreFolds()
    {
        const FoldNames collapsed = (tabConfig() / k_collapsedAttrName).get<FoldNames>();
        for (const Fold& fold : m_folds)
            fold.setExpanded(std::ranges::find(collapsed, fold.name) == collapsed.end());
    }

    void ThemePage::connectFoldMenu()
    {
        m_designScrollBox.onContextPopup([this](ContextPopupEvent& event) {
            showFoldMenu(event);
        });

        // Claimed either way, so a command with nothing left to turn stays in the menu, disabled.
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &FoldActions::expandAll)
                event.claim({ .enabled = anyFold(false) });
            else if (&event.action == &FoldActions::collapseAll)
                event.claim({ .enabled = anyFold(true) });
            else if (&event.action == &FoldActions::collapseOthers)
                event.claim({ .enabled = canCollapseOtherFolds() });
        });

        onActionClick([this](ActionClickEvent& event) {
            // Read before the folds turn - one closing over the owner takes the menu down with it.
            const Control* owner = event.form.popupTarget();
            if (&event.action == &FoldActions::expandAll)
                setAllFolds(true);
            else if (&event.action == &FoldActions::collapseAll)
                setAllFolds(false);
            else if (&event.action == &FoldActions::collapseOthers)
                collapseOtherFolds();
            else
                return;
            followFolds(owner);
        });
    }

    void ThemePage::showFoldMenu(ContextPopupEvent& event)
    {
        event.stopPropagation();
        // The menu gives the focus back to its owner, and finds this page's answers from it.
        Control* owner = event.control;
        while (owner && !owner->canTakeFocus())
            owner = owner->parent();
        if (!m_designScrollBox.containsNested(owner))
            owner = Input::focusedControl();
        if (!m_designScrollBox.containsNested(owner))
            owner = &m_designScrollBox;
        const ScopedPushPop raisedOn{ m_foldMenuTarget, event.control };
        Menu menu{ *owner };
        menu.add(FoldActions::expandAll);
        menu.add(FoldActions::collapseAll);
        menu.add(FoldActions::collapseOthers);
        menu.execute();
    }

    bool ThemePage::anyFold(bool expanded) const
    {
        return std::ranges::any_of(m_folds, [expanded](const Fold& fold) {
            return fold.expanded() == expanded;
        });
    }

    bool ThemePage::canCollapseOtherFolds() const
    {
        bool inFold = false;
        bool otherOpen = false;
        for (const Fold& fold : m_folds)
        {
            if (fold.holds(m_foldMenuTarget))
                inFold = true;
            else if (fold.expanded())
                otherOpen = true;
        }
        return inFold && otherOpen;
    }

    void ThemePage::setAllFolds(bool expanded)
    {
        for (const Fold& fold : m_folds)
            fold.setExpanded(expanded);
    }

    void ThemePage::collapseOtherFolds()
    {
        for (const Fold& fold : m_folds)
            if (!fold.holds(m_foldMenuTarget))
                fold.setExpanded(false);
    }

    void ThemePage::followFolds(const Control* owner)
    {
        // As on a tree, a fold that closes over the focus hands it to what opens the fold again.
        const Control* focus = owner;
        // The grid holds the focus for the row it has selected.
        if (m_grid.containsNested(owner))
            focus = m_grid.descriptor().selectedRow();
        if (const Fold* fold = hidingFold(focus))
            fold->stop->setFocus();
        // Every fold that opened asked to be scrolled to, and only the last request stands.
        Control* target = m_foldMenuTarget;
        if (const Fold* fold = hidingFold(target))
            target = fold->stop;
        if (target)
            target->scrollIntoViewOnAlign();
    }

    const ThemePage::Fold* ThemePage::hidingFold(const Control* control) const
    {
        const Fold* result = nullptr;
        for (const Fold& fold : m_folds)
        {
            // What the fold holds and its head does not stands in its body.
            if (fold.expanded() || !fold.holds(control) || fold.head->containsNested(control))
                continue;
            if (!result || fold.holds(result->head))
                result = &fold;
        }
        return result;
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
        tab().browserControl().goToLater(
            std::wstring{ Browser::ConfigNames::homePath }.append(fileName)
        );
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

    CellSet ThemePage::operationCells(ColumnTag actionKey, ColumnTag valueKey, ComboboxTarget target)
    {
        static const Text k_pushIconFormat{ InkWell::spotInk(), TextOp::PushBold };
        static const Text k_popIconFormat{ TextOp::PopBold, PopColor{} };
        static TextItems items{ {
            TextItem{
                Tag{ ColorRuleOp::NoChange },
                PlaceHolderText{ TextStyleId::SubBody, L"No change", PopTextStyle{} },
                TooltipText{ L"Keep parent value" }
            },
            TextItem{
                Tag{ ColorRuleOp::Offset },
                Text{ k_pushIconFormat, L"+", k_popIconFormat },
                TooltipText{ L"Add offset" }
            },
            TextItem{
                Tag{ ColorRuleOp::Scale },
                Text{ k_pushIconFormat, L"\u00D7", k_popIconFormat },
                TooltipText{ L"Scale value" }
            },
            TextItem{
                Tag{ ColorRuleOp::Set },
                Text{ k_pushIconFormat, L"=", k_popIconFormat },
                TooltipText{ L"Set exact value" }
            }
            } };
        static constexpr MinSize k_comboboxSize{ 90.0f, 0.0f };
        // From the theme, not the grid: the cell sets are built before the grid exists.
        // GridDescriptor's designCellMetrics is a reference to this same object.
        const Padding k_controlPadding = themeMetrics().listItem.padding;
        const Tag lumTag{ target };

        return CellSet{
            // Control arguments are copied into the node so the call can be replayed at
            // apply time, hence std::ref for the shared TextItems.
            CellWith<ComboBox>{ actionKey,
                std::ref(items),
                k_comboboxSize,
                VerticalTextAnchor::Center,
                k_controlPadding,
                VerticalAlign::Fill,
                ItemIndex{ 0ull },
                EditorMode::Editable,
                lumTag,
                OnEvent{ [this](GetStateEvent& event) {
                    // Set, and not the user's to move: the row states a colour nothing stands
                    // under. The slider stays live, because Set names a value.
                    event.state.enabled = !statesAbsoluteSurface(
                        *static_cast<Grids::Rt::RowContainer*>(event.control.parent()));
                } },
                // Captures the Tag, not the column: the columns do not exist yet when this
                // fragment is built. The grid resolves it when the event fires.
                OnEvent{ [this, valueKey](ComboboxChangeEvent& event) {
                    Grids::Rt::RowContainer& container = *static_cast<Grids::Rt::RowContainer*>(event.comboBox().parent());
                    Control* slider = container.controlAtColumn(m_grid.columnByTag(Tag{ valueKey }));
                    slider->invalidateState();
                    const ComboboxTarget targetOp = event.comboBox().tag<ComboboxTarget>();
                    const TextItem* item = event.comboBox().selectedItem();
                    const auto operation = item->tag().get<ColorRuleOp>();
                    ColorRule& rule = rowColorRule(container);
                    if (targetOp == ComboboxTarget::Elevation)
                        rule.elevation.setOperation(operation);
                    else
                        rule.saturation.setOperation(operation);
                    invalidatePreview();
                    storeViewState();
                } },
                OnEvent{ [this](ComboboxAcceptTextEvent& event) {
                    acceptOperationText(event);
                } },
                OnEvent{ [this](AdjustItemTextEvent& event) {
                    ColorRuleOp itemAction = event.item().tag().get<ColorRuleOp>();
                    if (itemAction == ColorRuleOp::NoChange)
                        return;
                    if (event.phase() == EventPhase::Calculate)
                    {
                        event.text() << TextStyleId::Code << L"-0.00";
                        return;
                    }
                    const ComboboxTarget isLum = event.comboBox().tag<ComboboxTarget>();
                    Grids::Rt::RowContainer& row = *static_cast<Grids::Rt::RowContainer*>(event.comboBox().parent());
                    const ColorRule& rule = rowColorRule(row);
                    const ColorRuleValue& ruleValue = isLum == ComboboxTarget::Elevation
                        ? rule.elevation
                        : rule.saturation;
                    float value = ruleValue.value(itemAction);
                    event.text() << FlexSpace{} << TextStyleId::Code << Fmt{ L"{:.2f}", value };
                } }
            },

            CellWith<RuleSlider>{ Tag{ valueKey },
                ScrollButtons::No,
                k_controlPadding,
                UiElement::Section,
                lumTag,
                // Setters with no constructor prop go here.
                Init<RuleSlider>{ [](RuleSlider& slider) {
                    slider.setMaxPosition(1.0f);
                    slider.setRelativePosition(0.5f, false);
                } },
                Events{
                    [this, actionKey](GetStateEvent& event) {
                        Control* row = event.control.parent();
                        Control* comboboxControl = static_cast<Grids::Rt::RowContainer*>(row)->controlAtColumn(m_grid.columnByTag(Tag{ actionKey }));
                        ItemIndexValue itemIndex = static_cast<ComboBox*>(comboboxControl)->itemIndex();
                        event.state.enabled = itemIndex.has_value() and itemIndex.value();
                    },
                    [this](SliderChangeEvent& event) {
                        Grids::Rt::RowContainer& row = *static_cast<Grids::Rt::RowContainer*>((event.slider.parent()));
                        const Slider& slider = static_cast<const Slider&>(event.slider);
                        ComboboxTarget isLum = slider.tag<ComboboxTarget>();
                        ColorRule& rule = rowColorRule(row);
                        if (isLum == ComboboxTarget::Elevation)
                            rule.elevation.setNormalizedValue(slider.relativePosition());
                        else
                            rule.saturation.setNormalizedValue(slider.relativePosition());
                        row.invalidate();
                        storeViewState();
                        invalidatePreview();
                    }
                },
            }
        };
    }

    void ThemePage::acceptOperationText(ComboboxAcceptTextEvent& event)
    {
        Grids::Rt::RowContainer& row = *static_cast<Grids::Rt::RowContainer*>(event.comboBox().parent());
        const ComboboxTarget target = event.comboBox().tag<ComboboxTarget>();
        ColorRule& rule = rowColorRule(row);
        ColorRuleValue& ruleValue = target == ComboboxTarget::Elevation
            ? rule.elevation
            : rule.saturation;
        const std::optional<ColorRuleValue> typed = typedRule(event.accept, ruleValue);
        if (!typed.has_value())
            return;

        // Taken, so the list does not look it up.
        event.stopPropagation();
        ruleValue = typed.value();
        if (target == ComboboxTarget::Elevation)
            updateElevationSliderAndCombobox(row);
        else
            updateSaturationSliderAndCombobox(row);
        row.invalidate();
        invalidatePreview();
        storeViewState();
    }

    Grids::Dt::Row ThemePage::staticRow(UiElement element) const
    {
        if (uiElementOf(element).states.count() > 1ull)
            unreachable("an element painting two states or more is a group, not one row");
        return Grids::Dt::Row{
            Tag{ RowTag::StaticElement, element, UiElementState::Surface, 0 },
            Grids::Dt::Cell{ Tag{ ColumnTag::StaticName }, this, &ThemePage::elementNameCell },
            m_elementCells
        };
    }

    Row ThemePage::elementRow(UiElement element, UiElementState state) const
    {
        return Row{
            Tag{ RowTag::DynamicElement, element, state, 0 },
            Cell{ Tag{ ColumnTag::State }, this, &ThemePage::elementStateCell },
            m_elementCells
        };
    }

    Group ThemePage::elementGroup(UiElement element)
    {
        const UiElementStates& states = uiElementOf(element).states;
        if (states.count() < 2ull)
            unreachable("an element painting fewer than two states is one row, not a group");

        Group group{
            Collapsible::Expanded,
            Init<Grids::Rt::RowGroup>{ [this, element](Grids::Rt::RowGroup& rowGroup) {
                addFold(element, rowGroup.span());
            } },
            Span{
                Tag{ RowTag::Group, element, 0, 0 },
                Cell{ ColumnTag::Name, this, &ThemePage::elementNameCell },
                // The mark reads the theme and the click writes it. Nothing binds this control to
                // its row: the row is what the event arrives through, and a group's span is not
                // one of the rows updateGridControls walks.
                CellWith<Checkbox>{ ColumnTag::ElevationFlip,
                    HorizontalAlign::Center,
                    OnEvent{ [this](GetStateEvent& event) {
                        elementFlipState(event);
                    } },
                    OnEvent{ [this](ClickEvent& event) {
                        elementFlipClicked(event);
                    } }
                }
            }
        };

        // WALKED IN ENUM ORDER, WHICH IS THE ORDER PAINTEVENT APPLIES THE RULES IN. A group's rows
        // are the states its descriptor names, so the order is stated once, on UiElementState,
        // rather than by each element's own list.
        for (std::size_t i = 0ull; i != static_cast<std::size_t>(UiElementState::Count); ++i)
        {
            const UiElementState state = static_cast<UiElementState>(i);
            if (!states.has(state))
                continue;
            group.children.push_back(std::make_unique<Row>(elementRow(element, state)));
        }
        return group;
    }

    bool ThemePage::statesAbsoluteSurface(const Grids::Rt::RowBase& row)
    {
        return row.tag<2, 4, UiElementState>() == UiElementState::Surface
            and uiElementOf(row.tag<1, 4, UiElement>()).isWindowRoot;
    }

    void ThemePage::elementNameCell(Grids::GetCellTextEvent& event) const
    {
        const Grids::Rt::RowBase& row = event.row();
        // A group's name follows the mark its span leads with, and a static row keeps the same
        // room - a check mark's width and one cell padding - so every name starts in one line.
        if (row.tag<0, 4, RowTag>() != RowTag::Group)
        {
            const float cellPadding = row.descriptor().designCellMetrics().padding.x;
            event.text() << Space{ themeMetrics().checkMark.minSize.x + cellPadding };
        }
        event.text() << uiElementOf(row.tag<1, 4, UiElement>()).name;
    }

    void ThemePage::elementStateCell(Grids::GetCellTextEvent& event) const
    {
        event.text() << uiElementStateOf(event.row().tag<2, 4, UiElementState>()).name;
    }

    // The mode an element stands in: the preview's, flipped once for every element on the way
    // down to it that states a flip - the same accumulation PaintEvent makes down the control
    // tree. Every rule the element carries is read in it, its surface and its text included.
    ColorMode ThemePage::elementColorMode(OptionalUiElement element)
    {
        if (!element)
            return previewColorMode();

        const UiElementDescriptor& descriptor = uiElementOf(*element);
        ColorMode result = elementColorMode(descriptor.base);
        if (descriptor.rules and (editColors().*descriptor.rules).flip)
            result = flipped(result);
        return result;
    }

    const ColorRule& ThemePage::rowRule(UiElement element, UiElementState state)
    {
        const UiElementDescriptor& descriptor = uiElementOf(element);
        if (descriptor.rule)
            return editColors().*descriptor.rule;
        if (!descriptor.rules)
            unreachable("element names no colour rule");
        ColorRule ControlColorRules::* stateRule = uiElementStateOf(state).rule;
        return (editColors().*descriptor.rules).*stateRule;
    }

    // The colour an element resolves to, walked down from the bare surface through everything
    // it sits on. Only the surface rule of each is applied: a state is what a row's own slider
    // is about to change, and nothing below that row is in a state.
    Hsl ThemePage::elementColor(OptionalUiElement element)
    {
        if (!element)
            return previewColorMode() == ColorMode::Dark
                ? Hsl{ 0.0f, 0.0f, 0.0f }
                : Hsl{ 0.0f, 0.0f, 1.0f };

        const UiElementDescriptor& descriptor = uiElementOf(*element);
        Hsl result = elementColor(descriptor.base);
        const ColorMode elementMode = elementColorMode(element);
        if (descriptor.rule)
        {
            (editColors().*descriptor.rule).applyTo(result, 1.0f, editColors(), elementMode);
        }
        else if (descriptor.rules)
        {
            (editColors().*descriptor.rules).surface.applyTo(result, 1.0f, editColors(),
                elementMode);
        }
        return result;
    }

    // The ink an element's text starts from, walked down the same nesting the surfaces use. Only
    // text rules are applied on the way: what an element is painted on is a separate chain and
    // does not reach the ink. No element is the bare ink of the colour mode, with nothing
    // applied - a window root's own text rule is the first thing on the chain, exactly as its
    // surface rule is the first thing on the other one.
    Hsl ThemePage::elementTextColor(OptionalUiElement element)
    {
        if (!element)
            return bareInk();

        const UiElementDescriptor& descriptor = uiElementOf(*element);
        Hsl result = elementTextColor(descriptor.base);
        if (descriptor.rules)
        {
            const ControlColorRules& rules = editColors().*descriptor.rules;
            // The ink crosses with the element, before the element's own rule is applied to it -
            // the same order PaintEvent takes when the mode flips.
            if (rules.flip)
                result.luminosity = 1.0f - result.luminosity;
            rules.text.applyTo(result, 1.0f, editColors(), elementColorMode(element));
        }
        return result;
    }

    // The ink of the preview's colour mode before any rule: white at the dark end, black at the
    // light one, carrying the form surface's hue so that a rule raising saturation alone tints
    // toward the theme's own family.
    Hsl ThemePage::bareInk()
    {
        const ColorMode mode = previewColorMode();
        return {
            editColors().formSurface(mode).hue,
            0.0f,
            mode == ColorMode::Dark ? 1.0f : 0.0f
        };
    }

    // What the slider's own value is about to change: everything under the row's rule, plus every
    // part of that rule the slider does not set. The hue and the other value are already chosen,
    // and a ramp drawn without them says less than it could - a row whose base carries no
    // saturation of its own would draw its elevation ramp in grey, and its hue cell two columns
    // left would be saying otherwise.
    RuleBase ThemePage::rowRuleBase(const Grids::Rt::RowBase& row, RuleChannel channel)
    {
        const UiElement element = row.tag<1, 4, UiElement>();
        const UiElementState state = row.tag<2, 4, UiElementState>();
        // Every part of this is the edited theme's, read in the preview's colour mode. The grid
        // is painted in the application's own theme, which has nothing to do with what a ramp
        // here shows.
        //
        // Every row of an element is read in the one direction that element stands in, so a
        // flipped element's ramps draw the path the painter will actually walk. A shadow row is
        // read at the dark end with no floor, from black carrying the form surface's hue - the
        // walk BakedColors::windowShadow makes.
        const bool shadowState = state == UiElementState::Shadow;
        const ColorMode elementMode = shadowState ? ColorMode::Dark : elementColorMode(element);
        // A text rule changes the ink an element carries and not the surface it stands on, so it
        // is shown against the ink chain: what the element inherits, which for a window root is
        // the bare ink of the colour mode.
        const bool inkState = state == UiElementState::Text
            or state == UiElementState::ActiveText;
        Hsl result{};
        if (shadowState)
        {
            const float formHue = editColors().formSurface(previewColorMode()).hue;
            result = { formHue, 0.0f, 0.0f };
        }
        else if (state == UiElementState::Text)
            result = elementTextColor(uiElementOf(element).base);
        // Drawn against the ink this element's own text rule has already established, since that
        // is the ink it modifies.
        else if (state == UiElementState::ActiveText)
            result = elementTextColor(element);
        else
            result = elementColor(uiElementOf(element).base);
        // A state is painted over its element's surface colour, and a shadow outside it.
        if (state != UiElementState::Surface and !inkState and !shadowState)
        {
            if (ControlColorRules ThemeColors::* rules = uiElementOf(element).rules)
                (editColors().*rules).surface.applyTo(result, 1.0f, editColors(), elementMode);
        }

        const float luminosityFloor = shadowState ? k_noFloor : editColors().darkModeFloor;
        const ColorRule& rule = rowRule(element, state);
        result.hue = rule.hue.actualHue(editColors(), result.hue);
        if (channel != RuleChannel::Saturation)
            rule.saturation.applyTo(result.saturation, 1.0f, ColorMode::Dark, k_noFloor);
        if (channel != RuleChannel::Elevation)
            rule.elevation.applyTo(result.luminosity, 1.0f, elementMode, luminosityFloor);
        return { result, elementMode, luminosityFloor };
    }

    ColorRule& ThemePage::rowColorRule(Grids::Row& row)
    {
        const UiElementDescriptor& descriptor = uiElementOf(row.tag().get<1, 4, UiElement>());
        if (descriptor.rule)
            return editColors().*descriptor.rule;
        ColorRule ControlColorRules::* rule = uiElementStateOf(row.tag().get<2, 4, UiElementState>()).rule;
        if (!descriptor.rules or !rule)
            unreachable("row names no editable colour rule");
        return (editColors().*descriptor.rules).*rule;
    }

    ControlColorRules& ThemePage::rowColorRules(Grids::Rt::Row& row)
    {
        ControlColorRules ThemeColors::* rules = uiElementOf(row.tag().get<1, 4, UiElement>()).rules;
        if (!rules)
            unreachable("element carries no state rules");
        return editColors().*rules;
    }

    ControlColorRules& ThemePage::rowColorRulesOf(const Control& control)
    {
        return rowColorRules(*static_cast<Grids::Rt::RowContainer*>(control.parent()));
    }

    void ThemePage::elementFlipState(GetStateEvent& event)
    {
        event.state.selected = rowColorRulesOf(event.control).flip;
    }

    void ThemePage::elementFlipClicked(ClickEvent& event)
    {
        ControlColorRules& rules = rowColorRulesOf(*event.control);
        rules.flip = !rules.flip;
        event.control->invalidateState();
        // Every row of this group draws a ramp against the direction the element stands in, so
        // the whole grid is redrawn rather than the one cell that was clicked.
        m_grid.invalidate();
        invalidatePreview();
        storeViewState();
    }

    void ThemePage::updateGridControls()
    {
        // Every RowContainer in the grid and its sub-grids. Expanders and groups are
        // not containers, so they are stepped over and descended into.
        m_grid.forEachRow([this](Grids::Rt::RowContainer& row) {
            updateHueControl(row);
            updateSaturationSliderAndCombobox(row);
            updateElevationSliderAndCombobox(row);
            });
    }

    void ThemePage::updateRowControls(ComboBox& combobox, RuleSlider& slider, ColorRuleValue& ruleValue)
    {
        combobox.setItemIndex(static_cast<std::size_t>(ruleValue.operation()));
        // Showing the value the rule holds, which is not changing it. Left to trigger, the
        // slider's own handler would write the position straight back into that rule - a round
        // trip through the bar's resolution, and an edit the user did not make.
        slider.setRelativePosition(ruleValue.normalizedValue(), false);
    }

    // The rule a hue control edits is settled here rather than at construction: the cell was
    // built from a blueprint that names the Hue column, and only the row says which rule that
    // column stands for. Both the rule and the colours it reads palette hues from live in
    // m_editTheme, which outlives every control in the grid.
    void ThemePage::updateHueControl(Grids::Rt::RowContainer& row)
    {
        HueRuleControl& control = row.controlAtColumnAs<HueRuleControl>(m_grid.columnByTag(Tag{ ColumnTag::Hue }));
        control.bind(rowColorRule(row).hue, editColors(), m_onHueRuleChanged,
            !statesAbsoluteSurface(row));
    }

    void ThemePage::updateSaturationSliderAndCombobox(Grids::Rt::RowContainer& row)
    {
        ComboBox& combobox = row.controlAtColumnAs<ComboBox>(m_grid.columnByTag(Tag{ ColumnTag::SaturationAction }));
        RuleSlider& slider = row.controlAtColumnAs<RuleSlider>(m_grid.columnByTag(Tag{ ColumnTag::SaturationAmount }));
        ColorRuleValue& ruleValue = rowColorRule(row).saturation;
        updateRowControls(combobox, slider, ruleValue);
        // The base is asked for at paint time rather than handed over here: every other rule in
        // the theme can move it, and this row is not told when one does.
        slider.bind(ruleValue, RuleChannel::Saturation,
            [this, &row]() { return rowRuleBase(row, RuleChannel::Saturation); });
    }

    void ThemePage::updateElevationSliderAndCombobox(Grids::Rt::RowContainer& row)
    {
        ComboBox& combobox = row.controlAtColumnAs<ComboBox>(m_grid.columnByTag(Tag{ ColumnTag::ElevationOperation }));
        RuleSlider& slider = row.controlAtColumnAs<RuleSlider>(m_grid.columnByTag(Tag{ ColumnTag::ElevationAmount }));
        ColorRuleValue& ruleValue = rowColorRule(row).elevation;
        updateRowControls(combobox, slider, ruleValue);
        slider.bind(ruleValue, RuleChannel::Elevation,
            [this, &row]() { return rowRuleBase(row, RuleChannel::Elevation); });
    }

    void ThemePage::hueRuleChanged()
    {
        // The swatch in the row that changed, and every swatch that reads the same palette
        // entry, move together.
        m_grid.invalidate();
        m_darkModeFloorSlider.invalidate();
        invalidatePreview();
        storeViewState();
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
