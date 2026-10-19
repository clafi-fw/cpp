module ClaFi.Controls.SearchBox;

import ClaFi.Controls.Button;
import ClaFi.Controls.DropList;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;
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
    namespace
    {
        // A button's name and the key doing the same, as an action's tooltip reads.
        void writeTooltip(GetTooltipEvent& event, const wchar_t* name, const Shortcut& shortcut)
        {
            if (!event.text.empty())
                return;
            event.text << name;
            const std::wstring shortcutText = shortcut.text();
            if (!shortcutText.empty())
                event.text << L" (" << shortcutText << L")";
        }
    }

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

    // An edit like typing, so it reaches the field's history and raises the search; a text the box
    // holds already raises nothing.
    void SearchBox::setSearchText(const std::wstring_view text)
    {
        if (text == searchText())
            return;
        m_field.selectAll();
        if (text.empty())
            m_field.deleteSelectedText();
        else
            m_field.replaceSelectedText(text);
    }

    // **A count taken over a place says so**: none found there is a count like any other, since
    // the search may find more elsewhere.
    void SearchBox::setResult(const std::optional<std::size_t> selected, const std::size_t count)
    {
        const bool foundBefore = m_foundCount > 0;
        m_foundCount = count;
        if (foundBefore != (count > 0))
        {
            m_previous.invalidateState();
            m_next.invalidateState();
        }

        Text reading{};
        if (!searchText().empty())
        {
            reading << TextStyle::SubBody;
            if (count == 0 && m_countPlace.empty())
                reading << InkWell::redInk() << L"No results";
            else
            {
                reading << InkWell::textInk(InkGrade::Muted);
                if (selected.has_value())
                    reading << Fmt{ L"{} of {}", *selected + 1, count };
                else if (!m_countPlace.empty())
                    reading << Fmt{ L"{} {}", count, m_countPlace };
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
        m_list.show(std::move(lines), current, m_listPlacement, *this);
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
                step(event.modifiers.shift ? SearchRequest::Previous : SearchRequest::Next,
                    event.modifiers);
                return;
            }
            case Keys::Escape:
            {
                event.handled = true;
                // **Escape steps back once a press**: the list goes down, then the focus goes back,
                // and the text stays - the search stands while what was searched is read.
                if (m_list.shown())
                {
                    m_list.hide();
                    return;
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
            // **Down before the focus moves on**: a list going down hands the focus back to the
            // field, which would take back the step Tab is about to make.
            case Keys::Tab:
                m_list.hide();
                break;
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

    void SearchBox::connectParts()
    {
        m_tools.hide();
        m_count.hide();
        m_field.onTextEdit([this](TextEditEvent&){
            textEdited();
        });
        m_previous.onClick([this](ClickEvent& event){
            step(SearchRequest::Previous, event.modifiers);
        });
        m_next.onClick([this](ClickEvent& event){
            step(SearchRequest::Next, event.modifiers);
        });
        m_clear.onClick([this](ClickEvent&){
            clear();
        });
        const auto foundAny = [this](GetStateEvent& event){
            event.state.enabled = m_foundCount > 0;
        };
        m_previous.onGetState(foundAny);
        m_next.onGetState(foundAny);
        m_previous.onGetTooltip([](GetTooltipEvent& event){
            writeTooltip(event, L"Previous", { Keys::Return, { .shift = true } });
        });
        m_next.onGetTooltip([](GetTooltipEvent& event){
            writeTooltip(event, L"Next", { Keys::Return });
        });
    }

    void SearchBox::textEdited()
    {
        m_tools.setVisible(!searchText().empty());
        request(SearchRequest::Search);
    }

    void SearchBox::step(const SearchRequest value, const KeyModifiers modifiers)
    {
        m_list.hide();
        request(value, modifiers);
    }

    // The emptied box takes the focus, as what follows a clear is typing.
    void SearchBox::clear()
    {
        m_list.hide();
        setSearchText({});
        m_field.setFocus();
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
