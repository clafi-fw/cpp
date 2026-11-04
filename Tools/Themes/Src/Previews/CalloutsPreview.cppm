export module Themes_App.CalloutsPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.Callout;

import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // A callout of each kind, one under another, each holding one line.
    export class CalloutsPreview : public PreviewPageBase
    {
    public:
        using PreviewPageBase::PreviewPageBase;
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        Callout& addCallout(MessageIcon kind, std::wstring_view line);
    private:
        static constexpr std::wstring_view k_whatItShows{
            L"Info, question, warning, error and ok callouts"
        };

        Callout& m_info{ addCallout(
            MessageIcon::Information,
            L"A pike hunts by sight, so clear water suits it."
        ) };

        Callout& m_question{ addCallout(
            MessageIcon::Question,
            L"Does a carp sleep with its eyes open?"
        ) };

        Callout& m_warning{ addCallout(
            MessageIcon::Warning,
            L"Ice forms first along the shallow bank."
        ) };

        Callout& m_error{ addCallout(
            MessageIcon::Error,
            L"The net has a hole near the rim."
        ) };

        Callout& m_ok{ addCallout(
            MessageIcon::Ok,
            L"All six fish counted at dusk."
        ) };
    };
}
