module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.HexView;

import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Which of the two byte columns the caret stands in. See Controls
    export enum class HexPane
    {
        Bytes,
        Characters
    };

    // A run of bytes, counted from the first byte the view was given.
    export struct HexRange
    {
        std::size_t start{ 0 };
        std::size_t length{ 0 };
    };

    export class HexView;

    // The selection has come to rest somewhere new. See Controls
    export struct SelectionMoveEvent : public EventOf<HexView>
    {
        using EventOf<HexView>::EventOf;
    };

    // Bytes drawn as the three columns a hex dump has: offset, digits, characters. See Controls
    export class HexView : public Control
    {
    public:
        using ByteView = std::span<const unsigned char>;
    public:
        template<typename... Args>
        explicit HexView(const CreateParams&, Args&&...);
    public:
        // The clear space kept inside the view's edge, around its three columns.
        DECLARE_WRITABLE_PROPERTY(Padding, padding, setPadding, Padding{})
    public:
        // The selection has come to rest somewhere new. See Controls
        DECLARE_EVENT(SelectionMoveEvent, OnSelectionMove, onSelectionMove)
    public:
        void setBytes(ByteView);
        [[nodiscard]] ByteView bytes() const { return m_bytes; }
        [[nodiscard]] std::size_t lineCount() const;
        // The byte the caret stands on. There is always one while the view holds any bytes, so
        // what a copy acts on is never empty.
        [[nodiscard]] std::size_t caretOffset() const { return m_caret; }
        // Moves the caret, and the selection with it: the range runs from the anchor, which
        // keeping the selection leaves where it is and dropping it brings to the caret.
        void setCaretOffset(std::size_t offset, bool keepSelection = false);
        [[nodiscard]] HexRange selection() const;
        // The selected bytes as the digits they are drawn in, one space between them. This is
        // what a copy puts on the clipboard.
        [[nodiscard]] std::wstring selectionAsHex() const;
        // The stamp is the input event that asked for the copy. A display server that authorises
        // the request refuses one naming no event - see Transfer::Clipboard::set.
        void copySelection(InputStamp);
        void selectAll();
        // An offset spelled the way the view's own left column spells it, so a status line and
        // the column cannot disagree about how wide an offset is.
        [[nodiscard]] std::wstring offsetText(std::size_t offset) const;
        void setPadding(Padding);
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"HexView"; }
    protected:
        [[nodiscard]] Interactivity interactivity() const override { return Interactivity::Focusable; }
        [[nodiscard]] CursorShape cursor() const override
        {
            return enabled(true) ? CursorShape::IBeam : CursorShape::Arrow;
        }
        // The caret's line, not the view. A view holding more than a screenful is on screen the
        // moment its first line is, so its own bounds name a scroll that is answered while the
        // caret is nowhere near sight.
        [[nodiscard]] FloatRect scrollHotspot() const override;
        // The caret's byte, and for the same reason. A menu raised by the keyboard drops under
        // the part of its owner the commands are about, and on a view taller than the screen
        // that is one byte rather than the whole dump.
        [[nodiscard]] FloatRect contextMenuAnchor() const override;
        void adjustMetrics(AdjustMetricsEvent&) const override;
        [[nodiscard]] ScaledDimensions calculateContent(AlignEvent&) override;
        void paintSurface(PaintEvent&) override;
        void pressDown(PressDownEvent&) override;
        void doubleClick(DoubleClickEvent&) override;
        void tripleClick(TripleClickEvent&) override;
        void drag(DragEvent&) override;
        void mouseMove(MouseMoveEvent&) override;
        void hoverLeave() override;
        void contextPopup(ContextPopupEvent&) override;
        void keyDown(KeyDownEvent&) override;
        void focusChanged() override;
    private:
        // Where a point fell: the byte it names, and the column it fell in.
        struct Hit
        {
            std::size_t offset{ 0 };
            HexPane pane{ HexPane::Bytes };
        };
    private:
        void measureCharacterCell(const FormContext&);
        // The view volunteering as the subject of the commands that act on a selection.
        // Connected from the constructor, the way TextBox connects the edit actions.
        void connectEditActions();
        void announceSelectionMove();
        // The digits of a value, written into a text most significant first. This is the paint
        // path's - it is called for every line of every frame and allocates nothing, which is
        // what separates it from offsetText.
        static void appendHex(Text&, std::size_t value, int digits);
        // A point in the form, in the view's own coordinates - which is what hitAt reads.
        [[nodiscard]] FloatPoint pointInControl(PointInForm) const;
        // The byte a point in the view's own coordinates names, clamped into the line it fell
        // on. None where the point lies above the first line or below the last.
        [[nodiscard]] std::optional<Hit> hitAt(FloatPoint) const;
        // Whether the pointer stands on a byte the selection covers. Asked of a MOUSE-raised
        // menu only - a menu the keyboard raised has no point to test.
        [[nodiscard]] bool pointerInSelection() const;
        // Takes the press or the drag move at that point: the caret goes to the byte it names,
        // and the pane it fell in becomes the one the ring is drawn in.
        void takeHit(FloatPoint, bool keepSelection);
        // Takes a run of clicks at that point: the selection becomes the aligned span of that
        // many bytes the pointer's byte falls in - the group a double click claims, the line a
        // triple click claims. Both spans are aligned from the first byte, which is where the
        // view draws its own group break. False where the point named no byte.
        [[nodiscard]] bool takeAlignedSpan(FloatPoint, std::size_t span);
        [[nodiscard]] int lineColumns() const;
        // Where a byte's characters start, counted from the line's first column.
        [[nodiscard]] int byteColumn(std::size_t indexInLine, HexPane) const;
        [[nodiscard]] FloatRect contentRect(const FloatRect& bounds, ScaledPadding) const;
        // The same rect outside a paint, in the view's own coordinates and in the form's. The
        // hit test and the scroll work in the first; a menu anchor is stated in the second.
        [[nodiscard]] FloatRect contentRectInControl() const;
        [[nodiscard]] FloatRect contentRectInForm() const;
        [[nodiscard]] FloatRect byteRect(const FloatRect& content, std::size_t offset, HexPane) const;
        // How many whole lines the viewport above the view shows, and never fewer than one: it
        // is what the page keys travel by.
        [[nodiscard]] std::size_t pageLines() const;
        void buildLineText(std::size_t line);
        void paintLine(PaintEvent&, const FloatRect& content, std::size_t line);
        void paintLineBands(PaintEvent&, const FloatRect& content, std::size_t line);
    private:
        static constexpr std::size_t k_bytesPerLine = 16;
        // Where the wider space between the two halves of the bytes column falls.
        static constexpr std::size_t k_groupSize = 8;
        // The clear characters between the offset and the bytes, and between the bytes and the
        // characters they read as.
        static constexpr int k_offsetGap = 2;
        static constexpr int k_paneGap = 2;
        // Two digits a byte, one space between them, and one more where the group breaks.
        static constexpr int k_bytesColumns = static_cast<int>(k_bytesPerLine) * 2
            + static_cast<int>(k_bytesPerLine) - 1
            + 1;
        static constexpr int k_characterColumns = static_cast<int>(k_bytesPerLine);
        // Six digits address sixteen megabytes, which is more than a clipboard payload reaches;
        // past that the column widens to eight.
        static constexpr int k_shortOffsetColumns = 6;
        static constexpr int k_longOffsetColumns = 8;
        static constexpr std::size_t k_shortOffsetLimit = 0x1000000ull;
        // The run measured to get one character's width. Long enough that the rounding of a
        // single advance does not set the whole grid.
        static constexpr std::wstring_view k_measuredRun = L"0000000000000000";
        static constexpr std::wstring_view k_digits = L"0123456789abcdef";
        // What a byte no font draws is shown as, in both columns.
        static constexpr wchar_t k_unprintable = L'.';
        static constexpr unsigned char k_firstPrintable = 0x20;
        static constexpr unsigned char k_lastPrintable = 0x7e;
    private:
        ByteView m_bytes{};
        std::size_t m_anchor{ 0 };
        std::size_t m_caret{ 0 };
        // The byte under the pointer, or none while the pointer is off the view.
        std::optional<std::size_t> m_hovered{};
        HexPane m_pane{ HexPane::Bytes };
        // How many digits the offset column carries, which the size of the buffer decides.
        int m_offsetColumns{ k_shortOffsetColumns };
        // The character grid the last layout pass measured, in scaled pixels. Zero says nothing
        // has been measured yet, and a paint arriving before the first align draws nothing.
        float m_charWidth{ 0.0f };
        float m_lineHeight{ 0.0f };
        // One line's characters, and the layout they are shaped into. Both are held across the
        // paint so that drawing a screenful takes one of each, and the text outlives the layout,
        // which names it rather than copying it.
        Text m_lineText{};
        TextLayout m_layout{};
    };


    //-------------------------------------------------------------------------


    // HexView
    //
    // The constructor is the one definition that has to stay here: it is a template, so every
    // caller instantiates it from this interface. Every other body lives in HexView.cpp.

    template<typename... Args>
    HexView::HexView(const CreateParams& params, Args&&... args)
        :
        Control{
            params,
            // A LINE IS A FIXED GRID OF CHARACTERS, so nothing about it can be broken to the box
            // it is drawn in: the view is as wide as its line whatever width it is offered, and
            // the horizontal bar is what carries the view across the rest. It is also what tells
            // the align pass that a scrolled body keeps the size it measured - a wrapping body is
            // cut to its slot instead, which leaves a view of any length exactly one screen tall
            // and nothing for the vertical bar to carry. It comes before the caller's own
            // arguments, so a stated one still wins - Props takes the last match.
            WordWrap::No,
            std::forward<Args>(args)... },
        INIT_PROPERTY(padding)
    {
        connectEditActions();
    }

}
