export module ClaFi.Tools.WhatsClip.LanguagePick;

import ClaFi.Tools.WhatsClip.AutoPick;
import ClaFi.Tools.WhatsClip.Languages;

import ClaFi.Controls.ComboBox;

import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    // Auto and the list's languages, the face on Auto reading the language detected.
    export class LanguagePick : public AutoPicker
    {
    public:
        template<typename... Args>
        LanguagePick(const CreateParams&, const LanguageList&, float fontSize, Args&&...);
    public:
        // The language picked by hand, or none while Auto is picked.
        [[nodiscard]] std::optional<Syntax::Language> pickedLanguage() const;
        // Asks the list, puts the name found on the face, and answers the language.
        Syntax::Language detect(std::wstring_view formatName, std::wstring_view text);
    private:
        [[nodiscard]] static Names namesOf(const LanguageList&);
    private:
        const LanguageList& m_languages;
    };


//-----------------------------------------------------------------------------


    template<typename... Args>
    LanguagePick::LanguagePick(const CreateParams& params, const LanguageList& languages,
        const float fontSize, Args&&... args)
        :
        AutoPicker{ params, namesOf(languages), fontSize, std::forward<Args>(args)... },
        m_languages{ languages }
    {
    }
}
