export module ClaFi.Core.TextEngine.Searcher;

import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    // The words of a search and where a text holds them, case aside. See TextEngine-Types#searcher
    export class Searcher
    {
    public:
        // Searches for the words of the text, split at white space; answers whether they moved.
        bool setText(std::wstring_view text);
        [[nodiscard]] const std::wstring& text() const { return m_text; }
        [[nodiscard]] bool empty() const { return m_words.empty(); }
        // Adds each place the text holds one of the words to the hits, in no order.
        void addHits(std::wstring_view text, std::vector<TextRange>& hits) const;
    private:
        std::wstring m_text{};                 // as it was given
        std::vector<std::wstring> m_words{};   // folded, none empty and none twice
        // The text being searched, folded: kept, so a paragraph painted costs no allocation.
        mutable std::wstring m_folded{};
    };
}
