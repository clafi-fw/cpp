module ClaFi.Controls.HexView;

import ClaFi.Controls.Menu;

import ClaFi.StdActions;

import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.Core.Context.FormContext;

import ClaFi.Core.Transfer.Clipboard;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Transfer.Source;

import ClaFi.Core.Graphics.Canvas;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    void HexView::setBytes(const ByteView value)
    {
        m_bytes = value;
        m_anchor = 0;
        m_caret = 0;
        m_hovered.reset();
        m_pane = HexPane::Bytes;
        m_offsetColumns = m_bytes.size() > k_shortOffsetLimit
            ? k_longOffsetColumns
            : k_shortOffsetColumns;

        // What the two commands answer turns on whether there is anything to act on, and a
        // presenter keeps the answer it was last given until it is asked again.
        StdActions::copy.invalidateState();
        StdActions::selectAll.invalidateState();
        invalidateFormAlign();
        announceSelectionMove();
    }

    std::size_t HexView::lineCount() const
    {
        return (m_bytes.size() + k_bytesPerLine - 1) / k_bytesPerLine;
    }

    void HexView::setCaretOffset(const std::size_t offset, const bool keepSelection)
    {
        if (m_bytes.empty())
            return;

        m_caret = std::min(offset, m_bytes.size() - 1);
        if (!keepSelection)
            m_anchor = m_caret;

        scrollIntoView();
        invalidate();
        announceSelectionMove();
    }

    HexRange HexView::selection() const
    {
        if (m_bytes.empty())
            return {};

        const std::size_t first = std::min(m_anchor, m_caret);
        const std::size_t last = std::max(m_anchor, m_caret);
        return { first, last - first + 1 };
    }

    std::wstring HexView::selectionAsHex() const
    {
        const HexRange range = selection();
        std::wstring result{};
        if (!range.length)
            return result;

        // Three characters a byte, less the space the last one does not carry.
        result.reserve(range.length * 3 - 1);
        for (std::size_t at = range.start; at != range.start + range.length; ++at)
        {
            if (at != range.start)
                result += L' ';

            const unsigned char value = m_bytes[at];
            result += k_digits[value >> 4];
            result += k_digits[value & 0x0f];
        }
        return result;
    }

    void HexView::copySelection(const InputStamp stamp)
    {
        if (m_bytes.empty())
            return;

        // ONE FORMAT OUT. The digits are what survives the crossing: an application that has
        // never heard of this one reads them as text, and they read back as the bytes they are.
        Transfer::Source source{};
        source.add<Transfer::PlainText>(selectionAsHex());
        formContext().clipboard().set(std::move(source), stamp);
    }

    void HexView::selectAll()
    {
        if (m_bytes.empty())
            return;

        m_anchor = 0;
        m_caret = m_bytes.size() - 1;
        scrollIntoView();
        invalidate();
        announceSelectionMove();
    }

    std::wstring HexView::offsetText(const std::size_t offset) const
    {
        std::wstring result{};
        result.reserve(static_cast<std::size_t>(m_offsetColumns));
        for (int shift = (m_offsetColumns - 1) * 4; shift >= 0; shift -= 4)
        {
            result += k_digits[(offset >> shift) & 0xf];
        }
        return result;
    }

    void HexView::setPadding(const Padding value)
    {
        m_padding = value;
        invalidateFormAlign();
    }

    // The caret's cell, not its line - a wider-than-view rect only scrolls away from the caret.
    FloatRect HexView::scrollHotspot() const
    {
        if (m_bytes.empty() || m_lineHeight <= 0.0f)
            return Control::scrollHotspot();

        return byteRect(contentRectInControl(), m_caret, m_pane);
    }

    FloatRect HexView::contextMenuAnchor() const
    {
        if (m_bytes.empty() || m_lineHeight <= 0.0f)
            return Control::contextMenuAnchor();

        return byteRect(contentRectInForm(), m_caret, m_pane);
    }

    void HexView::adjustMetrics(AdjustMetricsEvent& event) const
    {
        event.metrics.padding = m_padding;
    }

    ScaledDimensions HexView::calculateContent(AlignEvent& event)
    {
        measureCharacterCell(event.formContext());

        if (m_bytes.empty())
            return { 0.0f, 0.0f };

        return {
            static_cast<float>(lineColumns()) * m_charWidth,
            static_cast<float>(lineCount()) * m_lineHeight,
        };
    }

    void HexView::paintSurface(PaintEvent& event)
    {
        Control::paintSurface(event);

        if (m_bytes.empty() || m_lineHeight <= 0.0f)
            return;

        const FloatRect content = contentRect(event.controlBounds(), event.padding());
        const FloatRect visible = FloatRect::intersection(event.viewport(), content);
        if (visible.empty())
            return;

        // THE LINES THE VIEWPORT REACHES AND NO OTHERS. The bound is taken before a line is
        // built, so what a paint costs is set by the size of the view and not by the size of the
        // buffer behind it.
        const float firstLine = std::max(0.0f,
            std::floor((visible.top - content.top) / m_lineHeight));
        const float lastLine = std::max(0.0f,
            std::ceil((visible.bottom - content.top) / m_lineHeight));
        const std::size_t first = static_cast<std::size_t>(firstLine);
        const std::size_t last = std::min(static_cast<std::size_t>(lastLine), lineCount());

        for (std::size_t line = first; line < last; ++line)
        {
            paintLine(event, content, line);
        }
    }

    void HexView::pressDown(PressDownEvent& event)
    {
        Control::pressDown(event);
        if (event.propagationStopped())
            return;

        takeHit(pointInControl(event.clickPos()), event.modifiers.shift);
    }

    void HexView::doubleClick(DoubleClickEvent& event)
    {
        Control::doubleClick(event);
        if (event.propagationStopped())
            return;

        // A gesture that claimed bytes is the whole of the press: letting it through would put
        // the caret back on the one byte under it. One that claimed none is let through as the
        // press it also is, so the view still takes the focus from it.
        if (takeAlignedSpan(pointInControl(event.clickPos()), k_groupSize))
            event.stopPropagation();
    }

    void HexView::tripleClick(TripleClickEvent& event)
    {
        Control::tripleClick(event);
        if (event.propagationStopped())
            return;

        if (takeAlignedSpan(pointInControl(event.clickPos()), k_bytesPerLine))
            event.stopPropagation();
    }

    void HexView::drag(DragEvent& event)
    {
        const std::optional<Hit> hit = hitAt(pointInControl(event.currentPos()));
        if (!hit)
            return;

        // The column the press landed in is the one the drag stays in. The range is over bytes,
        // and the column the pointer wanders into does not change which bytes those are.
        setCaretOffset(hit->offset, true);
    }

    void HexView::mouseMove(MouseMoveEvent& event)
    {
        Control::mouseMove(event);

        const std::optional<Hit> hit = hitAt(event.posOnControl);
        std::optional<std::size_t> hovered{};
        if (hit)
            hovered = hit->offset;

        if (hovered == m_hovered)
            return;

        m_hovered = hovered;
        invalidate();
    }

    void HexView::hoverLeave()
    {
        Control::hoverLeave();

        if (!m_hovered)
            return;

        m_hovered.reset();
        invalidate();
    }

    void HexView::contextPopup(ContextPopupEvent& event)
    {
        // Outside the selection the press states a new caret, as a left one does. Inside it, the
        // press is aimed at the range the menu's Copy acts on, and moving the caret there would
        // collapse that range before the menu is even up. A menu the keyboard raised moves
        // nothing: the caret already stands where the user put it.
        if (Input::device() == InputDevice::Mouse && !pointerInSelection())
            takeHit(pointInControl(form().mouseDownPos()), false);

        // The application gets first refusal, and a handler that stops the event has replaced
        // the menu outright.
        Control::contextPopup(event);
        if (event.propagationStopped())
            return;

        ActionList items{
            &StdActions::copy,
            nullptr,
            &StdActions::selectAll,
        };

        // The second refusal, and a narrower one: this menu is the standard edit menu for a view
        // that cannot be edited, and a handler here adjusts it rather than replacing it. Each
        // action answers for its own state, so a command the view cannot run right now arrives
        // disabled without anything here sorting them.
        EditContextPopupEvent editEvent{ *this, event.form, items, event.mousePos };
        editContextPopup(editEvent);
        if (editEvent.propagationStopped())
            return;

        // NO COMMAND STRIP. The strip across the top of a menu compresses a long edit menu into
        // icons, and two commands have nothing to compress - see TextBox::contextPopup, which
        // has seven.
        Menu menu{ *this };
        menu.add(items);
        menu.execute();
    }

    void HexView::keyDown(KeyDownEvent& event)
    {
        Control::keyDown(event);
        if (event.propagationStopped())
            return;
        if (m_bytes.empty())
            return;

        const std::size_t last = m_bytes.size() - 1;
        const std::size_t indexInLine = m_caret % k_bytesPerLine;
        const std::size_t page = pageLines() * k_bytesPerLine;
        std::size_t target;

        switch (event.key)
        {

        case Keys::Left:
            target = m_caret ? m_caret - 1 : 0;
            break;

        case Keys::Right:
            target = m_caret + 1;
            break;

        case Keys::Up:
            target = m_caret >= k_bytesPerLine ? m_caret - k_bytesPerLine : m_caret;
            break;

        case Keys::Down:
            target = m_caret + k_bytesPerLine;
            break;

        case Keys::Prior:
            target = m_caret >= page ? m_caret - page : indexInLine;
            break;

        case Keys::Next:
            target = m_caret + page;
            break;

        case Keys::Home:
            target = event.modifiers.ctrl ? 0 : m_caret - indexInLine;
            break;

        case Keys::End:
            target = event.modifiers.ctrl ? last : m_caret - indexInLine + k_bytesPerLine - 1;
            break;

        default:
            return;
        }

        event.handled = true;
        setCaretOffset(target, event.modifiers.shift);
    }

    void HexView::focusChanged()
    {
        Control::focusChanged();
        invalidate();
    }

    void HexView::measureCharacterCell(const FormContext& formContext)
    {
        m_lineText.clear();
        m_lineText << TextStyleId::Code << k_measuredRun;

        const CalculatedDimensions measured = textEngine().calculateText(formContext, m_lineText,
            { k_maxFloat, k_maxFloat }, false, false);
        m_charWidth = measured.x / static_cast<float>(k_measuredRun.size());
        m_lineHeight = measured.y;
    }

    void HexView::connectEditActions()
    {
        // Claiming says this control is what the command acts on; what it claims says whether it
        // can act right now. A view with no bytes claims both and reports them disabled, which is
        // not the same as the commands being about something else.
        onGetActionState([this](GetActionStateEvent& event){
            const bool hasBytes = !m_bytes.empty();
            if (&event.action == &StdActions::copy)
                event.claim({ .enabled = hasBytes });
            else if (&event.action == &StdActions::selectAll)
                event.claim({ .enabled = hasBytes });
        });

        onActionClick([this](ActionClickEvent& event){
            if (&event.action == &StdActions::copy)
                copySelection(event.stamp);
            else if (&event.action == &StdActions::selectAll)
                selectAll();
        });
    }

    void HexView::announceSelectionMove()
    {
        SelectionMoveEvent event{ *this };
        emitEvent(event);
    }

    void HexView::appendHex(Text& text, const std::size_t value, const int digits)
    {
        for (int shift = (digits - 1) * 4; shift >= 0; shift -= 4)
        {
            text << k_digits[(value >> shift) & 0xf];
        }
    }

    FloatPoint HexView::pointInControl(const PointInForm point) const
    {
        const FloatRect bounds = boundsInForm();
        return { point.x - bounds.left, point.y - bounds.top };
    }

    std::optional<HexView::Hit> HexView::hitAt(const FloatPoint point) const
    {
        if (m_bytes.empty() || m_charWidth <= 0.0f || m_lineHeight <= 0.0f)
            return {};

        const FloatRect content = contentRectInControl();
        const float lineOffset = (point.y - content.top) / m_lineHeight;
        if (lineOffset < 0.0f)
            return {};

        const std::size_t line = static_cast<std::size_t>(lineOffset);
        if (line >= lineCount())
            return {};

        const std::size_t lineStart = line * k_bytesPerLine;
        const std::size_t lineLast = std::min(lineStart + k_bytesPerLine, m_bytes.size()) - 1;
        const int column = static_cast<int>(std::floor((point.x - content.left) / m_charWidth));

        const int charactersStart = byteColumn(0, HexPane::Characters);
        if (column >= charactersStart)
        {
            const std::size_t indexInLine = static_cast<std::size_t>(column - charactersStart);
            return Hit{ std::min(lineStart + indexInLine, lineLast), HexPane::Characters };
        }

        // EVERYTHING LEFT OF THE CHARACTERS NAMES A BYTE IN THE DIGITS. The offset column and
        // both gaps round to the byte nearest them, so a press anywhere on the line lands on one
        // rather than falling through to nothing.
        const int bytesStart = byteColumn(0, HexPane::Bytes);
        int withinField = column - bytesStart;
        if (withinField < 0)
            withinField = 0;
        if (withinField >= byteColumn(k_groupSize, HexPane::Bytes) - bytesStart)
            withinField -= 1;

        const std::size_t indexInLine = static_cast<std::size_t>(withinField) / 3;
        return Hit{ std::min(lineStart + indexInLine, lineLast), HexPane::Bytes };
    }

    bool HexView::pointerInSelection() const
    {
        const std::optional<Hit> hit = hitAt(pointInControl(form().mouseDownPos()));
        if (!hit)
            return false;

        const HexRange range = selection();
        return hit->offset >= range.start && hit->offset < range.start + range.length;
    }

    void HexView::takeHit(const FloatPoint point, const bool keepSelection)
    {
        const std::optional<Hit> hit = hitAt(point);
        if (!hit)
            return;

        m_pane = hit->pane;
        setCaretOffset(hit->offset, keepSelection);
    }

    bool HexView::takeAlignedSpan(const FloatPoint point, const std::size_t span)
    {
        const std::optional<Hit> hit = hitAt(point);
        if (!hit)
            return false;

        // The anchor at the near end and the caret at the far one, so Shift carries the range on
        // from where the run ended rather than from the middle of it.
        m_pane = hit->pane;
        m_anchor = hit->offset - hit->offset % span;
        m_caret = std::min(m_anchor + span, m_bytes.size()) - 1;
        invalidate();
        announceSelectionMove();
        return true;
    }

    int HexView::lineColumns() const
    {
        return m_offsetColumns + k_offsetGap + k_bytesColumns + k_paneGap + k_characterColumns;
    }

    int HexView::byteColumn(const std::size_t indexInLine, const HexPane pane) const
    {
        const int fieldStart = m_offsetColumns + k_offsetGap;
        if (pane == HexPane::Characters)
            return fieldStart + k_bytesColumns + k_paneGap + static_cast<int>(indexInLine);

        const int groupBreak = indexInLine >= k_groupSize ? 1 : 0;
        return fieldStart + static_cast<int>(indexInLine) * 3 + groupBreak;
    }

    FloatRect HexView::contentRect(const FloatRect& bounds, const ScaledPadding padding) const
    {
        FloatRect result = bounds;
        result.inflate(-padding.x, -padding.y);
        return result;
    }

    FloatRect HexView::contentRectInControl() const
    {
        return contentRect({ 0.0f, 0.0f, width(), height() }, scaledPadding());
    }

    FloatRect HexView::contentRectInForm() const
    {
        return contentRect(boundsInForm(), scaledPadding());
    }

    FloatRect HexView::byteRect(const FloatRect& content, const std::size_t offset,
        const HexPane pane) const
    {
        const std::size_t line = offset / k_bytesPerLine;
        const std::size_t indexInLine = offset % k_bytesPerLine;
        const float left = content.left
            + static_cast<float>(byteColumn(indexInLine, pane)) * m_charWidth;
        const float cellWidth = (pane == HexPane::Bytes ? 2.0f : 1.0f) * m_charWidth;
        const float top = content.top + static_cast<float>(line) * m_lineHeight;
        return {
            left,
            top,
            left + cellWidth,
            top + m_lineHeight,
        };
    }

    std::size_t HexView::pageLines() const
    {
        if (m_lineHeight <= 0.0f)
            return 1;

        const float visible = visibleRectInForm().height();
        const std::size_t lines = static_cast<std::size_t>(visible / m_lineHeight);
        return std::max<std::size_t>(lines, 1);
    }

    void HexView::buildLineText(const std::size_t line)
    {
        const std::size_t start = line * k_bytesPerLine;
        const std::size_t end = std::min(start + k_bytesPerLine, m_bytes.size());

        static_assert(k_offsetGap == 2 && k_paneGap == 2,
            "both gaps are written below as a pair of literal spaces");

        m_lineText.clear();
        m_lineText << TextStyleId::Code;

        m_lineText << InkGrade::Muted;
        appendHex(m_lineText, start, m_offsetColumns);
        m_lineText << PopColor{};
        m_lineText << L"  ";

        // A BYTE OF NOTHING IS DRAWN BACK TOWARDS THE SURFACE, so what the buffer carries is
        // what the eye finds first. The runs of zeroes a structure is padded with are most of
        // what a dump shows, and reading them at full strength is reading the padding.
        bool dimmed = false;
        for (std::size_t at = start; at != end; ++at)
        {
            if (at != start)
                m_lineText << L' ';
            if (at - start == k_groupSize)
                m_lineText << L' ';

            const unsigned char value = m_bytes[at];
            const bool dim = value == 0;
            if (dim != dimmed)
            {
                if (dim)
                    m_lineText << InkGrade::Subtle;
                else
                    m_lineText << PopColor{};
                dimmed = dim;
            }
            m_lineText << k_digits[value >> 4];
            m_lineText << k_digits[value & 0x0f];
        }
        if (dimmed)
            m_lineText << PopColor{};

        // The short last line is filled out to the width of a full one, so the characters beside
        // it stand in the column every other line put them in.
        const int drawnTo = byteColumn(end - start - 1, HexPane::Bytes) + 2;
        const int fieldEnd = byteColumn(0, HexPane::Bytes) + k_bytesColumns;
        for (int column = drawnTo; column != fieldEnd; ++column)
        {
            m_lineText << L' ';
        }
        m_lineText << L"  ";

        dimmed = false;
        for (std::size_t at = start; at != end; ++at)
        {
            const unsigned char value = m_bytes[at];
            const bool printable = value >= k_firstPrintable && value <= k_lastPrintable;
            const bool dim = !printable;
            if (dim != dimmed)
            {
                if (dim)
                    m_lineText << InkGrade::Subtle;
                else
                    m_lineText << PopColor{};
                dimmed = dim;
            }
            m_lineText << (printable ? static_cast<wchar_t>(value) : k_unprintable);
        }
        if (dimmed)
            m_lineText << PopColor{};
    }

    void HexView::paintLine(PaintEvent& event, const FloatRect& content, const std::size_t line)
    {
        paintLineBands(event, content, line);
        buildLineText(line);

        const float top = content.top + static_cast<float>(line) * m_lineHeight;
        m_layout.setEventPhase(EventPhase::Paint);
        m_layout.setText(m_lineText);
        m_layout.setWrap(false);
        m_layout.setBoundsAndScale({ content.width(), m_lineHeight },
            event.formContext().scaleFactor());
        m_layout.draw(event.controlContext(), { content.left, top }, nullptr, textRenderMode());
    }

    void HexView::paintLineBands(PaintEvent& event, const FloatRect& content,
        const std::size_t line)
    {
        const std::size_t lineStart = line * k_bytesPerLine;
        const std::size_t lineEnd = std::min(lineStart + k_bytesPerLine, m_bytes.size());

        auto fillRun = [&](std::size_t from, std::size_t to, HexPane pane, Color color){
            FloatRect rect = byteRect(content, from, pane);
            rect.right = byteRect(content, to, pane).right;
            event.canvas().fillRectangle(rect, color);
        };

        const HexRange range = selection();
        const std::size_t selectedFrom = std::max(range.start, lineStart);
        const std::size_t selectedTo = std::min(range.start + range.length, lineEnd);
        if (selectedFrom < selectedTo)
        {
            const Color band = event.controlContext().selectionRgb();
            fillRun(selectedFrom, selectedTo - 1, HexPane::Bytes, band);
            fillRun(selectedFrom, selectedTo - 1, HexPane::Characters, band);
        }

        // THE POINTER'S BYTE IS MARKED IN BOTH COLUMNS AT ONCE, which is the whole of what says
        // that the digits and the character beside them are one byte. Inside the selection there
        // is a band there already, and a second mark over it would read as a third state.
        const bool hoveredHere = m_hovered
            && *m_hovered >= lineStart
            && *m_hovered < lineEnd
            && (*m_hovered < selectedFrom || *m_hovered >= selectedTo);
        if (hoveredHere)
        {
            const Color tint = event.textRgb(InkGrade::Faint);
            fillRun(*m_hovered, *m_hovered, HexPane::Bytes, tint);
            fillRun(*m_hovered, *m_hovered, HexPane::Characters, tint);
        }

        // The ring stands in the column the press landed in, and only while the view has the
        // focus: the band says what the commands act on, and the ring says where the keys are.
        const float focused = event.focusedFactor();
        if (focused <= 0.0f || m_caret < lineStart || m_caret >= lineEnd)
            return;

        Color ring = event.indicatorRgb();
        ring.alpha = static_cast<ColorByte>(static_cast<float>(ring.alpha) * focused);
        event.canvas().drawRectangle(byteRect(content, m_caret, m_pane), ring,
            event.borderWidth());
    }
}
