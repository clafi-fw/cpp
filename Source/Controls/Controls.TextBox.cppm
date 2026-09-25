module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.TextBox;

import ClaFi.Controls.Label;

import ClaFi.StdActions.Transfer;

import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.History;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Fmt;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.Transfer.Formats;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;
import ClaFi.Diagnostic.Log;

namespace ClaFi::Controls
{

    // Whether the text the box shows may be changed through the box. See Controls
    export enum class ReadOnly
    {
        No,
        Yes
    };

    export class TextBox;

    // The caret has come to rest somewhere new. See Controls
    export struct CaretMoveEvent : public EventOf<TextBox>
    {
        using EventOf<TextBox>::EventOf;
    };

    // The text has been changed through the box. See Controls
    export struct TextEditEvent : public EventOf<TextBox>
    {
        using EventOf<TextBox>::EventOf;
    };

    // A link in the box's text was followed. See Controls
    export struct LinkClickEvent : public EventOf<TextBox>
    {
        LinkClickEvent(TextBox&, std::wstring target, InputStamp);
        std::wstring target;   // what the link names, as the text states it
        InputStamp stamp;      // the press that followed the link
    };

    // The hint of the link under the pointer is being asked for. See Controls#link-hints
    export struct GetLinkTooltipEvent : public EventOf<TextBox>
    {
        GetLinkTooltipEvent(TextBox&, std::wstring_view target, TextRange range, GetTooltipEvent&);
        std::wstring_view target;   // what the link names, as the text states it
        TextRange range;            // the text the link covers
        GetTooltipEvent& tooltip;   // the hint itself: its text, placement and anchor
    };

    // A label the user can type into.
    export class TextBox : public WithTextLayout<Label>
    {
    public:
        template<typename... Args>
        explicit TextBox(const CreateParams&, Args&&...);
    public:
        // Whether the text the box shows may be changed through the box.
        DECLARE_WRITABLE_PROPERTY(ReadOnly, readOnly, setReadOnly, ReadOnly::No)
    public:
        // The caret has come to rest somewhere new. See Controls
        DECLARE_EVENT(CaretMoveEvent, OnCaretMove, onCaretMove)
        // The text has been changed through the box. See Controls
        DECLARE_EVENT(TextEditEvent, OnTextEdit, onTextEdit)
        // A link in the box's text was followed. See Controls
        DECLARE_EVENT(LinkClickEvent, OnLinkClick, onLinkClick)
        // The hint of the link under the pointer is being asked for. See Controls#link-hints
        DECLARE_EVENT(GetLinkTooltipEvent, OnGetLinkTooltip, onGetLinkTooltip)
    public:
        // setCaretPosFromMouse Is calling from pressDown, contextPopup
        // and on popping in in-place edit form
        void setCaretPosFromMouse();
        // Puts the caret where a host names it, with nothing selected.
        void setCaretPos(std::size_t pos);
        void selectAll();
        // Where the caret stands, both numbers counted from one - see TextLineColumn. A caret
        // that was never placed answers the start of the text, which is where ensureCaret puts
        // one.
        [[nodiscard]] TextLineColumn caretLineColumn() const;

        // Defined in TextBox.cpp: a mode change alters what the edit actions answer, and saying so
        // means naming them.
        void setReadOnly(ReadOnly);

        // WHAT A BOX TAKES FROM THE CLIPBOARD, best first. Rich before plain: both are reachable
        // whenever either is, because the conversion between them runs in both directions, so this
        // order is the whole of what decides whether formatting survives a paste.
        [[nodiscard]] static Transfer::FormatList pasteFormats();

        // The stamp is the input event that asked for the copy. A display server that authorises
        // the request refuses one naming no event, so a copy made outside an input event cannot
        // take the clipboard - see Transfer::Clipboard::set.
        void copySelectedText(InputStamp);
        void cutSelectedText(InputStamp);
        void pasteClipboardText();

        void replaceSelectedText(std::wstring_view);
        void deleteSelectedText();

        // Takes back the newest edit, or does the newest one taken back over again. Each leaves
        // the caret where the step it moved left it, so the user is looking at what changed.
        void undo();
        void redo();
        [[nodiscard]] bool canUndo() const { return m_history.canUndo(); }
        [[nodiscard]] bool canRedo() const { return m_history.canRedo(); }

