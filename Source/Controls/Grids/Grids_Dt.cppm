export module ClaFi.Controls.Grids_Dt;

import ClaFi.Controls.Grids;
import ClaFi.Core.System.Events;

import ClaFi.Core.Dt;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

export namespace ClaFi::Controls::Grids::Rt
{
    using Column = Column;
    using ColumnCollection = ColumnCollection;
    using GridDescriptor = GridDescriptor;
    using GridBase = GridBase;
    using Grid = Grid;
    using RowBase = RowBase;
    using Row = Row;
    using RowContainer = RowContainer;
    using RowExpander = RowExpander;
    using RowGroup = RowGroup;
}


namespace ClaFi::Controls::Grids::Dt
{
    export using Grids::ShowInHeader;
    export using Grids::CellHighlightMode;
    export using Grids::ColumnWidthMode;
    export using Grids::MovingText;
    export using Grids::Collapsible;

    // The shared declarative-tree machinery. It needs an alias: `Dt` on its own
    // resolves to this namespace, not to ClaFi::Dt.
    namespace Tree = ::ClaFi::Dt;

    // Re-exported so call sites keep writing Grids::Dt::OnEvent and
    // Grids::Dt::Init<T>. The event deduction traits behind them live in
    // ClaFi::Dt as EventParamOf, event_param_t and NamesAnEvent.
    export using Tree::OnEvent;
    export using Tree::makeOnEvent;
    export using Tree::Init;
    export using Tree::Header;

    export using TagCollection = std::vector<TagValue>;

    // Build-time bookkeeping for the two debug checks, and nothing else.
    //
    // It lives in the applying constructor's frame rather than in the grid, so a
    // finished grid carries no trace of having been built from a design.
    struct ApplyState
    {
        TagCollection columnTags;
        TagCollection unresolvedTags;

        void noteColumnTag(Tag);
        void noteUnresolved(Tag);
        void checkColumnTags() const;
        void checkResolvedCells() const;
    };

    // A column is named by its Tag. The design requires those Tags to be unique
    // among the columns of one grid, and checks it in debug builds. A column
    // declared without a Tag is legal, it simply cannot be named by a cell.
    //
    // The Tag is an ordinary property: it is forwarded to ColumnCollection::add
    // like any other, so the live column carries it and Rt::GridBase::columnByTag
    // can answer for it long after the design has been applied.

    // =====================================================================
    // Cells
    // =====================================================================

     export using ColumnList = std::vector<const Rt::Column*>;

    // One resolved cell of one row.
    //
    // A cell's text provider is an ordinary GetCellTextEvent handler - the same
    // signature a row level and a grid level handler take, so one method serves at
    // any of the three levels without being rewritten. The event already carries
    // row(), column() and text(), so there is no argument type of the DSL's own, and
    // a provider can call stopPropagation() to suppress the handlers behind it.
    struct CellSpec
    {
        const Rt::Column* column{};
        CellTextFunc text{};
    };

    // Accumulates a row's resolved cells while the design is being applied. Purely
    // a build-time structure: Row::apply moves it into the handlers it connects to
    // the row, and nothing keeps it afterwards.
    class CellTable
    {
    public:
        void addCell(const Rt::Column& column, CellTextFunc text);
        [[nodiscard]] const CellSpec* findCell(const Rt::Column& column) const;
        [[nodiscard]] bool hasCell(const Rt::Column& column) const;
        [[nodiscard]] std::size_t cellCount() const { return m_cells.size(); }
        [[nodiscard]] ColumnList columnList() const;
        [[nodiscard]] bool hasAnyText() const;
    private:
        using CellSpecCollection = std::vector<CellSpec>;
    private:
        // Rows hold a handful of cells, so a linear scan beats hashing here.
        CellSpecCollection m_cells;
    };

    // Connects a row's cell answers to the row itself.
    //
    // The row owns the connections, so the design's state dies with the grid and
    // nothing has to outlive the apply.
    //
    // Content is answered authoritatively and stops propagation, matching what the
    // design knows: these are exactly the columns this row fills. A row that
    // declared no cells gets no handler at all, so it keeps falling through to
    // whatever grid level handler answered for it before.
    //
    // Text is additive and never stops propagation. GetCellTextEvent carries a Text
    // stream that RowBase::doGetCellText clears once per query, so every level may
    // contribute or adjust. A handler that wants to replace what an earlier one
    // wrote calls event.text().clear() itself.
    void connectCells(Rt::RowBase& row, CellTable&& spec);

