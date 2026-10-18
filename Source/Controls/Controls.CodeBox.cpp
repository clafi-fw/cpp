module ClaFi.Controls.CodeBox;

import ClaFi.Controls.DropList;
import ClaFi.Controls.TextBox;

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

        // Where the run of items starting at `at` ends. The member items of one name stand
        // together, ordered by name as they are, and every other item is a run of its own.
        [[nodiscard]] std::size_t endOfRun(const Syntax::Language& language,
            const CompletionItems& items, const std::size_t at)
        {
            const CompletionItem& first = items[at];
            std::size_t end = at + 1;
            if (first.scope != CompletionScope::Member)
                return end;
            while (end != items.size()
                && items[end].scope == CompletionScope::Member
                && Syntax::completionNamesEqual(language, items[end].name, first.name))
                ++end;
            return end;
        }

        // How far from the first of the owners the item's owner stands - none for an item read
        // off another, and the first place for an item of the Global scope, which is read off none.
        [[nodiscard]] std::optional<std::size_t> rankOf(const CompletionItem& item,
            const Syntax::CompletionClasses& owners)
        {
            if (item.scope == CompletionScope::Global)
                return std::size_t{ 0 };
            for (std::size_t rank = 0; rank != owners.size(); ++rank)
            {
                if (owners[rank] == item.owner)
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

    // CompletionItem

    bool CompletionItem::inForceAt(const std::size_t pos) const
    {
        if (!declaration)
            return true;
        const TextRange& range = declaration->scope;
        return range.start <= pos && pos <= range.end();
    }

    // ParameterTooltip

    void ParameterTooltip::setCall(const Syntax::CompletionEntry& entry,
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
            // A caret past the last parameter marks none, and the tooltip stands as it is.
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

    void ParameterTooltip::getText(GetTextEvent& event) const
    {
        event.text = m_text;
    }

    ScaledSize ParameterTooltip::measureText(AlignEvent& event, ScaledSize asked,
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
        hideParameterTooltip();
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

    void CodeBox::showParameterTooltip()
    {
        requestParameterTooltip(true);
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

    ScrollMetrics CodeBox::scrollMetrics() const
    {
        ScrollMetrics result = TextBox::scrollMetrics();
        result.pastEnd = readOnly() == ReadOnly::No;
        return result;
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
            m_lines.applyEdit(text.plainText(), *edit);
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
        // A tooltip up is about the call as it stood before the edit.
        if (parameterTooltipShown())
            requestParameterTooltip(false);
    }

    void CodeBox::caretMoved()
    {
        TextBox::caretMoved();
        m_tabLeaves = false;
        // The focus arriving or leaving is a caret move, and it changes what Reindent is about.
        StdActions::reindent.invalidateState();
        // The caret has left the name the list was about - a click, an arrow key, the focus going.
        hideCompletion();
        // The tooltip follows the caret through the call and comes down where it leaves it - and
        // where the focus leaves the box, whatever the caret stands in.
        if (parameterTooltipShown())
        {
            if (isFocused())
                requestParameterTooltip(false);
            else
                hideParameterTooltip();
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
            // Ctrl+Space is the list's and Ctrl+Shift+Space the tooltip's, only in a box that has
            // one: elsewhere each is a space.
            if (event.modifiers.ctrl && !event.modifiers.alt && writable)
            {
                const bool tooltip = event.modifiers.shift;
                const bool answered = tooltip
                    ? m_completion && m_lines.language().parameters
                    : m_completion || m_titleBlock;
                if (answered)
                {
                    event.handled = true;
                    m_completionTakesChar = false;
                    if (tooltip)
                        showParameterTooltip();
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
                if (const std::optional<std::size_t> line = m_completionList->current())
                {
                    event.handled = true;
                    m_completionTakesChar = false;
                    takeCompletion(m_completionItems[m_completionListed[*line]]);
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
            // The list first, then the tooltip: each press takes one window down.
            if (shown)
            {
                event.handled = true;
                hideCompletion();
                return;
            }
            if (parameterTooltipShown())
            {
                event.handled = true;
                hideParameterTooltip();
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
                m_completionList->moveCurrent(
                    event.key == Keys::Up ? ScrollDirection::ToBegin : ScrollDirection::ToEnd);
                return;
            }
            break;

        case Keys::Prior:
        case Keys::Next:
            if (shown && event.modifiers.empty())
            {
                event.handled = true;
                m_completionList->moveCurrentByPage(
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
            requestParameterTooltip(true);
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

    void CodeBox::adjustContextPopup(AdjustContextPopupEvent& event)
    {
        if (readOnly() == ReadOnly::No)
        {
            event.actions.push_back(nullptr);
            event.actions.push_back(&StdActions::reindent);
        }
        TextBox::adjustContextPopup(event);
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
        return m_completionList.has_value() && m_completionList->shown();
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
        // **The list stands on a box that has been laid out** - see
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
        // known. See Syntax#completionmemberclasses
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
        if (std::exchange(m_completionRowsStale, false))
            buildCompletionItems(language);
        const std::wstring_view typed = line.text.substr(wordStart, line.caret - wordStart);
        filterCompletionItems(language, scope, owners, typed, line.start + line.caret, stated);
        if (m_completionListed.empty())
        {
            hideCompletion();
            return;
        }
        DropListLines lines;
        lines.reserve(m_completionListed.size());
        for (const std::size_t item : m_completionListed)
            lines.push_back(completionLine(m_completionItems[item], typed.size()));

        // Under the caret's line, from where the name starts, so the rows stand under the letters
        // they complete, the first current - an exact match, where there is one. In the form's
        // coordinates, which a popup's placement is stated in.
        FloatRect anchor = caretRectInForm(line.start + line.caret);
        anchor.left = caretRectInForm(line.start + wordStart).left;
        m_completionList->show(std::move(lines), 0, FormPlacement::BottomLeft, anchor);
        // The tooltip takes the side of the line the list has left - see updateParameterTooltip.
        if (parameterTooltipShown())
            requestParameterTooltip(false);
    }

    void CodeBox::buildCompletionItems(const Syntax::Language& language)
    {
        std::vector<RowSource> sources;
        for (const Syntax::Declaration& declaration : m_declarations)
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
        if (m_completion)
        {
            for (const Syntax::CompletionEntry& entry : *m_completion)
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
        if (m_titleBlock)
        {
            // A value keeps its key, the way a member keeps its class.
            for (const Syntax::TitleKey& key : m_titleBlock->keys)
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

        m_completionItems.clear();
        m_completionItems.reserve(sources.size());
        for (const RowSource& source : sources)
        {
            m_completionItems.push_back({
                .scope = source.scope,
                .owner = source.owner,
                .name = std::wstring{ source.name },
                .kind = source.kind,
                .entry = source.entry,
                .declaration = source.declaration
            });
        }
    }

    void CodeBox::filterCompletionItems(const Syntax::Language& language,
        const CompletionScope scope, const Syntax::CompletionClasses& owners,
        const std::wstring_view typed, const std::size_t caret,
        const Syntax::CompletionNames& stated)
    {
        m_completionListed.clear();
        const CompletionItems& items = m_completionItems;
        std::size_t at = 0;
        while (at != items.size())
        {
            // A run lists one item at most: the one read off the nearest owner.
            const std::size_t end = endOfRun(language, items, at);
            std::optional<std::size_t> pick{};
            std::size_t pickRank = 0;
            for (std::size_t i = at; i != end; ++i)
            {
                const CompletionItem& item = items[i];
                const std::optional<std::size_t> rank = rankOf(item, owners);
                const bool eligible = item.scope == scope
                    && item.inForceAt(caret)
                    && rank.has_value()
                    && !isStated(language, stated, item.name);
                if (eligible && (!pick.has_value() || *rank < pickRank))
                {
                    pick = i;
                    pickRank = *rank;
                }
            }
            if (pick.has_value() && Syntax::completionMatches(language, items[*pick].name, typed))
                m_completionListed.push_back(*pick);
            at = end;
        }
    }

    // The part typed so far in the spot, the rest in the text's own ink, and the kind muted at
    // the end of the line - a flex space lands every kind on the list's right edge. The tooltip is
    // the signature in the code style and the sentence under it; a keyword, or an entry that says
    // no more than its name, has none.
    DropListLine CodeBox::completionLine(const CompletionItem& item, const std::size_t typed)
    {
        DropListLine line{};
        const std::wstring_view name = item.name;
        if (typed)
            line.text << InkWell::spotInk() << name.substr(0, typed) << PopColor{};
        line.text << name.substr(typed);
        line.text << FlexSpace{ k_kindGap } << InkWell::textInk(InkGrade::Muted)
            << Syntax::completionKindName(item.kind) << PopColor{};

        const Syntax::CompletionEntry* entry = item.entry;
        if (!entry || (entry->signature.empty() && entry->hint.empty()))
            return line;
        const std::wstring& signature = entry->signature.empty() ? entry->name : entry->signature;
        line.tooltip << TextStyleId::Code << signature << PopTextStyle{};
        if (!entry->hint.empty())
            line.tooltip << L"\n" << InkGrade::Muted << entry->hint << PopColor{};
        return line;
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
        // The tooltip beside the current row goes with the list - see DropList::hide.
        if (m_completionList.has_value())
            m_completionList->hide();
        // And the tooltip over the call comes back to its own side of the line - asked even of a
        // list already down, as one a click took a line from is. See DropList::pick
        if (parameterTooltipShown())
            requestParameterTooltip(false);
    }

    void CodeBox::ensureCompletionList()
    {
        if (m_completionList.has_value())
            return;
        // Ten rows and a bar past that, and a width of its own - the caret it stands under has
        // none to give it.
        m_completionList.emplace(*this, [this](const DropListPick& picked) {
            takeCompletion(m_completionItems[m_completionListed[picked.line]]);
        }, DropListOptions{
            .shownLines = k_completionRows,
            .minWidth = k_completionMinWidth
        });
        // A pass that sized the list again may have moved it to the other side of the line.
        m_completionList->setPlacedFunc([this]() {
            if (parameterTooltipShown())
                requestParameterTooltip(false);
        });
    }

    void CodeBox::takeCompletion(const CompletionItem& item)
    {
        const CaretLine line = caretLine();
        const bool title = item.scope == CompletionScope::Title;
        std::size_t wordStart = 0;
        std::wstring written{ item.name };
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
        what << L"Complete " << InkWell::accentInk() << item.name << PopColor{};
        m_takingCompletion = true;
        setSelection(line.start + wordStart, line.start + line.caret);
        applyEdit(ensureCaret(), Text{ written }, EditKind::Replace, what);
        m_takingCompletion = false;
        if (valueFollows)
            requestCompletion(true);
        else if (!title)
            realignFinishedWord(false);
    }

    bool CodeBox::parameterTooltipShown() const
    {
        return m_parameterTooltip.has_value() && m_parameterTooltip->visible();
    }

    void CodeBox::requestParameterTooltip(const bool opening)
    {
        // The tooltip reads the list's signatures in the language's own way: a box with no list,
        // one nothing can be typed into, or a language that reads no signature, shows none.
        if (!m_completion || readOnly() == ReadOnly::Yes || !m_lines.language().parameters)
            return;
        m_parameterTooltipOpening = m_parameterTooltipOpening || opening;
        m_parameterTooltipRequest.start(MilliSeconds{ 0u });
    }

    void CodeBox::updateParameterTooltip()
    {
        // **The tooltip stands on a box that has been laid out** - see updateCompletion.
        if (!form().contentAligned())
        {
            m_parameterTooltipWaitsOnAlign = true;
            return;
        }
        const bool opening = std::exchange(m_parameterTooltipOpening, false);
        if (!opening && !parameterTooltipShown())
            return;

        const Syntax::Language& language = m_lines.language();
        const Syntax::IndentLines lines = sourceLines();
        const CaretLine line = caretLine();
        // A bracket typed inside a comment or a string opens no call.
        m_lines.tokensOf(line.index, line.text, m_tokens);
        if (!parameterTooltipShown() && Syntax::completionBlocked(m_tokens, line.caret))
        {
            hideParameterTooltip();
            return;
        }
        const std::optional<Syntax::CompletionCall> call =
            Syntax::completionCall(language, lines, line.index, line.caret);
        if (!call)
        {
            hideParameterTooltip();
            return;
        }
        // The text's own routines are read as the tooltip opens; a tooltip up keeps its reading.
        if (!parameterTooltipShown())
            readDeclarations();
        const std::size_t caret = line.start + line.caret;
        const Syntax::CompletionEntry* entry = Syntax::completionCallee(language, *m_completion,
            m_declarations, call->callee, caret);
        if (!entry)
        {
            hideParameterTooltip();
            return;
        }
        // A routine with no parameters says so. Any other name whose signature lists none - a type
        // or a class cast, a property with no index, a variable - shows nothing.
        const Syntax::SignatureParameters parameters = language.parameters(entry->signature);
        const bool routine = entry->kind == Syntax::CompletionKind::Function
            || entry->kind == Syntax::CompletionKind::Procedure;
        if (parameters.names.empty() && !routine)
        {
            hideParameterTooltip();
            return;
        }

        ensureParameterTooltip();
        m_parameterTooltip->content().setCall(*entry, parameters, call->argument);
        // Above the caret's line, from where the call's bracket stands - on that line, which
        // may be one above. In the form's coordinates, which a placement is stated in. A list
        // that stands above the line, for want of room below it, leaves the tooltip the side below.
        FloatRect anchor = caretRectInForm(caret);
        anchor.left = caretRectInForm(lines.start(call->line) + call->bracket).left;
        const bool listAbove = m_completionList.has_value() && m_completionList->standsAbove();
        m_parameterTooltip->setPlacement(listAbove ? FormPlacement::BottomLeft : FormPlacement::Top,
            anchor);
        if (!parameterTooltipShown())
            m_parameterTooltip->show();
    }

    void CodeBox::hideParameterTooltip()
    {
        // A request still pending would put the tooltip back up after it was taken down.
        m_parameterTooltipRequest.stop();
        m_parameterTooltipOpening = false;
        m_parameterTooltipWaitsOnAlign = false;
        if (parameterTooltipShown())
            m_parameterTooltip->hide();
    }

    void CodeBox::ensureParameterTooltip()
    {
        if (m_parameterTooltip.has_value())
            return;
        const ThemeMetrics& metrics = themeMetrics();
        m_parameterTooltip.emplace(
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
        m_parameterTooltip->setAutoFit(AutoFit::Yes);
        m_parameterTooltip->setDropdownClearance(2.0f);
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
