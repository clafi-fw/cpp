module ClaFi.Core.System.Url;

import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        constexpr wchar_t k_anchorMark = L'#';
        constexpr std::wstring_view k_escapedAnchorMark{ L"%23" };

        void appendEscaped(std::wstring& spelling, const std::wstring_view part)
        {
            for (const wchar_t character : part)
            {
                if (character == k_anchorMark)
                    spelling.append(k_escapedAnchorMark);
                else
                    spelling.push_back(character);
            }
        }

        // Only the escape appendEscaped writes is read back. Any other - a web address's %20 - is
        // left as it stands, so such an address goes through str() unchanged.
        [[nodiscard]] std::wstring unescaped(const std::wstring_view part)
        {
            std::wstring result{};
            result.reserve(part.size());
            std::size_t i = 0;
            while (i != part.size())
            {
                if (part.substr(i).starts_with(k_escapedAnchorMark))
                {
                    result.push_back(k_anchorMark);
                    i += k_escapedAnchorMark.size();
                }
                else
                {
                    result.push_back(part[i]);
                    ++i;
                }
            }
            return result;
        }
    }

    Url::Url(const std::wstring_view path, const std::wstring_view anchor)
        :
        m_path{ path },
        m_anchor{ anchor }
    {
    }

    Url Url::parse(const std::wstring_view spelling)
    {
        const std::size_t mark = spelling.find(k_anchorMark);
        if (mark == spelling.npos)
            return Url{ unescaped(spelling) };

        return Url{ unescaped(spelling.substr(0, mark)), unescaped(spelling.substr(mark + 1)) };
    }

    Url Url::withAnchor(const std::wstring_view anchor) const
    {
        return Url{ m_path, anchor };
    }

    std::wstring Url::str() const
    {
        std::wstring result{};
        appendEscaped(result, m_path);
        if (!m_anchor.empty())
        {
            result.push_back(k_anchorMark);
            appendEscaped(result, m_anchor);
        }
        return result;
    }
}