        // Selects the text of the anchor of that name and brings its line to the top of the view,
        // remembering where the box stood. Answers false, and moves nothing, where the text holds
        // no such anchor. See Controls#anchors
        bool goToAnchor(std::wstring_view name);
        // Back to where the box stood before the last jump, and forward again - Alt+Left and
        // Alt+Right. See Controls#anchors
        void goBack();
        void goForward();
        [[nodiscard]] bool canGoBack() const { return !m_backPlaces.empty(); }
        [[nodiscard]] bool canGoForward() const { return !m_forwardPlaces.empty(); }
    protected:
        const EditProps* editProps() const override { return &m_editProps; }
        // The hand over a link the box follows, and the I-beam over the rest of the text. A box
        // that takes no input takes no caret either, so the arrow is the honest answer while it
        // is disabled.
        [[nodiscard]] CursorShape cursor() const override;
        // The caret, not the box. A text box long enough to need scrolling is taller than the
        // viewport holding it, and the whole of it is on screen the moment its first line is, so
        // its own bounds name a scroll that is answered before the caret is anywhere near view.
        [[nodiscard]] FloatRect scrollHotspot() const override;
        // The caret, not the box, and for the same reason as scrollHotspot: what the commands in
        // the menu act on is the caret and the selection around it, so a menu dropped under a box
        // several lines tall is nowhere near either.
        [[nodiscard]] FloatRect contextMenuAnchor() const override;
        void setSelection(std::size_t otherPos, std::size_t caretPos);
        // Where the blink and the visual state meet. The caret is drawn from this one place, so
        // both are read here rather than mirrored into a member a missed change could leave stale.
        void paintText(PaintEvent&) override;
        // What the layout is told when the box's text has moved. An edit reaches a paragraph or
        // two, and the rest of the document keeps the shaping it has - which is what the base's
        // answer, stating the whole text, cannot do.
        void takeText(const Text&) const override;
        // The text the layout now holds, and the edit that carried it there - or nothing, where
        // the whole text was stated. A box keeping an account of its text beside the layout's -
        // the line states a CodeBox colours from - brings it into step here, on the same terms.
        virtual void textTaken(const Text&, const TextEdit*) const {}
        void focusChanged() override;
        // Mouse events
        void mouseMove(MouseMoveEvent&) override;
        void hoverLeave() override;
        // The hint of the link under the pointer, where there is one, and the box's own
        // otherwise. See Controls#link-hints
        void getTooltip(GetTooltipEvent&) override;
        void pressDown(PressDownEvent&) override;
        // Follows a link when the press and the release stayed on it and nothing was selected
        // between them. Defined in TextBox.cpp, with the rest of what a link does.
        void click(ClickEvent&) override;
        void doubleClick(DoubleClickEvent&) override;
        void tripleClick(TripleClickEvent&) override;
        // Defined in TextBox.cpp: the standard edit menu is built there, and the menu pulls in
        // the Grid the whole of it is built on. Nothing that merely uses a text box should have
        // to import that.
        void contextPopup(ContextPopupEvent&) override;
        void drag(DragEvent&) override;
        // Keyboard events
        void keyDown(KeyDownEvent&) override;
        void charPress(CharPressEvent&) override;
    private:
        // What one page press crosses, and the rect the view is moved by to show it.
        struct PageMove
        {
            CaretHit hit;
            FloatRect page;
        };
        // The link the pointer stands on, live or not: what its hint is about.
        struct PointedLink
        {
            TextRange range{};
            std::wstring target{};
            // The character under the pointer, which names the line the hint stands under.
            std::size_t pos{ 0 };
        };
        // Where the box stands: what is selected, and the position the view shows at its top.
        struct Place
        {
            EditSelection selection{};
            std::size_t viewTop{ 0 };
        };
        using Places = std::vector<Place>;
    private:
        // Answers the framework's edit actions - see the definition for why they are not
        // answered in keyDown. Defined in TextBox.cpp, which is what keeps the standard
        // actions out of the interface every user of a text box imports.
        void connectEditActions();
        // The rect the text is laid out in, in this control's own coordinates. Everything the
        // keyboard measures goes through here rather than through boundsInForm: targetX below
        // outlives a single press, while the box's place on the form does not, so a scroll under
        // the caret would move a remembered form x out from under the column it names.
        [[nodiscard]] FloatRect textBoundsInControl() const;
        // The position a point names, the point being in the same space as textBounds.
        [[nodiscard]] CaretHit caretHitAt(const FormContext&, const FloatRect& textBounds, FloatPoint) const;
        [[nodiscard]] FloatRect caretRect(const FloatRect& textBounds, CaretHit) const;
        [[nodiscard]] CaretHit posOnNextRow(const FloatRect& textBounds, CaretHit, ScrollDirection) const;
        [[nodiscard]] PageMove posOnNextPage(const FloatRect& textBounds, CaretHit, ScrollDirection) const;
        // Whether the point the mouse went down at falls inside the current selection. What it
        // is for is the right click: the menu it opens carries Copy and Cut, which act on the
        // selection, so a click aimed at the selection must leave the selection standing.
        [[nodiscard]] bool mousePosInSelection() const;
        // Whether a link is live - underlined and pointed at under the pointer, and followed on a
        // click. Always in a read-only box, and while Ctrl is held in one the user types into,
        // where a plain click has to go on placing the caret.
        [[nodiscard]] bool followsLinks(KeyModifiers) const;
        // The link under a point in form space, or nothing.
        [[nodiscard]] std::optional<LinkHit> linkAt(PointInForm) const;
        // Underlines a link, or none for an empty range, repainting only when that changes.
        void hoverLink(TextRange);
        // Records the link the pointer stands on, and has the hint asked again when it changed.
        void pointAtLink(const std::optional<LinkHit>&);
        // What a link's hint says when no handler says it. See Controls#link-hints
        void writeLinkTooltip(Text&) const;
        // The line of the pointed link the pointer is on, in form coordinates.
        [[nodiscard]] FloatRect pointedLinkLine() const;
        [[nodiscard]] Place currentPlace() const;
        // Puts the selection and the view back where a place names them, both clamped to the text
        // as it now stands.
        void restorePlace(const Place&);
        // Scrolls the line holding a position to the top of the view.
        void scrollLineToTop(std::size_t pos);
        // Moves the places the history names by what an edit made through the box did to the text.
        void carryPlaces(const TextEdit&);
        // Drops the history, for a change to the text the box cannot account for.
        void forgetPlaces();
        // A selection range starts unset, which is what a text box with no caret in it looks
        // like - EditProps::caretPos answers k_maxSize and no caret is painted. Anything
        // that measures from the caret has to be given one first, and the start of the text is
        // where a caret that was never placed belongs. Answers the range an edit acts on once
        // there is a caret to answer from: what is selected, which is empty at a bare caret.
        TextRange ensureCaret();
        // The one door every edit goes through, and what keeps the history whole: a text written
        // to around this would leave every position recorded in the history naming a string that
        // is gone. Being that door is also what makes it the whole of what read-only refuses.
        // The kind is what says whether this edit joins the run before it.
        //
        // The range is passed rather than read off the selection, because for the delete keys
        // the two differ: the key names a range of its own with nothing selected, and the
        // selection is still where undoing the key has to put the caret back.
        void applyEdit(TextRange range, const Text& inserted, EditKind);
        // Puts the caret where an undone or redone step left it.
        void applySelection(const EditSelection&);
        // The tail every edit shares. The caret is asked for after the pass rather than now: an
        // edit can add or drop a row, and the box's height and the scroll range that follows it
        // are the alignment's to give - see Control::scrollHotspot.
        void textEdited();
        // The tail every caret move shares, the box gaining or losing the focus included. It
        // ends the undo run as well as restarting the blink: a run is what the user typed
        // without looking away, so a click or an arrow key closes one with the text untouched.
        void caretMoved();
        // Emits CaretMoveEvent. Both of the tails above end here: an edit moves the caret as
        // surely as an arrow key does, and a listener has the same question either way.
        void announceCaretMove();
        void rememberTargetX(const FloatRect& textBounds, std::size_t caretPos);
        // Makes the caret solid and starts its interval over. Every move of the caret and every
        // edit ends here: a caret left on its own schedule is dark for half the time, including
        // the moment the user has just put it somewhere and is looking for it.
        void restartCaretBlink();
        void scheduleCaretBlink();
        void blinkCaret();
    private:
        EditProps m_editProps;
        // Deltas, not snapshots - see TextHistory. It holds no reference to the box's text, so
        // it may stand anywhere among these members.
        TextHistory m_history;
        UiTimer m_caretTimer;
        // Where the box stood before each jump, newest last, and where it stood after each step
        // back. See Controls#anchors
        Places m_backPlaces;
        Places m_forwardPlaces;
        PointedLink m_pointedLink;
        // Which half of the blink the caret stands in. Whether it is DRAWN is this and
        // VisualState::focused together - see paintText.
        bool m_caretOn{ true };
        // The edit that produced a text the layout has not been told about yet, recorded by
        // applyEdit and consumed by the next syncedLayout - which is what lets a paragraph the
        // edit did not reach keep the shaping it has. Every other route that writes the text -
        // undo, redo, a host calling setText - leaves this empty, and the layout is handed the
        // whole text the way it always was.
        mutable std::optional<TextEdit> m_pendingEdit;
    };


