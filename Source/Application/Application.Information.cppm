export module ClaFi.App.Information;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;

import ClaFi.StdLib;

namespace ClaFi
{
    // The backstage page naming the application, what it does and its framework. See Application
    export class InformationPage : public Controls::StackPanel
    {
    public:
        explicit InformationPage(const CreateParams&);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"InformationPage"; }
    private:
        void addUpdateRows(); // the two rows the update check answers in
        // The status, the button's caption and the last check's age, as the check stands now.
        void stateUpdateRows();
    private:
        // Null where the check is not available, as are the three below.
        Controls::TextBox* m_status{ nullptr };
        Controls::Button* m_checkButton{ nullptr };
        Controls::CheckBox* m_checkAutomaticallyBox{ nullptr };
        Controls::TextBox* m_lastCheckLine{ nullptr };
        // Dropped with the page - the check outlives it, on the application's context.
        ScopedEventConnection m_updateConnection{};
    };
}
