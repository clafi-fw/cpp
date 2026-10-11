module;
#include <immintrin.h>
module ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    void writeZerosIntoBinaryStream(std::ofstream& outFile, std::size_t count)
    {
        if (!(count && outFile.is_open()))
            return;
        static std::vector<char> buffer;
        if (count > buffer.size())
            buffer.resize(count, 0x00);
        outFile.write(buffer.data(), count);
    }
    void writeString(std::wostream& stream, std::wstring_view value)
    {
        stream.write(value.data(), value.size());
    }
    void writeString(std::wostream& stream, wchar_t value)
    {
        stream.write(&value, 1);
    }
    void writeIndent(std::wostream& stream, wchar_t indentChar, std::size_t level)
    {
        std::size_t i = level;
        while (i)
        {
            stream.write(&indentChar, 1);
            --i;
        }
    }
    void setFlag(FlagByte& flags, const FlagByte flag, const bool value)
    {
        if (value)
            flags |= flag;
        else
            flags &= ~flag;
    }
    bool tryStrToFloat(std::wstring_view str, float& result)
    {
        std::string narrow(str.begin(), str.end());
        float val;
        auto [ptr, ec] = std::from_chars(narrow.data(), narrow.data() + narrow.size(), val);
        if (ec == std::errc())
        {
            result = val;
            return true; // Partial parsing is accepted (res.ptr may not be at end)
        }
        return false;
    }
    std::wstring floatToStr(float val)
    {
        std::array<char, 64> buffer;
        auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), val);
        if (ec == std::errc())
            return std::wstring(buffer.data(), ptr);
        return L"";
    }
    void trimLeft(std::wstring& s)
    {
        std::size_t l = s.size();
        std::size_t i = 0;
        while (i < l && static_cast<std::wint_t>(std::iswblank(s[i])))
            ++i;
        if (i)
            s.erase(0, i);
    }
    void trimLeft(std::wstring_view& s)
    {
        std::size_t l = s.size();
        std::size_t i = 0;
        while (i < l && static_cast<std::wint_t>(std::iswblank(s[i])))
            ++i;
        s = s.substr(i, l - i);
    }
    void trimRight(std::wstring& s)
    {
        if (s.empty())
            return;
        std::size_t l = s.size();
        std::size_t i = l;
        while (i && static_cast<std::wint_t>(std::iswblank(s[i - 1])))
            --i;
        if (i < l)
            s.erase(i, l - i);
    }
    void trimRight(std::wstring_view& s)
    {
        if (s.empty())
            return;
        std::size_t l = s.size();
        std::size_t i = l;
        while (i && static_cast<std::wint_t>(std::iswblank(s[i - 1])))
            --i;
        if (i < l)
            s = s.substr(0, i);
    }
    void trim(std::wstring& s)
    {
        trimLeft(s);
        trimRight(s);
    }
    void trim(std::wstring_view& s)
    {
        trimLeft(s);
        trimRight(s);
    }

    namespace
    {
        // The locale the environment names, or the classic one where this machine has no such
        // locale installed.
        [[nodiscard]] std::locale userLocale()
        {
            try
            {
                return std::locale{ "" };
            }
            catch (const std::runtime_error&)
            {
                return std::locale::classic();
            }
        }
    }

    // The user's locale rather than std::towlower, which reads the C locale and folds A to Z alone.
    // Looked up once: a search in force folds every paragraph it draws.
    void foldCase(std::wstring& text)
    {
        static const std::locale locale = userLocale();
        static const std::ctype<wchar_t>& folding = std::use_facet<std::ctype<wchar_t>>(locale);
        folding.tolower(text.data(), text.data() + text.size());
    }

    // A count of a unit, plural where it is not one, and how long ago.
    static std::wstring countAgo(const std::int64_t count, const std::wstring_view unit)
    {
        std::wstring spelling = std::format(L"{} {}", count, unit);
        if (count != 1)
            spelling += L's';
        spelling += L" ago";
        return spelling;
    }

    // A moment ahead of now - the clock was set back since - reads as just now.
    std::wstring ageSpelling(const std::chrono::sys_seconds moment,
        const std::chrono::sys_seconds now)
    {
        const std::chrono::minutes minutes =
            std::chrono::floor<std::chrono::minutes>(now - moment);
        if (minutes < std::chrono::minutes{ 1 })
            return L"just now";
        if (minutes < std::chrono::hours{ 1 })
            return countAgo(minutes.count(), L"minute");
        const std::chrono::hours hours = std::chrono::floor<std::chrono::hours>(minutes);
        if (hours < std::chrono::days{ 1 })
            return countAgo(hours.count(), L"hour");
        return countAgo(std::chrono::floor<std::chrono::days>(hours).count(), L"day");
    }

    //-------------------------------------------------------------------------
    // UTF-8

    static constexpr char32_t k_replacement = 0xFFFD;

    static void appendUtf8(std::string& out, char32_t codePoint)
    {
        if (codePoint < 0x80)
        {
            out.push_back(static_cast<char>(codePoint));
        }
        else if (codePoint < 0x800)
        {
            out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
        else if (codePoint < 0x10000)
        {
            out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
        else
        {
            out.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
            out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
    }

    // A code point above the basic plane needs a surrogate pair where wchar_t is 16 bits and
    // fits a single unit where it is 32.
    static void appendWide(std::wstring& out, char32_t codePoint)
    {
        if constexpr (sizeof(wchar_t) == 2)
        {
            if (codePoint >= 0x10000)
            {
                codePoint -= 0x10000;
                out.push_back(static_cast<wchar_t>(0xD800 + (codePoint >> 10)));
                out.push_back(static_cast<wchar_t>(0xDC00 + (codePoint & 0x3FF)));
                return;
            }
        }
        out.push_back(static_cast<wchar_t>(codePoint));
    }

    // wchar_t is signed where it is an int, so a unit is widened through its unsigned form.
    // Sign extension would otherwise carry a unit past 0x10FFFF and every one of them would
    // read as malformed.
    static char32_t unitAt(std::wstring_view text, std::size_t i)
    {
        return static_cast<char32_t>(static_cast<std::make_unsigned_t<wchar_t>>(text[i]));
    }

    std::string toUtf8(std::wstring_view text)
    {
        std::string result;
        result.reserve(text.size());
        for (std::size_t i = 0; i != text.size(); ++i)
        {
            char32_t codePoint = unitAt(text, i);
            if constexpr (sizeof(wchar_t) == 2)
            {
                if (codePoint >= 0xD800 && codePoint <= 0xDBFF)
                {
                    // The unit after a high surrogate is only consumed once it has been read as
                    // a low one, so a high surrogate standing on its own leaves whatever
                    // follows it to be encoded on its own terms.
                    char32_t low = i + 1 != text.size() ? unitAt(text, i + 1) : 0;
                    if (low >= 0xDC00 && low <= 0xDFFF)
                    {
                        codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00);
                        ++i;
                    }
                    else
                    {
                        codePoint = k_replacement;
                    }
                }
                else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF)
                {
                    codePoint = k_replacement;
                }
            }
            else
            {
                if (codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF))
                {
                    codePoint = k_replacement;
                }
            }
            appendUtf8(result, codePoint);
        }
        return result;
    }

    std::wstring fromUtf8(std::string_view bytes)
    {
        std::wstring result;
        result.reserve(bytes.size());
        std::size_t i = 0;
        while (i != bytes.size())
        {
            const unsigned char lead = static_cast<unsigned char>(bytes[i]);
            char32_t codePoint = 0;
            std::size_t continuations = 0;
            // What this many continuation bytes is allowed to encode. Anything below it is the
            // same character spelled longer than it needs to be, which is rejected: two
            // spellings of one character is what lets a check on the short one be walked past.
            char32_t lowest = 0;
            if (lead < 0x80)
            {
                codePoint = lead;
            }
            else if ((lead & 0xE0) == 0xC0)
            {
                codePoint = lead & 0x1Fu;
                continuations = 1;
                lowest = 0x80;
            }
            else if ((lead & 0xF0) == 0xE0)
            {
                codePoint = lead & 0x0Fu;
                continuations = 2;
                lowest = 0x800;
            }
            else if ((lead & 0xF8) == 0xF0)
            {
                codePoint = lead & 0x07u;
                continuations = 3;
                lowest = 0x10000;
            }
            else
            {
                appendWide(result, k_replacement);
                ++i;
                continue;
            }

            // Only the continuation bytes that are there are consumed, so one malformed
            // sequence costs one replacement and the byte that ended it is read again as a
            // lead rather than swallowed with it.
            std::size_t consumed = 1;
            bool incomplete = false;
            for (std::size_t k = 0; k != continuations; ++k)
            {
                if (i + consumed == bytes.size())
                {
                    incomplete = true;
                    break;
                }
                const unsigned char next = static_cast<unsigned char>(bytes[i + consumed]);
                if ((next & 0xC0) != 0x80)
                {
                    incomplete = true;
                    break;
                }
                codePoint = (codePoint << 6) | (next & 0x3Fu);
                ++consumed;
            }

            const bool valid = !incomplete
                && codePoint >= lowest
                && codePoint <= 0x10FFFF
                && !(codePoint >= 0xD800 && codePoint <= 0xDFFF);
            appendWide(result, valid ? codePoint : k_replacement);
            i += consumed;
        }
        return result;
    }
}
