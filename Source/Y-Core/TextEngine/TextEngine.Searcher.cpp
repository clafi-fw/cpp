module ClaFi.Core.TextEngine.Searcher;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    // Searcher

    bool Searcher::setText(const std::wstring_view text)
    {
        m_text = text;
        std::vector<std::wstring> words;
        std::size_t at = 0;
        while (at < text.size())
        {
            while (at < text.size() && std::iswspace(text[at]))
                ++at;
            std::size_t end = at;
            while (end < text.size() && !std::iswspace(text[end]))
                ++end;
            if (end > at)
            {
                std::wstring word{ text.substr(at, end - at) };
                foldCase(word);
                if (std::ranges::find(words, word) == words.end())
                    words.push_back(std::move(word));
            }
            at = end;
        }
        if (words == m_words)
            return false;
        m_words = std::move(words);
        return true;
    }

    // A place is a word as it stands, wherever it stands - inside a longer word too, as a find
    // over a page reads it.
    void Searcher::addHits(const std::wstring_view text, std::vector<TextRange>& hits) const
    {
        if (m_words.empty())
            return;
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
    }
}
