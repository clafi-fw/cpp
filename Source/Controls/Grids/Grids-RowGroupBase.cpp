module ClaFi.Controls.Grids;

import :RowBase;
import :SubGrid;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    RowGroupBase::~RowGroupBase()
    {
        releaseChildren();
    }

    SubGrid& RowGroupBase::body()
    {
        return static_cast<SubGrid&>(*m_controls[k_body]);
    }

    void RowGroupBase::preCalcColumn(Column& column)
    {
        for (RowBase& row : body().controlsAs<RowBase>())
            row.preCalcColumn(column);
    }

    GridBase* RowGroupBase::subGrid()
    {
        return &body();
    }

    ScaledDimensions RowGroupBase::calculateContent(AlignEvent& event)
    {
        return Control::calculateColumns(event, controls(), false);
    }

    void RowGroupBase::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& contentDimensions)
    {
        ScaledDimensions calculatedContent = dimensions() - event.padding * 2;
        calculatedContent.x = contentDimensions.x;// std::max(calculatedContent.x, contentDimensions.x);
        contentDimensions = { 0, 0 };
        return Control::alignSingleColumn(event, controls().begin(), controls().end(),
            position, calculatedContent, contentDimensions);
    }

    void RowGroupBase::createSubGrid(GridDescriptor& descriptor)
    {
        CreateParams childParams{ *this };
        m_controls[k_body] = std::make_unique<SubGrid>(childParams, descriptor);
    }
}
