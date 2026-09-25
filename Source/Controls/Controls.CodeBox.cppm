module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.CodeBox;

import ClaFi.Controls.TextBox;

import ClaFi.Core.Syntax.Lines;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export class CodeBox;

    // Whether the box asks which language each text it is handed is in. See Controls
    export enum class DetectLanguage
    {
        No,
        Yes
    };

    // Asked which language the text the box was handed is in. See Controls
    export struct DetectLanguageEvent : public EventOf<const CodeBox>
    {
        DetectLanguageEvent(const CodeBox&, std::wstring_view wholeText);
        std::wstring_view text;        // the whole text, plain
        Syntax::Language language{};   // the answer, or none, which leaves the text plain
    };

    // A text box that colours its text as source in a language. See Controls
    export class CodeBox : public TextBox, private ColorOverlay
    {
    public:
        template<typename... Args>
        explicit CodeBox(const CreateParams&, Args&&...);
    public:
        // The language the text is read as. The default reads nothing, and the text stands plain.
        DECLARE_WRITABLE_PROPERTY(Syntax::Language, language, setLanguage, Syntax::Language{})
        // What each kind of token is drawn in.
        DECLARE_REF_PROPERTY(Syntax::Inks, inks, Syntax::defaultInks())
        // Whether the language is asked of OnDetectLanguage instead of read from language.
        DECLARE_WRITABLE_PROPERTY(DetectLanguage, detectLanguage, setDetectLanguage, DetectLanguage::No)
    public:
        // Asked which language a text the box was handed whole is in. See Controls
        DECLARE_EVENT(DetectLanguageEvent, OnDetectLanguage, onDetectLanguage)
    public:
        // States the language and ends detection: the whole text is read again in it.
        void setLanguage(const Syntax::Language&);
        // Yes asks at once for the text the box holds, and again for every text handed whole.
        void setDetectLanguage(DetectLanguage);
    protected:
        void textTaken(const Text&, const TextEdit*) const override;
    private:
        // The language the text is read in from now on: the answer while detecting, else the one
        // stated. Reads the whole text again.
        void readWhole(std::wstring_view text) const;
        void paragraphColors(std::size_t paragraph, std::wstring_view paragraphText,
            std::vector<ColorSpan>& out) const override;
    private:
        // The state every line starts in, kept beside the layout's shaping and on the same terms
        // - a memo of the box's own text, brought into step whenever the layout is.
        mutable Syntax::LineStates m_lines;
        // The tokens of the paragraph being drawn, grown once and reused for every paragraph after.
        mutable Syntax::Tokens m_tokens;
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    CodeBox::CodeBox(const CreateParams& params, Args&&... args)
        :
        // Source is lines, and a line is never broken to the box it is drawn in - see HexView.
        // Before the caller's own arguments, so a stated WordWrap still wins.
        TextBox{ params, WordWrap::No, std::forward<Args>(args)... },
        INIT_PROPERTY(language),
        INIT_PROPERTY(inks),
        INIT_PROPERTY(detectLanguage)
    {
        m_layout.setColorOverlay(this);
    }
}
