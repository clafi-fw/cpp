module Themes_App.SelectionPreview;

import Themes_App.FishIcon;
import Themes_App.PreviewPageBase;

import ClaFi.Controls.Button;
import ClaFi.Controls.SearchBox;
import ClaFi.Controls.StackView;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Searcher;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    SelectionPreview::SelectionPreview(const CreateParams& params)
        :
        PreviewPageBase{ params }
    {
        const std::array<Control*, 1> selectedTiles{ &m_pikeTile };
        m_tiles.setSelection(selectedTiles);
        m_search.onSearch([this](SearchEvent& event) {
            searchRequested(event.request);
        });
        m_search.setSearchText(L"pike");
    }

    // A Button dressed as a document tile is: the view picks the item a press focuses, and a
    // ToolButton's press focuses nothing.
    Button& SelectionPreview::addTile(StackView& tiles, const std::wstring_view name)
    {
        return tiles.add<Button>(
            Text{ name },
            MetricsSet::ToolButton,
            UiElement::ToolButton,
            ShowSelectionOnSurface::Yes,
            IndicatorVisibility::Hover,
            IndicatorPlacement::TopLeftIn,
            ButtonViewMode::TopCenterIcon,
            IconSize{ 28.0f },
            HorizontalTextAnchor::Center,
            TextFormat{ TextAlign::Center },
            MinSize{ k_tileWidth, 0.0f },
            Control::OnDrawIcon{ FishIcon::draw }
        );
    }

    // The searcher knows nothing of the passage, so the bands are repainted from here.
    void SelectionPreview::searchRequested(const SearchRequest request)
    {
        switch (request)
        {
            case SearchRequest::Search:
                if (m_searcher.setText(m_search.searchText(), SearchSplit::Phrase))
                    m_passage.invalidate();
                m_found.step(FindTarget::AtSelection);
                break;
            case SearchRequest::Next:
                m_found.step(FindTarget::Next);
                break;
            case SearchRequest::Previous:
                m_found.step(FindTarget::Previous);
                break;
            case SearchRequest::End:
                m_passage.setFocus();
                break;
            // The box shows no list, so nothing is picked from one.
            case SearchRequest::Pick:
                break;
        }
        m_search.setResult(m_found.currentIndex(), m_found.count());
    }
}
