module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Grids :RowGroup;

import :Columns;
import :RowContainer;
import :RowBase;
import :RowGroupBase;
import :Descriptor;

import ClaFi.Controls.Base.ExpanderBase;
import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    // Whether a group's rows fold away under its span, and how the group starts. See Grids
    export enum class Collapsible
    {
        No,
        Expanded,
        Collapsed
    };

    // The group's own row, whose cells span the rows beside them. See Grids
    export class RowGroupSpan : public RowContainer
    {
    public:
        template<typename... Args>
        RowGroupSpan(const CreateParams&, GridDescriptor&, Collapsible, Args&&...);
        // Whether the group's rows are showing, as they always are in a group that cannot fold.
        [[nodiscard]] bool expanded() const { return m_expanded; }
        void setExpanded(bool value);
        void toggleExpanded() { setExpanded(!expanded()); }
        [[nodiscard]] float expandedFactor() const { return m_expandedFactor; }
    protected:
        // Its cells and its blanks take the press; elsewhere the rows showing through it do.
        void hitTest(HitTestEvent& event) const override;
        [[nodiscard]] bool hasBlank(const Column&) const override;
        [[nodiscard]] ScaledDimensions cellLead(const Column&) const override;
        void adjustChildMetrics(AdjustMetricsEvent&) const override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        // On the mark's cell Right opens the group and Left closes it, as on a tree node.
        void keyDown(KeyDownEvent&) override;
        // A double click on the mark is two presses of it, and one on a blank opens the group.
        void doubleClick(DoubleClickEvent&) override;
    private:
        void createMark();
        // The cell the mark leads: the first column this span fills, left to right.
        [[nodiscard]] const Column* markColumn() const;
        [[nodiscard]] const Column* firstFilledColumn(const ColumnCollection&) const;
    private:
        bool m_expanded;
        float m_expandedFactor;
        ExpanderButton* m_mark{}; // null in a group that cannot fold
    };

    // A group of rows beside a span row of its own.
    export class RowGroup : public RowGroupBase
    {
        friend RowGroupSpan;
    public:
        template<typename ...Args>
        RowGroup(const CreateParams& params, GridDescriptor& descriptor, Args&&... args);
        [[nodiscard]] RowGroupSpan& span() { return static_cast<RowGroupSpan&>(*m_controls[k_head]); }
        [[nodiscard]] const RowGroupSpan& span() const { return static_cast<RowGroupSpan&>(*m_controls[k_head]); }
    protected:
        // A group's span carries cells of its own and stands beside the rows it spans, so it
        // is part of the same field they are. An expander's header is a panel, not a row, which
        // is why this answers for the group only.
        RowBase* spanRow() override { return &span(); }
        void preCalcColumn(Column& column) override;
        ScaledDimensions calculateContent(AlignEvent& event) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
    private:
        // Shows and hides the body as the span opens and closes.
        void connectFolding();
        // Whether the span of a group this one stands in fills the column, stretched over it.
        [[nodiscard]] bool outerSpanFills(const Column&) const;
    };

    template<typename... Args>
    RowGroupSpan::RowGroupSpan(const CreateParams& params, GridDescriptor& descriptor,
        Collapsible collapsible, Args&&... args)
        :
        RowContainer{ params, descriptor, std::forward<Args>(args)... },
        m_expanded{ collapsible != Collapsible::Collapsed },
        m_expandedFactor{ m_expanded ? 1.0f : 0.0f }
    {
        if (collapsible != Collapsible::No)
            createMark();
    }

    template<typename ...Args>
    RowGroup::RowGroup(const CreateParams& params, GridDescriptor& descriptor, Args && ...args)
        :
        RowGroupBase{ params, descriptor, std::forward<Args>(args)... }
    {
        CreateParams childParams{ *this };

        Tag spanTag = 0;
        Props::ifThereIs<SpanTag>([&](const auto& tagWrapper) {
            spanTag = tagWrapper.value;
            }, std::forward<Args>(args)...);

        // Whether the rows fold away under the span, and how the group starts. See Grids
        const Collapsible collapsible = READ_PROPERTY(Collapsible, Collapsible::No);
        m_controls[k_head] = std::make_unique<RowGroupSpan>(childParams, descriptor,
            collapsible,
            spanTag
            );
        if (collapsible != Collapsible::No)
            connectFolding();
    }

}
