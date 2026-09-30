module ThisApp.ScriptPage;

import ClaFi.Documents.Page;

import ClaFi.Controls.CodeBox;

import ClaFi.Core.DomEngine;
// The std::wstring serializer: without it the node's set and get read a string as a sequence.
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    void ScriptPage::restoreViewState()
    {
        m_box.text().clear();
        // Source is columns, and only a monospaced style keeps them.
        m_box.text() << TextStyleId::Code << documentNode().get<std::wstring>();
        // The document is what the box measures, so a new one is a new size for the scroll box
        // around it.
        m_box.invalidateFormAlign();
    }

    void ScriptPage::storeViewState() const
    {
        documentNode().set(m_box.text().plainText());
    }

    void ScriptPage::writeCaretReadout()
    {
        m_caretReadout.text() = caretReading(m_box.caretLineColumn());
        // The width is stated, so a repaint is the whole of what a new reading costs.
        m_caretReadout.invalidate();
    }

    Text ScriptPage::caretReading(const TextLineColumn caret)
    {
        Text reading{};
        reading << PushFontSize{ k_caretReadoutFontSize };
        reading << Fmt{ L"Ln {}, Col {}", caret.line, caret.column };
        return reading;
    }
}
