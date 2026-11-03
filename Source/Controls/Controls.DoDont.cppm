module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.DoDont;

import ClaFi.Controls.Stack;

import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // How a Do and Don't block lays its two lists out. See Controls#dodontshape
    export enum class DoDontShape
    {
        Columns, // Do beside Don't, each side under its heading
        Stacked, // Do over Don't, each side under its heading
        Marks    // one list, each item led by its side's mark
    };

    // Which list an item of a Do and Don't block stands in.
    export enum class DoDontSide
    {
        Do,
        Dont
    };

    // What to do and what not to do, as two lists. See Controls#dodont
    export class DoDont : public StackPanelBase
    {
    public:
        template<typename... Args>
        explicit DoDont(const CreateParams&, Args&&...);
    public:
        DECLARE_PROPERTY(DoDontShape, shape, DoDontShape::Columns) // how the two lists are laid out
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"DoDont"; }
        void addItem(DoDontSide, const Text&); // adds an item at the end of the side's list
    protected:
        // The sides stand apart by the spacing the theme's set states. See Controls#dodont
        void adjustMetrics(AdjustMetricsEvent&) const override;
        void adjustChildMetrics(AdjustMetricsEvent&) const override;
        // Columns hand each side its share of the width. See Controls#dodont
        [[nodiscard]] bool isChildWidthGiven(const Control&) const override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledSize&) override;
    private:
        [[nodiscard]] static Orientation orientationOf(DoDontShape);
        [[nodiscard]] static ItemSizing itemSizingOf(DoDontShape);
        // The side's list, under its heading where the shape gives it one.
        StackPanel& addSide(DoDontSide);
        [[nodiscard]] StackPanel& sideOf(DoDontSide);
    private:
        StackPanel& m_doSide;
        StackPanel& m_dontSide;
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    DoDont::DoDont(const CreateParams& params, Args&&... args)
        :
        StackPanelBase{
            params,
            // The shape, read ahead of its property, picks which way the sides stand.
            orientationOf(READ_PROPERTY(DoDontShape, DoDontShape::Columns)),
            // And whether they share the width evenly.
            itemSizingOf(READ_PROPERTY(DoDontShape, DoDontShape::Columns)),
            std::forward<Args>(args)...
        },
        INIT_PROPERTY(shape),
        m_doSide{ addSide(DoDontSide::Do) },
        m_dontSide{ addSide(DoDontSide::Dont) }
    {
    }
}
