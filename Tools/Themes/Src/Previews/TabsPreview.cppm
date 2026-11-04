export module Themes_App.TabsPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.Label;
import ClaFi.Controls.TabbedBox;
import ClaFi.Controls.TabStrip;

import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // Tabs across the top of a box and down the left of another, each over a page of one line.
    export class TabsPreview : public PreviewPageBase
    {
    public:
        using PreviewPageBase::PreviewPageBase;
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        static constexpr std::wstring_view k_whatItShows{
            L"Tabs and their lines, along the top and the left"
        };
        static constexpr Padding k_pagePadding{ 12.0f, 10.0f };

        TabbedBox& m_waters{ add<TabbedBox>(
            TabsOrientation::HorizontalTop
        ) };

        Label& m_pondPage{ m_waters.pageControl().add<Label>(
            L"Carp and tench under the lilies.",
            k_pagePadding
        ) };

        Label& m_reefPage{ m_waters.pageControl().add<Label>(
            L"Clownfish among the anemones.",
            k_pagePadding
        ) };

        Label& m_riverPage{ m_waters.pageControl().add<Label>(
            L"Trout in the riffles below the weir.",
            k_pagePadding
        ) };

        Tab& m_pondTab{ m_waters.strip().addTab(L"Pond", Page{ m_pondPage }).select() };
        Tab& m_reefTab{ m_waters.strip().addTab(L"Reef", Page{ m_reefPage }) };
        Tab& m_riverTab{ m_waters.strip().addTab(L"River", Page{ m_riverPage }) };

        TabbedBox& m_hours{ add<TabbedBox>(
            TabsOrientation::VerticalLeft
        ) };

        Label& m_dawnPage{ m_hours.pageControl().add<Label>(
            L"The pike hunts the margins.",
            k_pagePadding
        ) };

        Label& m_noonPage{ m_hours.pageControl().add<Label>(
            L"The carp bask near the surface.",
            k_pagePadding
        ) };

        Label& m_duskPage{ m_hours.pageControl().add<Label>(
            L"The eels leave the mud.",
            k_pagePadding
        ) };

        Tab& m_dawnTab{ m_hours.strip().addTab(L"Dawn", Page{ m_dawnPage }) };
        Tab& m_noonTab{ m_hours.strip().addTab(L"Noon", Page{ m_noonPage }).select() };
        Tab& m_duskTab{ m_hours.strip().addTab(L"Dusk", Page{ m_duskPage }) };
    };
}