    //-------------------------------------------------------------------------


    template<typename ...Args>
    TextBox::TextBox(const CreateParams& params, Args&&... args)
        :
        WithTextLayout<Label>{ params, Interactivity::Focusable, std::forward<Args>(args)... },
        INIT_PROPERTY(readOnly)
    {
        m_caretTimer.onTick([this](TimerEvent&){
            blinkCaret();
        });
        // Pointed at the box's own text once, and re-pointed only when the text changes value -
        // setText invalidates, so calling it per measurement would rebuild the layout each time.
        // Phase and editable never change for a box: it is drawn in the paint phase, and it keeps
        // the row a trailing newline opens because a caret has to be able to stand on it.
        m_layout.setText(m_layoutText);
        m_layout.setEventPhase(EventPhase::Paint);
        m_layout.setEditable(true);
        connectEditActions();
    }

    void TextBox::setCaretPosFromMouse()
    {
        if (Input::device() == InputDevice::Mouse)
        {
            PointInForm mousePos = form().mouseDownPos();
            const FormContext& formContext = this->formContext();
            FloatRect thisRect = boundsInForm();
            FloatRect textBounds = this->textBounds(formContext, thisRect);

            CaretHit hit = caretHitAt(formContext, textBounds, mousePos);
            m_editProps.selRange = { hit.pos, 0 };
            m_editProps.affinityTrailing = hit.trailing;

            m_editProps.targetX.reset();
            caretMoved();
        }
    }

