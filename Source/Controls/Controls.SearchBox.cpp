module ClaFi.Controls.SearchBox;

import ClaFi.Controls.DropList;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.FormContext;
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

    SearchEvent::SearchEvent(SearchBox& box, const DropListPick& picked)
        :
        EventOf<SearchBox>{ box },
        request{ SearchRequest::Pick },
        line{ picked.line },
        modifiers{ picked.modifiers },
        stamp{ picked.stamp }
    {
    }

    // SearchField

    void SearchField::focusChanged()
    {
        TextBox::focusChanged();
        if (!isFocused())
            m_box.hideList();
    }

    // **The walk reaches the field before the box**, and the field would move the caret on these
    // keys; left alone here, they reach the box next. Down is the box's while it has lines to drop
    // again, and the page keys and the arrows while its list is up.
    void SearchField::nestedKeyDown(KeyDownEvent& event)
    {
        const bool arrow = event.key == Keys::Up || event.key == Keys::Down;
        const bool page = event.key == Keys::Prior || event.key == Keys::Next;
        const bool dropsAgain = event.key == Keys::Down && m_box.hasList();
        if (dropsAgain || ((arrow || page) && m_box.listShown()))
            return;
        TextBox::nestedKeyDown(event);
    }

    void SearchField::charPress(CharPressEvent& event)
    {
        if (event.character() == Keys::Return)
            return;
        TextBox::charPress(event);
    }

    // SearchBox

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

    void SearchBox::showList(DropListLines lines, const std::optional<std::size_t> current)
    {
        m_list.show(std::move(lines), current, m_listPlacement);
    }

    void SearchBox::nestedKeyDown(KeyDownEvent& event)
    {
        switch (event.key)
        {
            case Keys::Return:
            {
                event.handled = true;
                // The current line is what Enter means while the list is up.
                if (m_list.pickCurrent(event.modifiers, event.stamp))
                    return;
                m_list.hide();
                request(event.modifiers.shift ? SearchRequest::Previous : SearchRequest::Next,
                    event.modifiers);
                return;
            }
            case Keys::Escape:
            {
                event.handled = true;
                // **With the list up, Escape takes the list down** and leaves the text standing;
                // the next one empties the box.
                if (m_list.shown())
                {
                    m_list.hide();
                    return;
                }
                // An edit like any other, so the search it raises marks nothing.
                if (!searchText().empty())
                {
                    m_field.selectAll();
                    m_field.deleteSelectedText();
                }
                request(SearchRequest::End);
                return;
            }
            case Keys::Up:
            case Keys::Down:
            {
                const ScrollDirection direction = event.key == Keys::Up
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd;
                if (m_list.moveCurrent(direction))
                {
                    event.handled = true;
                    return;
                }
                // A list taken down drops again, over the text as it stands now.
                if (event.key == Keys::Down && hasList())
                {
                    event.handled = true;
                    request(SearchRequest::Search);
                    return;
                }
                break;
            }
            case Keys::Prior:
            case Keys::Next:
            {
                const ScrollDirection direction = event.key == Keys::Prior
                    ? ScrollDirection::ToBegin
                    : ScrollDirection::ToEnd;
                if (m_list.moveCurrentByPage(direction))
                {
                    event.handled = true;
                    return;
                }
                break;
            }
            default:
                break;
        }
        Panel::nestedKeyDown(event);
    }

    void SearchBox::request(const SearchRequest value, const KeyModifiers modifiers)
    {
        SearchEvent event{ *this, value };
        event.modifiers = modifiers;
        emitEvent(event);
    }

    void SearchBox::pick(const DropListPick& picked)
    {
        SearchEvent event{ *this, picked };
        emitEvent(event);
    }
}
