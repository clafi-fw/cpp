module;
#include "../Y-Core/System/Switches.h"

module ClaFi.Controls.TextBox;

import ClaFi.Controls.Menu;
import ClaFi.StdActions;
import ClaFi.Core.Foundation;
import ClaFi.Core.Foundation.EditHistory;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.TextEngine;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
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

    LinkContextPopupEvent::LinkContextPopupEvent(TextBox& box, std::wstring target,
        const TextRange range, ContextPopupEvent& popup)
        :
        EventOf<TextBox>{ box },
        target{ std::move(target) },
        range{ range },
        popup{ popup }
    {
    }

    TextRange TextBox::selection() const
    {
        // An unplaced caret stands past the text, and a whole-text selection runs past it.
        const std::size_t size = text().plainText().size();
        const std::size_t start = std::min(m_editProps.selRange.start, size);
        return { start, std::min(m_editProps.selRange.length, size - start) };
    }

    void TextBox::setReadOnly(ReadOnly value)
    {
        if (value == m_readOnly)
            return;
        m_readOnly = value;
        invalidateEditActions();
    }

    std::size_t TextBox::undoDepth() const
    {
        if (m_readOnly == ReadOnly::Yes)
            return 0;
        return m_history.undoDepth();
    }

    std::size_t TextBox::redoDepth() const
    {
        if (m_readOnly == ReadOnly::Yes)
            return 0;
        return m_history.redoDepth();
    }

    void TextBox::writeUndoStep(const std::size_t i, Text& text) const
    {
        m_history.writeUndoStep(i, text);
    }

    void TextBox::writeRedoStep(const std::size_t i, Text& text) const
    {
        m_history.writeRedoStep(i, text);
    }

    // Undo and redo act on the history rather than on a range, so they are the two edits that do
    // not come through applyEdit, and read-only is stated for them here.
    void TextBox::undo(const std::size_t steps)
    {
        if (m_readOnly == ReadOnly::Yes)
            return;
        const std::optional<EditSelection> selection = m_history.undo(text(), steps);
        if (!selection.has_value())
            return;
        forgetPlaces();
        applySelection(selection.value());
        textEdited();
    }

    void TextBox::redo(const std::size_t steps)
    {
        if (m_readOnly == ReadOnly::Yes)
            return;
        const std::optional<EditSelection> selection = m_history.redo(text(), steps);
        if (!selection.has_value())
            return;
        forgetPlaces();
        applySelection(selection.value());
        textEdited();
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
        // The view moves last, once the selection is written - the order nestedKeyDown keeps.
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

    // Read without syncing: the measure that runs ahead of every align has stated the text, and a
    // sync to the bounds the body is about to leave would have its fit read again at the paint.
    ScrollMetrics TextBox::scrollMetrics() const
    {
        ScrollMetrics result{};
        result.step.y = m_layout.lastLineHeight() / scaler().factor();
        return result;
    }

    const EditProps* TextBox::editProps() const
    {
        if (isInEditMode())
            return &m_editProps;
        return nullptr;
    }

    // The view moves last, once the selection is written - the order nestedKeyDown keeps.
    void TextBox::showFound(std::size_t, const TextRange range)
    {
        m_editProps.targetX.reset();
        setSelection(range.start, range.end());
        scrollIntoView();
    }

    void TextBox::hideFound(std::size_t, const TextRange range)
    {
        if (m_editProps.selRange == range)
            setCaretPos(m_editProps.caretPos());
    }

    CursorShape TextBox::cursor() const
    {
        if (!enabled(true))
            return CursorShape::Arrow;
        return m_layout.hoveredLink().length ? CursorShape::Hand : CursorShape::IBeam;
    }

    // The placeholder goes through the engine's cache with no edit state, so the caret and the
    // selection are drawn once, by the box's own layout over it.
    DrawTextResult TextBox::drawText(PaintEvent& event, const FloatRect& textBounds,
        const Text& text)
    {
        if (text.plainText().empty() && !m_placeHolderText.empty())
        {
            const Text shown{ InkGrade::Muted, m_placeHolderText };
#if CLAFI_TEXT_MOVING
            textEngine().drawText(event.controlContext(), textBounds, shown, textAnchor(), nullptr,
                textRenderMode(), wordWrap(), textFormat());
#else
            textEngine().drawText(event.controlContext(), textBounds, shown, textAnchor(), nullptr,
                wordWrap(), textFormat());
#endif
        }
        return TextBoxBase::drawText(event, textBounds, text);
    }

    // **A flex space is measured at its floor**, in the Calculate phase the engine measures a
    // Label's text in. The box's own layout is built for the paint, where a flex space takes the
    // room its line has over: measured on it, the text claims the whole width it is asked at, and a
    // measuring pass asks at an unbounded one.
    ScaledSize TextBox::measureText(AlignEvent& event, ScaledSize asked,
        const Text& text)
    {
        const ScaledSize measured = text.hasFlexSpace()
            ? textEngine().calculateText(event.formContext(), text, asked, true, wordWrap(),
                textFormat())
            : TextBoxBase::measureText(event, asked, text);
        if (!text.plainText().empty() || m_placeHolderText.empty())
            return measured;

        const ScaledSize placeHolder = textEngine().calculateText(event.formContext(),
            m_placeHolderText, asked, false, wordWrap(), textFormat());
        return {
            std::max(measured.x, placeHolder.x),
            std::max(measured.y, placeHolder.y),
        };
    }

    void TextBox::mouseMove(MouseMoveEvent& event)
    {
        TextBoxBase::mouseMove(event);

        // The tooltip is about any link, and the underline only about one a click would follow.
        const std::optional<LinkHit> hit = linkAt(event.posOnForm);
        pointAtLink(hit);
        const bool live = hit.has_value() && followsLinks(Platform::keyModifiers());
        hoverLink(live ? hit->link.range : TextRange{});
    }

    void TextBox::hoverLeave()
    {
        TextBoxBase::hoverLeave();
        // The tooltip has heard about the pointer leaving from the hover change itself, and
        // telling it again would start a second wait for whatever the pointer went to.
        m_pointedLink = {};
        hoverLink({});
    }

    void TextBox::nestedGetTooltip(GetTooltipEvent& event)
    {
        if (!m_pointedLink.range.length)
        {
            TextBoxBase::nestedGetTooltip(event);
            return;
        }

        // Over the line the pointer is on, from where the link starts on it, and under it where
        // the screen has no room above: the tooltip covers neither the link nor the line it is in.
        event.placement = FormPlacement::Top;
        event.anchorRect = pointedLinkLine();

        GetLinkTooltipEvent linkEvent{ *this, m_pointedLink.target, m_pointedLink.range, event };
        emitEvent(linkEvent);
        if (event.text.empty())
            writeLinkTooltip(event);
        writeFollowLine(event.text);
    }

    // Only a pointer click lands on a link - one a key made has no point to land on, and Return
    // answers for the caret in nestedKeyDown. A press that ended in a selection was a drag, and a
    // drag does not follow the link it started on. The caret the press placed stands where the
    // release did when the two are one place.
    void TextBox::click(ClickEvent& event)
    {
        std::optional<LinkSpan> link{};
        const bool pointerFollows = Input::mouse().active() && followsLinks(event.modifiers)
            && m_editProps.selRange.length == 0;
        if (pointerFollows)
        {
            if (const std::optional<LinkHit> hit = linkAt(event.clickPos()))
                link = hit->link;
        }
        if (!link.has_value())
        {
            TextBoxBase::click(event);
            return;
        }
        // The click was the link's, and goes no further up.
        event.stopPropagation();
        followLink(*link, event.stamp);
    }

    void TextBox::nestedContextPopup(ContextPopupEvent& event)
    {
        // Outside the selection the click states a new caret, as a left one does. Inside it, the
        // click is aimed at the selection the menu's Copy and Cut act on, and placing a caret
        // there would collapse the range before the menu is even up.
        if (!mousePosInSelection())
            setCaretPosFromMouse();

        // **A link is what the menu is about** where one stands under the pointer or, from the
        // keyboard, at the caret - ahead of the box: the box's menu is its edit commands, and none
        // of them is about where the link goes. The target is copied out before anyone is told, as
        // a click's is. A handler that has shown the link's menu stops the request, and the box
        // raises nothing over it; one that has not leaves the box to answer as for a click
        // anywhere else in it.
        std::optional<LinkSpan> link{};
        if (!event.mousePos)
            link = linkAtCaret();
        else if (const std::optional<LinkHit> hit = linkAt(*event.mousePos))
            link = hit->link;
        if (link.has_value())
        {
            LinkContextPopupEvent linkEvent{ *this, std::wstring{ link->value }, link->range,
                event };
            emitEvent(linkEvent);
            if (event.propagationStopped())
                return;
        }

        // The application gets first refusal, and a handler that stops the event has replaced
        // the menu outright.
        TextBoxBase::nestedContextPopup(event);
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
        menu.addActions(items);
        menu.execute();
    }

    void TextBox::editModeKeyDown(KeyDownEvent& event)
    {
        TextBoxBase::editModeKeyDown(event);

        // The jump history. With nothing to go back or forward to the key is left unhandled, so
        // it carries on to whatever stands above the box.
        const bool historyKey = event.modifiers.alt && !event.modifiers.ctrl
            && !event.modifiers.shift && (event.key == Keys::Left || event.key == Keys::Right);
        if (historyKey)
        {
            const bool back = event.key == Keys::Left;
            if (back ? !canGoBack() : !canGoForward())
                return;
            if (back)
                goBack();
            else
                goForward();
            event.handled = true;
            return;
        }

        ensureCaret();

        // don't know, why I still have that handled member there
        event.handled = true;
        bool toLeft = false;

        // The remembered x is what holds a run of vertical presses to one column. Every other key
        // states a new column, including the ones that turn out to move nothing.
        const bool verticalKey = event.key == Keys::Up
            || event.key == Keys::Down
            || event.key == Keys::Prior
            || event.key == Keys::Next;
        if (!verticalKey)
            m_editProps.targetX.reset();

        // Both keys change the text, which a read-only box does not, so it leaves them unhandled
        // rather than swallowing them: they carry on to the form's shortcut scopes, where
        // StdActions::del is bound to Delete and something standing above the box may be what the
        // key is about. The box claims that action and reports it disabled, so the scope walk
        // passes it by - see connectEditActions.
        if (m_readOnly == ReadOnly::Yes && (event.key == Keys::Delete || event.key == Keys::BackSpace))
        {
            event.handled = false;
            return;
        }

        switch (event.key)
        {

        case Keys::Delete:
        {
            // Shift+Delete is Cut, an action like Ctrl+X - see connectEditActions.
            if (event.modifiers.shift && !event.modifiers.ctrl)
            {
                event.handled = false;
                return;
            }
            // The key is answered here rather than left to the Delete action, which removes
            // the selection and only that. With nothing selected this key deletes forward, by
            // a character or by a word, and that is a meaning of its own - the action is what
            // a menu item and a toolbar button reach, and it lands on applyEdit either way.
            //
            // Removing a selection is a step of its own to undo, because the user drew the
            // boundary it acted on. A run of the bare key is one step.
            //
            // The range the key names is a local, and the selection is left alone: undoing
            // the key puts the caret back where the user had it, and that is where it still
            // stands rather than around what the key is about to take out.
            TextRange range = m_editProps.selRange;
            EditKind kind = EditKind::Replace;
            if (!range.length)
            {
                kind = EditKind::DeletingForward;
                const std::size_t from = range.start;
                const std::size_t to = event.modifiers.ctrl
                    ? textEngine().nextWord(text(), from)
                    : from + 1;
                range.length = std::min(to, text().plainText().size()) - from;
            }
            // A caret at the end of the text names nothing to remove, and an edit of no
            // characters would still take a place in the history.
            if (!range.length)
                return;
            applyEdit(range, Text{}, kind);
            return;
        }

        case Keys::BackSpace:
        {
            TextRange range = m_editProps.selRange;
            EditKind kind = EditKind::Replace;
            if (!range.length)
            {
                kind = EditKind::DeletingBack;
                const std::size_t to = range.start;
                if (!to)
                    return;
                const std::size_t from = event.modifiers.ctrl
                    ? textEngine().prevWord(text(), to)
                    : to - 1;
                range = { from, to - from };
            }
            if (!range.length)
                return;
            applyEdit(range, Text{}, kind);
            return;
        }

        // The edit actions are answered as actions rather than here, so that a shortcut and a
        // menu item carrying the same action reach one implementation - see connectEditActions.
        // Left unhandled, the key carries on to the form's shortcut scopes, which resolve the
        // subject back to this control. These labels state that; `default` below would leave
        // them unhandled anyway.
        case 'A':
        case 'C':
        case 'X':
        case 'V':
        case 'Y':
        case 'Z':
        case Keys::Insert:
            event.handled = false;
            return;

        case Keys::Left:
        case Keys::Up:
        case Keys::Prior:
        case Keys::Home:
            toLeft = true;
            [[fallthrough]];
        case Keys::Right:
        case Keys::Down:
        case Keys::Next:
        case Keys::End:
            if ((!event.modifiers.shift && !event.modifiers.ctrl) || !m_editProps.selRange.length)
                m_editProps.caretOnLeft = toLeft;
            break;

        case Keys::Return:
            if (followLinkAtCaret(event))
                return;
            event.handled = false;
            break;

        default:
            event.handled = false;
        }
        if (event.handled)
        {
            std::size_t otherPos = m_editProps.selRange.end();
            std::size_t caretPos;
            if (m_editProps.caretOnLeft)
                caretPos = m_editProps.selRange.start;
            else
            {
                caretPos = std::min(otherPos, text().plainText().size());
                otherPos = m_editProps.selRange.start;
            }

            const FormContext& formContext = this->formContext();
            const FloatRect textBounds = textBoundsInControl();
            // A page press names the screenful the view is moved by, which is more than the rect
            // the caret alone would ask for. Every other key leaves this empty and the caret
            // answers for itself.
            std::optional<FloatRect> pageRect;

            switch (event.key)
            {
            case Keys::Left:
                if (event.modifiers.ctrl)
                    caretPos = textEngine().prevWord(text(), caretPos);
                else if (caretPos && (event.modifiers.shift || !m_editProps.selRange.length))
                    --caretPos;
                m_editProps.affinityTrailing = false;
                break;

            case Keys::Right:
                if (event.modifiers.ctrl)
                    caretPos = textEngine().nextWord(text(), caretPos);
                else if (event.modifiers.shift || !m_editProps.selRange.length)
                    ++caretPos;
                m_editProps.affinityTrailing = false;
                break;

            case Keys::Home:
                if (event.modifiers.ctrl)
                    caretPos = 0;
                else
                    caretPos = rowHome({ caretPos, m_editProps.affinityTrailing });
                m_editProps.affinityTrailing = false; // Home **always** snaps to start of line
                break;

            case Keys::End:
                if (event.modifiers.ctrl)
                    caretPos = text().plainText().size();
                else
                    caretPos = syncedLayout(formContext, textBounds).rowEnd(
                        { caretPos, m_editProps.affinityTrailing });
                m_editProps.affinityTrailing = true; // End **always** snaps to end of line
                break;

            case Keys::Up:
            case Keys::Down:
            {
                rememberTargetX(textBounds, caretPos);
                const ScrollDirection dir = event.key == Keys::Up
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd;
                const CaretHit hit = posOnNextRow(textBounds,
                    { caretPos, m_editProps.affinityTrailing }, dir);

                caretPos = hit.pos;
                m_editProps.affinityTrailing = hit.trailing;
                break;
            }

            case Keys::Prior:
            case Keys::Next:
            {
                rememberTargetX(textBounds, caretPos);
                const ScrollDirection dir = event.key == Keys::Prior
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd;
                const PageMove move = posOnNextPage(textBounds,
                    { caretPos, m_editProps.affinityTrailing }, dir);

                caretPos = move.hit.pos;
                m_editProps.affinityTrailing = move.hit.trailing;
                pageRect = move.page;
                break;
            }
            }

            caretPos = std::min(caretPos, text().plainText().size());
            if (!event.modifiers.shift)
                otherPos = caretPos;

            setSelection(otherPos, caretPos);

            // Everything read out of the layout has been written back, so the view may move now.
            // A scroll changes where the caret sits on the form, and nothing below reads it.
            if (pageRect.has_value())
                scrollIntoView(pageRect.value());
            else
                scrollIntoView();
        }
    }

    void TextBox::connectEditActions()
    {
        // Answered here rather than in nestedKeyDown, so that the shortcut and a menu item carrying
        // the same action reach one implementation. nestedKeyDown hands those keys on for exactly
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
                event.claim({ .enabled = canUndo() });
            // Redo
            else if (&event.action == &StdActions::redo)
                event.claim({ .enabled = canRedo() });
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
                undo(1);
            // Redo
            else if (&event.action == &StdActions::redo)
                redo(1);
            });

        // The list behind an Undo or Redo button is this box's own steps.
        connectEvent([this](GetEditHistoryEvent& event) {
            event.history = this;
            });
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

    std::optional<LinkSpan> TextBox::linkAtCaret() const
    {
        if (m_editProps.selRange.start == k_maxSize)
            return std::nullopt;
        return text().linkAtCaret(m_editProps.caretPos());
    }

    // Handled before the follow, which may take the box down with the page it opens.
    bool TextBox::followLinkAtCaret(KeyDownEvent& event)
    {
        if (!followsLinks(event.modifiers))
            return false;
        const std::optional<LinkSpan> link = linkAtCaret();
        if (!link.has_value())
            return false;
        event.handled = true;
        followLink(*link, event.stamp);
        return true;
    }

    // An anchor in the box's own text is the box's to go to, and nobody is told about it: the caret
    // move that follows says where the box went. A name the text does not hold goes on to
    // OnLinkClick like any other target - a link to another page's anchor, say.
    void TextBox::followLink(const LinkSpan& link, const InputStamp stamp)
    {
        if (link.value.starts_with(L'#') && goToAnchor(link.value.substr(1)))
            return;

        // Copied out before anyone is told: a handler is free to rewrite the text the target
        // stands in, or to take the box down, and nothing below reaches the box or its form. No
        // form is named to the shell for that reason - it would only own the shell's own message
        // if the target cannot be opened.
        LinkClickEvent linkEvent{ *this, std::wstring{ link.value }, stamp };
        emitEvent(linkEvent);
        if (!linkEvent.propagationStopped() && opensByDefault(linkEvent.target))
            Platform::shellExecute(nullptr, linkEvent.target);
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
        // **A tooltip is asked for once per hovered control**, and a link is a part of this one the
        // pointer enters and leaves - so the box says when the part changed, the way a grid says
        // its hovered cell did. A tooltip already up is asked again where it stands, and one that
        // is not starts the wait.
        Tooltip::hoveredZoneChanged();
    }

    void TextBox::writeLinkTooltip(GetTooltipEvent& event) const
    {
        // **The paragraph, not the anchor.** An anchor holds a few words - a clue's number, a
        // heading - and what the reader wants before jumping is what stands around them. Taken
        // with its formatting through selectedText, which closes what it opens, without the
        // newline that ends it, and cut as every preview is.
        Text& out = event.text;
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
            event.writePreview(m_layoutText.selectedText(paragraph));
        }
        else
        {
            out << target;
        }
    }

    // A plain click places the caret here, and nothing else on screen says a link can be followed
    // at all. The words above can state an indent or an alignment, which hold until a paragraph
    // states others, so this line states its own.
    void TextBox::writeFollowLine(Text& out) const
    {
        if (m_readOnly != ReadOnly::No)
            return;
        out << L'\n';
        out << ParaIndent{ 0.0f };
        out << TextAlign::Left;
        out << InkGrade::Muted;
        out << L"Ctrl+click to follow";
        out << PopColor{};
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
        const std::size_t end = edit.replacedRange.end();
        const auto carried = [&edit, end](std::size_t pos){
            if (pos == k_maxSize || pos <= edit.replacedRange.start)
                return pos;
            if (pos < end)
                return edit.replacedRange.start;
            return pos - edit.replacedRange.length + edit.insertedLength;
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

    void TextBox::invalidateEditActions()
    {
        StdActions::cut.invalidateState();
        StdActions::copy.invalidateState();
        StdActions::paste.invalidateState();
        StdActions::del.invalidateState();
        StdActions::selectAll.invalidateState();
        StdActions::undo.invalidateState();
        StdActions::redo.invalidateState();
    }
}
