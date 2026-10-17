export module SeeDocs_App.Editing;

import SeeDocs_App.Edits;
import SeeDocs_App.Pages;
import SeeDocs_App.Surface;

import ClaFi.Controls.InPlaceEdit;
import ClaFi.Controls.Base.WithEditMode;
import ClaFi.Controls.Label;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

// **A page's words edited where they stand**: the commands that start an edit, what a page writes
// the words back through, words typed into the box that shows them - a hint, the project's name -
// and a footnote's markdown typed in the footnote's place. See the README beside the project.
namespace SeeDocs_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The commands a page answers about the words they are raised on. Constructed before main,
    // so nothing here touches the platform, the theme or the text engine.
    namespace Actions
    {
        export extern Action editFootnote;
        export extern Action addFootnote;
        // Puts them in the application's scope, once, which is where F2 is looked up.
        export void registerAll();
    }

    // A footnote as its editor opens on it: the words, the section they are written into, and
    // what the editor says of that place - or why it cannot open.
    export struct FootnoteDraft
    {
        std::wstring text;                   // flowed - a paragraph a line
        std::wstring place;                  // the note and the section, as the editor's bar says
        Reference reference;
        std::optional<NewFootnote> placed;   // a footnote not written yet: where it goes
        std::wstring refusal;                // why it cannot be edited; empty where it can
    };

    // What a page writes its words back through - whoever holds the surface it was built from.
    export class IPageWriter
    {
    public:
        virtual ~IPageWriter() = default;
        // Whether a hint edited where it stands is written into the comment the site names.
        [[nodiscard]] virtual bool writesHint(const CommentSite&) const = 0;
        // Writes the hint into the comment the site names; the reason where it was refused.
        [[nodiscard]] virtual std::wstring writeHint(const CommentSite&,
            std::wstring_view hint) = 0;
        // The footnote's words as its note holds them.
        [[nodiscard]] virtual FootnoteDraft draftFootnote(const Reference&) = 0;
        // A footnote for the declaration, empty, placed where the page's new footnotes go.
        [[nodiscard]] virtual FootnoteDraft draftFootnote(const CommentSite&,
            const FootnoteHome&) = 0;
        // Writes the words into the draft's note; the reason where they were refused.
        [[nodiscard]] virtual std::wstring writeFootnote(const FootnoteDraft&,
            std::wstring_view text) = 0;
    };

    // One line typed into where it stands: Return or leaving writes it, Escape puts it back.
    export class WordsBox : public PromptBox
    {
    public:
        template<typename... Args>
        explicit WordsBox(const CreateParams&, Args&&...);
    public:
        void showWords(const Text&);   // what the box shows until edited, and Escape puts back
    protected:
        // Writes the words typed where they belong; the reason where they were refused.
        [[nodiscard]] virtual std::wstring writeWords(std::wstring_view words) = 0;
        // The words a write hands on, read off what the box holds.
        [[nodiscard]] virtual std::wstring wordsOf(const Text&) const;
        [[nodiscard]] bool edited() const;
        void editModeLeft(bool accepted) override;   // leaving writes, Escape puts back
        void charPress(CharPressEvent&) override;   // one line, and Return writes it
    private:
        void write();
        void putBack();
    private:
        Text m_shown{};
    };

    // A hint typed into where it stands, written into the comment it was read from.
    export class HintBox : public WordsBox
    {
    public:
        using WordsBox::WordsBox;
    public:
        // Takes edits into the comment the site names, read where it stands at each write.
        void connectSite(IPageWriter&, const CommentSite&);
    protected:
        [[nodiscard]] std::wstring writeWords(std::wstring_view words) override;
        // The bold and the code the box shows, written back as the comment marks them.
        [[nodiscard]] std::wstring wordsOf(const Text&) const override;
    private:
        IPageWriter* m_writer{ nullptr };
        const CommentSite* m_site{ nullptr };
    };

    // One footnote on a page: its words as the page shows them, and while it is edited, its
    // markdown in their place with where it goes and the ways out - Escape or Cancel leaves it,
    // Ctrl+S or Save writes it.
    export class FootnoteSlot : public StackPanel
    {
    public:
        template<typename... Args>
        explicit FootnoteSlot(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] TextBox& shown() const { return m_shown; }
        [[nodiscard]] bool editing() const { return m_editing; }
        // Opens the editor on the draft in place of the words shown, or says why it cannot.
        void edit(IPageWriter&, FootnoteDraft);
    protected:
        void nestedKeyDown(KeyDownEvent&) override;
    private:
        void buildEditor();
        void finish(bool write);
        void showRefusal(std::wstring_view reason);
    private:
        static constexpr float k_editorSpacing = 6.0f;
        static constexpr float k_editorPadding = 8.0f;
        static constexpr float k_barSpacing = 8.0f;

        IPageWriter* m_writer{ nullptr };
        FootnoteDraft m_draft{};
        bool m_editing{ false };
        TextBox& m_shown{ add<TextBox>(ReadOnly::Yes, EditModeSwitching::Yes, Padding{ 0.0f }) };
        StackPanel* m_editor{ nullptr };
        TextBox* m_box{ nullptr };
        Label* m_place{ nullptr };
        Control* m_cancel{ nullptr };
        Control* m_save{ nullptr };
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    WordsBox::WordsBox(const CreateParams& params, Args&&... args)
        :
        PromptBox{ params, EditModeSwitching::Yes, std::forward<Args>(args)... }
    {
    }

    template<typename... Args>
    FootnoteSlot::FootnoteSlot(const CreateParams& params, Args&&... args)
        :
        StackPanel{
            params,
            Orientation::Vertical,
            Spacing{ k_editorSpacing },
            std::forward<Args>(args)...
        }
    {
    }
}
