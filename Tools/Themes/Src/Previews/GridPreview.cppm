export module Themes_App.GridPreview;

import Themes_App.PreviewPageBase;

import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Controls::Grids::Dt;

    // A grid of fish: its header, a group open with three rows, and a group shut.
    export class GridPreview : public PreviewPageBase
    {
    public:
        using PreviewPageBase::PreviewPageBase;
    public:
        std::wstring_view whatItShows() const override { return k_whatItShows; }
    private:
        // The grid's columns, as their tags name them.
        enum class FishColumn : TagValue
        {
            Name,
            Water,
            Length
        };
        // What a row of the grid shows.
        struct Fish
        {
            std::wstring_view name{};
            std::wstring_view water{};
            int lengthCm{};
        };
    private:
        // The row of the fish at that place in k_fish.
        [[nodiscard]] Row fishRow(std::size_t fishIndex) const;
        void writeFishCell(Grids::GetCellTextEvent&) const;
    private:
        static constexpr std::wstring_view k_whatItShows{
            L"Grid, header and row surfaces, cell lines"
        };
        static constexpr auto k_fish = std::to_array<Fish>({
            { L"Pike", L"Lake", 64 },
            { L"Perch", L"River", 25 },
            { L"Tench", L"Pond", 30 },
            { L"Cod", L"Sea", 80 },
            { L"Plaice", L"Shore", 40 }
        });

        Grid& m_grid{ add<Grid>(
            UiElement::Grid,
            HorizontalAlign::Fill,
            Columns{
                Column{ Tag{ FishColumn::Name }, Text{ L"Fish" }, ColumnWidthMode::Fill },
                Column{ Tag{ FishColumn::Water }, Text{ L"Found in" } },
                Column{ Tag{ FishColumn::Length }, Text{ L"Length" }, TextAlign::Right }
            },
            Header{},
            Rows{
                Expander{
                    Header{ Text{ L"Fresh water" } },
                    fishRow(0),
                    fishRow(1),
                    fishRow(2)
                },
                Expander{
                    Header{ Text{ L"Salt water" } },
                    Init<Grids::RowExpander>{ [](Grids::RowExpander& expander) {
                        expander.header().setExpanded(false);
                    } },
                    fishRow(3),
                    fishRow(4)
                }
            }
        ) };
    };
}
