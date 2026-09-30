module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.SearchBox;

import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.TextBox;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // What the user asked of a search box. See Controls#search
    export enum class SearchRequest
    {
        Search,     // the text in the box changed, emptied included
        Next,       // Enter
        Previous,   // Shift+Enter
        End         // Escape, once the box is empty: the focus goes back to what was searched
    };

    export class SearchBox;

    // The user asked a search box for something. See Controls#search
    export struct SearchEvent : public EventOf<SearchBox>
    {
        SearchEvent(SearchBox&, SearchRequest);
        SearchRequest request; // what was asked
    };

    // The line a search is typed on, which Return does not break.
    class SearchField : public TextBox
    {
    public:
        using TextBox::TextBox;
    protected:
        void charPress(CharPressEvent&) override;
    };

    // A box a search is typed into, with how much it found. See Controls#search
    export class SearchBox : public Panel
    {
    public:
        template<typename... Args>
        explicit SearchBox(const CreateParams&, Args&&...);
    public:
        // The user asked the box for something. See Controls#search
        DECLARE_EVENT(SearchEvent, OnSearch, onSearch)
    public:
        std::wstring_view diagnosticText() const override { return L"SearchBox"; }
        [[nodiscard]] const std::wstring& searchText() const { return m_field.text().plainText(); }
        // Focuses the box with its text selected, a text given replacing it. See Controls#search
        void startSearch(std::wstring_view text = {});
        // Shows which found range is selected, and how many were found. See Controls#search
        void setResult(std::optional<std::size_t> selected, std::size_t count);
    protected:
        // Enter and Shift+Enter step; Escape empties the box and hands the focus back.
        void nestedKeyDown(KeyDownEvent&) override;
    private:
        // Holds its own layout: a text per step would crowd the shared layout cache.
        using CountReadout = WithTextLayout<Label>;
    private:
        void request(SearchRequest);
    private:
        SearchField& m_field{ createBody<SearchField>(
            PlaceHolderText{ L"Search..." },
            WordWrap::No,
            Padding{ 6.0f, 4.0f },
            VerticalTextAnchor::Center
        ) };
        // Hidden while the box is empty, so the field takes the whole width.
        CountReadout& m_count{ createRightBar<CountReadout>(
            WordWrap::No,
            Padding{ 6.0f, 0.0f },
            VerticalTextAnchor::Center
        ) };
    };


    //-------------------------------------------------------------------------


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
        }
    {
        m_count.hide();
        m_field.onTextEdit([this](TextEditEvent&){
            request(SearchRequest::Search);
        });
    }
}
