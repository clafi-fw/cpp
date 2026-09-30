module ThisApp.ScriptPage;

import ClaFi.Documents.Page;

import ClaFi.Controls.CodeBox;
import ClaFi.Controls.SearchBox;
import ClaFi.Controls.TextBox;

import ClaFi.StdActions;

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

    void ScriptPage::connectSearchActions()
    {
        onGetActionState([this](GetActionStateEvent& event) {
            if (&event.action == &StdActions::find)
                event.claim({});
            else if (&event.action == &StdActions::findNext
                || &event.action == &StdActions::findPrevious)
            {
                event.claim({ .enabled = m_box.foundCount() != 0 });
            }
        });
        onActionClick([this](ActionClickEvent& event) {
            if (&event.action == &StdActions::find)
                startSearch();
            else if (&event.action == &StdActions::findNext)
                m_box.find(FindTarget::Next);
            else if (&event.action == &StdActions::findPrevious)
                m_box.find(FindTarget::Previous);
        });
    }

    void ScriptPage::startSearch()
    {
        // One line at most: the box keeps to one line, and a longer selection is not a search.
        std::wstring_view selected{};
        if (m_box.isFocused())
        {
            const TextRange range = m_box.selection();
            const std::wstring_view script = m_box.text().plainText();
            selected = script.substr(range.start, range.length);
            if (selected.find(L'\n') != std::wstring_view::npos)
                selected = {};
        }
        m_search.startSearch(selected);
    }

    void ScriptPage::searchRequested(const SearchRequest request)
    {
        switch (request)
        {
            case SearchRequest::Search:
                m_box.setSearchText(m_search.searchText());
                m_box.find(FindTarget::AtSelection);
                break;
            case SearchRequest::Next:
                m_box.find(FindTarget::Next);
                break;
            case SearchRequest::Previous:
                m_box.find(FindTarget::Previous);
                break;
            case SearchRequest::End:
                m_box.setFocus();
                break;
        }
        writeSearchResult();
    }

    void ScriptPage::writeSearchResult()
    {
        m_search.setResult(m_box.selectedFound(), m_box.foundCount());
        // A presenter keeps the answer it was last given until it is asked again.
        StdActions::findNext.invalidateState();
        StdActions::findPrevious.invalidateState();
    }
}
