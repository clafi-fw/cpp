export module ClaFi.Documents.BasePage;

import ClaFi.Documents.Folder;

import ClaFi.Browser.Control;

import ClaFi.Controls.Divider;
import ClaFi.Controls.Panel;
import ClaFi.Controls.StackPanel;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Documents
{
    using namespace Controls;

    // A page of a documents browser: a tool bar, and the browser's questions. See Documents#pages
    export class DocumentsBasePage : public Browser::BrowserPage
    {
    public:
        template<typename... Args>
        explicit DocumentsBasePage(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] DocumentsFolder& folder() const { return m_folder; }
        // Brings the page up on what its tab's view state holds.
        virtual void restoreViewState() {}
        // Whether this page holds work the file behind it has never seen.
        [[nodiscard]] virtual bool hasUnsavedEdits() const { return false; }
        // Whether saveEdits writes without asking anything. A page whose work has no file of its
        // own answers no - it still saves, and asks where first.
        [[nodiscard]] virtual bool canSaveEdits() const { return true; }
        // Puts this page's work on the disk, and answers whether it got there. A question of the
        // page's own stands under the initiator - the crumb, the Up button, the tab being closed.
        [[nodiscard]] virtual bool saveEdits(Control& /*initiator*/) const { return true; }
    protected:
        [[nodiscard]] StackPanel& toolBar() const { return m_toolBar; }
    private:
        DocumentsFolder& m_folder;
        StackPanel& m_topStack{ createTopBar<StackPanel>(
            Orientation::Vertical,
            UiElement::ToolBar
        ) };
        Panel& m_topPanel{ m_topStack.add<Panel>(
            Padding{ 4.0f },
            Spacing{ 0.0f, 4.0f }
        ) };
        Divider& m_topDivider{ m_topStack.add<Divider>(
            Padding{ 0.0f, 0.0f }
        ) };
        StackPanel& m_toolBar{ m_topPanel.createBody<StackPanel>(
            Orientation::Horizontal,
            Interactivity::ActiveContainer
        ) };
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    DocumentsBasePage::DocumentsBasePage(const CreateParams& params, Args&&... args)
        :
        BrowserPage{
            params,
            Padding{ 0.0f },
            Spacing{ 0.0f },
            UiElement::Page,
            std::forward<Args>(args)...
        },
        m_folder{ *Props::get<DocumentsFolder*>(nullptr, args...) }
    {
    }
}
