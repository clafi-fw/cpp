module ClaFi.Tools.WhatsClip.TextPage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.CheckBox;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.MessageBar;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace Controls;

    TextPage::TextPage(const CreateParams& params, const PickLists& picks)
        :
        TextPageBase{
            params,
            picks,
            HostProps{ ScrollBars::Both },
            BodyProps{ ReadOnly::Yes, Padding{ 12.0f } }
        },
        m_question{ createMessageBar() }
    {
        // The connection needs no scope: the band is this page's own and goes when it does.
        m_question.addAnswer(L"Read as text", [this](ClickEvent&) {
            readHeldBack();
        });
        m_question.setVisible(false);
    }

    // WHAT IS ON THE CLIPBOARD, AS TEXT: the format's own bytes, read in the encoding picked in
    // the corner, controls marked - see decode. Every format is a platform format here, its
    // bytes as they arrived - CF_UNICODETEXT's UTF-16 beside CF_TEXT's ANSI - so every one
    // goes through the pick. The framework's rich text comes out as the ClaFi it travels as,
    // and a format for which no encoding is right comes out as replacement characters -
    // itself an answer, saying this is not text, go and look at the bytes.
    void TextPage::showContent(Transfer::Offer& offer, const Transfer::Format& format)
    {
        m_format = format;
        // The code page a code-page reading uses is the clipboard's, not this machine's - resolve
        // it from CF_LOCALE before decoding.
        prepareLocale(offer);
        // Kept up to the limit only, so a screenshot's megabytes go with this scope.
        const std::string bytes = offer.readBytes(format);
        m_byteCount = bytes.size();
        m_bytes.assign(bytes, 0, k_textLimit);
        m_binary = m_byteCount > k_askAbove && isBinary(m_bytes, format);
        m_heldBack = m_binary;
        showBytes();
    }

    // A different text of the same bytes is a different document, so the view goes home as it
    // does for a read, and waits as it does for one.
    void TextPage::encodingPicked()
    {
        // Nothing to read again before the first read; the pick is not even in view then.
        if (!m_format)
            return;

        const ScopedWaitCursor waitCursor{};
        showBytes();
        scrollToBegin();
    }

    void TextPage::showBytes()
    {
        m_question.setVisible(m_heldBack);
        if (m_heldBack)
        {
            showHeldBack();
            return;
        }

        // A character the font lacks sends its line to DirectWrite, and binary bytes put one on
        // nearly every line - so those stay on cells whatever they read as.
        body().setCellFallback(m_binary ? CellFallback::MissingGlyph : CellFallback::Native);
        showReading(decode(m_bytes, *m_format), m_byteCount, L"bytes");
    }

    // THE BOX IS EMPTIED rather than left showing the last format's text under a question about
    // this one's, and it names no format, so the language is judged from nothing.
    void TextPage::showHeldBack()
    {
        Text status{};
        status << Fmt{ L"{} bytes", m_byteCount };
        writeStatus(std::move(status));

        Text question{};
        question << Fmt{ L"This is not a text format: it holds {} bytes of binary data. "
            L"Reading them as text can take minutes and slow the application down. "
            L"Read them anyway?", m_byteCount };
        m_question.setMessage(MessageIcon::Warning, question);
        showText(Text{});
    }

    void TextPage::readHeldBack()
    {
        m_heldBack = false;
        const ScopedWaitCursor waitCursor{};
        showBytes();
        scrollToBegin();
    }

    void TextPage::showReading(std::optional<std::wstring>&& text, const std::size_t wholeSize,
        const std::wstring_view unit)
    {
        Text status{};
        if (!text)
            status << k_noAnswer;
        else if (wholeSize > k_textLimit)
            status << Fmt{ L"{} characters, the first {} of {} {}",
                text->size(), k_textLimit, wholeSize, unit };
        else
            status << Fmt{ L"{} characters", text->size() };

        writeStatus(std::move(status));

        Text report{};
        if (text)
            report << *text;

        showText(std::move(report), *m_format);
    }
}
