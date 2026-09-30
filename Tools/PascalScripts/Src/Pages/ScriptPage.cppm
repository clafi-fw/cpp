export module ThisApp.ScriptPage;

import ThisApp.Language;
import ThisApp.Scripts;

import ClaFi.Documents.Page;

import ClaFi.Controls.CodeBox;
import ClaFi.Controls.Label;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.SearchBox;
import ClaFi.Controls.StackPanel;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Syntax.Completion;
import ClaFi.Core.Syntax.Languages;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

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
        // Holds its own layout: a text per caret move would crowd the shared layout cache.
        using CaretReadout = WithTextLayout<Label>;
    private:
        // Every edit ends here: the tab node takes the text as it stands on screen.
        void storeViewState() const;
        void writeCaretReadout();
        [[nodiscard]] static Text caretReading(TextLineColumn caret);
        // Find, Find next and Find previous, answered for the script from anywhere on the page.
        void connectSearchActions();
        // Ctrl+F: the box takes the script's selection where the script holds the focus.
        void startSearch();
        void searchRequested(SearchRequest);
        void writeSearchResult();
    private:
        static constexpr float k_caretReadoutWidth = 104.0f;
        static constexpr float k_caretReadoutFontSize = 11.0f;
        static constexpr float k_searchWidth = 260.0f;
        // Both bars at all times: the corner the caret readout stands in shows only with both up.
        ScrollBox& m_scrollBox{ createBody<ScrollBox>(
            ScrollBars::Both
        ) };
        // The script, read as Pascal.
        CodeBox& m_box{ m_scrollBox.createBody<CodeBox>(
            Syntax::Languages::pascal,
            Padding{ 12.0f }
        ) };
        Panel& m_corner{ m_scrollBox.createCorner<Panel>() };
        // Anchored left in a stated width: a digit gained moves neither the reading nor the bar.
        CaretReadout& m_caretReadout{ m_corner.createBody<CaretReadout>(
            caretReading(TextLineColumn{}),
            MinSize{ k_caretReadoutWidth, 0.0f },
            MaxSize{ k_caretReadoutWidth, k_maxFloat },
            WordWrap::No,
            Padding{ 8.0f, 0.0f },
            HorizontalTextAnchor::Left,
            VerticalTextAnchor::Center
        ) };
        // At the tool bar's right end, over the script below it.
        SearchBox& m_search{ topPanel().createRightBar<SearchBox>(
            MinSize{ k_searchWidth, 0.0f },
            MaxSize{ k_searchWidth, k_maxFloat }
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
        m_box.onCaretMove([this](CaretMoveEvent&) {
            writeCaretReadout();
            writeSearchResult();
        });
        m_search.onSearch([this](SearchEvent& event) {
            searchRequested(event.request);
        });
        connectSearchActions();
    }
}