    // =====================================================================
    // Build contexts
    // =====================================================================

    // Exported, because they are the parameter of the node hierarchies below. An
    // application that writes a node of its own needs to name them.

    export struct ColumnBuildContext
    {
        ApplyState& state;
        Rt::ColumnCollection& target;
    };

    struct RowBuildContext
    {
        ApplyState& state;
        GridBase& target;
    };

    // `grid` resolves a cell's Tag. Any GridBase in the tree answers the same, because
    // sub-grids share the root descriptor and therefore the root column collection, so
    // a row inside an Expander or a Group needs no special handling.
    // `row` is where the cell lands. `container` is the same object when it can host
    // controls, and null when it cannot - a row that is not a RowContainer takes text cells
    // only.
    struct CellBuildContext
    {
        ApplyState& state;
        GridBase& grid;
        CellTable& spec;
        Rt::RowBase& row;
        Rt::RowContainer* container;
    };

    // =====================================================================
    // Nodes
    // =====================================================================

    // Three hierarchies, one per build context. The context type keeps a RowNode
    // from binding where a CellNode is expected, and it is what the pack routing
    // in ClaFi::Dt discriminates on.
    export using ColumnNode = Tree::NodeBase<ColumnBuildContext>;
    export using RowNode = Tree::NodeBase<RowBuildContext>;
    export using CellNode = Tree::NodeBase<CellBuildContext>;

    export using ColumnNodePtr = Tree::NodePtr<ColumnBuildContext>;
    export using RowNodePtr = Tree::NodePtr<RowBuildContext>;
    export using CellNodePtr = Tree::NodePtr<CellBuildContext>;

    export using ColumnNodeList = Tree::NodeList<ColumnBuildContext>;
    export using RowNodeList = Tree::NodeList<RowBuildContext>;
    export using CellNodeList = Tree::NodeList<CellBuildContext>;

    // =====================================================================
    // Columns
    // =====================================================================

    // One column of the described grid, and any sub-columns nested in it. See Grids
    export struct Column : public ColumnNode
    {
        using ColumnFactory = std::function<Rt::Column& (Rt::ColumnCollection&)>;

        ColumnFactory create;
        ColumnNodeList children;
        Tag tag{};
        // Not `tag.value != 0`: zero is the first enumerator of any plain enum, so
        // it is an ordinary tag and not a marker for absent.
        bool hasTag{};

        template <typename... Args>
            requires Tree::NotSelfCopy<Column, Args...>
        explicit Column(Args&&... args);

        Column(Column&&) noexcept = default;
        Column& operator=(Column&&) noexcept = default;
        Column(const Column&) = delete;
        Column& operator=(const Column&) = delete;
        // A live column is not a design node. Naming one here is a mistake worth
        // its own message rather than a property forwarded into add().
        explicit Column(const Rt::Column&) = delete;
        Column& operator=(const Rt::Column&) = delete;

        void apply(ColumnBuildContext&) const override;
    };

    // An anonymous group whose children are spliced into the enclosing collection. See Grids
    export struct Columns : public ColumnNode
    {
        ColumnNodeList children;

        template <typename... Args>
            requires Tree::NotSelfCopy<Columns, Args...>
        explicit Columns(Args&&... args);

        Columns(Columns&&) noexcept = default;
        Columns& operator=(Columns&&) noexcept = default;
        Columns(const Columns&) = delete;
        Columns& operator=(const Columns&) = delete;

        void apply(ColumnBuildContext&) const override;
    };

    // =====================================================================
    // Cell nodes
    // =====================================================================

    // One cell of a described row: the column it fills, and where its text comes from. See Grids
    export struct Cell : public CellNode
    {
        Tag tag{};
        CellTextFunc text{};

        explicit Cell(Tag);

        template <typename F>
            requires std::is_invocable_v<F&, GetCellTextEvent&>
        Cell(Tag, F&& textProvider);

        template <typename TObject, typename Method>
            requires std::is_member_function_pointer_v<Method>
        Cell(Tag, TObject* object, Method method);

        void apply(CellBuildContext&) const override;
    };

