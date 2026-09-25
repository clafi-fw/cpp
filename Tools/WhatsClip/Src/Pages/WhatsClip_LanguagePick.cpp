module ClaFi.Tools.WhatsClip.LanguagePick;

import ClaFi.Tools.WhatsClip.AutoPick;
import ClaFi.Tools.WhatsClip.Languages;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    std::optional<Syntax::Language> LanguagePick::pickedLanguage() const
    {
        const std::optional<std::size_t> picked = pickedIndex();
        if (!picked)
            return std::nullopt;

        return m_languages[*picked];
    }

    Syntax::Language LanguagePick::detect(const std::wstring_view formatName,
        const std::wstring_view text)
    {
        const Syntax::Language language = detectLanguage(m_languages, formatName, text);
        setFoundName(language.name);
        return language;
    }

    AutoPicker::Names LanguagePick::namesOf(const LanguageList& languages)
    {
        Names names{};
        names.reserve(languages.size());
        for (const Syntax::Language& language : languages)
            names.push_back(language.name);

        return names;
    }
}
