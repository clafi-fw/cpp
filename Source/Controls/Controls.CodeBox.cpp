module ClaFi.Controls.CodeBox;

import ClaFi.Controls.TextBox;

import ClaFi.Core.Syntax.Lines;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    DetectLanguageEvent::DetectLanguageEvent(const CodeBox& box, const std::wstring_view wholeText)
        :
        EventOf<const CodeBox>{ box },
        text{ wholeText }
    {
    }

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

    void CodeBox::textTaken(const Text& text, const TextEdit* edit) const
    {
        if (edit)
            m_lines.applyEdit(text.plainText(), edit->replaced, edit->insertedLength);
        else
            readWhole(text.plainText());
    }

    void CodeBox::readWhole(const std::wstring_view text) const
    {
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
}
