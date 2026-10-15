module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.SearchBox;

import ClaFi.Controls.Button;
import ClaFi.Controls.DropList;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Icons.OpenChevron;
import ClaFi.Icons.XMark;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // What the user asked of a search box.
    export enum class SearchRequest
    {
        Search,     // the text in the box changed, emptied included
        Next,       // Enter
        Previous,   // Shift+Enter
        Pick,       // a line of the box's list taken - Enter on the current one, or a click on any
        End         // Escape with the list down: the focus goes back to what was searched
    };

    export class SearchBox;

    // The user asked a search box for something.
    export struct SearchEvent : public EventOf<SearchBox>
    {
        SearchEvent(SearchBox&, SearchRequest);
        SearchEvent(SearchBox&, const DropListPick&);
        SearchRequest request;      // what was asked
        std::size_t line{ 0 };      // the line taken, for Pick
        KeyModifiers modifiers{};   // held with the key or the click that asked
        InputStamp stamp{};         // the input that took the line, for Pick
    };

    // The line a search is typed on, which Return does not break.
    class SearchField : public TextBox
    {
    public:
        template<typename... Args>
        explicit SearchField(const CreateParams&, SearchBox&, Args&&...);
    protected:
        void focusChanged() override;   // the list goes down with the focus
        void nestedKeyDown(KeyDownEvent&) override;   // the list's keys go on to the box
        void charPress(CharPressEvent&) override;
    private:
        SearchBox& m_box;
    };

    // A box a search is typed into, with how much it found. See Controls
    export class SearchBox : public Panel
    {
    public:
        template<typename... Args>
        explicit SearchBox(const CreateParams&, Args&&...);
    public:
        // Where the box's list drops: under it from its left edge, or from its right (BottomRight).
        DECLARE_PROPERTY(FormPlacement, listPlacement, FormPlacement::BottomLeft)
        // The modifiers that keep the list up across a pick - Alt, say.
        DECLARE_PROPERTY(KeyModifiers, listKeptBy, KeyModifiers{})
    public:
        DECLARE_EVENT(SearchEvent, OnSearch, onSearch) // the user asked the box for something
    public:
        std::wstring_view diagnosticText() const override { return L"SearchBox"; }
        [[nodiscard]] const std::wstring& searchText() const { return m_field.text().plainText(); }
        // Focuses the box with its text selected, a text given replacing it.
        void startSearch(std::wstring_view text = {});
        // Puts the text in the box as typing would, the focus staying where it is.
        void setSearchText(std::wstring_view);
        // Counts by place - "15 on page" - in a box searching wider than it counts.
        void setCountPlace(std::wstring_view value) { m_countPlace = value; }
        // Shows which found range is selected, and how many were found.
        void setResult(std::optional<std::size_t> selected, std::size_t count);
        // Drops the lines under the box, the one given current.
        void showList(DropListLines, std::optional<std::size_t> current);
        void hideList() { m_list.hide(); }
        [[nodiscard]] bool listShown() const { return m_list.shown(); }
        // Whether the box has lines to drop, up or down - the last ones its owner gave it.
        [[nodiscard]] bool hasList() const { return !m_list.lines().empty(); }
    protected:
        // Enter takes the list's line or steps; Escape closes the list, then ends.
        void nestedKeyDown(KeyDownEvent&) override;
    private:
        // Holds its own layout: a text per step would crowd the shared layout cache.
        using CountReadout = WithTextLayout<Label>;
    private:
        void connectParts();
        void textEdited();
        void step(SearchRequest, KeyModifiers);
        void clear();
        void request(SearchRequest, KeyModifiers = {});
        void pick(const DropListPick&);
    private:
        SearchField& m_field{ createBody<SearchField>(
            *this,
            PlaceHolderText{ L"Search..." },
            WordWrap::No,
            Padding{ 6.0f, 4.0f },
            VerticalTextAnchor::Center
        ) };
        // The count and the buttons, hidden while the box is empty so the field takes its width.
        StackPanel& m_tools{ createRightBar<StackPanel>(
            Orientation::Horizontal,
            VerticalAlign::Center,
            Spacing{ 0.0f },
            Padding{ 2.0f, 0.0f }
        ) };
        CountReadout& m_count{ m_tools.add<CountReadout>(
            WordWrap::No,
            Padding{ 6.0f, 0.0f },
            VerticalAlign::Center,
            VerticalTextAnchor::Center
        ) };
        ToolButton& m_previous{ m_tools.add<ToolButton>(
            FixedSize{ 20.0f },
            ButtonViewMode::IconOnly,
            IconSize{ 10.0f },
            Interactivity::MouseOnly,
            ButtonBase::OnPaintIcon{ Icons::OpenChevron::paintUp },
            L"Previous"
        ) };
        ToolButton& m_next{ m_tools.add<ToolButton>(
            FixedSize{ 20.0f },
            ButtonViewMode::IconOnly,
            IconSize{ 10.0f },
            Interactivity::MouseOnly,
            ButtonBase::OnPaintIcon{ Icons::OpenChevron::paintDown },
            L"Next"
        ) };
        ToolButton& m_clear{ m_tools.add<ToolButton>(
            FixedSize{ 20.0f },
            ButtonViewMode::IconOnly,
            IconSize{ 8.0f },
            Interactivity::MouseOnly,
            ButtonBase::OnPaintIcon{ Icons::XMark::paint },
            L"Clear"
        ) };
        // The field keeps the focus, and Up on the list's first line gives the keys back to it.
        DropList m_list{
            m_field,
            std::bind_front(&SearchBox::pick, this),
            DropListOptions{ .keptBy = m_listKeptBy, .upFromFirst = UpFromFirst::Leaves }
        };
        std::wstring m_countPlace{};   // what the count is taken over; empty for all of it
        std::size_t m_foundCount{ 0 };   // the count setResult was last given
    };


    //-------------------------------------------------------------------------


    // SearchField

    template<typename... Args>
    SearchField::SearchField(const CreateParams& params, SearchBox& box, Args&&... args)
        :
        TextBox{ params, std::forward<Args>(args)... },
        m_box{ box }
    {
    }

    // SearchBox

    template<typename... Args>
    SearchBox::SearchBox(const CreateParams& params, Args&&... args)
        :
        // Stated ahead of the caller's own properties: Props::get takes the last of the matching
        // arguments.
        Panel{
            params,
            params.themeMetrics().page,
            UiElement::Page,
            std::forward<Args>(args)...
        },
        INIT_PROPERTY(listPlacement),
        INIT_PROPERTY(listKeptBy)
    {
        connectParts();
    }
}
