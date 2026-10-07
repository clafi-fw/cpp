module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Documents.BasePage;

import ClaFi.Documents.Folder;

import ClaFi.Browser.Control;

import ClaFi.Controls.Divider;
import ClaFi.Controls.Panel;
import ClaFi.Controls.Stack;

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
        [[nodiscard]] Stack& toolBar() const { return m_toolBar; }
        // The panel the tool bar is the body of, for a page adding a bar of its own beside it.
        [[nodiscard]] Panel& topPanel() const { return m_topPanel; }
    private:
        DocumentsFolder& m_folder;
        Stack& m_topStack{ createTopBar<Stack>(
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
        Stack& m_toolBar{ m_topPanel.createBody<Stack>(
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
        // The folder the page's document stands in, which outlives the page.
        m_folder{ *REQUIRE_PROPERTY(DocumentsFolder*) }
    {
    }
}
