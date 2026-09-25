module ClaFi.Tools.WhatsClip.Page;

import ClaFi.Tools.WhatsClip.EncodingPick;
import ClaFi.Tools.WhatsClip.LanguagePick;

import ClaFi.Controls.Checkbox;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Foundation;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    namespace
    {
        constexpr wchar_t k_nulSymbol = L'\u2400';
        // A NUL ends the line after this much text - the least strings(1) calls a string.
        constexpr std::size_t k_minStringLength = 4;

        // A text format whose block is one NUL-terminated string, and the byte width of
        // that terminator. The content is cut to the terminator before the encoding is judged.
        // The encoding is judged from the bytes, and a run of zero padding reads as the high
        // halves of UTF-16 units - so left in, the slack carries an ASCII format off to UTF-16.
        struct TerminatedText
        {
            std::wstring_view name;
            std::size_t unit;
        };

        constexpr TerminatedText k_terminatedTextFormats[] = {
            { L"CF_TEXT", 1 },
            { L"CF_OEMTEXT", 1 },
            { L"CF_UNICODETEXT", 2 },
            { L"Rich Text Format", 1 }   // by every writer and reader, not by a definition
        };

        // The width of a terminated-string format's terminator, or zero for a format whose
        // structure this viewer does not know and reads whole.
        [[nodiscard]] std::size_t terminatorWidthOf(const std::wstring_view formatName)
        {
            for (const TerminatedText& format : k_terminatedTextFormats)
            {
                if (format.name == formatName)
                    return format.unit;
            }

            return 0;
        }

        // The offset of the first wide NUL - two zero bytes on an even boundary - or the size
        // where there is none.
        [[nodiscard]] std::size_t firstWideNul(const std::string_view bytes)
        {
            std::size_t index = 0;
            while (index + 1 < bytes.size())
            {
                if (bytes[index] == '\0' && bytes[index + 1] == '\0')
                    return index;

                index += 2;
            }

            return bytes.size();
        }

        // A format's content, without the slack past its terminator. A width of one ends at the
        // first NUL byte, two at the first wide NUL; a width of zero is a format this viewer
        // cannot place the end of, so its bytes are read whole.
        [[nodiscard]] std::string_view contentOf(const std::string_view bytes,
            const std::size_t terminatorWidth)
        {
            if (terminatorWidth == 1)
                return bytes.substr(0, bytes.find('\0'));
            if (terminatorWidth == 2)
                return bytes.substr(0, firstWideNul(bytes));

            return bytes;
        }

        // The text without the run of NULs a block pads its tail with - a NUL between strings is
        // a delimiter and stays. For a format read whole, whose slack the byte reading left on.
        [[nodiscard]] std::wstring_view withoutTrailingNuls(const std::wstring_view text)
        {
            const std::size_t end = text.find_last_not_of(L'\0');
            return end == std::wstring_view::npos ? std::wstring_view{} : text.substr(0, end + 1);
        }

        // A tab counts as text; a control and a replacement character - a failed reading - do not.
        [[nodiscard]] bool isStringCharacter(const wchar_t value)
        {
            if (value == L'\t')
                return true;
            if (value < L' ' || (value >= L'\x7F' && value <= L'\x9F'))
                return false;
            return value != L'\uFFFD';
        }

        [[nodiscard]] std::wstring withNulsMarked(const std::wstring_view text)
        {
            std::wstring result{};
            result.reserve(text.size());
            std::size_t stringLength = 0;
            for (const wchar_t value : text)
            {
                if (value == L'\0')
                {
                    result.push_back(k_nulSymbol);
                    if (stringLength >= k_minStringLength)
                        result.push_back(L'\n');
                    stringLength = 0;
                }
                else
                {
                    result.push_back(value);
                    stringLength = isStringCharacter(value) ? stringLength + 1 : 0;
                }
            }

            return result;
        }
    }

    // WHAT A PAGE ANSWERS A READ WITH, and where the view is sent home. What stands under the
    // bars afterwards is a different document, so a bar left where the last one was scrolled to
    // would open this one part-way down, at a place it may not even have. Every page is read
    // through here, so one written later is carried by this rather than having to remember it.
    // THE WAIT SHAPE IS SET HERE, not left to the align pass: the read is a transfer out of
    // another process, and the caret's reading shapes the whole document before this returns.
    void RepresentationPage::show(Transfer::Offer& offer, const Transfer::Format& format)
    {
        const ScopedWaitCursor waitCursor{};
        showContent(offer, format);
        scrollToBegin();
    }

    void RepresentationPage::writeStatus(Text&& value)
    {
        m_status.text() = std::move(value);
        // A read is where the status changes, so the pass it asks for is paid once a format is
        // opened rather than once a byte is.
        m_status.invalidateFormAlign();
    }

    void RepresentationPage::writeReadout(Text&& value)
    {
        // The width is stated, so a new reading moves nothing on the page: a repaint of the
        // corner is the whole of what a move costs outside the control it moved in.
        m_readout.text() = std::move(value);
        m_readout.invalidate();
    }

    void RepresentationPage::setReadoutWidth(const float value)
    {
        m_readout.setMinSize(MinSize{ value, 0.0f });
        m_readout.setMaxSize(MaxSize{ value, k_maxFloat });
    }

    StackPanel& RepresentationPage::cornerBars()
    {
        if (!m_cornerBars)
        {
            m_cornerBars = &m_corner.createRightBar<StackPanel>(
                Orientation::Horizontal,
                Padding{ 0.0f }
            );
        }

        return *m_cornerBars;
    }

    StackPanel& RepresentationPage::stripBars()
    {
        if (!m_stripBars)
        {
            m_stripBars = &m_strip.createRightBar<StackPanel>(
                Orientation::Horizontal,
                Padding{ k_stripBarPaddingX, 0.0f },
                Spacing{ k_stripBarSpacing },
                VerticalAlign::Center
            );
        }

        return *m_stripBars;
    }

    void TextPageBase::showText(Text&& value, const Transfer::Format& format)
    {
        // Kept for the box's question, which comes when its layout takes the text and not now.
        m_formatName = nameOf(format);
        placeText(std::move(value));
    }

    void TextPageBase::showText(Text&& value)
    {
        m_formatName.clear();
        placeText(std::move(value));
    }

    std::wstring TextPageBase::decode(const std::string_view bytes,
        const Transfer::Format& format)
    {
        m_encodingPick.setVisible(true);
        const std::wstring name = nameOf(format);
        // The block is bigger than the string it carries: an HGLOBAL is rounded up, and past a
        // terminator sits whatever the allocation held. A terminated-string format is cut to its
        // content before the pick, so the slack sways neither the encoding it is read in nor the
        // text; any other is read whole and drops only the run of NULs its tail is padded with,
        // keeping the ones it sets between strings.
        const std::size_t terminatorWidth = terminatorWidthOf(name);
        const std::string_view content = contentOf(bytes, terminatorWidth);
        const std::wstring decoded = m_encodingPick.decode(name, content);
        const std::wstring_view body = terminatorWidth != 0
            ? std::wstring_view{ decoded }
            : withoutTrailingNuls(decoded);
        return withNulsMarked(body);
    }

    void TextPageBase::prepareLocale(Transfer::Offer& offer)
    {
        if (m_readLocale)
            m_readLocale(offer);
    }

    void TextPageBase::placeText(Text&& value)
    {
        body().text() = std::move(value);
        body().invalidateFormAlign();
        // The caret stood in the text before this one. Sending it back to the start announces the
        // move, and the corner is written from that announcement - see the constructor.
        body().setCaretPos(0);
    }

    // BOTH NUMBERS COUNTED FROM ONE - see TextLineColumn. A caret that was never placed answers
    // the start of the text, so a page nobody has clicked in reads the first character rather
    // than nothing at all.
    void TextPageBase::writeCaretReadout()
    {
        const TextLineColumn caret = body().caretLineColumn();

        Text reading{};
        reading << PushFontSize{ k_readoutFontSize };
        reading << Fmt{ L"Ln {}, Col {}", caret.line, caret.column };

        writeReadout(std::move(reading));
    }

    void TextPageBase::languagePicked()
    {
        const std::optional<Syntax::Language> picked = m_languagePick.pickedLanguage();
        if (picked.has_value())
            body().setLanguage(picked.value());
        else
            body().setDetectLanguage(DetectLanguage::Yes);
    }

    void TextPageBase::answerLanguage(DetectLanguageEvent& event)
    {
        event.language = m_languagePick.detect(m_formatName, event.text);
    }

    void TextPageBase::wrapState(GetStateEvent& event) const
    {
        event.state.selected = body().wordWrap();
    }

    void TextPageBase::wrapClicked()
    {
        body().setWordWrap(body().wordWrap() ? WordWrap::No : WordWrap::Yes);
        m_wrapCheck.invalidateState();
        // The property is read on the next pass - see WithTextLayout::syncedLayout - and a text
        // shaped to another width is another height and another longest line, which is what the
        // bars range over.
        body().invalidateFormAlign();
    }

    Text TextPageBase::wrapLabel()
    {
        Text text{};
        text << PushFontSize{ k_readoutFontSize };
        text << L"Wrap";
        return text;
    }

    std::wstring TextPageBase::nameOf(const Transfer::Format& format)
    {
        return format.kind() == Transfer::Format::Kind::Custom
            ? format.name()
            : std::wstring{};
    }
}
