export module TextEngine_App.Main;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.Panel;
import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.SearchBox;
import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.App.ApplicationProperties;
import ClaFi.App.WindowApplication;

import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace TextEngine_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    export [[nodiscard]] ApplicationProperties textEngineAppParams();
    export void applyTextEngineDefaults(WindowApplicationBase&);

    export class MainForm : public WithBody<Panel, TabbedBox>
    {
    public:
        explicit MainForm(const CreateParams& params);
        ~MainForm() override;
    private:
        class PageSearch;
    private:
        ScrollBoxWith<TextBox>& m_featuresPage;
        ScrollBoxWith<CodeBox>& m_codePage;
        ScrollBoxWith<TextBox>& m_ledgerPage;
        ScrollBoxWith<TextBox>& m_emojiPage;
        ScrollBoxWith<TextBox>& m_scriptsPage;
        ScrollBoxWith<TextBox>& m_formulasPage;
        DialogTitle& m_titleBar;
        AppButton& m_appButton;
        SearchBox& m_searchBox;
        std::unique_ptr<PageSearch> m_search;
    };
}