    // CLAMPED, because a position is only ever a position in some text. A host writing a new text
    // into the box names one against the text it is writing, and the box holds that text only
    // once the write has happened.
    void TextBox::setCaretPos(const std::size_t pos)
    {
        const std::size_t caretPos = std::min(pos, text().plainText().size());
        // The remembered column belongs to a run of vertical presses, and this is not one - see
        // keyDown.
        m_editProps.targetX.reset();
        setSelection(caretPos, caretPos);
    }

    void TextBox::selectAll()
    {
        m_editProps.selRange = { 0, k_maxSize };
        // The caret stands at the end, as HexView's does - a select all names no side of its own.
        m_editProps.caretOnLeft = false;
        caretMoved();
        scrollIntoView();
    }

    TextLineColumn TextBox::caretLineColumn() const
    {
        // The start of the text is where a caret that was never placed belongs - the same answer
        // ensureCaret gives an edit made in a box nobody has clicked in yet.
        const std::size_t caretPos = m_editProps.selRange.start == k_maxSize ? 0 : m_editProps.caretPos();
        // selRange.length holds k_maxSize while the whole text is selected, so a caret at the far
        // end of that range names a position past the last character. The end of the text is what
        // it stands for.
        const std::size_t pos = std::min(caretPos, text().plainText().size());
        // Through the synced layout, which is what carries an edit the layout has not been told
        // about yet into the paragraph offsets this reads. Asking here does the work the next
        // paint would have done and nothing more.
        return syncedLayout(formContext(), textBoundsInControl()).lineColumnAt(pos);
    }

    Transfer::FormatList TextBox::pasteFormats()
    {
        return { Transfer::ClaFiText::format(), Transfer::PlainText::format() };
    }

    void TextBox::copySelectedText(const InputStamp stamp)
    {
        // An empty selection is not an empty clipboard - there is nothing to put on it, and the
        // range holds k_maxSize while no caret has been placed, which no substring survives.
        if (!m_editProps.selRange.length)
            return;

        // ONE FORMAT IN. Plain text is advertised out of the conversion table, so an application
        // that has never heard of this framework pastes what it can read without this site knowing
        // such an application exists.
        Transfer::Source source{};
        source.add<Transfer::ClaFiText>(text().selectedText(m_editProps.selRange));
        formContext().clipboard().set(std::move(source), stamp);
    }

    void TextBox::cutSelectedText(const InputStamp stamp)
    {
        copySelectedText(stamp);
        deleteSelectedText();
    }

    void TextBox::pasteClipboardText()
    {
        Transfer::Offer* offer = formContext().clipboard().offer();
        if (!offer)
            return;

        // Walked in the order pasteFormats states. Each asks for nothing unless the clipboard can
        // answer it, so at most one of them reads.
        if (const std::optional<Text> rich = Transfer::take<Transfer::ClaFiText>(*offer))
        {
            applyEdit(ensureCaret(), *rich, EditKind::Replace);
            return;
        }

        if (const std::optional<std::wstring> plain = Transfer::take<Transfer::PlainText>(*offer))
            applyEdit(ensureCaret(), Text{ *plain }, EditKind::Replace);
    }

    void TextBox::replaceSelectedText(std::wstring_view sw)
    {
        applyEdit(ensureCaret(), Text{ sw }, EditKind::Replace);
    }

    void TextBox::deleteSelectedText()
    {
        if (!m_editProps.selRange.length)
            return;
        applyEdit(ensureCaret(), Text{}, EditKind::Replace);
    }

    void TextBox::undo()
    {
        // Undo and redo act on the history rather than on a range, so they are the two edits that
        // do not come through applyEdit, and read-only is stated for them here.
        if (m_readOnly == ReadOnly::Yes)
            return;
        const std::optional<EditSelection> selection = m_history.undo(text());
        if (!selection.has_value())
            return;
        forgetPlaces();
        applySelection(selection.value());
        textEdited();
    }

    void TextBox::redo()
    {
        if (m_readOnly == ReadOnly::Yes)
            return;
        const std::optional<EditSelection> selection = m_history.redo(text());
        if (!selection.has_value())
            return;
        forgetPlaces();
        applySelection(selection.value());
        textEdited();
    }

