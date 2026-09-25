module ClaFi.Tools.WhatsClip.Languages;

import ClaFi.StdActions.Transfer;

import ClaFi.Core.Syntax.Languages;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    namespace
    {
        // Whether the name carries the word, whatever the case it is spelled in.
        [[nodiscard]] bool nameCarries(const std::wstring_view name,
            const std::wstring_view upperWord)
        {
            std::wstring upper{ name };
            for (wchar_t& value : upper)
                value = static_cast<wchar_t>(std::towupper(value));

            return upper.find(upperWord) != std::wstring::npos;
        }

        // The list's language of that name, if it offers one.
        [[nodiscard]] std::optional<Syntax::Language> listed(const LanguageList& languages,
            const std::wstring_view name)
        {
            for (const Syntax::Language& language : languages)
            {
                if (language.name == name)
                    return language;
            }
            return std::nullopt;
        }

        // WHAT A FORMAT'S NAME SETTLES BEFORE ITS BYTES ARE READ. The framework's rich text is
        // written as ClaFi, whose strings may hold any language's source - a reading of the text
        // would find that language instead. A format named after XML or HTML carries XML under a
        // header no reading of the text sees past.
        [[nodiscard]] std::optional<Syntax::Language> namedLanguage(const LanguageList& languages,
            const std::wstring_view formatName)
        {
            if (formatName == Transfer::ClaFiText::k_name)
                return listed(languages, Syntax::Languages::claFi.name);

            if (nameCarries(formatName, L"XML") || nameCarries(formatName, L"HTML"))
                return listed(languages, Syntax::Languages::xml.name);

            return std::nullopt;
        }
    }

    Syntax::Language detectLanguage(const LanguageList& languages,
        const std::wstring_view formatName, const std::wstring_view text)
    {
        const std::optional<Syntax::Language> named = namedLanguage(languages, formatName);
        if (named.has_value())
            return named.value();

        return Syntax::detect(languages, text);
    }

    LanguageList defaultLanguages()
    {
        return {
            Syntax::Languages::claFi,
            Syntax::Languages::json,
            Syntax::Languages::xml,
            Syntax::Languages::cpp,
            Syntax::Languages::pascal,
            Syntax::Languages::text,
        };
    }
}