    // CellWith<Combobox>{ tag, ...ctor args..., OnEvent{ ... }, Init<Combobox>{ ... } }
    //
    // Adds a live control to the cell through RowContainer::addControl. Arguments
    // are copied into the node, so wrap anything expensive or shared in std::ref.
    export template <typename ControlClass>
        struct CellWith : public CellNode
    {
        using ControlFactory = std::function<void(Rt::RowContainer&, Rt::Column&)>;

        Tag tag{};
        ControlFactory create;

        template <typename... Args>
        explicit CellWith(Tag, Args&&... args);

        void apply(CellBuildContext&) const override;
    };

    // A reusable bundle of cells, referenced from every row that shares it. See Grids
    export struct CellSet : public CellNode
    {
        CellNodeList children;

        template <typename... Args>
            requires Tree::NotSelfCopy<CellSet, Args...>
        explicit CellSet(Args&&... args);

        CellSet(CellSet&&) noexcept = default;
        CellSet& operator=(CellSet&&) noexcept = default;
        CellSet(const CellSet&) = delete;
        CellSet& operator=(const CellSet&) = delete;

        void apply(CellBuildContext&) const override;
    };

    // =====================================================================
    // Rows
    // =====================================================================

    // One row of the described grid, an ordinary RowContainer with its cells. See Grids
    export struct Row : public RowNode
    {
        using RowFactory = std::function<Rt::RowContainer& (GridBase&)>;

        RowFactory create;
        CellNodeList cells;

        template <typename... Args>
            requires Tree::NotSelfCopy<Row, Args...>
        explicit Row(Args&&... args);

        Row(Row&&) noexcept = default;
        Row& operator=(Row&&) noexcept = default;
        Row(const Row&) = delete;
        Row& operator=(const Row&) = delete;

        void apply(RowBuildContext&) const override;
    };

    // A described expander: a header of text alone, and the rows of its body. See Grids
    export struct Expander : public RowNode
    {
        using ExpanderFactory = std::function<RowExpander& (GridBase&)>;

        ExpanderFactory create;
        RowNodeList children;
        Text headerText;
        bool hasHeaderText{};
        Tree::PostCreate<RowExpander> post; // its handlers and its Init, run before its rows

        template <typename... Args>
            requires Tree::NotSelfCopy<Expander, Args...>
        explicit Expander(Args&&... args);

        Expander(Expander&&) noexcept = default;
        Expander& operator=(Expander&&) noexcept = default;
        Expander(const Expander&) = delete;
        Expander& operator=(const Expander&) = delete;

        void apply(RowBuildContext&) const override;
    };

    // A group's own row, whose cells span the group's rows. It goes directly inside a Group.
    export template <typename... Args>
        struct Span : public Tree::Part<Args...>
    {
        explicit Span(Args&&... args);
    };

    export template <typename... Args> Span(Args&&...) -> Span<Args...>;

    // A described group: its rows, and a span row sectioned by the same columns. See Grids
    export struct Group : public RowNode
    {
        using GroupFactory = std::function<RowGroup& (GridBase&)>;

        GroupFactory create;
        RowNodeList children;
        CellNodeList spanCells;
        Tag spanTag{};
        bool hasSpanTag{};
        Tree::PostCreate<RowGroup> post; // its handlers and its Init, run before its rows

        template <typename... Args>
            requires Tree::NotSelfCopy<Group, Args...>
        explicit Group(Args&&... args);

        Group(Group&&) noexcept = default;
        Group& operator=(Group&&) noexcept = default;
        Group(const Group&) = delete;
        Group& operator=(const Group&) = delete;

        void apply(RowBuildContext&) const override;
    };

    // A divider row in the described grid.
    export struct Divider : public RowNode
    {
        void apply(RowBuildContext&) const override;
    };

    // Rows{ ... } - anonymous group, spliced into the enclosing grid.
    export struct Rows : public RowNode
    {
        RowNodeList children;

        template <typename... Args>
            requires Tree::NotSelfCopy<Rows, Args...>
        explicit Rows(Args&&... args);

        Rows(Rows&&) noexcept = default;
        Rows& operator=(Rows&&) noexcept = default;
        Rows(const Rows&) = delete;
        Rows& operator=(const Rows&) = delete;

        void apply(RowBuildContext&) const override;
    };

    // =====================================================================
    // Grid
    // =====================================================================

