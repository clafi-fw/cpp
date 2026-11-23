export module ClaFi.Tools.WhatsClip.Main;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.AppButton;
import ClaFi.Controls.DialogTitle;
import ClaFi.Controls.Panel;
import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.App.ApplicationProperties;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    using namespace Controls;

    export [[nodiscard]] ApplicationProperties applicationParams();

    export class MainForm : public WithBody<Panel, TabbedBox>
    {
    public:
        MainForm(const CreateParams& params, PickLists picks);
        ~MainForm() override;
    private:
        class Viewer;
    private:
        DialogTitle& m_titleBar;
        AppButton& m_appButton;
        std::unique_ptr<Viewer> m_viewer{};
        ScopedEventConnection m_clipboardChanged{};
    };
}
