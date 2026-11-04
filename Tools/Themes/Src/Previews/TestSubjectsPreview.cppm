export module Themes_App.TestSubjectsPreview;

import Themes_App.Consts;
import Themes_App.PreviewPageBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;

    // The two test subjects, each a bare control wearing its element.
    export class TestSubjectsPreview : public PreviewPageBase
    {
    public:
        using PreviewPageBase::PreviewPageBase;
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        static constexpr std::wstring_view k_whatItShows{ L"Testee and Bestee" };

        // A button's metrics give a stroke something to be drawn at; the focus gives the states.
        RichControl& m_testee{ add<RichControl>(
            MetricsSet::Button,
            NavigationReach::Yes,
            MouseFocus::Self,
            ClickTarget::Self,
            k_testee,
            Text{ k_testeeName }
        ) };

        RichControl& m_bestee{ add<RichControl>(
            MetricsSet::Button,
            NavigationReach::Yes,
            MouseFocus::Self,
            ClickTarget::Self,
            k_bestee,
            Text{ k_besteeName }
        ) };
    };
}