    // A grid that is described rather than assembled. See Grids
    export class Grid : public Rt::Grid
    {
    public:
        template <typename... Args>
        explicit Grid(const CreateParams& params, Args&&... args);
    };
}

// =========================================================================
// IMPLEMENTATIONS
// =========================================================================

namespace ClaFi::Controls::Grids::Dt
{
    // --- CellTable ---

    void CellTable::addCell(const Rt::Column& column, CellTextFunc text)
    {
        m_cells.push_back(CellSpec{ &column, std::move(text) });
    }

    const CellSpec* CellTable::findCell(const Rt::Column& column) const
    {
        for (const CellSpec& cell : m_cells)
        {
            if (cell.column == &column)
            {
                return &cell;
            }
        }
        return nullptr;
    }

    bool CellTable::hasCell(const Rt::Column& column) const
    {
        return findCell(column) != nullptr;
    }

    ColumnList CellTable::columnList() const
    {
        ColumnList result;
        result.reserve(m_cells.size());
        for (const CellSpec& cell : m_cells)
        {
            result.push_back(cell.column);
        }
        return result;
    }

    bool CellTable::hasAnyText() const
    {
        for (const CellSpec& cell : m_cells)
        {
            if (cell.text)
            {
                return true;
            }
        }
        return false;
    }

    // --- connectCells ---

    void connectCells(Rt::RowBase& row, CellTable&& spec)
    {
        if (spec.cellCount() == 0)
        {
            // The design declared nothing for this row, so it must not claim to
            // answer for it. Leaving both events unconnected keeps whatever grid
            // level handler answered before.
            return;
        }

        ColumnList columns = spec.columnList();
        row.onCellContent([columns = std::move(columns)](CellContentEvent& event) {
            const ColumnList::const_iterator it = std::find(columns.begin(), columns.end(), &event.column());
            event.hasContent = it != columns.end();
            event.stopPropagation();
            });

        if (!spec.hasAnyText())
        {
            // Every cell here is a control, or defers its text to the grid. No
            // text handler, so the query does not even reach this row's listeners.
            return;
        }
        row.onGetCellText([spec = std::move(spec)](GetCellTextEvent& event) {
            const CellSpec* cell = spec.findCell(event.column());
            if (cell and cell->text)
            {
                cell->text(event);
            }
            });
    }

    // --- ApplyState ---

    void ApplyState::noteColumnTag(Tag tag)
    {
        columnTags.push_back(tag.value);
    }

    void ApplyState::noteUnresolved(Tag tag)
    {
        unresolvedTags.push_back(tag.value);
    }

    void ApplyState::checkColumnTags() const
    {
        // Cells name columns by Tag, so two columns sharing one Tag makes every
        // cell naming it resolve to whichever was declared first. Silent, and
        // impossible to see in the result.
        for (std::size_t i = 0; i < columnTags.size(); ++i)
        {
            for (std::size_t j = i + 1; j < columnTags.size(); ++j)
            {
                if (columnTags[i] != columnTags[j])
                {
                    continue;
                }
                std::string message = "Grid design: two columns share the Tag ";
                message += std::to_string(columnTags[i]);
                message +=
                    ". A design names its columns by Tag, so they have to be unique "
                    "within one grid. Give one of them a different Tag, or drop the Tag "
                    "from the column no cell needs to name.";
                throw std::logic_error(message);
            }
        }
    }

    void ApplyState::checkResolvedCells() const
    {
        // A cell whose Tag matches no column must not be dropped in silence. It
        // surfaces as a row of zero height - no cells means the row declares no
        // content, which means no sections, which means no height - and nothing
        // about that points back at the Tag. Say so at the point of the mistake.
        if (unresolvedTags.empty())
        {
            return;
        }
        std::string message = "Grid design: cell Tags matched no column: ";
        for (TagValue tag : unresolvedTags)
        {
            message += std::to_string(tag) + " ";
        }
        message +=
            "(these are the numeric values of your Tag enum). A column a cell names "
            "must be declared with that Tag - Column{ Tag{ ColumnTag::X }, ... }. A "
            "column with no matching Tag drops every cell naming it, leaving rows "
            "with no declared content and therefore no height.";
        throw std::logic_error(message);
    }

    // --- Column ---

