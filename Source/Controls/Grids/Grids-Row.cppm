export module ClaFi.Controls.Grids :Row;

import :RowBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;


namespace ClaFi::Controls::Grids
{

    // One row of a grid, its cells cut by the grid's columns.
    export class Row : public RowBase
    {
    public:
        using RowBase::RowBase;
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"Row"; }
    protected:
        NavigationReach navigationReach() const override { return NavigationReach::Yes; }
        MouseFocus mouseFocus() const override { return MouseFocus::Self; }
        ClickTarget clickTarget() const override { return ClickTarget::Self; }
        void paintSurface(PaintEvent&) override;
    };

}
