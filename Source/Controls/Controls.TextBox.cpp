module ClaFi.Controls.TextBox;

import ClaFi.Controls.Menu;
import ClaFi.StdActions;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        // What a link is opened for when no handler answers it: a page, or a message to write. A
        // text can come out of a file or off the clipboard and name anything, and the shell runs
        // a program it is handed, so every other target is left to the application.
        constexpr std::array<std::wstring_view, 3> k_openedSchemes{
            L"http:",
            L"https:",
            L"mailto:",
        };

        [[nodiscard]] bool opensByDefault(std::wstring_view target)
        {
            const auto sameLetter = [](wchar_t left, wchar_t right){
                return std::towlower(left) == std::towlower(right);
            };
            for (const std::wstring_view scheme : k_openedSchemes)
            {
                if (target.size() >= scheme.size()
                    && std::ranges::equal(target.substr(0, scheme.size()), scheme, sameLetter))
                {
                    return true;
                }
            }
            return false;
        }

        // How many lines of a paragraph a link's hint shows before it cuts the rest off.
        constexpr std::size_t k_previewLines = 6;

        // Where the first lines of a laid-out text end, or where the text does for one with no
        // more lines than that.
        [[nodiscard]] std::size_t linesEnd(TextLayout& layout, std::size_t lines)
        {
            std::size_t end = 0;
            for (std::size_t line = 0; line != lines; ++line)
                end = layout.rowEnd({ end, false });
            return end;
        }

        // Cuts a preview to the lines a hint shows, broken at the width the hint breaks at. The
        // ellipsis ending it takes the styles still open where it stands.
        void cutPreview(Text& preview, ScaleFactor scale)
        {
            TextLayout layout;
            layout.setWrap(true);
            layout.setBoundsAndScale({ Tooltip::k_lineWidth * scale, k_maxFloat }, scale);
            layout.setText(preview);
            const std::size_t length = preview.plainText().size();
            std::size_t cut = linesEnd(layout, k_previewLines);
            if (cut == length)
                return;

            const std::size_t lastLine = linesEnd(layout, k_previewLines - 1);
            const Text whole = preview;
            const std::wstring& plain = whole.plainText();
            while (true)
            {
                while (cut > lastLine && std::iswspace(plain[cut - 1]))
                    --cut;
                preview = whole;
                preview.replaceText({ cut, length - cut }, L"\u2026");
                layout.setText(preview);
                const bool fits = linesEnd(layout, k_previewLines) == preview.plainText().size();
                if (fits || cut == lastLine)
                    return;

                // The ellipsis pushed a word onto a line of its own: that word goes instead, or a
                // character where the line holds a single word.
                std::size_t word = cut;
                while (word > lastLine && !std::iswspace(plain[word - 1]))
                    --word;
                if (word > lastLine)
                    cut = word;
                else
                    cut -= plain[cut - 1] >= 0xDC00 && plain[cut - 1] <= 0xDFFF ? 2 : 1;
            }
        }
    }

    LinkClickEvent::LinkClickEvent(TextBox& box, std::wstring target, InputStamp stamp)
        :
        EventOf<TextBox>{ box },
        target{ std::move(target) },
        stamp{ stamp }
    {
    }

    GetLinkTooltipEvent::GetLinkTooltipEvent(TextBox& box, std::wstring_view target,
        TextRange range, GetTooltipEvent& tooltip)
        :
        EventOf<TextBox>{ box },
        target{ target },
        range{ range },
        tooltip{ tooltip }
    {
    }

    void TextBox::setReadOnly(ReadOnly value)
    {
        if (value == m_readOnly)
            return;
        m_readOnly = value;
        // Nothing the box draws turns on the mode: the caret, the selection band and the focus
        // ring are the same either way. What the mode changes is the answer five of the edit
        // actions give, and a presenter keeps the answer it was last given until it is asked
        // again - see connectEditActions.
        StdActions::cut.invalidateState();
        StdActions::paste.invalidateState();
        StdActions::del.invalidateState();
        StdActions::undo.invalidateState();
        StdActions::redo.invalidateState();
    }

    bool TextBox::goToAnchor(std::wstring_view name)
    {
        const std::optional<TextRange> anchor = text().anchorRange(name);
        if (!anchor.has_value())
            return false;

        m_backPlaces.push_back(currentPlace());
        m_forwardPlaces.clear();
        m_editProps.targetX.reset();
        setSelection(anchor->start, anchor->end());
        // The view moves last, once the selection is written - the order keyDown keeps.
        scrollLineToTop(anchor->start);
        return true;
    }

    void TextBox::goBack()
    {
        if (m_backPlaces.empty())
            return;
        m_forwardPlaces.push_back(currentPlace());
        const Place place = m_backPlaces.back();
        m_backPlaces.pop_back();
        restorePlace(place);
    }

    void TextBox::goForward()
    {
        if (m_forwardPlaces.empty())
            return;
        m_backPlaces.push_back(currentPlace());
        const Place place = m_forwardPlaces.back();
        m_forwardPlaces.pop_back();
        restorePlace(place);
    }

    CursorShape TextBox::cursor() const
    {
        if (!enabled(true))
            return CursorShape::Arrow;
        return m_layout.hoveredLink().length ? CursorShape::Hand : CursorShape::IBeam;
    }

    void TextBox::mouseMove(MouseMoveEvent& event)
    {
        Label::mouseMove(event);

        // The hint is about any link, and the underline only about one a click would follow.
        const std::optional<LinkHit> hit = linkAt(event.posOnForm);
        pointAtLink(hit);
        const bool live = hit.has_value() && followsLinks(Platform::keyModifiers());
        hoverLink(live ? hit->link.range : TextRange{});
    }

    void TextBox::hoverLeave()
    {
        Label::hoverLeave();
        // The tooltip has heard about the pointer leaving from the hover change itself, and
        // telling it again would start a second wait for whatever the pointer went to.
        m_pointedLink = {};
        hoverLink({});
    }

    void TextBox::getTooltip(GetTooltipEvent& event)
    {
        if (!m_pointedLink.range.length)
        {
            Label::getTooltip(event);
            return;
        }

        // Over the line the pointer is on, from where the link starts on it, and under it where
        // the screen has no room above: the hint covers neither the link nor the line it reads in.
        event.placement = FormPlacement::Top;
        event.anchorRect = pointedLinkLine();

        GetLinkTooltipEvent linkEvent{ *this, m_pointedLink.target, m_pointedLink.range, event };
        emitEvent(linkEvent);
        if (event.text.empty())
            writeLinkTooltip(event.text);
    }

    void TextBox::click(ClickEvent& event)
    {
        // A press that ended in a selection was a drag, and a drag does not follow the link it
        // started on. The caret the press placed stands where the release did when the two are
        // one place.
        std::optional<LinkSpan> link{};
        if (followsLinks(event.modifiers) && m_editProps.selRange.length == 0)
        {
            if (const std::optional<LinkHit> hit = linkAt(event.clickPos()))
                link = hit->link;
        }
        if (!link.has_value())
        {
            Label::click(event);
            return;
        }

        // An anchor in the box's own text is the box's to go to, and nobody is told about it: the
        // caret move that follows says where the box went. A name the text does not hold goes on
        // to OnLinkClick like any other target - a link to another page's anchor, say.
        if (link->value.starts_with(L'#') && goToAnchor(link->value.substr(1)))
        {
            event.stopPropagation();
            return;
        }

        // Copied out before anyone is told: a handler is free to rewrite the text the target
        // stands in, or to take the box down, and nothing below reaches the box or its form. No
        // form is named to the shell for that reason - it would only own the shell's own message
        // if the target cannot be opened.
        LinkClickEvent linkEvent{ *this, std::wstring{ link->value }, event.stamp };
        // The click was the link's, and goes no further up.
        event.stopPropagation();
        emitEvent(linkEvent);
        if (!linkEvent.propagationStopped() && opensByDefault(linkEvent.target))
            Platform::shellExecute(nullptr, linkEvent.target);
    }

    void TextBox::contextPopup(ContextPopupEvent& event)
    {
        // Outside the selection the click states a new caret, as a left one does. Inside it, the
        // click is aimed at the selection the menu's Copy and Cut act on, and placing a caret
        // there would collapse the range before the menu is even up.
        if (!mousePosInSelection())
            setCaretPosFromMouse();

        // The application gets first refusal, and a handler that stops the event has replaced
        // the menu outright.
        Label::contextPopup(event);
        if (event.propagationStopped())
            return;

        ActionList items{
            &StdActions::undo,
            &StdActions::redo,
            nullptr,
            &StdActions::cut,
            &StdActions::copy,
            &StdActions::paste,
            &StdActions::del,
            nullptr,
            &StdActions::selectAll,
        };

        // The second refusal, and a narrower one: this menu is the standard edit menu, and a
        // handler here adjusts it rather than replacing it. Each action answers for its own
        // state, so an item the box cannot run right now arrives disabled without anything here
        // sorting them.
        EditContextPopupEvent editEvent{ *this, event.form, items, event.mousePos };
        editContextPopup(editEvent);
        if (editEvent.propagationStopped())
            return;

        Menu menu{ *this };
        // The clipboard commands go up into the strip across the top, where an icon alone
        // says what each of them does; everything else stays a line with its name on it.
        // Each is taken only if the list still names it, so a handler that dropped one is
        // not answered by the strip putting it back.
        for (Action* command : { &StdActions::cut, &StdActions::copy, &StdActions::paste, &StdActions::selectAll, &StdActions::del })
        {
            if (std::ranges::find(items, command) != items.end())
                menu.addToCommandBar(*command);
        }
        // The list is handed over whole. Menu::add leaves out whatever the strip shows.
        menu.add(items);
        menu.execute();
    }

    void TextBox::connectEditActions()
    {
        // Answered here rather than in keyDown, so that the shortcut and a menu item carrying
        // the same action reach one implementation. keyDown hands those keys on for exactly
        // this reason.
        //
        // Claiming says this control is what the action acts on; what it claims says whether it
        // can act right now. A box with nothing selected claims Cut and reports it disabled,
        // which is not the same as Cut being about something else.
        //
        // A read-only box claims the five commands that change the text just as it claims the
        // others, and reports them disabled. The box is still what Paste would have gone into,
        // and a claim that fell silent instead would hand the key to whatever stands above it.
        //
        // Copy asks only that there is text to take. With nothing selected the click selects
        // all of it first, so an empty selection means the whole box rather than nothing.
        onGetActionState([this](GetActionStateEvent& event) {
            const bool hasSelection = m_editProps.selRange.length != 0;
            const bool editable = m_readOnly == ReadOnly::No;

            // Cut
            if (&event.action == &StdActions::cut)
                event.claim({ .enabled = editable && hasSelection });
            // Copy
            else if (&event.action == &StdActions::copy)
                event.claim({ .enabled = !text().plainText().empty() });
            // Paste
            else if (&event.action == &StdActions::paste)
                event.claim({ .enabled = editable
                    && formContext().clipboard().accepts(pasteFormats()).has_value() });
            // Delete
            else if (&event.action == &StdActions::del)
                event.claim({ .enabled = editable && hasSelection });
            // Select All
            else if (&event.action == &StdActions::selectAll)
                event.claim({ .enabled = !text().plainText().empty() });
            // Undo
            else if (&event.action == &StdActions::undo)
                event.claim({ .enabled = editable && canUndo() });
            // Redo
            else if (&event.action == &StdActions::redo)
                event.claim({ .enabled = editable && canRedo() });
            });

        onActionClick([this](ActionClickEvent& event) {
            // Cut
            if (&event.action == &StdActions::cut)
                cutSelectedText(event.stamp);
            // Copy
            else if (&event.action == &StdActions::copy)
            {
                if (m_editProps.selRange.length == 0)
                    selectAll();
                copySelectedText(event.stamp);
            }
            // Paste
            else if (&event.action == &StdActions::paste)
                pasteClipboardText();
            // Delete
            else if (&event.action == &StdActions::del)
                deleteSelectedText();
            // Select All
            else if (&event.action == &StdActions::selectAll)
                selectAll();
            // Undo
            else if (&event.action == &StdActions::undo)
                undo();
            // Redo
            else if (&event.action == &StdActions::redo)
                redo();
            });
        // TODO: a presenter of one of these actions is not refreshed when the answer changes -
        // an edit alters what undo, redo, copy and select all report, and a caret move alters
        // cut and delete. Should the box call invalidateState on each of them, or should a
        // subject be able to say "my answers changed" once?
    }

    bool TextBox::followsLinks(KeyModifiers modifiers) const
    {
        return enabled(true) && (m_readOnly == ReadOnly::Yes || modifiers.ctrl);
    }

    std::optional<LinkHit> TextBox::linkAt(PointInForm pt) const
    {
        const FormContext& formContext = this->formContext();
        const FloatRect textBounds = this->textBounds(formContext, boundsInForm());
        TextLayout& layout = syncedLayout(formContext, textBounds);
        const FloatPoint origin = anchoredOrigin(textBounds, layout.calculatedDimensions(),
            textAnchor());
        return layout.linkAt(pt - origin);
    }

    void TextBox::hoverLink(TextRange link)
    {
        if (link == m_layout.hoveredLink())
            return;
        m_layout.setHoveredLink(link);
        invalidate();
    }

    void TextBox::pointAtLink(const std::optional<LinkHit>& hit)
    {
        const TextRange range = hit.has_value() ? hit->link.range : TextRange{};
        if (hit.has_value())
            m_pointedLink.pos = hit->pos;
        if (range == m_pointedLink.range)
            return;

        m_pointedLink.range = range;
        m_pointedLink.target = hit.has_value() ? std::wstring{ hit->link.value } : std::wstring{};
        // A HINT IS ASKED FOR ONCE PER HOVERED CONTROL, and a link is a part of this one the
        // pointer enters and leaves - so the box says when the part changed, the way a grid says
        // its hovered cell did. A hint already up is asked again where it stands, and one that is
        // not starts the wait.
        Tooltip::hoveredZoneChanged();
    }

    void TextBox::writeLinkTooltip(Text& out) const
    {
        // THE PARAGRAPH, NOT THE ANCHOR. An anchor holds a few words - a clue's number, a
        // heading - and what the reader wants before jumping is what stands around them. Taken
        // with its formatting through selectedText, which closes what it opens, without the
        // newline that ends it, and no longer than a few lines.
        const std::wstring_view target = m_pointedLink.target;
        const std::optional<TextRange> anchor = target.starts_with(L'#')
            ? m_layoutText.anchorRange(target.substr(1))
            : std::nullopt;
        if (anchor.has_value())
        {
            TextRange paragraph = textEngine().paragraphAt(m_layoutText, anchor->start);
            const std::wstring& plain = m_layoutText.plainText();
            if (paragraph.length && plain[paragraph.end() - 1] == L'\n')
                --paragraph.length;
            Text preview = m_layoutText.selectedText(paragraph);
            cutPreview(preview, formContext().scaleFactor());
            out << preview;
        }
        else
        {
            out << target;
        }

        // A plain click places the caret here, and nothing else on screen says a link can be
        // followed at all. The paragraph taken above can state an indent or an alignment, which
        // hold until a paragraph states others, so this line states its own.
        if (m_readOnly == ReadOnly::No)
        {
            out << L'\n';
            out << SetIndent{ 0.0f };
            out << TextAlign::Left;
            out << InkGrade::Muted;
            out << L"Ctrl+click to follow";
            out << PopColor{};
        }
    }

    FloatRect TextBox::pointedLinkLine() const
    {
        const FormContext& formContext = this->formContext();
        const FloatRect textBounds = this->textBounds(formContext, boundsInForm());
        TextLayout& layout = syncedLayout(formContext, textBounds);
        const CaretHit hit{ m_pointedLink.pos, false };
        const std::size_t start = std::max(m_pointedLink.range.start, layout.rowStart(hit));
        const std::size_t end = std::min(m_pointedLink.range.end(), layout.rowEnd(hit));
        const FloatRect first = caretRect(textBounds, { start, false });
        const FloatRect last = caretRect(textBounds, { std::max(end, start + 1), true });
        return {
            std::min(first.left, last.left),
            first.top,
            std::max(first.left, last.left),
            first.bottom,
        };
    }

    TextBox::Place TextBox::currentPlace() const
    {
        // The view a glide is heading for, which is where the box stands once it settles - see
        // posOnNextPage.
        FloatRect view = visibleRectInForm();
        view.offset(-boundsInForm().topLeft());
        view.offset(viewTravelRemaining());
        const CaretHit top = caretHitAt(formContext(), textBoundsInControl(),
            { view.left, view.top + scaler().scaled8 });
        return {
            .selection = {
                m_editProps.selRange,
                m_editProps.caretOnLeft,
                m_editProps.affinityTrailing,
            },
            .viewTop = top.pos,
        };
    }

    void TextBox::restorePlace(const Place& place)
    {
        // A host writing the text leaves the history standing, so a place can name a position
        // past the end of what the box now holds. A box with no caret placed stays without one.
        const std::size_t size = text().plainText().size();
        EditSelection selection = place.selection;
        if (selection.range.start != k_maxSize)
        {
            selection.range.start = std::min(selection.range.start, size);
            if (selection.range.length != k_maxSize)
            {
                selection.range.length = std::min(selection.range.length,
                    size - selection.range.start);
            }
        }
        applySelection(selection);
        m_editProps.targetX.reset();
        caretMoved();
        scrollLineToTop(std::min(place.viewTop, size));
    }

    void TextBox::scrollLineToTop(std::size_t pos)
    {
        // A rect exactly as tall as the client area ScrollBox measures against - the viewport
        // inset by this margin at both ends. It fits only with its top against the top of that
        // area, so the view travels until the line stands there, whichever way it has to go. The
        // same request the page keys make - see posOnNextPage.
        const FloatRect line = caretRect(textBoundsInControl(), { pos, false });
        const float margin = scaler().scaled8;
        const float pageExtent = std::max(visibleRectInForm().height() - 2.0f * margin,
            line.height());
        scrollIntoView(FloatRect{ line.left, line.top, line.left, line.top + pageExtent });
    }

    void TextBox::carryPlaces(const TextEdit& edit)
    {
        // A position in what went collapses onto where it went from, and one after it moves by
        // what the text gained or lost.
        const std::size_t end = edit.replaced.end();
        const auto carried = [&edit, end](std::size_t pos){
            if (pos == k_maxSize || pos <= edit.replaced.start)
                return pos;
            if (pos < end)
                return edit.replaced.start;
            return pos - edit.replaced.length + edit.insertedLength;
        };

        for (Places* places : { &m_backPlaces, &m_forwardPlaces })
        {
            for (Place& place : *places)
            {
                TextRange& range = place.selection.range;
                if (range.length != k_maxSize)
                {
                    const std::size_t rangeEnd = carried(range.end());
                    range.start = carried(range.start);
                    range.length = rangeEnd - range.start;
                }
                place.viewTop = carried(place.viewTop);
            }
        }
    }

    void TextBox::forgetPlaces()
    {
        m_backPlaces.clear();
        m_forwardPlaces.clear();
    }
}
