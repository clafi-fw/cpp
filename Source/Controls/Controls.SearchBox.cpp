module ClaFi.Controls.SearchBox;

import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    SearchEvent::SearchEvent(SearchBox& box, const SearchRequest request)
        :
        EventOf<SearchBox>{ box },
        request{ request }
    {
    }

    void SearchField::charPress(CharPressEvent& event)
    {
        if (event.character() == Keys::Return)
            return;
        TextBox::charPress(event);
    }

    // The text goes in as an edit rather than as a host write, so it reaches the field's history
    // and raises the search through the one route typing takes.
    void SearchBox::startSearch(const std::wstring_view text)
    {
        m_field.selectAll();
        if (!text.empty() && text != searchText())
        {
            m_field.replaceSelectedText(text);
            m_field.selectAll();
        }
        m_field.setFocus();
    }

    void SearchBox::setResult(const std::optional<std::size_t> selected, const std::size_t count)
    {
        Text reading{};
        if (!searchText().empty())
        {
            reading << TextStyleId::SubBody;
            if (count == 0)
                reading << InkWell::redInk() << L"No results";
            else
            {
                reading << InkWell::textInk(InkGrade::Muted);
                if (selected.has_value())
                    reading << Fmt{ L"{} of {}", *selected + 1, count };
                else
                    reading << Fmt{ L"? of {}", count };
            }
        }
        if (reading == m_count.text())
            return;

        m_count.text() = reading;
        m_count.setVisible(!reading.empty());
        // A reading of another width moves the field's right edge.
        m_count.invalidateFormAlign();
    }

    void SearchBox::nestedKeyDown(KeyDownEvent& event)
    {
        switch (event.key)
        {
            case Keys::Return:
            {
                event.handled = true;
                request(event.modifiers.shift ? SearchRequest::Previous : SearchRequest::Next);
                return;
            }
            case Keys::Escape:
            {
                event.handled = true;
                // An edit like any other, so the search it raises marks nothing.
                if (!searchText().empty())
                {
                    m_field.selectAll();
                    m_field.deleteSelectedText();
                }
                request(SearchRequest::End);
                return;
            }
            default:
                break;
        }
        Panel::nestedKeyDown(event);
    }

    void SearchBox::request(const SearchRequest value)
    {
        SearchEvent event{ *this, value };
        emitEvent(event);
    }
}
