module ClaFi.Core.TextEngine.Searcher;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    // Searcher

    bool Searcher::setText(const std::wstring_view text, const SearchSplit split)
    {
        m_text = text;
        std::vector<std::wstring> words;
        if (split == SearchSplit::Phrase && !text.empty())
            words.emplace_back(text);
        std::size_t at = 0;
        while (split == SearchSplit::Words && at < text.size())
        {
            while (at < text.size() && std::iswspace(text[at]))
                ++at;
            std::size_t end = at;
            while (end < text.size() && !std::iswspace(text[end]))
                ++end;
            if (end > at)
                words.emplace_back(text.substr(at, end - at));
            at = end;
        }
        for (std::wstring& word : words)
            foldCase(word);
        std::ranges::sort(words);
        words.erase(std::ranges::unique(words).begin(), words.end());
        if (words == m_words)
            return false;
        m_words = std::move(words);
        return true;
    }

    // **A place is a word wherever it stands**, inside a longer word too, as a find reads it. Two
    // places that overlap are one, so the hits stand apart in the order a draw takes them and a
    // find steps through them.
    void Searcher::findIn(const std::wstring_view text, std::vector<TextRange>& hits) const
    {
        hits.clear();
        if (m_words.empty())
            return;
        // m_folded is a member only to reuse its internal buffer, not to save the work
        m_folded.assign(text);
        foldCase(m_folded);
        for (const std::wstring& word : m_words)
        {
            for (std::size_t at = m_folded.find(word); at != std::wstring::npos;
                at = m_folded.find(word, at + word.size()))
            {
                hits.push_back({ at, word.size() });
            }
        }
        std::ranges::sort(hits, {}, &TextRange::start);
        std::size_t kept = 0;
        for (std::size_t i = 0; i != hits.size(); ++i)
        {
            if (kept != 0 && hits[kept - 1].end() > hits[i].start)
            {
                TextRange& last = hits[kept - 1];
                last.length = std::max(last.end(), hits[i].end()) - last.start;
                continue;
            }
            hits[kept] = hits[i];
            ++kept;
        }
        hits.resize(kept);
    }
}