    template <typename... Args>
        requires Tree::NotSelfCopy<Column, Args...>
    Column::Column(Args&&... args)
    {
        static_assert(!(Tree::IsPart<Args> || ...),
            "Grid design: a Header belongs directly inside a Grid or an Expander, and a Span "
            "directly inside a Group.");
        // Read, not consumed: add() receives the Tag like any other property.
        // Passed as lvalues so the forwarding below still owns the arguments.
        Props::ifThereIs<Tag>([this](Tag value) {
            tag = value;
            hasTag = true;
            }, args...);

        auto props = Tree::propsOf<ColumnBuildContext>(std::forward<Args>(args)...);
        create = [props = std::move(props)](Rt::ColumnCollection& target) mutable -> Rt::Column& {
            auto addColumn = [&target](auto&... properties) -> Rt::Column& {
                return target.add(properties...);
                };
            return std::apply(addColumn, props);
            };
        children = Tree::makeNodeVector<ColumnBuildContext>(std::forward<Args>(args)...);
    }

    void Column::apply(ColumnBuildContext& context) const
    {
        Rt::Column& created = create(context.target);
        if (hasTag)
        {
            context.state.noteColumnTag(tag);
        }
        if (!children.empty())
        {
            ColumnBuildContext childContext{ context.state, created.createSubColumns() };
            for (const ColumnNodePtr& child : children)
            {
                child->apply(childContext);
            }
        }
    }

    // --- Columns ---

    template <typename... Args>
        requires Tree::NotSelfCopy<Columns, Args...>
    Columns::Columns(Args&&... args)
        :
        children{ Tree::makeNodeVector<ColumnBuildContext>(std::forward<Args>(args)...) }
    {
        static_assert(!(Tree::IsPart<Args> || ...),
            "Grid design: a Header belongs directly inside a Grid or an Expander, and a Span "
            "directly inside a Group.");
    }

    void Columns::apply(ColumnBuildContext& context) const
    {
        for (const ColumnNodePtr& child : children)
        {
            child->apply(context);
        }
    }

    // --- Cell ---

    Cell::Cell(Tag columnTag)
        :
        tag{ columnTag }
    {
    }

    template <typename F>
        requires std::is_invocable_v<F&, GetCellTextEvent&>
    Cell::Cell(Tag columnTag, F&& textProvider)
        :
        tag{ columnTag },
        text{ std::forward<F>(textProvider) }
    {
    }

    template <typename TObject, typename Method>
        requires std::is_member_function_pointer_v<Method>
    Cell::Cell(Tag columnTag, TObject* object, Method method)
        :
        tag{ columnTag },
        text{ [object, method](GetCellTextEvent& event) {
            (object->*method)(event);
        } }
    {
    }

    void Cell::apply(CellBuildContext& context) const
    {
        Rt::Column* column = context.grid.findColumnByTag(tag);
        if (column)
        {
            context.spec.addCell(*column, text);
        }
        else
        {
            context.state.noteUnresolved(tag);
        }
    }

    // --- CellWith ---

    template <typename ControlClass>
    template <typename... Args>
    CellWith<ControlClass>::CellWith(Tag columnTag, Args&&... args)
        :
        tag{ columnTag }
    {
        // ctorPropsOf moves the constructor arguments out of the pack.
        // postCreateOf only copies the handlers, and the two see a disjoint set
        // of arguments, so the order the captures are initialised in does not
        // matter.
        create = [props = Tree::ctorPropsOf<ControlClass>(std::forward<Args>(args)...),
            post = Tree::postCreateOf<ControlClass>(args...)]
            (Rt::RowContainer& row, Rt::Column& column) mutable {
            auto addControl = [&row, &column](auto&... properties) -> ControlClass& {
                return row.template addControl<ControlClass>(column, properties...);
                };
            ControlClass& control = std::apply(addControl, props);
            post(control);
            };
    }

    template <typename ControlClass>
    void CellWith<ControlClass>::apply(CellBuildContext& context) const
    {
        Rt::Column* column = context.grid.findColumnByTag(tag);
        if (!column)
        {
            context.state.noteUnresolved(tag);
            return;
        }
        // A control cell declares content without declaring text.
        context.spec.addCell(*column, CellTextFunc{});
        if (!context.container)
        {
            unreachable(
                "Grid design: CellWith was used in a row that cannot host controls. Only a "
                "RowContainer holds controls, and every row a design creates is one - as is a "
                "group's span.");
        }
        create(*context.container, *column);
    }

