export module ClaFi.App.Information;

import ClaFi.Controls.Button;
import ClaFi.Controls.StackPanel;
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
        // The button stands while there is something to ask. See Application
        void stateCheckButton();
        // The update check has moved: the text and the button state what it found.
        void showUpdateState();
    private:
        Controls::TextBox& m_text;
        Controls::Button& m_checkButton;
        // Dropped with the page - the check outlives it, on the application's context.
        ScopedEventConnection m_updateConnection;
    };
}
