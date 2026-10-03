export module ClaFi.Controls.Grids :RowContainer;

import :RowBase;
import :Columns;
import :Cell;
import :Row;

import ClaFi.Core.Foundation;
import ClaFi.Core.Context.FormContext;
import ClaFi.StdLib;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Controls::Grids
{
    using ControlIndex = std::size_t;
    using ControlMap = std::unordered_map<const Column*, ControlIndex>;

    // A row whose cells can hold controls. See Grids
    export class RowContainer : public Row
    {
    public:
        using Row::Row;
    public:
        ~RowContainer() override;
    public:
        template <IsControl ControlClass, typename... Args>
        ControlClass& addControl(Column&, Args&&... args);
        const Control* controlAtColumn(const Column&) const;
        Control* controlAtColumn(const Column&);
        template <IsControl ControlClass>
        ControlClass& controlAtColumnAs(const Column& column)
        {
            return *static_cast<ControlClass*>(controlAtColumn(column));
        }
    protected:
        using Row::controls; // const version
        // Adds a control the row holds in no column and places itself - a group's mark.
        template <IsControl ControlClass, typename... Args>
        ControlClass& addOwnControl(Args&&... args);
        ControlSpan controls() override { return { m_controls }; }
        ScaledDimensions calculateCellContent(ScaledCellMetrics&, const Column&, float boundW) override;
        RowContainer* asRowContainer() override { return this; }
        Control* cellControl(const Column& column) override { return controlAtColumn(column); }
        // A hosted control and a group's mark are the row's: a press on one lands on the grid.
        bool isChildPart(const Control&) const override { return true; }
        // A column fitted to its content reads a control's width off it. See Grids
        [[nodiscard]] bool isChildWidthGiven(const Control&) const override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        void nestedMouseMove(const MouseMoveEvent&) override;
        void nestedControlFocusing(FocusEvent&) override;
        // Refuses a popup on a press that picks a cell or changes the selection.
        void nestedControlDroppingPopup(DropPopupEvent&) override;
        // A click on a cell's control reaches the grid as a click on this row, as the focus does.
        void nestedClick(ClickEvent&) override;
        // The row holds the focus for its cells' controls and hands them the keys the grid leaves.
        void nestedKeyDown(KeyDownEvent&) override;
        void charPress(CharPressEvent&) override;
        // A menu asked from the keyboard is the selected cell's control's, as under the pointer.
        void nestedContextPopup(ContextPopupEvent&) override;
    private:
        // The control in the selected cell, while it stands in this row and takes input.
        [[nodiscard]] Control* selectedCellControl();
        // Whether the press refuses the popup the event asks about. See Grids#picking
        [[nodiscard]] bool refusesPopup(const DropPopupEvent&) const;
    private:
        ControlMap m_controlMap{};
        ControlCollection m_controls{};
    };


    //-------------------------------------------------------------------------


    template<IsControl ControlClass, typename ...Args>
    ControlClass& RowContainer::addControl(Column& column, Args && ...args)
    {
        // One control per column. A silent second insert would be dropped by the map and
        // the control would never be aligned - owned, parented and invisible.
        if (!m_controlMap.insert({ &column, m_controls.size() }).second)
            unreachable("Grid: this column already holds a control in this row");
        CreateParams params{ *this };
        m_controls.emplace_back(std::make_unique<ControlClass>(
            params,
            std::forward<Args>(args)...,
            Interactivity::MouseOnly
            )
        );
        return static_cast<ControlClass&>(*m_controls.back());
    }

    template<IsControl ControlClass, typename ...Args>
    ControlClass& RowContainer::addOwnControl(Args&&... args)
    {
        CreateParams params{ *this };
        m_controls.emplace_back(std::make_unique<ControlClass>(
            params,
            std::forward<Args>(args)...
            )
        );
        return static_cast<ControlClass&>(*m_controls.back());
    }


    //-------------------------------------------------------------------------


    RowContainer::~RowContainer()
    {
        releaseChildren();
    }

    const Control* RowContainer::controlAtColumn(const Column& column) const
    {
        ControlMap::const_iterator it = m_controlMap.find(&column);
        if (it == m_controlMap.end())
            return nullptr;
        return &*m_controls[it->second];
    }

    Control* RowContainer::controlAtColumn(const Column& column)
    {
        const RowContainer& self = *this;
        return const_cast<Control*>(self.controlAtColumn(column));
    }

    bool RowContainer::isChildWidthGiven(const Control& child) const
    {
        for (const ControlMap::value_type& entry : m_controlMap)
            if (&*m_controls[entry.second] == &child)
                return entry.first->calcMode() != ColumnWidthMode::FitContent;
        return false;
    }

    ScaledDimensions RowContainer::calculateCellContent(ScaledCellMetrics& cellMetrics, const Column& column, float boundW)
    {
        if (Control* control = controlAtColumn(column))
            return control->dimensions() - cellMetrics.padding * 2.0f;
        return Row::calculateCellContent(cellMetrics, column, boundW);
    }

    void RowContainer::alignContent(AlignEvent& event, ScaledPosition position, ScaledDimensions& dimensions)
    {
        Row::alignContent(event, position, dimensions);

        // Ask each column where it is instead of accumulating leaf widths along the
        // way. The accumulation only lands in the right place when every parent is
        // exactly tiled by its children, and it is not: a column with no content in
        // any row has zero width, and a parent whose own text is wider than its
        // children's total keeps the surplus. Either one shifts every control after
        // it to the left. calcLeft() has already computed the answer.
        //
        // A control is given the box the cell's lines leave - the same one the cell's text is
        // drawn into, which is what puts a control's own frame on the cell's. The row's lead for
        // the cell comes off the start of that box. calculateCell measures the cell as the
        // control's dimensions plus the border and the lead, so this hands the control back
        // exactly the size it asked for.
        const float border = descriptor().scaledCellMetrics().border;
        using TraverseFunc = std::function<void (ColumnCollection& columns)>;
        TraverseFunc traverseColumns;
        traverseColumns = [&](ColumnCollection& columns) {
            for (Column* column : columns)
            {
                if (column->hasSubColumns())
                    traverseColumns(*column->subColumns());
                else if (Control* control = controlAtColumn(*column))
                {
                    const float lead = cellLead(*column).x;
                    const FloatRect cellRect = FloatRect::fromDimensions(
                        { position.x + column->left(), position.y },
                        { column->calculatedWidth(), dimensions.y }
                    );
                    const FloatRect innerRect = descriptor().cellInnerRect(cellRect);
                    ScaledDimensions controlDimensions = {
                        innerRect.width() - lead,
                        innerRect.height(),
                    };
                    ScaledPosition controlPosition = {
                        innerRect.left + lead,
                        innerRect.top,
                    };
                    alignControl(control, event, controlPosition, controlDimensions);
                    dimensions.y = std::max(controlDimensions.y + border, dimensions.y);
                }
            };
            };
        traverseColumns(descriptor().columns());
    }

    void RowContainer::nestedMouseMove(const MouseMoveEvent& event)
    {
        Column* columnToHover = columnAt(event.posOnForm);
        setHoveredColumn(columnToHover);
    }

    void RowContainer::nestedControlFocusing(FocusEvent& event)
    {
        event.control = this;
        Row::nestedControlFocusing(event);
    }

}
