export module ClaFi.Tools.WhatsClip.Languages;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    // What a text page picks from, in this order - which is also the order Auto breaks a tie in.
    export using LanguageList = std::vector<Syntax::Language>;

    // The language of a format's bytes left to Auto: what the name settles where the list offers
    // it - ClaFi for the framework's rich text, XML for a format named after XML or HTML - else
    // the list's surest claim on the text, else none. The name is the bytes' alone: a text
    // decoded from them is asked about with none.
    export [[nodiscard]] Syntax::Language detectLanguage(const LanguageList&,
        std::wstring_view formatName, std::wstring_view text);

    // The framework's languages: ClaFi, JSON, XML, C++, Pascal, and Text last of all.
    export [[nodiscard]] LanguageList defaultLanguages();
}
