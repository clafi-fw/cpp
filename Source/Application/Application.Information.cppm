export module ClaFi.App.Information;

import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;

import ClaFi.StdLib;

namespace ClaFi
{
    // The backstage page naming the application, what it does and its framework. See Application
    export class InformationPage : public Controls::TextBox
    {
    public:
        explicit InformationPage(const CreateParams&);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"InformationPage"; }
    };
}
