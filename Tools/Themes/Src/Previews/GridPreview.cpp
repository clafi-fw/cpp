module Themes_App.GridPreview;

import ClaFi.Controls.Grids;
import ClaFi.Controls.Grids_Dt;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App
{
    Row GridPreview::fishRow(const std::size_t fishIndex) const
    {
        return Row{
            Tag{ fishIndex },
            Cell{ Tag{ FishColumn::Name }, this, &GridPreview::writeFishCell },
            Cell{ Tag{ FishColumn::Water }, this, &GridPreview::writeFishCell },
            Cell{ Tag{ FishColumn::Length }, this, &GridPreview::writeFishCell }
        };
    }

    void GridPreview::writeFishCell(Grids::GetCellTextEvent& event) const
    {
        const Fish& fish = k_fish[event.row().tag().get<std::size_t>()];
        switch (event.column().tag().get<FishColumn>())
        {
            case FishColumn::Name:
                event.text() << fish.name;
                break;
            case FishColumn::Water:
                event.text() << fish.water;
                break;
            case FishColumn::Length:
                event.text() << fish.lengthCm << L" cm";
                break;
        }
    }
}