    // --- CellSet ---

    template <typename... Args>
        requires Tree::NotSelfCopy<CellSet, Args...>
    CellSet::CellSet(Args&&... args)
        :
        children{ Tree::makeNodeVector<CellBuildContext>(std::forward<Args>(args)...) }
    {
        static_assert(!(Tree::IsPart<Args> || ...),
            "Grid design: a Header belongs directly inside a Grid or an Expander, and a Span "
            "directly inside a Group.");
    }

    void CellSet::apply(CellBuildContext& context) const
    {
        for (const CellNodePtr& child : children)
        {
            child->apply(context);
        }
    }

    // --- Row ---

    template <typename... Args>
        requires Tree::NotSelfCopy<Row, Args...>
    Row::Row(Args&&... args)
    {
        static_assert(!(Tree::IsPart<Args> || ...),
            "Grid design: a Header belongs directly inside a Grid or an Expander, and a Span "
            "directly inside a Group.");
        auto props = Tree::propsOf<CellBuildContext>(std::forward<Args>(args)...);
        create = [props = std::move(props)](GridBase& target) mutable -> Rt::RowContainer& {
            auto addRow = [&target](auto&... properties) -> Rt::RowContainer& {
                return target.template add<Rt::RowContainer>(properties...);
                };
            return std::apply(addRow, props);
            };
        cells = Tree::makeNodeVector<CellBuildContext>(std::forward<Args>(args)...);
    }

    void Row::apply(RowBuildContext& context) const
    {
        Rt::RowContainer& row = create(context.target);
        CellTable spec = {};
        CellBuildContext cellContext{ context.state, context.target, spec, row, &row };
        for (const CellNodePtr& cell : cells)
        {
            cell->apply(cellContext);
        }
        connectCells(row, std::move(spec));
    }

    // --- Expander ---

    template <typename... Args>
        requires Tree::NotSelfCopy<Expander, Args...>
    Expander::Expander(Args&&... args)
    {
        static_assert(!(Tree::IsPartNamed<Args, Span> || ...),
            "Grid design: a Span belongs directly inside a Group. An Expander's own part is "
            "its Header.");
        Tree::withPart<Tree::Header>([this](auto&... headerArgs) {
            static_assert(!(Tree::IsNode<decltype(headerArgs), CellBuildContext> || ...),
                "Grid design: an Expander's header is a single text control and holds no "
                "cells. Put the Cell in a Row inside the Expander, or use a Group, whose "
                "Span is a row.");
            Props::ifThereIs<Text>([this](const Text& value) {
                headerText = value;
                hasHeaderText = true;
                }, headerArgs...);
            }, args...);

        post = Tree::postCreateOf<RowExpander>(args...);
        auto props = Tree::containerCtorPropsOf<RowBuildContext>(std::forward<Args>(args)...);
        create = [props = std::move(props)](GridBase& target) mutable -> RowExpander& {
            auto addExpander = [&target](auto&... properties) -> RowExpander& {
                return target.addExpander(properties...);
                };
            return std::apply(addExpander, props);
            };
        children = Tree::makeNodeVector<RowBuildContext>(std::forward<Args>(args)...);
    }

    void Expander::apply(RowBuildContext& context) const
    {
        RowExpander& expander = create(context.target);
        if (hasHeaderText)
        {
            expander.header().setHeaderText(headerText);
        }
        // The expander stands with its header written before any of its rows is applied.
        post(expander);
        RowBuildContext childContext{ context.state, expander.body() };
        for (const RowNodePtr& child : children)
        {
            child->apply(childContext);
        }
    }

    // --- Span ---

    template <typename... Args>
    Span<Args...>::Span(Args&&... args)
        :
        Tree::Part<Args...>{ std::forward<Args>(args)... }
    {
    }

    // --- Group ---

