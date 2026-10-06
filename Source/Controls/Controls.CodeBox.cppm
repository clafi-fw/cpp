module;
#include "../Y-Core/System/EventBindings.h"

export module ClaFi.Controls.CodeBox;

import ClaFi.Controls.TextBox;
import ClaFi.Controls.Button;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.Stack;

import ClaFi.Core.Syntax.Completion;
import ClaFi.Core.Syntax.Indent;
import ClaFi.Core.Syntax.Lines;
import ClaFi.Core.Syntax.Lexer;
import ClaFi.Core.Syntax.Types;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.History;
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

    // Whether a CodeBox draws a line down each block its text opens. See Controls#indentguides
    export enum class IndentGuides
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
        Member,   // what follows a dot - the methods and properties of the subject's classes
        Title     // what a title block's line takes - its keys, or the values of the key it states
    };

    // One row of the completion list: a name the box can complete to, its kind beside it.
    class CompletionRow : public Button
    {
    public:
        CompletionRow(const CreateParams&, CompletionStack&, CompletionScope,
            const Syntax::CompletionEntry* owner, std::wstring_view name, Syntax::CompletionKind,
            const Syntax::CompletionEntry*, const Syntax::Declaration*);
        [[nodiscard]] CompletionScope scope() const { return m_scope; }
        [[nodiscard]] const Syntax::CompletionEntry* owner() const { return m_owner; }
        [[nodiscard]] std::wstring_view name() const { return m_name; }
        // Whether the name is in force at that position of the text - everywhere, for a name
        // the text itself does not declare.
        [[nodiscard]] bool inForceAt(std::size_t pos) const;
    protected:
        void getText(GetTextEvent&) const override;
        // The signature and the hint beside the list. See Controls#completionlist
        void nestedGetHint(GetHintEvent&) override;
        void nestedClick(ClickEvent&) override;
    private:
        CompletionStack& m_stack;
        CompletionScope m_scope;
        const Syntax::CompletionEntry* m_owner;   // the class a member is read off - null elsewhere
        std::wstring m_name;
        Syntax::CompletionKind m_kind;
        const Syntax::CompletionEntry* m_entry;   // what the hint reads - null for a keyword
        // The text's own declaration the row stands for - null for a name from elsewhere.
        const Syntax::Declaration* m_declaration;
    };

    // The rows, shown by scope and by what is typed. See Controls#completionlist
    class CompletionStack : public Stack
    {
    public:
        CompletionStack(const CreateParams&, CodeBox&);
        [[nodiscard]] CodeBox& box() const { return m_box; }
        // The name as typed so far - what a row marks in its own.
        [[nodiscard]] std::wstring_view typed() const { return m_typed; }
        // Builds the rows anew: the text's own declarations first, the nearest scope's ahead,
        // then the list's names, the keywords and the title block's by scope - each run in name
        // order under the language's case rule, a member once for every class stating it.
        void rebuild(const Syntax::Language&, const Syntax::CompletionEntries*,
            const Syntax::Declarations&, const Syntax::TitleBlock*);
        // Shows the rows of the scope that begin with what is typed, are in force at the caret
        // and not among the stated, each read off the nearest of those owners to state it - a
        // Global row off none - the first shown current, and answers how many.
        std::size_t filter(const Syntax::Language&, CompletionScope,
            const Syntax::CompletionClasses& owners, std::wstring_view typed, std::size_t caret,
            const Syntax::CompletionNames& stated);
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

    // The window a call's signature shows in, above the caret's line. See Controls#parameterhint
    class ParameterHint : public WithTextLayout<FormControlBase>
    {
    public:
        using WithTextLayout<FormControlBase>::WithTextLayout;
        // States what the hint says: the signature from where its list opens with the argument
        // the caret stands in bold, or No parameters where it lists none, and the entry's hint.
        void setCall(const Syntax::CompletionEntry&, const Syntax::SignatureParameters&,
            std::size_t argument);
    protected:
        void getText(GetTextEvent&) const override;
        // Asked no wider than a hint's line, so a long signature wraps rather than runs.
        CalculatedDimensions measureText(AlignEvent&, ScaledDimensions asked,
            const Text&) override;
    private:
        Text m_text{};
    };

    using ParameterHintForm = Form<ParameterHint>;

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
        // The title block the box completes keys and values in, held by whoever states it.
        DECLARE_WRITABLE_PROPERTY(const Syntax::TitleBlock*, titleBlock, setTitleBlock, nullptr)
        // What one level of indent is written as, where the text says nothing of its own.
        DECLARE_WRITABLE_PROPERTY(Syntax::IndentUnit, indentUnit, setIndentUnit,
            Syntax::IndentUnit{})
        // Whether the unit is read off each text the box is handed. See Controls#indents
        DECLARE_WRITABLE_PROPERTY(DetectIndent, detectIndent, setDetectIndent, DetectIndent::Yes)
        // Whether a line runs down each block the text opens. See Controls#indentguides
        DECLARE_WRITABLE_PROPERTY(IndentGuides, indentGuides, setIndentGuides, IndentGuides::Yes)
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
        // States the title block the box completes in, or none. Named, not copied, as the list is.
        void setTitleBlock(const Syntax::TitleBlock*);
        // Lists what the caret's place can complete to - Ctrl+Space. See Controls#completionlist
        void showCompletion();
        // Shows the signature of the call the caret stands in - Ctrl+Shift+Space. See
        // Controls#parameterhint
        void showParameterHint();
        // States the unit and ends detection: every indent the box writes is written in it.
        void setIndentUnit(const Syntax::IndentUnit&);
        // Yes reads the unit off the text the box holds now, and off every text handed whole.
        void setDetectIndent(DetectIndent);
        void setIndentGuides(IndentGuides);
        // Past the end while it takes typing - the last line goes up to where the next is written.
        [[nodiscard]] ScrollMetrics scrollMetrics() const override;
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
        // The guides first, on the layout the text is drawn from, so a selection band covers them.
        DrawTextResult drawText(PaintEvent&, const FloatRect& textBounds, const Text&) override;
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
        // The place the caret names on a line of the title block, where it names one.
        [[nodiscard]] std::optional<Syntax::TitlePlace> titlePlace(const CaretLine&) const;
        // Shows the rows the place takes under the name being written, or takes the list down.
        void listCompletion(CompletionScope, const Syntax::CompletionClasses& owners,
            const Syntax::CompletionNames& stated, const CaretLine&, std::size_t wordStart);
        // Reads the text's own declarations anew where the text has changed since the last
        // reading; the rows follow only where the declarations did.
        void readDeclarations();
        void hideCompletion();
        // Makes the list's window, on the first request.
        void ensureCompletionList();
        // Puts the row's name in place of the name typed, in one edit, with the list down.
        void takeCompletion(const CompletionRow&);
        [[nodiscard]] bool parameterHintShown() const;
        // Asks for the hint to be brought up to the caret, once the input that asked has been
        // delivered. Opening shows the call the caret stands in; otherwise a hint up follows the
        // caret or comes down, and none is put up.
        void requestParameterHint(bool opening);
        // Shows the call the caret stands in, or takes the hint down. See Controls#parameterhint
        void updateParameterHint();
        void hideParameterHint();
        // Makes the hint's window, on the first request.
        void ensureParameterHint();
        [[nodiscard]] CaretLine caretLine() const;
        // Answers the reindent action. Defined in CodeBox.cpp, which keeps the standard actions
        // out of the interface - see TextBox::connectEditActions.
        void connectIndentActions();
        // The selection clamped to the text - select all states a length past its end.
        [[nodiscard]] TextRange selectedRange() const;
        // The lines as the indent is worked out over them, the line states in step with the text.
        [[nodiscard]] Syntax::IndentLines sourceLines() const;
        // Makes the indent edit a step, or part of the one before it, landing where it says.
        void applyIndentEdit(const std::optional<Syntax::IndentEdit>&, EditKind,
            const Text& what = {});
        // Tab with no selection, or one inside a line: the blanks to the next stop in its place.
        void insertStop();
        void shiftLines(bool back);
        // What a step moving that many lines is called - Indent 3 lines.
        [[nodiscard]] static Text linesStepName(std::wstring_view verb, std::size_t lines);
        // Backspace at a caret in a line's indent takes it back to the previous stop. Answers
        // whether it did, which a single blank to take never needs.
        [[nodiscard]] bool unindentAtCaret();
        // Places the caret's line where its first word says, where that word has just been
        // finished - by the character before the caret, or by the caret itself.
        void realignFinishedWord(bool byCharacter);
        // What a line break just typed goes on to do. See Syntax#indent
        void indentAfterBreak();
        // A line down each block in view, the caret's a grade stronger. See Controls#indentguides
        void paintIndentGuides(PaintEvent&, TextLayout&, FloatPoint origin);
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
        // The hint, a window of its own over the box, shown while the caret stands in a call.
        std::optional<ParameterHintForm> m_parameterHint{};
        // The hint asked for on a key, answered once the input that asked has been delivered.
        UiTimer m_parameterHintRequest{};
        bool m_parameterHintOpening{ false };   // whether the request may put the hint up
        // Whether a request found the layout unsettled and waits on the pass that settles it.
        bool m_parameterHintWaitsOnAlign{ false };
        // The unit the text read whole says it is indented in, where the box reads one.
        mutable std::optional<Syntax::IndentUnit> m_detectedIndent{};
        // Whether the key before this one was Escape, which hands a Tab on to the form.
        bool m_tabLeaves{ false };
        // The blocks the text opens, read again on the first paint after the text changes.
        mutable Syntax::SourceBlocks m_blocks{};
        mutable bool m_blocksStale{ true };
        ScopedEventConnection m_formAligned{};
        ScopedEventConnection m_formMoved{};
        ScopedEventConnection m_formFocusChanged{};
        ScopedEventConnection m_listAligned{};
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    CodeBox::CodeBox(const CreateParams& params, Args&&... args)
        :
        // Source is lines, and a line is never broken to the box it is drawn in - see HexView.
        // It is columns too, which only the monospace style keeps. Both before the caller's own
        // arguments, so a stated WordWrap or TextFormat still wins.
        TextBox{ params, WordWrap::No, TextFormat{ TextStyleId::Code }, std::forward<Args>(args)... },
        INIT_PROPERTY(language),
        INIT_PROPERTY(inks),
        INIT_PROPERTY(detectLanguage),
        INIT_PROPERTY(completion),
        INIT_PROPERTY(titleBlock),
        INIT_PROPERTY(indentUnit),
        INIT_PROPERTY(detectIndent),
        INIT_PROPERTY(indentGuides)
    {
        m_layout.setColorOverlay(this);
        // TextBox's constructor stated the empty text to the layout, where textTaken cannot reach
        // this class, so the line states are brought into step with it here.
        readWhole(m_layoutText.plainText());
        // Connected here rather than given to the timer as a construction property: MSVC rejects
        // a this-capturing lambda in a default member initializer.
        m_completionRequest.onTick([this](TimerEvent&){
            updateCompletion();
        });
        m_parameterHintRequest.onTick([this](TimerEvent&){
            updateParameterHint();
        });
        // A request that found the layout unsettled is answered once the pass that settles it
        // has run - see updateCompletion.
        m_formAligned = form().onAligned([this](FormAlignedEvent&){
            if (std::exchange(m_completionWaitsOnAlign, false))
                m_completionRequest.start(MilliSeconds{ 0u });
            if (std::exchange(m_parameterHintWaitsOnAlign, false))
                m_parameterHintRequest.start(MilliSeconds{ 0u });
        });
        // The hint stands where the window stood when it was placed, and it is the box's alone
        // to take down: the window moving out from under it, or losing the focus, takes it down
        // the way the form takes a popup down.
        m_formMoved = form().onPositionChange([this](FormPositionChangeEvent&){
            hideParameterHint();
        });
        m_formFocusChanged = form().onFocusChange([this](FormFocusChangeEvent&){
            if (!form().window().isFocused())
                hideParameterHint();
        });
        connectIndentActions();
    }
}