    FloatRect TextBox::scrollHotspot() const
    {
        // Placed or not is the range's start: a whole-text selection holds k_maxSize as its length.
        if (m_editProps.selRange.start == k_maxSize)
            return Label::scrollHotspot();

        const FloatRect textBounds = textBoundsInControl();
        FloatRect result = caretRect(textBounds, { m_editProps.caretPos(), m_editProps.affinityTrailing });
        // The caret is drawn at the left edge of the rect the layout answers with, and the width
        // of the character it stands before says nothing about where the eye is looking.
        result.right = result.left;
        return result;
    }

    FloatRect TextBox::contextMenuAnchor() const
    {
        // A box that was never given a caret has no place inside itself to name, and the whole
        // box is then the honest answer - the same test scrollHotspot makes above.
        if (m_editProps.selRange.start == k_maxSize)
            return Label::contextMenuAnchor();

        // Measured in FORM space by handing the form rect in: textBounds and caretRect both
        // answer in whatever space the rect they are given is in, so nothing is converted
        // afterwards. Same route Control::getTooltip takes for an OverText anchor.
        const FloatRect textBounds = this->textBounds(formContext(), boundsInForm());
        return caretRect(textBounds, { m_editProps.caretPos(), m_editProps.affinityTrailing });
    }

    void TextBox::setSelection(std::size_t otherPos, std::size_t caretPos)
    {
        // don't touch caretOnLeft if they are equal - it remembers the user's input
        if (caretPos < otherPos)
            m_editProps.caretOnLeft = true;
        else if (caretPos > otherPos)
            m_editProps.caretOnLeft = false;

        if (m_editProps.caretOnLeft)
            m_editProps.selRange = { caretPos, otherPos - caretPos };
        else
            m_editProps.selRange = { otherPos, caretPos - otherPos };
        caretMoved();
    }

    void TextBox::paintText(PaintEvent& event)
    {
        m_editProps.caretVisible = m_caretOn && visualState().focused;
        Label::paintText(event);
    }

    // The whole text only when this box cannot say what changed. An edit was measured against
    // text(), the box's own, while the layout is handed whatever the gather produced - and a box
    // whose text comes from its parent is not the same text. The lengths agreeing is what says the
    // two are one story; where they do not, the whole text is stated and nothing is assumed.
    void TextBox::takeText(const Text& text) const
    {
        bool incremental = false;
        if (m_pendingEdit.has_value())
        {
            const std::ptrdiff_t expected = static_cast<std::ptrdiff_t>(m_layoutText.plainText().size())
                + static_cast<std::ptrdiff_t>(m_pendingEdit.value().insertedLength)
                - static_cast<std::ptrdiff_t>(m_pendingEdit.value().replaced.length);
            incremental = expected == static_cast<std::ptrdiff_t>(text.plainText().size());
        }

        m_layoutText = text;
        if (!incremental || !m_layout.applyTextEdit(m_layoutText, m_pendingEdit.value()))
            m_layout.setText(m_layoutText);

        // The edit is handed on whenever the lengths agreed, whether or not the layout took it:
        // it describes what the text did either way, and only the layout has a reason to decline.
        textTaken(m_layoutText, incremental ? &m_pendingEdit.value() : nullptr);

        // One edit, one chance to use it: a record left standing would be applied to a change it
        // does not describe.
        m_pendingEdit.reset();
    }


    void TextBox::focusChanged()
    {
        Label::focusChanged();
        // A box holding the focus is where typing goes, so it shows a caret whether or not one was
        // ever placed in it - otherwise a box reached by Tab has nothing to blink. Leaving the box
        // takes the same route: restartCaretBlink schedules nothing while the focus is elsewhere,
        // so the timer stops on the spot rather than on its next tick.
        if (isFocused())
            ensureCaret();
        caretMoved();
    }

    void TextBox::pressDown(PressDownEvent& event)
    {
        setCaretPosFromMouse();
        Label::pressDown(event);
    }

    void TextBox::doubleClick(DoubleClickEvent& event)
    {
        Label::doubleClick(event);
        if (event.propagationStopped())
            return;
        const FormContext& formContext = this->formContext();
        FloatRect textBounds = this->textBounds(formContext, boundsInForm());
        // The word is looked for in the text the hit indexes, which is the one the layout was
        // built from and the one on screen.
        CaretHit hit = caretHitAt(formContext, textBounds, event.clickPos());
        m_editProps.selRange = textEngine().wordAt(m_layoutText, hit.pos);
        m_editProps.caretOnLeft = false;
        caretMoved();
        event.stopPropagation(); // to prevent pressDown()
    }

