module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.CodeBox;

import ClaFi.Controls.TextBox;
import ClaFi.Controls.Button;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.Syntax.Completion;
import ClaFi.Core.Syntax.Indent;
import ClaFi.Core.Syntax.Lines;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export class CodeBox;
    class CompletionStack;

    // Whether the box asks which language each text it is handed is in. See Controls
    export enum class DetectLanguage
    {
        No,
        Yes
    };

    // Whether the box reads the indent unit off each text it is handed. See Controls
    export enum class DetectIndent
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

    // Which names a place on a line lists. See Controls#completionlist
    enum class CompletionScope : std::uint8_t
    {
        Global,   // what stands on its own - the text's own names, the list's, the keywords
        Member    // what follows a dot - every class's methods and properties
    };

    // One row of the completion list: a name the box can complete to, its kind beside it.
    class CompletionRow : public Button
    {
    public:
        CompletionRow(const CreateParams&, CompletionStack&, CompletionScope,
            std::wstring_view name, Syntax::CompletionKind, const Syntax::CompletionEntry*,
            const Syntax::Declaration*);
        [[nodiscard]] CompletionScope scope() const { return m_scope; }
        [[nodiscard]] std::wstring_view name() const { return m_name; }
        // Whether the name is in force at that position of the text - everywhere, for a name
        // the text itself does not declare.
        [[nodiscard]] bool inForceAt(std::size_t pos) const;
    protected:
        void getText(GetTextEvent&) const override;
        // The signature and the hint beside the list. See Controls#completionlist
        void nestedGetTooltip(GetTooltipEvent&) override;
        void nestedClick(ClickEvent&) override;
    private:
        CompletionStack& m_stack;
        CompletionScope m_scope;
        std::wstring m_name;
        Syntax::CompletionKind m_kind;
        const Syntax::CompletionEntry* m_entry;   // what the hint reads - null for a keyword
        // The text's own declaration the row stands for - null for a name from elsewhere.
        const Syntax::Declaration* m_declaration;
    };

    // The rows, shown by scope and by what is typed. See Controls#completionlist
    class CompletionStack : public StackPanel
    {
    public:
        CompletionStack(const CreateParams&, CodeBox&);
        [[nodiscard]] CodeBox& box() const { return m_box; }
        // The name as typed so far - what a row marks in its own.
        [[nodiscard]] std::wstring_view typed() const { return m_typed; }
        // Builds the rows anew: the text's own declarations first, the nearest scope's ahead,
        // then the list's names by scope and the language's keywords - each run in name order
        // under the language's case rule, a member two classes name listed once.
        void rebuild(const Syntax::Language&, const Syntax::CompletionEntries*,
            const Syntax::Declarations&);
        // Shows the rows of the scope that begin with what is typed and are in force at the
        // caret's position in the text, the first of them current, and answers how many.
        std::size_t filter(const Syntax::Language&, CompletionScope, std::wstring_view typed,
            std::size_t caret);
        // Moves the current row one shown row up or down, staying at either end.
        void moveCurrent(ScrollDirection);
        // Moves the current row a view less one row up or down, staying at either end.
        void moveCurrentByPage(ScrollDirection);
        [[nodiscard]] CompletionRow* currentRow() const;
        // Puts the current row's hint up beside it, or takes the hint down with no row current.
        void showCurrentHint();
    protected:
        // Every row may be current: the box moves it, and the base's answer would rule that out.
        bool defaultCanFocusItem(Control&) override;
        // The current row reads selected - the base says so only for a stack that follows the user.
        void getControlState(GetStateEvent&) const override;
    private:
        [[nodiscard]] std::vector<CompletionRow*> shownRows() const;
        void setCurrentRow(CompletionRow*);
    private:
        CodeBox& m_box;
        std::wstring m_typed;
    };

    // The completion list as it is dropped: a box that scrolls, holding the rows.
    using CompletionList = ScrollBoxWith<CompletionStack>;

    // A text box that colours its text as source in a language. See Controls
    export class CodeBox : public TextBox, private ColorOverlay
    {
        friend CompletionRow;
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
        // The names the box completes to, held by whoever states them. None completes nothing.
        DECLARE_WRITABLE_PROPERTY(const Syntax::CompletionEntries*, completion, setCompletion,
            nullptr)
        // What one level of indent is written as, where the text says nothing of its own.
        DECLARE_WRITABLE_PROPERTY(Syntax::IndentUnit, indentUnit, setIndentUnit,
            Syntax::IndentUnit{})
        // Whether the unit is read off each text the box is handed. See Controls#indents
        DECLARE_WRITABLE_PROPERTY(DetectIndent, detectIndent, setDetectIndent, DetectIndent::Yes)
    public:
        // Asked which language a text the box was handed whole is in. See Controls
        DECLARE_EVENT(DetectLanguageEvent, OnDetectLanguage, onDetectLanguage)
    public:
        // States the language and ends detection: the whole text is read again in it.
        void setLanguage(const Syntax::Language&);
        // Yes asks at once for the text the box holds, and again for every text handed whole.
        void setDetectLanguage(DetectLanguage);
        // States the list the box completes from, or none. Named, not copied: it outlives the box.
        void setCompletion(const Syntax::CompletionEntries*);
        // Lists what the caret's place can complete to - Ctrl+Space. See Controls#completionlist
        void showCompletion();
        // States the unit and ends detection: every indent the box writes is written in it.
        void setIndentUnit(const Syntax::IndentUnit&);
        // Yes reads the unit off the text the box holds now, and off every text handed whole.
        void setDetectIndent(DetectIndent);
        // What the box writes a level as: what the text says where it is read, else the property.
        [[nodiscard]] Syntax::IndentUnit indentUnitInUse() const;
        // Moves the lines the selection reaches, or the caret's, one stop on. See Controls#indents
        void indentLines();
        // Moves the lines the selection reaches, or the caret's, one stop back.
        void outdentLines();
        // Places the lines the selection reaches, or every line, where the language places them.
        void reindentLines();
    protected:
        void textTaken(const Text&, const TextEdit*) const override;
        void textEdited() override;
        void caretMoved() override;
        void nestedKeyDown(KeyDownEvent&) override;
        void charPress(CharPressEvent&) override;
        // Past the blanks the line opens with, and to the line's start from there.
        [[nodiscard]] std::size_t rowHome(CaretHit) const override;
        // A paste of several lines moved as one block to where the language places it.
        void textPasted(const TextRange&) override;
        // Reindent joins the edit menu of a box that can be typed into.
        void editContextPopup(EditContextPopupEvent&) override;
    private:
        // The line the caret stands on, and where the caret stands in it.
        struct CaretLine
        {
            std::size_t index{ 0 };      // which line, counted from zero - the paragraph's index
            std::size_t start{ 0 };      // where the line starts in the text
            std::wstring_view text{};    // the line, without its newline
            std::size_t caret{ 0 };      // the caret's index in the line
        };
    private:
        // The language the text is read in from now on: the answer while detecting, else the one
        // stated. Reads the whole text again.
        void readWhole(std::wstring_view text) const;
        void paragraphColors(std::size_t paragraph, std::wstring_view paragraphText,
            std::vector<ColorSpan>& out) const override;
        [[nodiscard]] bool completionShown() const;
        // Asks for the list to be brought up to the text, once the input that asked has been
        // delivered. Opening lists a name being typed; otherwise a list up is narrowed or taken
        // down, and none is put up.
        void requestCompletion(bool opening);
        // Lists what the caret's place names, or takes the list down. See Controls#completionlist
        void updateCompletion();
        // Reads the text's own declarations anew where the text has changed since the last
        // reading; the rows follow only where the declarations did.
        void readDeclarations();
        void hideCompletion();
        // Makes the list's window, on the first request.
        void ensureCompletionList();
        // Puts the row's name in place of the name typed, in one edit, with the list down.
        void takeCompletion(const CompletionRow&);
        [[nodiscard]] CaretLine caretLine() const;
        // Answers the reindent action. Defined in CodeBox.cpp, which keeps the standard actions
        // out of the interface - see TextBox::connectEditActions.
        void connectIndentActions();
        // The selection clamped to the text - select all states a length past its end.
        [[nodiscard]] TextRange selectedRange() const;
        // The lines as the indent is worked out over them, the line states in step with the text.
        [[nodiscard]] Syntax::IndentLines sourceLines() const;
        // Makes the indent edit the box's next step, the editor's own, landing where it says.
        void applyIndentEdit(const std::optional<Syntax::IndentEdit>&);
        // Tab with no selection, or one inside a line: the blanks to the next stop in its place.
        void insertStop();
        void shiftLines(bool back);
        // Backspace at a caret in a line's indent takes it back to the previous stop. Answers
        // whether it did, which a single blank to take never needs.
        [[nodiscard]] bool unindentAtCaret();
        // Places the caret's line where its first word says, where that word has just been
        // finished - by the character before the caret, or by the caret itself.
        void realignFinishedWord(bool byCharacter);
        // What a line break just typed goes on to do. See Syntax#indent
        void indentAfterBreak();
    private:
        // The list's rows, as many as the theme's tool button makes tall; a longer list scrolls.
        static constexpr float k_completionRows{ 10.0f };
        static constexpr float k_completionPadding{ 4.0f };
        static constexpr float k_completionMinWidth{ 180.0f };
        // The state every line starts in, kept beside the layout's shaping and on the same terms
        // - a memo of the box's own text, brought into step whenever the layout is.
        mutable Syntax::LineStates m_lines;
        // The tokens of the paragraph being drawn, grown once and reused for every paragraph after.
        mutable Syntax::Tokens m_tokens;
        // The list, a window of its own on the box, shown and hidden as what is typed changes.
        std::optional<Form<CompletionList>> m_completionList{};
        // The list asked for on a key, answered once the input that asked has been delivered.
        UiTimer m_completionRequest{};
        bool m_completionOpening{ false };    // whether the request may put the list up
        bool m_completionExplicit{ false };   // whether the request lists with nothing typed
        // Whether a request found the layout unsettled and waits on the pass that settles it.
        bool m_completionWaitsOnAlign{ false };
        // Whether the rows are to be built anew - the language, the list or the declarations
        // have changed.
        mutable bool m_completionRowsStale{ true };
        // The names the text declares for itself, as the language read them when the list last
        // opened, and whether the text has changed since. See Controls#completionlist
        Syntax::Declarations m_declarations{};
        mutable bool m_declarationsStale{ true };
        // Whether the character of the press being answered belongs in the text. THE CHARACTER OF
        // A PRESS IS QUEUED BEFORE THE PRESS IS ANSWERED, so a Return that took a row would still
        // break the line: the press settles this and the character reads it - see charPress.
        bool m_completionTakesChar{ true };
        // Whether the edit under way is a row being taken, which is the one edit no list follows.
        bool m_takingCompletion{ false };
        // The unit the text read whole says it is indented in, where the box reads one.
        mutable std::optional<Syntax::IndentUnit> m_detectedIndent{};
        // Whether the key before this one was Escape, which hands a Tab on to the form.
        bool m_tabLeaves{ false };
        ScopedEventConnection m_formAligned{};
        ScopedEventConnection m_listAligned{};
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
        INIT_PROPERTY(detectLanguage),
        INIT_PROPERTY(completion),
        INIT_PROPERTY(indentUnit),
        INIT_PROPERTY(detectIndent)
    {
        m_layout.setColorOverlay(this);
        // Connected here rather than given to the timer as a construction property: MSVC rejects
        // a this-capturing lambda in a default member initializer.
        m_completionRequest.onTick([this](TimerEvent&){
            updateCompletion();
        });
        connectIndentActions();
    }
}
