export module ThisApp.ScriptPage;

import ThisApp.Language;
import ThisApp.Scripts;

import ClaFi.Documents.Page;

import ClaFi.Controls.CodeBox;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Syntax.Completion;
import ClaFi.Core.Syntax.Languages;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // One script, edited in place: the box on the page, over the file the page stands for.
    export class ScriptPage : public Documents::DocumentPage
    {
    public:
        template<typename... Args>
        explicit ScriptPage(const CreateParams&, Args&&...);
    public:
        void restoreViewState() override;
    private:
        // Every edit ends here: the tab node takes the text as it stands on screen.
        void storeViewState() const;
    private:
        ScrollBox& m_scrollBox{ createBody<ScrollBox>(
            ScrollBars::Both
        ) };
        // The script, read as Pascal.
        CodeBox& m_box{ m_scrollBox.createBody<CodeBox>(
            Syntax::Languages::pascal,
            Padding{ 12.0f }
        ) };
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    ScriptPage::ScriptPage(const CreateParams& params, Args&&... args)
        :
        DocumentPage{ params, std::forward<Args>(args)... }
    {
        // The folder the browser hands every page is the application's own, which is what
        // knows the language; the cast states that.
        const ScriptsFolder& scripts = static_cast<const ScriptsFolder&>(folder());
        m_box.setCompletion(&scripts.language().completion());
        m_box.setTitleBlock(&scripts.language().titleBlock());
        // The box keeps the script's history, and the page's Undo and Redo walk it.
        setEditHistory(m_box);
        m_box.onTextEdit([this](TextEditEvent&) {
            storeViewState();
        });
    }
}