    template <typename... Args>
        requires Tree::NotSelfCopy<Group, Args...>
    Group::Group(Args&&... args)
    {
        static_assert(!(Tree::IsPartNamed<Args, Tree::Header> || ...),
            "Grid design: a Group's own row stands beside its rows rather than above them - "
            "it is a Span{ ... }, not a Header{ ... }.");
        Tree::withPart<Span>([this](auto&... spanArgs) {
            Props::ifThereIs<Tag>([this](Tag value) {
                spanTag = value;
                hasSpanTag = true;
                }, spanArgs...);
            spanCells = Tree::makeNodeVector<CellBuildContext>(std::move(spanArgs)...);
            }, args...);

        post = Tree::postCreateOf<RowGroup>(args...);
        auto props = Tree::containerCtorPropsOf<RowBuildContext>(std::forward<Args>(args)...);
        create = [props = std::move(props)](GridBase& target) mutable -> RowGroup& {
            auto addGroup = [&target](auto&... properties) -> RowGroup& {
                return target.addGroup(properties...);
                };
            return std::apply(addGroup, props);
            };
        children = Tree::makeNodeVector<RowBuildContext>(std::forward<Args>(args)...);
    }

    void Group::apply(RowBuildContext& context) const
    {
        RowGroup& group = create(context.target);

        // The span is a row, and a RowContainer at that, so it is described and connected
        // exactly as one: its cells answer for themselves, controls included, and no grid
        // level handler is involved.
        Rt::RowContainer& span = group.span();
        if (hasSpanTag)
        {
            span.setTag(spanTag);
        }
        if (!spanCells.empty())
        {
            CellTable spanSpec = {};
            CellBuildContext spanContext{ context.state, context.target, spanSpec, span, &span };
            for (const CellNodePtr& cell : spanCells)
            {
                cell->apply(spanContext);
            }
            connectCells(span, std::move(spanSpec));
        }

        // The group stands with its span described before any of its rows is applied.
        post(group);

        RowBuildContext childContext{ context.state, group.body() };
        for (const RowNodePtr& child : children)
        {
            child->apply(childContext);
        }
    }

    // --- Divider ---

    void Divider::apply(RowBuildContext& context) const
    {
        context.target.addDivider();
    }

    // --- Rows ---

    template <typename... Args>
        requires Tree::NotSelfCopy<Rows, Args...>
    Rows::Rows(Args&&... args)
        :
        children{ Tree::makeNodeVector<RowBuildContext>(std::forward<Args>(args)...) }
    {
        static_assert(!(Tree::IsPart<Args> || ...),
            "Grid design: a Header belongs directly inside a Grid or an Expander, and a Span "
            "directly inside a Group.");
    }

    void Rows::apply(RowBuildContext& context) const
    {
        for (const RowNodePtr& child : children)
        {
            child->apply(context);
        }
    }

    // --- Grid ---

    template <typename... Args>
    Grid::Grid(const CreateParams& params, Args&&... args)
        :
        // The nodes ride along in this pack. Nothing below matches their types, so
        // they arrive here untouched and are moved from only by makeNodeVector.
        Rt::Grid{ params, std::forward<Args>(args)... }
    {
        static_assert(!(Tree::IsPartNamed<Args, Span> || ...),
            "Grid design: a Span belongs directly inside a Group.");
        // Build-time only, and it goes out of scope with this frame.
        ApplyState state = {};

        // Two passes over the same pack: columns first, so that cells can resolve
        // their Tags. Each pass moves only the nodes belonging to its own context, so
        // the two never touch the same argument.
        ColumnNodeList columnNodes = Tree::makeNodeVector<ColumnBuildContext>(std::forward<Args>(args)...);
        RowNodeList rowNodes = Tree::makeNodeVector<RowBuildContext>(std::forward<Args>(args)...);

        // The grid's own header takes nothing: the columns already say what belongs in
        // it, through ShowInHeader. It is looked up rather than applied in sequence, so
        // it is added between the columns and the rows wherever it was written.
        const bool wantsHeader = Tree::withPart<Tree::Header>([](auto&... headerArgs) {
            static_assert(sizeof...(headerArgs) == 0,
                "Grid design: a grid's header is described by its columns - give a column "
                "ShowInHeader::Yes rather than putting anything in Header{}.");
            }, args...);

        ColumnBuildContext columnContext{ state, columns() };
        for (const ColumnNodePtr& node : columnNodes)
        {
            node->apply(columnContext);
        }

#ifdef _DEBUG
        state.checkColumnTags();
#endif

        if (wantsHeader)
        {
            addHeader();
        }

        RowBuildContext rowContext{ state, *this };
        for (const RowNodePtr& node : rowNodes)
        {
            node->apply(rowContext);
        }

#ifdef _DEBUG
        state.checkResolvedCells();
#endif
    }
}
