module ClaFi.Controls.CodeBox;

import ClaFi.Controls.TextBox;
import ClaFi.Controls.Button;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.Syntax.Completion;
import ClaFi.Core.Syntax.Lines;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        // The least room between a row's name and the kind printed at the end of its line.
        constexpr float k_kindGap = 12.0f;
        // How far the hint stands off the list's edge, in design units.
        constexpr float k_hintGap = 6.0f;

        // What a row is made of before it is a control.
        struct RowSource
        {
            CompletionScope scope;
            std::wstring_view name;
            Syntax::CompletionKind kind;
            const Syntax::CompletionEntry* entry;
            const Syntax::Declaration* declaration;   // null for a name the text does not declare
        };

        // Whether one source lists before another: the text's own declarations ahead of the
        // rest, the deeper of two ahead, and names in the language's order after that - so among
        // the rows a typed prefix names the nearest scope's stand first, and within a scope the
        // shortest, an exact match, does. Two declarations of one name in one scope stand
        // together, in the text's order.
        [[nodiscard]] bool listsBefore(const Syntax::Language& language, const RowSource& left,
            const RowSource& right)
        {
            if (left.scope != right.scope)
                return left.scope < right.scope;
            const bool leftDeclared = left.declaration != nullptr;
            const bool rightDeclared = right.declaration != nullptr;
            if (leftDeclared != rightDeclared)
                return leftDeclared;
            if (leftDeclared && left.declaration->depth != right.declaration->depth)
                return left.declaration->depth > right.declaration->depth;
            if (Syntax::completionNameLess(language, left.name, right.name))
                return true;
            if (Syntax::completionNameLess(language, right.name, left.name))
                return false;
            return leftDeclared && left.declaration->scope.start < right.declaration->scope.start;
        }

        // Whether two sources are one row - a member two classes name, an entry stated twice.
        // The text's own declarations are never folded: each stands for its own scope.
        [[nodiscard]] bool sameRow(const Syntax::Language& language, const RowSource& left,
            const RowSource& right)
        {
            return left.scope == right.scope
                && !left.declaration
                && !right.declaration
                && Syntax::completionNamesEqual(language, left.name, right.name);
        }

        [[nodiscard]] CompletionRow& rowOf(const ControlPtr& control)
        {
            return static_cast<CompletionRow&>(*control);
        }
    }

    DetectLanguageEvent::DetectLanguageEvent(const CodeBox& box, const std::wstring_view wholeText)
        :
        EventOf<const CodeBox>{ box },
        text{ wholeText }
    {
    }

    // CompletionRow

    CompletionRow::CompletionRow(const CreateParams& params, CompletionStack& stack,
        const CompletionScope scope, const std::wstring_view name,
        const Syntax::CompletionKind kind, const Syntax::CompletionEntry* entry,
        const Syntax::Declaration* declaration)
        :
        Button{ params,
            // The look of a dropdown's line: no surface at rest, and the current row on its own.
            params.themeMetrics().toolButton,
            UiElement::ToolButton,
            ShowSelectionOnSurface::Yes,
            HorizontalTextAnchor::Left,
            // Every row is as wide as the list, which is what the kind at the end of the line is
            // measured against.
            HorizontalAlign::Fill,
            // Reached by the pointer and never focused - the caret stays in the box.
            Interactivity::MouseOnly
        },
        m_stack{ stack },
        m_scope{ scope },
        m_name{ name },
        m_kind{ kind },
        m_entry{ entry },
        m_declaration{ declaration }
    {
    }

    bool CompletionRow::inForceAt(const std::size_t pos) const
    {
        if (!m_declaration)
            return true;
        const TextRange& scope = m_declaration->scope;
        return scope.start <= pos && pos <= scope.end();
    }

    void CompletionRow::getText(GetTextEvent& event) const
    {
        // The part typed so far in the accent, the rest in the text's own ink, and the kind muted
        // at the end of the line - a flex space lands every kind on the list's right edge.
        const std::size_t typed = m_stack.typed().size();
        const std::wstring_view name = m_name;
        if (typed)
            event.text << InkWell::accentInk() << name.substr(0, typed) << PopColor{};
        event.text << name.substr(typed);
        event.text << FlexSpace{ k_kindGap } << InkWell::textInk(InkGrade::Muted)
            << Syntax::completionKindName(m_kind) << PopColor{};
    }

    void CompletionRow::nestedGetTooltip(GetTooltipEvent& event)
    {
        // A keyword, or an entry that says no more than its name, shows no hint.
        if (!m_entry)
            return;
        if (m_entry->signature.empty() && m_entry->hint.empty())
            return;
        const std::wstring& signature = m_entry->signature.empty()
            ? m_entry->name
            : m_entry->signature;

        // Beside the list at this row's height, so the rows stay in view. In the coordinates of
        // the list's form, which the row's own bounds are in.
        const float edge = form().content().boundsInForm().right + scaler().scale(k_hintGap);
        const FloatRect row = boundsInForm();
        event.anchorRect = { edge, row.top, edge, row.bottom };
        event.placement = FormPlacement::Right;
        event.text << TextStyleId::Code << signature << PopTextStyle{};
        if (!m_entry->hint.empty())
            event.text << L"\n" << InkGrade::Muted << m_entry->hint << PopColor{};
    }

    void CompletionRow::nestedClick(ClickEvent&)
    {
        m_stack.box().takeCompletion(*this);
    }

    // CompletionStack

    CompletionStack::CompletionStack(const CreateParams& params, CodeBox& box)
        :
        StackPanel{ params,
            Orientation::Vertical,
            // A SCROLLED BODY KEEPS THE SIZE IT MEASURED - see ComboBoxDropdownStack.
            WordWrap::No
        },
        m_box{ box }
    {
    }

    void CompletionStack::rebuild(const Syntax::Language& language,
        const Syntax::CompletionEntries* entries, const Syntax::Declarations& declarations)
    {
        setCurrentItem(nullptr);
        clearControls();

        std::vector<RowSource> sources;
        for (const Syntax::Declaration& declaration : declarations)
        {
            sources.push_back({
                CompletionScope::Global, declaration.entry.name, declaration.entry.kind,
                &declaration.entry, &declaration
            });
        }
        for (const std::wstring_view keyword : language.keywords)
        {
            sources.push_back({
                CompletionScope::Global, keyword, Syntax::CompletionKind::Keyword, nullptr, nullptr
            });
        }
        if (entries)
        {
            for (const Syntax::CompletionEntry& entry : *entries)
            {
                sources.push_back({
                    CompletionScope::Global, entry.name, entry.kind, &entry, nullptr
                });
                // A member is listed whatever class it is read off - see the note. The parent's
                // members stand in the list under the parent, so a class repeats none of them.
                for (const Syntax::CompletionEntry& method : entry.methods)
                {
                    sources.push_back({
                        CompletionScope::Member, method.name, method.kind, &method, nullptr
                    });
                }
                for (const Syntax::CompletionEntry& property : entry.properties)
                {
                    sources.push_back({
                        CompletionScope::Member, property.name, property.kind, &property, nullptr
                    });
                }
            }
        }

        // Stable, so of two classes naming one member the first in the list is the one kept.
        std::ranges::stable_sort(sources, [&](const RowSource& left, const RowSource& right){
            return listsBefore(language, left, right);
        });
        const auto duplicates = std::ranges::unique(sources,
            [&](const RowSource& left, const RowSource& right){
                return sameRow(language, left, right);
            });
        sources.erase(duplicates.begin(), duplicates.end());

        for (const RowSource& source : sources)
        {
            add<CompletionRow>(*this, source.scope, source.name, source.kind, source.entry,
                source.declaration);
        }
    }

    std::size_t CompletionStack::filter(const Syntax::Language& language,
        const CompletionScope scope, const std::wstring_view typed, const std::size_t caret)
    {
        m_typed = typed;
        std::size_t shown = 0;
        CompletionRow* first = nullptr;
        for (const ControlPtr& control : controls())
        {
            CompletionRow& row = rowOf(control);
            const bool matches = row.scope() == scope
                && row.inForceAt(caret)
                && Syntax::completionMatches(language, row.name(), typed);
            row.setVisible(matches);
            if (!matches)
                continue;
            ++shown;
            if (!first)
                first = &row;
        }
        setCurrentRow(first);
        return shown;
    }

    void CompletionStack::moveCurrent(const ScrollDirection direction)
    {
        const std::vector<CompletionRow*> shown = shownRows();
        if (shown.empty())
            return;

        const auto at = std::ranges::find(shown, currentRow());
        if (at == shown.end())
        {
            setCurrentRow(shown.front());
            return;
        }
        if (direction == ScrollDirection::ToEnd)
        {
            if (std::next(at) != shown.end())
                setCurrentRow(*std::next(at));
            return;
        }
        if (at != shown.begin())
            setCurrentRow(*std::prev(at));
    }

    void CompletionStack::moveCurrentByPage(const ScrollDirection direction)
    {
        const std::vector<CompletionRow*> shown = shownRows();
        if (shown.empty())
            return;

        const auto at = std::ranges::find(shown, currentRow());
        if (at == shown.end())
        {
            setCurrentRow(shown.front());
            return;
        }

        // The rows from the current one to the one it lands on fill the view at most, so the row
        // left current is still in sight - the step FocusNavigator pages any items view by.
        const float view = windowInForm().height();
        const bool forward = direction == ScrollDirection::ToEnd;
        std::size_t target = static_cast<std::size_t>(at - shown.begin());
        float filled = shown[target]->boundsInForm().height();
        while (forward ? target + 1 < shown.size() : target > 0)
        {
            const std::size_t next = forward ? target + 1 : target - 1;
            filled += shown[next]->boundsInForm().height();
            if (filled > view)
                break;
            target = next;
        }
        setCurrentRow(shown[target]);
    }

    CompletionRow* CompletionStack::currentRow() const
    {
        return static_cast<CompletionRow*>(currentItem());
    }

    void CompletionStack::showCurrentHint()
    {
        // A pass over a list that is down places nothing anyone can see.
        if (!form().visible())
            return;
        if (CompletionRow* row = currentRow())
            form().tooltip().showRightNow(*row);
        else
            Tooltip::stopAndHide();
    }

    bool CompletionStack::defaultCanFocusItem(Control& value)
    {
        return value.parent() == this;
    }

    void CompletionStack::getControlState(GetStateEvent& event) const
    {
        StackPanel::getControlState(event);
        if (event.propagationStopped())
            return;
        event.state.selected = &event.control == currentItem();
        event.stopPropagation();
    }

    std::vector<CompletionRow*> CompletionStack::shownRows() const
    {
        std::vector<CompletionRow*> shown;
        for (const ControlPtr& control : controls())
        {
            if (control->visible())
                shown.push_back(&rowOf(control));
        }
        return shown;
    }

    void CompletionStack::setCurrentRow(CompletionRow* row)
    {
        setCurrentItem(row);
        if (!row)
            return;
        // Into view on the pass that follows, which is also the pass the hint is put up from -
        // the hint is placed off the row's bounds, and those are the pass's to give.
        row->scrollIntoViewOnAlign();
        invalidateFormAlign();
    }

    // CodeBox

    void CodeBox::setLanguage(const Syntax::Language& language)
    {
        m_language = language;
        m_detectLanguage = DetectLanguage::No;
        // Read from the text the layout holds, which is the text the line states stand beside: a
        // text the box has since been handed reaches both together, through textTaken.
        readWhole(m_layoutText.plainText());
        invalidate();
    }

    void CodeBox::setDetectLanguage(const DetectLanguage value)
    {
        m_detectLanguage = value;
        readWhole(m_layoutText.plainText());
        invalidate();
    }

    void CodeBox::setCompletion(const Syntax::CompletionEntries* entries)
    {
        m_completion = entries;
        m_completionRowsStale = true;
        hideCompletion();
    }

    void CodeBox::showCompletion()
    {
        m_completionExplicit = true;
        requestCompletion(true);
    }

    void CodeBox::textTaken(const Text& text, const TextEdit* edit) const
    {
        m_declarationsStale = true;
        if (edit)
            m_lines.applyEdit(text.plainText(), edit->replaced, edit->insertedLength);
        else
            readWhole(text.plainText());
    }

    void CodeBox::textEdited()
    {
        TextBox::textEdited();
        // A list up is about the name as it stood before the edit. A row being taken is the one
        // edit it does not follow: the list has just come down for it.
        if (completionShown() && !m_takingCompletion)
            requestCompletion(false);
    }

    void CodeBox::caretMoved()
    {
        TextBox::caretMoved();
        // The caret has left the name the list was about - a click, an arrow key, the focus going.
        hideCompletion();
    }

    void CodeBox::nestedKeyDown(KeyDownEvent& event)
    {
        // Every press settles whether its character belongs in the text before the character
        // arrives - see charPress.
        m_completionTakesChar = true;
        const bool shown = completionShown();
        switch (event.key)
        {
        case Keys::Space:
            // Ctrl+Space is the list's only in a box that has one: elsewhere it is a space.
            if (event.modifiers.ctrl && !event.modifiers.alt && m_completion
                && readOnly() == ReadOnly::No)
            {
                event.handled = true;
                m_completionTakesChar = false;
                showCompletion();
                return;
            }
            break;

        case Keys::Return:
        case Keys::Tab:
            if (shown)
            {
                if (const CompletionRow* row = m_completionList->content().body().currentRow())
                {
                    event.handled = true;
                    m_completionTakesChar = false;
                    takeCompletion(*row);
                    return;
                }
            }
            break;

        case Keys::Escape:
            if (shown)
            {
                event.handled = true;
                hideCompletion();
                return;
            }
            break;

        case Keys::Up:
        case Keys::Down:
            if (shown && event.modifiers.empty())
            {
                event.handled = true;
                m_completionList->content().body().moveCurrent(
                    event.key == Keys::Up ? ScrollDirection::ToBegin : ScrollDirection::ToEnd);
                return;
            }
            break;

        case Keys::Prior:
        case Keys::Next:
            if (shown && event.modifiers.empty())
            {
                event.handled = true;
                m_completionList->content().body().moveCurrentByPage(
                    event.key == Keys::Prior ? ScrollDirection::ToBegin : ScrollDirection::ToEnd);
                return;
            }
            break;
        }
        TextBox::nestedKeyDown(event);
    }

    void CodeBox::charPress(CharPressEvent& event)
    {
        if (!m_completionTakesChar)
        {
            m_completionTakesChar = true;
            return;
        }
        TextBox::charPress(event);
        // A name being typed, or a dot, is what lists names by itself. Asked after the edit,
        // which is what the list is about.
        const wchar_t character = event.character();
        const bool names = character == L'.' || Syntax::isNameChar(character, m_lines.language());
        if (names)
            requestCompletion(true);
    }

    void CodeBox::readWhole(const std::wstring_view text) const
    {
        m_completionRowsStale = true;
        m_declarationsStale = true;
        if (m_detectLanguage == DetectLanguage::No)
        {
            m_lines.reset(m_language, text);
            return;
        }

        DetectLanguageEvent event{ *this, text };
        emitEvent(event);
        m_lines.reset(event.language, text);
    }

    void CodeBox::paragraphColors(const std::size_t paragraph,
        const std::wstring_view paragraphText, std::vector<ColorSpan>& out) const
    {
        m_lines.tokensOf(paragraph, paragraphText, m_tokens);

        // A kind with no ink of its own is drawn in the text's, and states no span for it.
        const Ink textInk{};
        for (const Syntax::Token& token : m_tokens)
        {
            const Ink& ink = m_inks[token.kind];
            if (ink == textInk)
                continue;

            // Two tokens in one ink standing side by side are one span.
            if (!out.empty() && out.back().range.end() == token.range.start
                && out.back().value == ColorDef{ ink })
            {
                out.back().range.length += token.range.length;
                continue;
            }
            out.push_back({ token.range, ink });
        }
    }

    bool CodeBox::completionShown() const
    {
        // The window's own state rather than isDroppedDown, which any popup on the box answers -
        // the context menu among them.
        return m_completionList.has_value() && m_completionList->visible();
    }

    void CodeBox::requestCompletion(const bool opening)
    {
        // A box with nothing to complete from, or one nothing can be typed into, lists nothing.
        if (!m_completion || readOnly() == ReadOnly::Yes)
            return;
        m_completionOpening = m_completionOpening || opening;
        m_completionRequest.start(MilliSeconds{ 0u });
    }

    void CodeBox::updateCompletion()
    {
        // THE LIST STANDS ON A BOX THAT HAS BEEN LAID OUT - see
        // InPlaceEditRoot::updateSuggestions.
        if (!form().contentAligned())
        {
            m_completionWaitsOnAlign = true;
            return;
        }
        const bool opening = std::exchange(m_completionOpening, false);
        const bool everything = std::exchange(m_completionExplicit, false);
        if (!opening && !completionShown())
            return;

        const Syntax::Language& language = m_lines.language();
        const CaretLine line = caretLine();
        const Syntax::CompletionPlace place =
            Syntax::completionPlace(language, line.text, line.caret);
        const std::wstring_view typed = place.typed(line.text);
        m_lines.tokensOf(line.index, line.text, m_tokens);
        // A name being typed, what follows a dot, or everything on demand - and none of them
        // inside a comment or a string.
        const bool lists = (everything || place.member || !typed.empty())
            && !Syntax::completionBlocked(m_tokens, line.caret);
        if (!lists)
        {
            hideCompletion();
            return;
        }

        // The text's own names are read as the list opens, and again on demand; a list up keeps
        // its reading, since the keys that narrow it change no declaration it is about.
        if (!completionShown() || everything)
            readDeclarations();
        ensureCompletionList();
        CompletionStack& rows = m_completionList->content().body();
        if (std::exchange(m_completionRowsStale, false))
            rows.rebuild(language, m_completion, m_declarations);
        const CompletionScope scope = place.member
            ? CompletionScope::Member
            : CompletionScope::Global;
        if (rows.filter(language, scope, typed, line.start + line.caret) == 0)
        {
            hideCompletion();
            return;
        }

        // Under the caret's line, from where the name starts, so the rows stand under the letters
        // they complete. In the form's coordinates, which a popup's placement is stated in.
        FloatRect anchor = caretRectInForm(line.start + place.caret);
        anchor.left = caretRectInForm(line.start + place.wordStart).left;
        m_completionList->setPlacement(FormPlacement::Bottom, anchor);
        if (!completionShown())
        {
            // Opened at its first row: where the list was left is not on screen to glide from.
            m_completionList->content().scrollToBegin();
            m_completionList->show();
        }
    }

    void CodeBox::readDeclarations()
    {
        if (!std::exchange(m_declarationsStale, false))
            return;
        const Syntax::DeclarationReader reader = m_lines.language().declarations;
        Syntax::Declarations read = reader ? reader(text().plainText()) : Syntax::Declarations{};
        if (read == m_declarations)
            return;
        m_declarations = std::move(read);
        m_completionRowsStale = true;
    }

    void CodeBox::hideCompletion()
    {
        // A request still pending would put the list back up after it was taken down.
        m_completionRequest.stop();
        m_completionOpening = false;
        m_completionExplicit = false;
        m_completionWaitsOnAlign = false;
        if (!completionShown())
            return;
        // The hint beside the current row goes with the list.
        Tooltip::stopAndHide();
        m_completionList->close();
    }

    void CodeBox::ensureCompletionList()
    {
        if (m_completionList.has_value())
            return;
        const ThemeMetrics& metrics = themeMetrics();
        m_completionList.emplace(
            appContext(),
            WindowRole::Menu,
            this,
            HostProps{
                metrics.secondaryWindow,
                metrics.secondaryWindowShadow,
                UiElement::Section,
                // A list taller than its room scrolls - see ComboBox::showDropdown.
                ScrollBars::Auto,
                // Ten rows, and a bar past that.
                MaxSize{
                    k_maxFloat,
                    k_completionRows * metrics.toolButton.minSize.y + 2.0f * k_completionPadding
                },
                Padding{ k_completionPadding },
                // Nothing in the list takes the focus: the rows are reached by the pointer, and
                // by the keys the box hands on.
                Interactivity::MouseOnly
            },
            BodyProps{ *this }
        );
        // Sized by what is listed, which changes under it as the text does; and hidden rather
        // than destroyed when nothing is listed, since the next character may list something
        // again.
        m_completionList->setAutoFit(AutoFit::Yes);
        m_completionList->setCloseAction(CloseAction::Hide);
        m_completionList->setDropdownClearance(2.0f);
        m_completionList->setMinWidth(k_completionMinWidth);
        // The hint is placed off the current row's bounds, which the pass that lays the rows out
        // gives, so it is put up from that pass.
        m_listAligned = m_completionList->onAligned([this](FormAlignedEvent&){
            m_completionList->content().body().showCurrentHint();
        });
        m_formAligned = form().onAligned([this](FormAlignedEvent&){
            if (std::exchange(m_completionWaitsOnAlign, false))
                m_completionRequest.start(MilliSeconds{ 0u });
        });
    }

    void CodeBox::takeCompletion(const CompletionRow& row)
    {
        const CaretLine line = caretLine();
        const Syntax::CompletionPlace place =
            Syntax::completionPlace(m_lines.language(), line.text, line.caret);
        hideCompletion();
        // One edit, undone as one: the name as typed goes, and the row's own spelling stands in
        // its place with the caret after it.
        m_takingCompletion = true;
        setSelection(line.start + place.wordStart, line.start + place.caret);
        replaceSelectedText(row.name());
        m_takingCompletion = false;
    }

    CodeBox::CaretLine CodeBox::caretLine() const
    {
        // Asked first: it brings the layout, and with it the line states, into step with an edit
        // the layout has not been told about yet - see TextBox::caretLineColumn.
        const TextLineColumn lineColumn = caretLineColumn();
        const std::wstring_view text = this->text().plainText();
        const std::size_t caret = std::min(editProps()->caretPos(), text.size());
        const std::size_t start = caret - (lineColumn.column - 1);
        std::size_t end = text.find(L'\n', caret);
        if (end == std::wstring_view::npos)
            end = text.size();
        return {
            .index = lineColumn.line - 1,
            .start = start,
            .text = text.substr(start, end - start),
            .caret = caret - start,
        };
    }
}