    void TextBox::tripleClick(TripleClickEvent& event)
    {
        Label::tripleClick(event);
        if (event.propagationStopped())
            return;
        const FormContext& formContext = this->formContext();
        FloatRect textBounds = this->textBounds(formContext, boundsInForm());
        // The paragraph is read out of the text the hit indexes, which is the one the layout was
        // built from and the one on screen - the same text the double click takes its word from.
        CaretHit hit = caretHitAt(formContext, textBounds, event.clickPos());
        m_editProps.selRange = textEngine().paragraphAt(m_layoutText, hit.pos);
        m_editProps.caretOnLeft = false;
        caretMoved();
        event.stopPropagation(); // to prevent pressDown()
    }

    void TextBox::drag(DragEvent& event)
    {
        Label::drag(event);

        if (event.propagationStopped())
            return;
        event.lockHoveredControl();

        const FormContext& formContext = this->formContext();
        FloatRect thisRect = boundsInForm();
        FloatRect textBounds = this->textBounds(formContext, thisRect);

        // Both ends of the drag are asked of ONE layout. caretHitAt syncs before it answers, and
        // a sync gathers and compares the whole text - so asking it twice pays that twice over,
        // for a text that cannot have changed between the two questions. This runs on every
        // pointer move, and a move that costs more than the pointer's own sample interval leaves
        // input pending at all times, which is what holds WM_PAINT off until the pointer stops.
        const FloatPoint startPos = event.startPos();
        const FloatPoint currentPos = event.currentPos();
        TextLayout& layout = syncedLayout(formContext, textBounds);
        const FloatPoint origin = anchoredOrigin(textBounds, layout.calculatedDimensions(),
            textAnchor());

        std::size_t otherPos = layout.caretPos(startPos - origin).pos;
        std::size_t caretPos = layout.caretPos(currentPos - origin).pos;
        setSelection(otherPos, caretPos);
        m_editProps.targetX.reset();
    }

