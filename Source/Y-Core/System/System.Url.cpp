module ClaFi.Core.System.Url;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        [[nodiscard]] constexpr bool asciiLetter(const wchar_t character)
        {
            return (character >= L'a' && character <= L'z')
                || (character >= L'A' && character <= L'Z');
        }

        [[nodiscard]] constexpr bool asciiDigit(const wchar_t character)
        {
            return character >= L'0' && character <= L'9';
        }

        [[nodiscard]] constexpr wchar_t lowerAscii(const wchar_t character)
        {
            return character >= L'A' && character <= L'Z' ? character + (L'a' - L'A') : character;
        }

        [[nodiscard]] bool validScheme(const std::wstring_view scheme)
        {
            if (scheme.empty() || !asciiLetter(scheme.front()))
                return false;
            return std::ranges::all_of(scheme.substr(1), [](const wchar_t character) {
                return asciiLetter(character) || asciiDigit(character)
                    || character == L'+' || character == L'-' || character == L'.';
            });
        }

        [[nodiscard]] constexpr int hexDigit(const wchar_t character)
        {
            if (asciiDigit(character))
                return character - L'0';
            const wchar_t lower = lowerAscii(character);
            return lower >= L'a' && lower <= L'f' ? lower - L'a' + 10 : -1;
        }

        [[nodiscard]] bool validSpelling(const std::wstring_view spelling)
        {
            for (std::size_t index = 0; index < spelling.size(); ++index)
            {
                const wchar_t character = spelling[index];
                if (character <= L' ' || character == L'\x7f')
                    return false;
                if (character != L'%')
                    continue;
                if (spelling.size() - index < 3
                    || hexDigit(spelling[index + 1]) < 0 || hexDigit(spelling[index + 2]) < 0)
                {
                    return false;
                }
                index += 2;
            }
            return true;
        }

        [[nodiscard]] std::wstring decoded(const std::wstring_view spelling)
        {
            if (spelling.find(L'%') == spelling.npos)
                return std::wstring{ spelling };

            std::wstring result;
            result.reserve(spelling.size());
            std::string bytes;
            for (std::size_t index = 0; index < spelling.size();)
            {
                if (spelling[index] != L'%')
                {
                    result.push_back(spelling[index]);
                    ++index;
                    continue;
                }
                bytes.clear();
                do
                {
                    const int high = hexDigit(spelling[index + 1]);
                    const int low = hexDigit(spelling[index + 2]);
                    bytes.push_back(static_cast<char>((high << 4) | low));
                    index += 3;
                } while (index < spelling.size() && spelling[index] == L'%');
                result += fromUtf8(bytes);
            }
            return result;
        }

        void appendEncoded(std::wstring& spelling, const std::wstring_view text,
            const bool path = false)
        {
            constexpr std::wstring_view k_hex = L"0123456789ABCDEF";
            constexpr std::string_view k_pathPunctuation = "/!$&'()*+,;=@";
            for (const unsigned char byte : toUtf8(text))
            {
                const bool unreserved = asciiLetter(byte) || asciiDigit(byte)
                    || byte == '-' || byte == '.' || byte == '_' || byte == '~';
                if (unreserved || (path && k_pathPunctuation.find(byte) != k_pathPunctuation.npos))
                {
                    spelling.push_back(byte);
                }
                else
                {
                    spelling.push_back(L'%');
                    spelling.push_back(k_hex[byte >> 4]);
                    spelling.push_back(k_hex[byte & 0x0F]);
                }
            }
        }
    }

    Url::Url(const std::wstring_view path, const std::wstring_view fragment)
        :
        Url{ Build({}, path, fragment) }
    {
    }

    std::optional<Url> Url::Parse(const std::wstring_view spelling)
    {
        if (!validSpelling(spelling))
            return std::nullopt;

        Url url;
        url.m_spelling = spelling;
        std::wstring_view rest = spelling;
        const std::wstring_view scheme = schemeOf(rest);
        if (!scheme.empty())
        {
            std::ranges::transform(scheme, std::back_inserter(url.m_scheme), lowerAscii);
            rest.remove_prefix(scheme.size() + 1);
        }
        else if (rest.substr(0, rest.find_first_of(L"/?#")).find(L':') != rest.npos)
        {
            return std::nullopt;
        }

        const std::size_t fragmentMark = rest.find(L'#');
        if (fragmentMark != rest.npos)
        {
            url.m_fragment = decoded(rest.substr(fragmentMark + 1));
            rest = rest.substr(0, fragmentMark);
        }

        const std::size_t queryMark = rest.find(L'?');
        if (queryMark != rest.npos)
        {
            std::wstring_view query = rest.substr(queryMark + 1);
            for (;;)
            {
                const std::size_t separator = query.find(L'&');
                const std::wstring_view parameter = query.substr(0, separator);
                const std::size_t equals = parameter.find(L'=');
                url.m_query.push_back({ .name = decoded(parameter.substr(0, equals)) });
                if (equals != parameter.npos)
                    url.m_query.back().value = decoded(parameter.substr(equals + 1));
                if (separator == query.npos)
                    break;
                query.remove_prefix(separator + 1);
            }
            rest = rest.substr(0, queryMark);
        }

        if (rest.starts_with(L"//"))
        {
            rest.remove_prefix(2);
            const std::size_t authorityEnd = rest.find(L'/');
            url.m_authority.emplace(rest.substr(0, authorityEnd));
            rest.remove_prefix(authorityEnd == rest.npos ? rest.size() : authorityEnd);
        }
        url.m_path = decoded(rest);
        return url;
    }

    Url Url::Build(const std::wstring_view scheme, const std::wstring_view path,
        const std::wstring_view fragment, const QueryParameters& query,
        const std::optional<std::wstring_view> authority)
    {
        if (!scheme.empty() && !validScheme(scheme))
            noReach("Invalid URL scheme");
        if (authority && (!validSpelling(*authority)
            || authority->find_first_of(L"/?#") != authority->npos
            || (!path.empty() && !path.starts_with(L'/'))))
        {
            noReach("Invalid URL authority or path");
        }

        Url url;
        std::ranges::transform(scheme, std::back_inserter(url.m_scheme), lowerAscii);
        std::wstring& spelling = url.m_spelling;
        if (!url.m_scheme.empty())
            spelling.append(url.m_scheme).push_back(L':');
        if (authority)
        {
            url.m_authority.emplace(*authority);
            spelling.append(L"//").append(*authority);
        }

        url.m_path = path;
        if (!authority && path.starts_with(L"//"))
        {
            spelling += L"/%2F";
            appendEncoded(spelling, path.substr(2), true);
        }
        else
        {
            appendEncoded(spelling, path, true);
        }

        url.m_query = query;
        wchar_t separator = L'?';
        for (const UrlParameter& parameter : query)
        {
            spelling.push_back(separator);
            separator = L'&';
            appendEncoded(spelling, parameter.name);
            if (parameter.value)
            {
                spelling.push_back(L'=');
                appendEncoded(spelling, *parameter.value);
            }
        }
        url.m_fragment = fragment;
        if (!fragment.empty())
        {
            spelling.push_back(L'#');
            appendEncoded(spelling, fragment);
        }
        return url;
    }

    std::wstring_view Url::schemeOf(const std::wstring_view spelling)
    {
        const std::size_t colon = spelling.find(L':');
        if (colon == spelling.npos)
            return {};
        const std::wstring_view scheme = spelling.substr(0, colon);
        return validScheme(scheme) ? scheme : std::wstring_view{};
    }
}
