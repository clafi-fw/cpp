module ClaFi.Controls.CodeBox;

import ClaFi.Controls.TextBox;
import ClaFi.Controls.Button;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;

import ClaFi.StdActions;

import ClaFi.Core.Syntax.Completion;
import ClaFi.Core.Syntax.Indent;
import ClaFi.Core.Syntax.Lines;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.History;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Scaler;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
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
            const Syntax::CompletionEntry* owner;   // a member's class - null elsewhere
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

        // Whether two sources are one row - an entry stated twice, a member its class states
        // twice. The text's own declarations are never folded: each stands for its own scope.
        [[nodiscard]] bool sameRow(const Syntax::Language& language, const RowSource& left,
            const RowSource& right)
        {
            return left.scope == right.scope
                && left.owner == right.owner
                && !left.declaration
                && !right.declaration
                && Syntax::completionNamesEqual(language, left.name, right.name);
        }

        [[nodiscard]] CompletionRow& rowOf(const ControlPtr& control)
        {
            return static_cast<CompletionRow&>(*control);
        }

        // Where the run of rows starting at `at` ends. The member rows of one name stand
        // together, ordered by name as they are, and every other row is a run of its own.
        [[nodiscard]] std::size_t endOfRun(const Syntax::Language& language,
            const ControlSpanC rows, const std::size_t at)
        {
            const CompletionRow& first = rowOf(rows[at]);
            std::size_t end = at + 1;
            if (first.scope() != CompletionScope::Member)
                return end;
            while (end != rows.size()
                && rowOf(rows[end]).scope() == CompletionScope::Member
                && Syntax::completionNamesEqual(language, rowOf(rows[end]).name(), first.name()))
                ++end;
            return end;
        }

        // How far from the first of the owners the row's owner stands - none for a row read off
        // another, and the first place for a row of the Global scope, which is read off none.
        [[nodiscard]] std::optional<std::size_t> rankOf(const CompletionRow& row,
            const Syntax::CompletionClasses& owners)
        {
            if (row.scope() == CompletionScope::Global)
                return std::size_t{ 0 };
            for (std::size_t rank = 0; rank != owners.size(); ++rank)
            {
                if (owners[rank] == row.owner())
                    return rank;
            }
            return std::nullopt;
        }

        [[nodiscard]] bool isStated(const Syntax::Language& language,
            const Syntax::CompletionNames& stated, const std::wstring_view name)
        {
            return std::ranges::any_of(stated, [&](const std::wstring_view other){
                return Syntax::completionNamesEqual(language, other, name);
            });
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
        const CompletionScope scope, const Syntax::CompletionEntry* owner,
        const std::wstring_view name, const Syntax::CompletionKind kind,
        const Syntax::CompletionEntry* entry, const Syntax::Declaration* declaration)
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
        m_owner{ owner },
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

        // Beside the list at this row's height, on a rect spanning it so either side stands clear
        // of the rows. In the list form's coordinates, which the row's own bounds are in.
        const FloatRect listBounds = form().content().boundsInForm();
        const float gap = scaler().scale(k_hintGap);
        const FloatRect row = boundsInForm();
        event.anchorRect = { listBounds.left - gap, row.top, listBounds.right + gap, row.bottom };
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
        const Syntax::CompletionEntries* entries, const Syntax::Declarations& declarations,
        const Syntax::TitleBlock* title)
    {
        setCurrentItem(nullptr);
        clearControls();

        std::vector<RowSource> sources;
        for (const Syntax::Declaration& declaration : declarations)
        {
            sources.push_back({
                CompletionScope::Global, nullptr, declaration.entry.name, declaration.entry.kind,
                &declaration.entry, &declaration
            });
        }
        for (const std::wstring_view keyword : language.keywords)
        {
            sources.push_back({
                CompletionScope::Global, nullptr, keyword, Syntax::CompletionKind::Keyword,
                nullptr, nullptr
            });
        }
        if (entries)
        {
            for (const Syntax::CompletionEntry& entry : *entries)
            {
                sources.push_back({
                    CompletionScope::Global, nullptr, entry.name, entry.kind, &entry, nullptr
                });
                // A member keeps its class, which is what a dot shows it by. The parent's members
                // stand under the parent, so a class repeats none of them.
                for (const Syntax::CompletionEntry& method : entry.methods)
                {
                    sources.push_back({
                        CompletionScope::Member, &entry, method.name, method.kind, &method,
                        nullptr
                    });
                }
                for (const Syntax::CompletionEntry& property : entry.properties)
                {
                    sources.push_back({
                        CompletionScope::Member, &entry, property.name, property.kind, &property,
                        nullptr
                    });
                }
            }
        }
        if (title)
        {
            // A value keeps its key, the way a member keeps its class.
            for (const Syntax::TitleKey& key : title->keys)
            {
                sources.push_back({
                    CompletionScope::Title, nullptr, key.entry.name, key.entry.kind, &key.entry,
                    nullptr
                });
                for (const Syntax::CompletionEntry& value : key.values)
                {
                    sources.push_back({
                        CompletionScope::Title, &key.entry, value.name, value.kind, &value, nullptr
                    });
                }
            }
        }

        // Stable, so of an entry stated twice the first statement is the one kept.
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
            add<CompletionRow>(*this, source.scope, source.owner, source.name, source.kind,
                source.entry, source.declaration);
        }
    }

    std::size_t CompletionStack::filter(const Syntax::Language& language,
        const CompletionScope scope, const Syntax::CompletionClasses& owners,
        const std::wstring_view typed, const std::size_t caret,
        const Syntax::CompletionNames& stated)
    {
        m_typed = typed;
        std::size_t shown = 0;
        CompletionRow* first = nullptr;
        const ControlSpanC rows = controls();
        std::size_t at = 0;
        while (at != rows.size())
        {
            // A run shows one row at most: the one read off the nearest owner.
            const std::size_t end = endOfRun(language, rows, at);
            CompletionRow* pick = nullptr;
            std::size_t pickRank = 0;
            for (std::size_t i = at; i != end; ++i)
            {
                CompletionRow& row = rowOf(rows[i]);
                const std::optional<std::size_t> rank = rankOf(row, owners);
                const bool eligible = row.scope() == scope
                    && row.inForceAt(caret)
                    && rank.has_value()
                    && !isStated(language, stated, row.name());
                if (eligible && (!pick || *rank < pickRank))
                {
                    pick = &row;
                    pickRank = *rank;
                }
            }
            const bool matches = pick && Syntax::completionMatches(language, pick->name(), typed);
            for (std::size_t i = at; i != end; ++i)
            {
                CompletionRow& row = rowOf(rows[i]);
                row.setVisible(matches && &row == pick);
            }
            if (matches)
            {
                ++shown;
                if (!first)
                    first = pick;
            }
            at = end;
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

    // ParameterHint

    void ParameterHint::setCall(const Syntax::CompletionEntry& entry,
        const Syntax::SignatureParameters& parameters, const std::size_t argument)
    {
        m_text.clear();
        if (parameters.names.empty())
        {
            m_text << L"No parameters";
        }
        else
        {
            const std::wstring_view signature = entry.signature;
            m_text << TextStyleId::Code;
            // A caret past the last parameter marks none, and the hint stands as it is.
            std::size_t at = parameters.bracket;
            if (argument < parameters.names.size())
            {
                const TextRange& name = parameters.names[argument];
                m_text << signature.substr(at, name.start - at)
                    << TextOp::PushBold << signature.substr(name.start, name.length)
                    << TextOp::PopBold;
                at = name.end();
            }
            m_text << signature.substr(at) << PopTextStyle{};
        }
        if (!entry.hint.empty())
            m_text << L"\n" << InkGrade::Muted << entry.hint << PopColor{};
        invalidate();
        invalidateFormAlign();
    }

    void ParameterHint::getText(GetTextEvent& event) const
    {
        event.text = m_text;
    }

    CalculatedDimensions ParameterHint::measureText(AlignEvent& event, ScaledDimensions asked,
        const Text& text)
    {
        asked.x = std::min(asked.x, Tooltip::k_lineWidth * event.scaleFactor());
        return WithTextLayout<FormControlBase>::measureText(event, asked, text);
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
        hideParameterHint();
    }

    void CodeBox::setTitleBlock(const Syntax::TitleBlock* block)
    {
        m_titleBlock = block;
        m_completionRowsStale = true;
        hideCompletion();
    }

    void CodeBox::showCompletion()
    {
        m_completionExplicit = true;
        requestCompletion(true);
    }

    void CodeBox::showParameterHint()
    {
        requestParameterHint(true);
    }

    void CodeBox::setIndentUnit(const Syntax::IndentUnit& value)
    {
        m_indentUnit = value;
        m_detectIndent = DetectIndent::No;
        m_detectedIndent.reset();
    }

    void CodeBox::setDetectIndent(const DetectIndent value)
    {
        m_detectIndent = value;
        m_detectedIndent.reset();
        if (value == DetectIndent::Yes)
            m_detectedIndent = Syntax::detectIndentUnit(m_layoutText.plainText());
    }

    void CodeBox::setIndentGuides(const IndentGuides value)
    {
        m_indentGuides = value;
        invalidate();
    }

    Syntax::IndentUnit CodeBox::indentUnitInUse() const
    {
        return m_detectedIndent.value_or(m_indentUnit);
    }

    void CodeBox::indentLines()
    {
        if (readOnly() == ReadOnly::Yes)
            return;
        shiftLines(false);
    }

    void CodeBox::outdentLines()
    {
        if (readOnly() == ReadOnly::Yes)
            return;
        shiftLines(true);
    }

    void CodeBox::reindentLines()
    {
        if (readOnly() == ReadOnly::Yes)
            return;
        ensureCaret();
        const TextRange selection = selectedRange();
        Syntax::IndentLines lines = sourceLines();
        // Nothing selected is the whole text.
        std::size_t first = 0;
        std::size_t last = lines.count() - 1;
        if (selection.length)
        {
            first = lines.lineAt(selection.start);
            last = lines.lineAt(selection.end());
            if (last > first && lines.start(last) == selection.end())
                --last;
        }
        applyIndentEdit(Syntax::reindentEdit(lines, first, last, indentUnitInUse(), selection,
            editProps()->caretOnLeft), EditKind::Replace,
            linesStepName(L"Reindent", last - first + 1));
    }

    void CodeBox::textTaken(const Text& text, const TextEdit* edit) const
    {
        m_declarationsStale = true;
        m_blocksStale = true;
        if (edit)
            m_lines.applyEdit(text.plainText(), edit->replaced, edit->insertedLength);
        else
            readWhole(text.plainText());
    }

    void CodeBox::textEdited()
    {
        TextBox::textEdited();
        m_tabLeaves = false;
        // A list up is about the name as it stood before the edit. A row being taken is the one
        // edit it does not follow: the list has just come down for it.
        if (completionShown() && !m_takingCompletion)
            requestCompletion(false);
        // A hint up is about the call as it stood before the edit.
        if (parameterHintShown())
            requestParameterHint(false);
    }

    void CodeBox::caretMoved()
    {
        TextBox::caretMoved();
        m_tabLeaves = false;
        // The focus arriving or leaving is a caret move, and it changes what Reindent is about.
        StdActions::reindent.invalidateState();
        // The caret has left the name the list was about - a click, an arrow key, the focus going.
        hideCompletion();
        // The hint follows the caret through the call and comes down where it leaves it - and
        // where the focus leaves the box, whatever the caret stands in.
        if (parameterHintShown())
        {
            if (isFocused())
                requestParameterHint(false);
            else
                hideParameterHint();
        }
    }

    void CodeBox::nestedKeyDown(KeyDownEvent& event)
    {
        // Every press settles whether its character belongs in the text before the character
        // arrives - see charPress.
        m_completionTakesChar = true;
        // Escape hands the key after it to the form, so a Tab right after one moves the focus on.
        // A modifier pressed on the way to that Tab is no key of its own.
        const bool modifierAlone = event.key == Keys::Shift || event.key == Keys::Ctrl
            || event.key == Keys::Alt;
        const bool tabLeaves = modifierAlone ? m_tabLeaves : std::exchange(m_tabLeaves, false);
        const bool writable = readOnly() == ReadOnly::No;
        const bool shown = completionShown();
        switch (event.key)
        {
        case Keys::Space:
            // Ctrl+Space is the list's and Ctrl+Shift+Space the hint's, only in a box that has
            // one: elsewhere each is a space.
            if (event.modifiers.ctrl && !event.modifiers.alt && writable)
            {
                const bool hint = event.modifiers.shift;
                const bool answered = hint
                    ? m_completion && m_lines.language().parameters
                    : m_completion || m_titleBlock;
                if (answered)
                {
                    event.handled = true;
                    m_completionTakesChar = false;
                    if (hint)
                        showParameterHint();
                    else
                        showCompletion();
                    return;
                }
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
            // Tab indents in a box that can be typed into; with Ctrl or Alt held, or right after
            // Escape, it is the form's. See Controls#indents
            if (event.key == Keys::Tab && writable && !tabLeaves && !event.modifiers.ctrl
                && !event.modifiers.alt)
            {
                event.handled = true;
                if (event.modifiers.shift)
                    outdentLines();
                else
                    insertStop();
                return;
            }
            break;

        case Keys::Escape:
            // The list first, then the hint: each press takes one window down.
            if (shown)
            {
                event.handled = true;
                hideCompletion();
                return;
            }
            if (parameterHintShown())
            {
                event.handled = true;
                hideParameterHint();
                return;
            }
            m_tabLeaves = true;
            break;

        case Keys::BackSpace:
            if (writable && event.modifiers.empty() && unindentAtCaret())
            {
                event.handled = true;
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
        if (readOnly() == ReadOnly::Yes)
            return;
        const wchar_t character = event.character();
        if (character == Keys::Return)
        {
            indentAfterBreak();
            return;
        }
        if (!std::iswprint(character))
            return;
        // A character that is no part of a name finishes the word before it, and a line's first
        // word finished is what places a closer.
        const bool names = Syntax::isNameChar(character, m_lines.language());
        if (!names)
            realignFinishedWord(true);
        // A name being typed, or a dot, is what lists names by itself - and the mark that opens a
        // title block's key, its equals sign and a comma. Asked after the edit, which is what the
        // list is about.
        const bool titleMark = m_titleBlock
            && (character == L'=' || character == L','
                || m_titleBlock->prefix.ends_with(character));
        if (names || character == L'.' || titleMark)
            requestCompletion(true);
        // A bracket opened is what shows a call's signature by itself.
        if (character == L'(' || character == L'[')
            requestParameterHint(true);
    }

    std::size_t CodeBox::rowHome(const CaretHit caretHit) const
    {
        const std::size_t rowStart = TextBox::rowHome(caretHit);
        const std::wstring_view text = this->text().plainText();
        // A row a long line wraps onto opens with no indent of its own.
        if (rowStart != 0 && (rowStart > text.size() || text[rowStart - 1] != L'\n'))
            return rowStart;
        std::size_t lineEnd = text.find(L'\n', rowStart);
        if (lineEnd == std::wstring_view::npos)
            lineEnd = text.size();
        const std::size_t caret = std::clamp(caretHit.pos, rowStart, lineEnd) - rowStart;
        return rowStart + Syntax::homeIndex(text.substr(rowStart, lineEnd - rowStart), caret);
    }

    void CodeBox::textPasted(const TextRange& pasted)
    {
        Syntax::IndentLines lines = sourceLines();
        applyIndentEdit(Syntax::pasteEdit(lines, pasted, indentUnitInUse()), EditKind::Automatic);
    }

    void CodeBox::editContextPopup(EditContextPopupEvent& event)
    {
        if (readOnly() == ReadOnly::No)
        {
            event.actions.push_back(nullptr);
            event.actions.push_back(&StdActions::reindent);
        }
        TextBox::editContextPopup(event);
    }

    DrawTextResult CodeBox::drawText(PaintEvent& event, const FloatRect& textBounds,
        const Text& text)
    {
        if (m_indentGuides == IndentGuides::Yes)
        {
            TextLayout& layout = syncedLayout(event.formContext(), textBounds, text);
            paintIndentGuides(event, layout,
                anchoredOrigin(textBounds, layout.calculatedDimensions(), textAnchor()));
        }
        return TextBox::drawText(event, textBounds, text);
    }

    void CodeBox::readWhole(const std::wstring_view text) const
    {
        m_completionRowsStale = true;
        m_declarationsStale = true;
        m_blocksStale = true;
        m_detectedIndent.reset();
        if (m_detectIndent == DetectIndent::Yes)
            m_detectedIndent = Syntax::detectIndentUnit(text);
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
        if ((!m_completion && !m_titleBlock) || readOnly() == ReadOnly::Yes)
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

        const CaretLine line = caretLine();
        // A title block's line lists its keys and values, typed or not. See Syntax#titleblock
        if (const std::optional<Syntax::TitlePlace> title = titlePlace(line))
        {
            const Syntax::CompletionClasses owners = { title->key ? &title->key->entry : nullptr };
            listCompletion(CompletionScope::Title, owners, title->stated, line, title->wordStart);
            return;
        }
        if (!m_completion)
        {
            hideCompletion();
            return;
        }

        const Syntax::Language& language = m_lines.language();
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
        // After a dot, the members of the subject's classes - and no list where they are not
        // known. See Syntax#completion
        Syntax::CompletionClasses classes;
        if (place.member)
        {
            classes = Syntax::completionMemberClasses(language, *m_completion, m_declarations,
                Syntax::completionSubject(language, line.text, m_tokens, place),
                line.start + line.caret);
            if (classes.empty())
            {
                hideCompletion();
                return;
            }
        }
        const CompletionScope scope = place.member
            ? CompletionScope::Member
            : CompletionScope::Global;
        listCompletion(scope, classes, {}, line, place.wordStart);
    }

    std::optional<Syntax::TitlePlace> CodeBox::titlePlace(const CaretLine& line) const
    {
        if (!m_titleBlock)
            return std::nullopt;
        return Syntax::completionTitlePlace(m_lines.language(), *m_titleBlock, sourceLines(),
            line.index, line.caret);
    }

    void CodeBox::listCompletion(const CompletionScope scope,
        const Syntax::CompletionClasses& owners, const Syntax::CompletionNames& stated,
        const CaretLine& line, const std::size_t wordStart)
    {
        const Syntax::Language& language = m_lines.language();
        ensureCompletionList();
        CompletionStack& rows = m_completionList->content().body();
        if (std::exchange(m_completionRowsStale, false))
            rows.rebuild(language, m_completion, m_declarations, m_titleBlock);
        const std::wstring_view typed = line.text.substr(wordStart, line.caret - wordStart);
        if (rows.filter(language, scope, owners, typed, line.start + line.caret, stated) == 0)
        {
            hideCompletion();
            return;
        }

        // Under the caret's line, from where the name starts, so the rows stand under the letters
        // they complete. In the form's coordinates, which a popup's placement is stated in.
        FloatRect anchor = caretRectInForm(line.start + line.caret);
        anchor.left = caretRectInForm(line.start + wordStart).left;
        m_completionList->setPlacement(FormPlacement::Bottom, anchor);
        if (!completionShown())
        {
            // Opened at its first row: where the list was left is not on screen to glide from.
            m_completionList->content().scrollToBegin();
            m_completionList->show();
        }
        // The hint takes the side of the line the list has left - see updateParameterHint.
        if (parameterHintShown())
            requestParameterHint(false);
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
        // And the hint over the call comes back to its own side of the line.
        if (parameterHintShown())
            requestParameterHint(false);
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
            // A pass that sized the list again may have moved it to the other side of the line.
            if (parameterHintShown())
                requestParameterHint(false);
        });
    }

    void CodeBox::takeCompletion(const CompletionRow& row)
    {
        const CaretLine line = caretLine();
        const bool title = row.scope() == CompletionScope::Title;
        std::size_t wordStart = 0;
        std::wstring written{ row.name() };
        bool valueFollows = false;
        if (title)
        {
            const std::optional<Syntax::TitlePlace> place = titlePlace(line);
            if (!place)
            {
                hideCompletion();
                return;
            }
            // A key is written with its equals sign where nothing follows the caret, and its
            // values are listed next; a value stands a blank off the sign or the comma before it.
            // See Controls#completionlist
            wordStart = place->wordStart;
            const bool lineEnds =
                line.text.find_first_not_of(L" \t", line.caret) == std::wstring_view::npos;
            const wchar_t before = wordStart != 0 ? line.text[wordStart - 1] : L'\0';
            if (!place->key && lineEnds)
            {
                written += L" = ";
                valueFollows = true;
            }
            else if (place->key && (before == L'=' || before == L','))
            {
                written = L" " + written;
            }
        }
        else
        {
            const Syntax::CompletionPlace place =
                Syntax::completionPlace(m_lines.language(), line.text, line.caret);
            wordStart = place.wordStart;
        }
        hideCompletion();
        // One edit, undone as one: the name as typed goes, and the row's own spelling stands in
        // its place with the caret after it.
        Text what{};
        what << L"Complete " << InkWell::accentInk() << row.name() << PopColor{};
        m_takingCompletion = true;
        setSelection(line.start + wordStart, line.start + line.caret);
        applyEdit(ensureCaret(), Text{ written }, EditKind::Replace, what);
        m_takingCompletion = false;
        if (valueFollows)
            requestCompletion(true);
        else if (!title)
            realignFinishedWord(false);
    }

    bool CodeBox::parameterHintShown() const
    {
        return m_parameterHint.has_value() && m_parameterHint->visible();
    }

    void CodeBox::requestParameterHint(const bool opening)
    {
        // The hint reads the list's signatures in the language's own way: a box with no list,
        // one nothing can be typed into, or a language that reads no signature, shows none.
        if (!m_completion || readOnly() == ReadOnly::Yes || !m_lines.language().parameters)
            return;
        m_parameterHintOpening = m_parameterHintOpening || opening;
        m_parameterHintRequest.start(MilliSeconds{ 0u });
    }

    void CodeBox::updateParameterHint()
    {
        // THE HINT STANDS ON A BOX THAT HAS BEEN LAID OUT - see updateCompletion.
        if (!form().contentAligned())
        {
            m_parameterHintWaitsOnAlign = true;
            return;
        }
        const bool opening = std::exchange(m_parameterHintOpening, false);
        if (!opening && !parameterHintShown())
            return;

        const Syntax::Language& language = m_lines.language();
        const Syntax::IndentLines lines = sourceLines();
        const CaretLine line = caretLine();
        // A bracket typed inside a comment or a string opens no call.
        m_lines.tokensOf(line.index, line.text, m_tokens);
        if (!parameterHintShown() && Syntax::completionBlocked(m_tokens, line.caret))
        {
            hideParameterHint();
            return;
        }
        const std::optional<Syntax::CompletionCall> call =
            Syntax::completionCall(language, lines, line.index, line.caret);
        if (!call)
        {
            hideParameterHint();
            return;
        }
        // The text's own routines are read as the hint opens; a hint up keeps its reading.
        if (!parameterHintShown())
            readDeclarations();
        const std::size_t caret = line.start + line.caret;
        const Syntax::CompletionEntry* entry = Syntax::completionCallee(language, *m_completion,
            m_declarations, call->callee, caret);
        if (!entry)
        {
            hideParameterHint();
            return;
        }
        // A routine with no parameters says so. Any other name whose signature lists none - a type
        // or a class cast, a property with no index, a variable - shows nothing.
        const Syntax::SignatureParameters parameters = language.parameters(entry->signature);
        const bool routine = entry->kind == Syntax::CompletionKind::Function
            || entry->kind == Syntax::CompletionKind::Procedure;
        if (parameters.names.empty() && !routine)
        {
            hideParameterHint();
            return;
        }

        ensureParameterHint();
        m_parameterHint->content().setCall(*entry, parameters, call->argument);
        // Above the caret's line, from where the call's bracket stands - on that line, which
        // may be one above. In the form's coordinates, which a placement is stated in. A list
        // that stands above the line, for want of room below it, leaves the hint the side below.
        FloatRect anchor = caretRectInForm(caret);
        anchor.left = caretRectInForm(lines.start(call->line) + call->bracket).left;
        const bool listAbove = completionShown() && m_completionList->window().standsAbove();
        m_parameterHint->setPlacement(listAbove ? FormPlacement::Bottom : FormPlacement::Top,
            anchor);
        if (!parameterHintShown())
            m_parameterHint->show();
    }

    void CodeBox::hideParameterHint()
    {
        // A request still pending would put the hint back up after it was taken down.
        m_parameterHintRequest.stop();
        m_parameterHintOpening = false;
        m_parameterHintWaitsOnAlign = false;
        if (parameterHintShown())
            m_parameterHint->hide();
    }

    void CodeBox::ensureParameterHint()
    {
        if (m_parameterHint.has_value())
            return;
        const ThemeMetrics& metrics = themeMetrics();
        m_parameterHint.emplace(
            appContext(),
            // A window the pointer goes through, standing beside the list's own without taking
            // its place as the popup on the box - which one window at a time is.
            WindowRole::Tooltip,
            this,
            Interactivity::None,
            UiElement::Tooltip,
            WordWrap::Yes,
            metrics.secondaryWindow,
            metrics.secondaryWindowShadow
        );
        // Sized by the signature it shows, which changes under it as the caret moves.
        m_parameterHint->setAutoFit(AutoFit::Yes);
        m_parameterHint->setDropdownClearance(2.0f);
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

    void CodeBox::connectIndentActions()
    {
        onGetActionState([this](GetActionStateEvent& event){
            if (&event.action == &StdActions::reindent)
                event.claim({ .enabled = readOnly() == ReadOnly::No });
        });
        onActionClick([this](ActionClickEvent& event){
            if (&event.action == &StdActions::reindent)
                reindentLines();
        });
    }

    TextRange CodeBox::selectedRange() const
    {
        const TextRange selection = editProps()->selRange;
        if (selection.start == k_maxSize)
            return { 0, 0 };
        const std::size_t size = text().plainText().size();
        const std::size_t start = std::min(selection.start, size);
        return { start, std::min(selection.length, size - start) };
    }

    Syntax::IndentLines CodeBox::sourceLines() const
    {
        // Asked first: it brings the layout, and with it the line states, into step with an edit
        // the layout has not been told about yet - see caretLine.
        std::ignore = caretLineColumn();
        return Syntax::IndentLines{ m_lines, text().plainText() };
    }

    void CodeBox::applyIndentEdit(const std::optional<Syntax::IndentEdit>& edit,
        const EditKind kind, const Text& what)
    {
        if (!edit.has_value())
            return;
        const EditSelection landing = {
            .range = edit->selection,
            .caretOnLeft = edit->caretOnLeft,
        };
        applyEdit(edit->replaced, Text{ edit->inserted }, kind, what, landing);
    }

    void CodeBox::insertStop()
    {
        const TextRange range = ensureCaret();
        const TextRange selection = selectedRange();
        const Syntax::IndentLines lines = sourceLines();
        const std::size_t line = lines.lineAt(selection.start);
        const std::size_t lineStart = lines.start(line);
        const std::wstring_view lineText = lines.text(line);
        // A selection reaching past its line, or holding the whole of one, moves lines instead.
        const bool withinLine = selection.end() <= lineStart + lineText.size();
        const bool wholeLine = selection.start == lineStart
            && selection.end() == lineStart + lineText.size();
        if (selection.length && (!withinLine || wholeLine))
        {
            shiftLines(false);
            return;
        }
        const std::size_t column = Syntax::columnAt(lineText, selection.start - lineStart);
        const std::wstring blanks = Syntax::stopInsertion(column, indentUnitInUse());
        applyEdit(range, Text{ blanks }, range.length ? EditKind::Replace : EditKind::Typing);
    }

    void CodeBox::shiftLines(const bool back)
    {
        ensureCaret();
        const TextRange selection = selectedRange();
        Syntax::IndentLines lines = sourceLines();
        const std::size_t first = lines.lineAt(selection.start);
        std::size_t last = lines.lineAt(selection.end());
        // A selection ending at a line's start takes nothing of that line.
        if (last > first && lines.start(last) == selection.end())
            --last;
        applyIndentEdit(Syntax::shiftEdit(lines, first, last, back, indentUnitInUse(), selection,
            editProps()->caretOnLeft), EditKind::Replace,
            linesStepName(back ? L"Outdent" : L"Indent", last - first + 1));
    }

    Text CodeBox::linesStepName(const std::wstring_view verb, const std::size_t lines)
    {
        Text result{};
        result << verb << L' ' << InkWell::accentInk() << lines;
        result << (lines == 1 ? L" line" : L" lines") << PopColor{};
        return result;
    }

    bool CodeBox::unindentAtCaret()
    {
        const TextRange selection = selectedRange();
        if (editProps()->selRange.start == k_maxSize || selection.length)
            return false;
        const CaretLine line = caretLine();
        const std::optional<TextRange> blanks =
            Syntax::unindentRange(line.text, line.caret, indentUnitInUse().width);
        if (!blanks.has_value() || blanks->length < 2)
            return false;
        applyEdit({ line.start + blanks->start, blanks->length }, Text{}, EditKind::DeletingBack);
        return true;
    }

    void CodeBox::realignFinishedWord(const bool byCharacter)
    {
        const CaretLine line = caretLine();
        const Syntax::Language& language = m_lines.language();
        bool finished = Syntax::firstWordEndsAt(language, line.text, line.caret);
        if (byCharacter && line.caret != 0)
            finished = finished || Syntax::firstWordEndsAt(language, line.text, line.caret - 1);
        if (!finished)
            return;
        Syntax::IndentLines lines = sourceLines();
        applyIndentEdit(Syntax::realignEdit(lines, line.index, indentUnitInUse(),
            line.start + line.caret), EditKind::Automatic);
    }

    void CodeBox::indentAfterBreak()
    {
        const CaretLine line = caretLine();
        if (line.index == 0 || line.caret != 0)
            return;
        Syntax::IndentLines lines = sourceLines();
        applyIndentEdit(Syntax::breakEdit(lines, line.index, indentUnitInUse()),
            EditKind::Automatic);
    }

    void CodeBox::paintIndentGuides(PaintEvent& event, TextLayout& layout, const FloatPoint origin)
    {
        // The layout's text is the one the line states stand beside, and syncing the layout
        // brought both up to the box's own.
        const std::wstring_view text = m_layoutText.plainText();
        if (text.empty())
            return;
        const Syntax::IndentLines lines{ m_lines, text };
        if (std::exchange(m_blocksStale, false))
            m_blocks = Syntax::sourceBlocks(m_lines.language(), lines);
        if (m_blocks.empty())
            return;

        // The lines in view, by where the viewport's top and bottom fall in the text.
        const FloatRect view = event.viewport();
        const std::size_t firstShown =
            m_lines.lineAt(layout.caretPos({ 0.0f, view.top - origin.y }).pos);
        const std::size_t lastShown =
            m_lines.lineAt(layout.caretPos({ 0.0f, view.bottom - origin.y }).pos);

        // The caret's block: of the blocks whose opener and closer lie either side of its line,
        // the last to open, since the blocks stand in the order they open.
        std::size_t caretBlock = m_blocks.size();
        if (editProps()->selRange.start != k_maxSize)
        {
            const std::size_t caretLine =
                m_lines.lineAt(std::min(editProps()->caretPos(), text.size()));
            for (std::size_t index = 0; index != m_blocks.size(); ++index)
            {
                const Syntax::SourceBlock& block = m_blocks[index];
                if (block.opener < caretLine && caretLine < block.closer)
                    caretBlock = index;
            }
        }

        // A guide is one device pixel wide, at the middle of its pixel column, which is where a
        // stroke that wide lands whole. The caret's block stands a grade over the rest.
        const Color guideInk = event.textRgb(InkGrade::Faint);
        const Color caretGuideInk = event.textRgb(InkGrade::Subtle);
        Graphics::Canvas& canvas = event.canvas();
        for (std::size_t index = 0; index != m_blocks.size(); ++index)
        {
            const Syntax::SourceBlock& block = m_blocks[index];
            if (block.closer <= firstShown || block.opener >= lastShown)
                continue;

            // At the opener line's first character, over the lines between the opener and the
            // closer that are blank or start right of it - not a middle word standing at the
            // block's own column, a private or an except.
            const std::wstring_view openerText = lines.text(block.opener);
            const std::size_t column = Syntax::indentColumns(openerText);
            const CaretHit openerHit = {
                m_lines.lineStart(block.opener) + Syntax::leadingBlanks(openerText),
                false,
            };
            const float x = std::floor(origin.x + layout.getCaretRect(openerHit).left) + 0.5f;
            const Color& ink = index == caretBlock ? caretGuideInk : guideInk;
            const auto crosses = [&](const std::size_t line){
                const std::wstring_view lineText = lines.text(line);
                return Syntax::leadingBlanks(lineText) == lineText.size()
                    || Syntax::indentColumns(lineText) > column;
            };

            std::size_t line = std::max(block.opener + 1, firstShown);
            const std::size_t end = std::min(block.closer, lastShown + 1);
            while (line < end)
            {
                if (!crosses(line))
                {
                    ++line;
                    continue;
                }
                const std::size_t runFirst = line;
                while (line < end && crosses(line))
                    ++line;
                const std::size_t runLast = line - 1;
                const CaretHit firstHit = { m_lines.lineStart(runFirst), false };
                const CaretHit lastHit = { m_lines.lineStart(runLast), false };
                const FloatPoint top = {
                    x,
                    std::round(origin.y + layout.getCaretRect(firstHit).top),
                };
                const FloatPoint bottom = {
                    x,
                    std::round(origin.y + layout.getCaretRect(lastHit).bottom),
                };
                canvas.drawLine(top, bottom, ink, 1.0f);
            }
        }
    }
}