    void TextBox::keyDown(KeyDownEvent& event)
    {
        Label::keyDown(event);
        if (event.propagationStopped())
            return;

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
                    caretPos = syncedLayout(formContext, textBounds).rowStart(
                        { caretPos, m_editProps.affinityTrailing });
                m_editProps.affinityTrailing = false; // Home ALWAYS snaps to start of line
                break;

            case Keys::End:
                if (event.modifiers.ctrl)
                    caretPos = text().plainText().size();
                else
                    caretPos = syncedLayout(formContext, textBounds).rowEnd(
                        { caretPos, m_editProps.affinityTrailing });
                m_editProps.affinityTrailing = true; // End ALWAYS snaps to end of line
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

    void TextBox::charPress(CharPressEvent& event)
    {
        std::wstring_view sw;
        if (event.character() == Keys::Return)
            sw = L"\n";
        else
        {
            if (!std::iswprint(event.character()))
                return;
            sw = std::wstring_view{ &event.character(), 1 };
        }
        // A run of plain typing is one step to undo. A character written over a selection is a
        // step of its own: the user drew the boundary it replaced.
        const TextRange range = ensureCaret();
        const EditKind kind = range.length
            ? EditKind::Replace
            : EditKind::Typing;
        applyEdit(range, Text{ sw }, kind);
    }

    FloatRect TextBox::textBoundsInControl() const
    {
        return textBounds(formContext(), FloatRect::fromDimensions({ 0.0f, 0.0f }, dimensions()));
    }

    CaretHit TextBox::caretHitAt(const FormContext& formContext, const FloatRect& textBounds,
        FloatPoint pt) const
    {
        TextLayout& layout = syncedLayout(formContext, textBounds);
        const FloatPoint origin = anchoredOrigin(textBounds, layout.calculatedDimensions(),
            textAnchor());
        return layout.caretPos(pt - origin);
    }

    FloatRect TextBox::caretRect(const FloatRect& textBounds, CaretHit caretHit) const
    {
        TextLayout& layout = syncedLayout(formContext(), textBounds);
        const FloatPoint origin = anchoredOrigin(textBounds, layout.calculatedDimensions(),
            textAnchor());
        FloatRect result = layout.getCaretRect(caretHit);
        result.offset(origin);
        return result;
    }

    CaretHit TextBox::posOnNextRow(const FloatRect& textBounds, CaretHit caretHit,
        ScrollDirection direction) const
    {
        TextLayout& layout = syncedLayout(formContext(), textBounds);
        // targetX is a column in the caller's coordinates, and the row it names is looked for in
        // the layout's - so it crosses the same origin every position here crosses.
        const FloatPoint origin = anchoredOrigin(textBounds, layout.calculatedDimensions(),
            textAnchor());
        std::optional<float> targetX = m_editProps.targetX;
        if (targetX.has_value())
            targetX = targetX.value() - origin.x;
        return layout.posOnNextRow(caretHit, direction, targetX);
    }

    TextBox::PageMove TextBox::posOnNextPage(const FloatRect& textBounds, CaretHit caretHit,
        ScrollDirection direction) const
    {
        const bool forward = direction == ScrollDirection::ToEnd;
        const FloatRect sourceRect = caretRect(textBounds, caretHit);

        // The viewport in this control's own coordinates, which is the space the caret is
        // measured in. A page is a distance on screen, and nothing about where the caret happens
        // to stand inside the view says what that distance is.
        //
        // THE VIEW A GLIDE IS HEADING FOR, NOT THE ONE ON SHOW. A held page key repeats faster
        // than the 220ms glide it starts, so the view a press reads is still on its way to where
        // the press before it sent it. Measuring from that view names the screenful the previous
        // press already claimed, and the caret lands back inside it - a held key would then
        // travel one page and stop. ScrollBox answers the request the same way, so the page this
        // names is laid against the one in flight and the two accumulate.
        FloatRect view = visibleRectInForm();
        view.offset(-boundsInForm().topLeft());
        view.offset(viewTravelRemaining());

        // ScrollBox measures against its client rect, which is the viewport inset by this margin
        // at both ends. A page of exactly that height is what makes the two overlaps it compares
        // equal below, and their minimum is the distance the view travels. A box shorter than one
        // row still moves by a row, so a page press is never weaker than an arrow.
        const float margin = scaler().scaled8;
        const float pageExtent = std::max(view.height() - 2.0f * margin, sourceRect.height());

        // The caret's own row, with one scrolled out of sight standing at the edge of the view it
        // went off: a row nobody can see names no place to measure from.
        FloatRect sourceRow = sourceRect;
        if (sourceRow.top < view.top)
            sourceRow.offset(0.0f, view.top - sourceRow.top);
        else if (sourceRow.bottom > view.bottom)
            sourceRow.offset(0.0f, view.bottom - sourceRow.bottom);

        // A screenful on from the edge the move leads with - the row's top going forward, its
        // bottom going back - so what the user was reading is still under the caret one screenful
        // further on, and a press passes the same text whichever way it goes.
        const float targetY = forward
            ? sourceRow.top + pageExtent
            : sourceRow.bottom - pageExtent;
        PageMove result{};
        result.hit = caretHitAt(formContext(), textBounds,
            { m_editProps.targetX.value_or(sourceRect.left), targetY });
        FloatRect hitRect = caretRect(textBounds, result.hit);

        // THE VIEW TRAVELS THE DISTANCE THE CARET TRAVELLED, WHICH IS THE SCREENFUL SETTLED ON A
        // ROW. A screenful names a point, and the caret can only stand on the row holding that
        // point, so it arrives short of the screenful by however far into that row the point
        // fell. A view given the whole screenful takes that difference out of the caret's place
        // in it, and a held key adds one up per press until the caret has climbed out of the
        // view. Measured off the caret instead, the two keep step for as long as the key is
        // held, and what the view gives up is a sliver of a row it has already shown - a press
        // passes nothing unseen either way.
        //
        // TOP TO TOP, BOTH DIRECTIONS. The distance is the caret's, and the leading edge is only
        // where the screenful was counted from: measured from the row's bottom going back, the
        // view is handed the row's own height on top of the screenful, so it passes a row unseen
        // and the caret slides a whole row down the view on every press.
        //
        // The page is stated as the rect the view BECOMES: the client area carried by that
        // distance. ScrollBox puts a rect no taller than its client area against the leading
        // edge, so naming that rect moves the view by exactly the travel, whichever way it goes.
        const float travel = hitRect.top - sourceRow.top;
        result.page.top = view.top + margin + travel;
        result.page.bottom = result.page.top + pageExtent;

        // The caret keeps the place in the view it had, and the row it lands on is not the row it
        // left: a taller row at the same offset reaches past the edge, and a caret that was out
        // of sight starts from the edge itself. Either way the caret is drawn on the sliver of
        // the row that is left, inside the fade ScrollBox paints at that edge - on screen and
        // impossible to find. One step INTO the page puts it on the first row shown whole. Only
        // the caret moves: the view keeps the travel it was given, so the step costs no coverage.
        if (hitRect.top < result.page.top || hitRect.bottom > result.page.bottom)
        {
            const ScrollDirection inward = hitRect.top < result.page.top
                ? ScrollDirection::ToEnd
                : ScrollDirection::ToBegin;
            result.hit = posOnNextRow(textBounds, result.hit, inward);
            hitRect = caretRect(textBounds, result.hit);
        }

        // The page carries the column the caret came to rest in, so a view that has to move
        // sideways to show the caret moves with the same request.
        result.page.left = hitRect.left;
        result.page.right = hitRect.left;
        return result;
    }

    bool TextBox::mousePosInSelection() const
    {
        // A keyboard-raised menu has no point to test, and an empty selection contains nothing.
        if (Input::device() != InputDevice::Mouse || !m_editProps.selRange.length)
            return false;

        const std::wstring& plainText = text().plainText();
        std::size_t start = m_editProps.selRange.start;
        if (start >= plainText.size())
            return false;
        // selRange.length holds k_maxSize while the whole text is selected, so the end is the
        // shorter of the range and the text - adding the two would wrap.
        std::size_t end = start + std::min(m_editProps.selRange.length, plainText.size() - start);

        const FormContext& formContext = this->formContext();
        FloatRect textBounds = this->textBounds(formContext, boundsInForm());
        CaretHit hit = caretHitAt(formContext, textBounds, form().mouseDownPos());
        // The character under the pointer, not the insertion point the hit names: a click on the
        // trailing half of the last selected character answers the position after it, which the
        // range does not contain, and that half of the glyph is highlighted like the rest.
        std::size_t charPos = hit.trailing ? hit.pos - 1 : hit.pos;
        return charPos >= start && charPos < end;
    }

    TextRange TextBox::ensureCaret()
    {
        if (m_editProps.selRange.start == k_maxSize)
            m_editProps.selRange = { 0, 0 };
        return m_editProps.selRange;
    }

    void TextBox::applyEdit(TextRange range, const Text& inserted, EditKind kind)
    {
        // Every route that changes the text arrives here - the delete keys, typing, the edit
        // actions and the box's own replace and delete calls - so one refusal covers all of them.
        // A host still writes the text of a read-only box through setText.
        if (m_readOnly == ReadOnly::Yes)
            return;

        const EditSelection before = {
            m_editProps.selRange,
            m_editProps.caretOnLeft,
            m_editProps.affinityTrailing,
        };

        // What the text is about to have done to it, clamped the way replaceText clamps it, so the
        // layout is told the edit that happened rather than the one that was asked for.
        const std::size_t textSize = text().plainText().size();
        const std::size_t editStart = std::min(range.start, textSize);
        const TextEdit thisEdit{
            .replaced = { editStart, std::min(range.length, textSize - editStart) },
            .insertedLength = inserted.plainText().size(),
        };
        // Merged rather than replaced: a held key delivers characters faster than the paint loop
        // consumes them, so several edits reach the text before anything asks the layout about it.
        // A record that only carried the last of them would describe a change the text did not
        // make, and syncedLayout would then state the whole text - which is the freeze a run of
        // typing used to end in.
        m_pendingEdit = m_pendingEdit.has_value()
            ? mergedEdits(m_pendingEdit.value(), thisEdit)
            : thisEdit;
        carryPlaces(thisEdit);

        applySelection(m_history.apply(text(), kind, range, inserted, before));
        textEdited();
    }

    void TextBox::applySelection(const EditSelection& selection)
    {
        m_editProps.selRange = selection.range;
        m_editProps.caretOnLeft = selection.caretOnLeft;
        m_editProps.affinityTrailing = selection.affinityTrailing;
    }

    void TextBox::textEdited()
    {
        m_editProps.targetX.reset();
        // The link the pointer stood on was found in the text before the edit. The next move of
        // the pointer finds it again in this one.
        m_pointedLink = {};
        invalidateFormAlign();
        scrollIntoViewOnAlign();
        restartCaretBlink();
        announceCaretMove();
        // LAST, once the box has finished with the edit. A listener is free to do anything with
        // the text it has just been told about, the box included.
        TextEditEvent event{ *this };
        emitEvent(event);
    }

    void TextBox::caretMoved()
    {
        m_history.breakRun();
        restartCaretBlink();
        announceCaretMove();
    }

    void TextBox::announceCaretMove()
    {
        CaretMoveEvent event{ *this };
        emitEvent(event);
    }

    void TextBox::rememberTargetX(const FloatRect& textBounds, std::size_t caretPos)
    {
        if (m_editProps.targetX.has_value())
            return;
        m_editProps.targetX = caretRect(textBounds, { caretPos, m_editProps.affinityTrailing }).left;
    }

    void TextBox::restartCaretBlink()
    {
        m_caretOn = true;
        scheduleCaretBlink();
        invalidate();
    }

    void TextBox::scheduleCaretBlink()
    {
        // A box nobody is typing in has no caret to blink, and the rate is the user's to set -
        // zero is the setting for a caret that holds still. Either way nothing is scheduled, and
        // the caret keeps the state it was last put in.
        //
        // The VISUAL focus, not Input's: a menu opened on this box holds the real one, and the
        // box goes on being what the user is typing into - see Control::visualState.
        const MilliSeconds interval = Platform::caretBlinkTime();
        if (visualState().focused && interval.value)
            m_caretTimer.start(interval);
        else
            m_caretTimer.stop();
    }

    void TextBox::blinkCaret()
    {
        // focusChanged stops the timer when the focus leaves, so this is the backstop for a focus
        // that moved some other way - and it costs one state read per half second.
        if (!visualState().focused)
        {
            m_caretTimer.stop();
            return;
        }
        m_caretOn = !m_caretOn;
        scheduleCaretBlink();
        invalidate();
    }

}
