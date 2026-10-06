export module ClaFi.App.Information;

import ClaFi.Controls.Button;
import ClaFi.Controls.Stack;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Events;

import ClaFi.StdLib;

namespace ClaFi
{
    // The backstage page naming the application, what it does and its framework. See Application
    export class InformationPage : public Controls::Stack
    {
    public:
        explicit InformationPage(const CreateParams&);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"InformationPage"; }
    private:
        // The row the update check answers in, and the line under it. See Application
        void addUpdateRow();
        // The status, the line under it and the button's caption, as the check stands now.
        void stateUpdateRow();
    private:
        // Null where the check is not available, as the button is.
        Controls::TextBox* m_status{ nullptr };
        Controls::Button* m_checkButton{ nullptr };
        // Dropped with the page - the check outlives it, on the application's context.
        ScopedEventConnection m_updateConnection{};
    };
}
