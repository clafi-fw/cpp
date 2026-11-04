export module Themes_App.SelectionPreview;

import Themes_App.PreviewPageBase;
import Themes_App.WaterChoice;

import ClaFi.Controls.Button;
import ClaFi.Controls.CheckBox;
import ClaFi.Controls.SearchBox;
import ClaFi.Controls.Stack;
import ClaFi.Controls.StackView;
import ClaFi.Controls.TextBox;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Searcher;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;

    // The marks of checks, radios and tiles, and a passage with a selection and found words.
    export class SelectionPreview : public PreviewPageBase
    {
    public:
        explicit SelectionPreview(const CreateParams&);
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        // A tile of the view, its mark showing while the pointer is over it or it is selected.
        static Button& addTile(StackView&, std::wstring_view name);
        void searchRequested(SearchRequest);
    private:
        static constexpr std::wstring_view k_whatItShows{
            L"Selection and hover indicators, search result highlighting"
        };
        static constexpr float k_tileWidth = 64.0f;

        StackPanel& m_marks{ add<StackPanel>(
            Orientation::Horizontal,
            Spacing{ 16.0f }
        ) };

        StackPanel& m_checks{ m_marks.add<StackPanel>(
            Orientation::Vertical
        ) };

        CheckBox2& m_pumpCheck{ m_checks.add<CheckBox2>(L"Pump on", Checked::Yes) };
        CheckBox2& m_heaterCheck{ m_checks.add<CheckBox2>(L"Heater on") };
        WaterChoice& m_waterChoice{ m_marks.add<WaterChoice>() };

        StackView& m_tiles{ add<StackView>(
            Orientation::Horizontal,
            SelectionMode::Multi,
            Spacing{ 4.0f }
        ) };

        Button& m_pikeTile{ addTile(m_tiles, L"Pike") };
        Button& m_perchTile{ addTile(m_tiles, L"Perch") };
        Button& m_tenchTile{ addTile(m_tiles, L"Tench") };

        SearchBox& m_search{ add<SearchBox>(
            HorizontalAlign::Fill
        ) };

        // Only a container keeps a search result; this one holds the passage and nothing else.
        StackPanel& m_searchScope{ add<StackPanel>(
            Orientation::Vertical
        ) };

        TextBox& m_passage{ m_searchScope.add<TextBox>(
            Text{ L"The pike waits in the reeds by the north bank, and a second pike circles the "
                L"flat stone." },
            ReadOnly::Yes
        ) };

        Searcher m_searcher{}; // what the search box holds, found in the passage
        SearchResult m_found{ m_searcher, m_searchScope };
    };
}
