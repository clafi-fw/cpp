export module ClaFi.Core.TextEngine.Searcher;

import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    // How the text of a search is taken.
    export enum class SearchSplit
    {
        Words,    // split at white space, each word found on its own - a search over a page
        Phrase    // whole, as typed - a find in an editor
    };

    // The words of a search and where a text holds them, case aside. See TextEngine-Types#searcher
    export class Searcher
    {
    public:
        // Searches for the text, taken as the split says; answers whether the words moved.
        bool setText(std::wstring_view text, SearchSplit);
        [[nodiscard]] const std::wstring& text() const { return m_text; }
        [[nodiscard]] bool empty() const { return m_words.empty(); }
        // Moves whenever the words do, so what was found with the old ones can tell.
        [[nodiscard]] std::size_t revision() const { return m_revision; }
        // Fills the hits with the places the text holds a word, in text order and apart.
        void findIn(std::wstring_view text, std::vector<TextRange>& hits) const;
    private:
        std::wstring m_text{};                 // as it was given
        std::vector<std::wstring> m_words{};   // folded, none empty and none twice
        std::size_t m_revision{ 0 };
        // The text being searched, folded - kept, so it grows only for a text longer than any yet.
        mutable std::wstring m_folded{};
    };
}
