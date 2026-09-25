module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Grids :Columns;


import ClaFi.Core.System.Props;

import ClaFi.Core.Foundation.Fit;
import ClaFi.Core.Foundation;

import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.System.UiTypes;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.TextEngine.Text;


namespace ClaFi::Controls::Grids
{
    export class GridBase;
    export class RowBase;
    export class GridDescriptor;
    export class ColumnCollection;

    // How a column resolves its width during layout.
    export enum class ColumnWidthMode
    {
        FitContent, // designWidth is ignored, the width comes from the rows' content
        Fixed,      // designWidth is the width, in design units
        Fill        // content sets the minimum, then designWidth divides the parent's
                    // slack as a ratio against the sibling Fill columns. A single Fill
                    // column takes the whole remainder, so its ratio is ignored.
    };

    // Whether the column's name is written in the grid's header.
    export enum class ShowInHeader
    {
        Yes,
        No
    };

    // Dictates how a column's cells render hover and selection states.
    export enum class CellHighlightMode
    {
        None,    // Visuals never change, regardless of cursor or selection state.
        Grid,    // The grid renders the state directly (e.g., drawing a border frame).
        Control  // The hosted control renders the state using factors provided by the grid.
                 // Falls back to 'Grid' if the cell contains no control.
    };

    // Whether a column's cells say something different from one paint to the next. See Grids
    export enum class MovingText
    {
        No,
        Yes
    };

    // A control laid out against a grid's columns.
    export class LaneBase : public Control
    {
    public:
        GridDescriptor& descriptor() { return m_descriptor; }
        const GridDescriptor& descriptor() const { return m_descriptor; }
    protected:
        // used by rows
        template<typename... Args>
        LaneBase(const CreateParams&, GridDescriptor&, Args&&...);
        // used by columns
        template<typename... Args>
        explicit LaneBase(GridDescriptor&, Args&&...);
    protected:
        virtual const BakedRule* color(const PaintEvent&) const { return nullptr; }
    protected:
        GridDescriptor& m_descriptor;
    };

    // That may be an outdated comment, as now we have DSL and everything in Dt:
    // TODO: ColumnProps routing is yet to be implemented (example in the ScrollBoxWith template)
    namespace {
        template<typename... Args>
            struct ColumnProps : public Props::PropsRouter<Args...>
        {
            using Props::PropsRouter<Args...>::PropsRouter;
        };
        template<typename... Args>
            ColumnProps(Args&&...) -> ColumnProps<Args...>;
    }

    export class Column : public IFittableList, public IFittable
    {
        friend ColumnCollection;
        friend GridDescriptor;
        friend RowBase;
    public:
        //std::wstring_view diagnosticText() const override { return L"Column"; }
        template <typename... Args>
        Column(GridDescriptor&, ColumnCollection*, Args&&... args);
        ~Column() override;
        Column& operator=(const Column&) = delete;
    public:
        // How the column's width is worked out.
        DECLARE_PROPERTY(ColumnWidthMode, calcMode, ColumnWidthMode::FitContent)
        // Whether the column's name is written in the grid's header.
        DECLARE_WRITABLE_PROPERTY(ShowInHeader, showInHeader, setShowInHeader, ShowInHeader::Yes)
        // What a hover or a selection highlights in this column.
        DECLARE_WRITABLE_PROPERTY(CellHighlightMode, cellHighlightMode, setCellHighlightMode, CellHighlightMode::Grid)
        // What an in-place editor over this column's cells is for, and whether one opens.
        DECLARE_WRITABLE_PROPERTY(EditorMode, editorMode, setEditorMode, EditorMode::ReadOnly)
        // How the text of this column's cells is aligned.
        DECLARE_PROPERTY(TextAlign, textAlign, TextAlign::Left)
        // Where that text sits down its cell.
        DECLARE_PROPERTY(VerticalTextAnchor, verticalTextAnchor, VerticalTextAnchor::Top)
        // Whether the column's cells say something different from one paint to the next.
        DECLARE_PROPERTY(MovingText, movingText, MovingText::No)
        DECLARE_PROPERTY(Tag, tag, Tag{}) // whatever the caller hangs on the column
    public:
        Column* parent() const { return m_parent; }
    public:
        [[nodiscard]] std::size_t index() const { return m_index; }
        [[nodiscard]] const Text& text() const { return m_text; }
        // Where a cell's text sits in a row taller than it - a row another cell of it made tall.
        const BakedRule* color(const PaintEvent& event) const
        {
            return m_color.of(event.bakedColors());
        }
        void setColor(ThemeRule value) { m_color = value; }
        //
        [[nodiscard]] float neededWidth() const { return m_neededWidth; }
        ColumnCollection* subColumns() const { return m_subColumns; }
        ColumnCollection* ownerCollection() const { return m_ownerCollection; }
        ColumnCollection& createSubColumns();
        [[nodiscard]] std::size_t level() const;
        [[nodiscard]] bool hasSubColumns() const;
        [[nodiscard]] std::size_t subColumnsCount() const;

        void setShowInHeader(ShowInHeader value) { m_showInHeader = value; }
        void setCellHighlightMode(CellHighlightMode value) { m_cellHighlightMode = value; }
        void setEditorMode(EditorMode value) { m_editorMode = value; }
    public:
        float calculatedWidth() const { return m_calculatedWidth; }
        float finalWidth() const { return m_finalWidth; }
        float calculatedSubColumnsWidth2() const { return m_calculatedSubColumnsWidth2; }
        [[nodiscard]] float left() const { return m_left; }
        // IFittable
        FitData fitData() const override { return { m_calculatedWidth }; }
        void takeOffWidth(float value) override;
        // IFittableList
        void traverseFittables(const FittableFunc) override;
    private:
        // addNeededWidth is called for every row at calcualating.
        // the w param is the single cell width
        void addNeededWidth(const float w);
        float subColumnsNeededWidth() const;
        // Fill columns divide their parent's slack in these proportions. A column that
        // names no ratio counts as one. Meaningless for the other width modes.
        [[nodiscard]] float fillRatio() const { return m_designWidth > 0.0f ? m_designWidth : 1.0f; }
        //
        void clearCalculatedWidth();
        void preCalcRows(GridBase&);
        void finishCalcStage1();
        void stretchStretched(const float boundsW);
        void calcLeft(float left);
    private:
        // initialize
        GridDescriptor& m_descriptor;
        Column* m_parent;
        ColumnCollection* m_ownerCollection;
        std::size_t m_index;
        //
        ColumnCollection* m_subColumns{ nullptr };
        //
        Text m_text;
        ThemeRule m_color;
        //
        float m_designWidth;
        // all bellow is calculated
        float m_neededWidth{};
        float m_calculatedWidth{};
        float m_finalWidth{};
        std::size_t m_stretchedCnt{};
        float m_stretchedWidth{};
        float m_subColumnsWidth{};
        float m_calculatedSubColumnsWidth2{};
        float m_left{};
    };

    using ColumnCollectionBase = std::vector<Column*>;
    export class ColumnCollection : private ColumnCollectionBase
    {
        friend GridDescriptor;
        friend Column;
    public:
        using ColumnCollectionBase::begin;
        using ColumnCollectionBase::end;
        using ColumnCollectionBase::rbegin;
        using ColumnCollectionBase::rend;
        using ColumnCollectionBase::empty;
        using ColumnCollectionBase::iterator;
        using ColumnCollectionBase::reverse_iterator;
    public:
        ColumnCollection(GridDescriptor& descriptor, Column* parent);
        ~ColumnCollection();
        ColumnCollection& operator = (const ColumnCollection&) = delete;
    public:
        std::size_t level() const { return m_level; }
        template<typename... Args>
        Column& add(Args&&... args);
        [[nodiscard]] Column* findByTag(Tag) const;   // columns().findByTag(tag)
        // The tag is a precondition: a miss is unreachable(), not a return value.
        [[nodiscard]] Column& byTag(Tag) const;
    private:
        // Defined in Columns.cpp: GridDescriptor is incomplete in this partition.
        void invalidateLayout() const;
    private:
        GridDescriptor& m_descriptor;
        Column* m_parent;
        std::size_t m_level;
    };


    //-------------------------------------------------------------------------


    // LaneBase

    template<typename ...Args>
    LaneBase::LaneBase(const CreateParams& params, GridDescriptor& descriptor, Args&&... args)
        :
        Control{ params, std::forward<Args>(args)... },
        m_descriptor{ descriptor }
    {
    }

    template<typename ...Args>
    LaneBase::LaneBase(GridDescriptor& descriptor, Args&&... args)
        :
        Control{ nullptr, std::forward<Args>(args)... },
        m_descriptor{ descriptor }
    {
    }

    // Column

    template<typename ...Args>
    Column::Column(GridDescriptor& descriptor, ColumnCollection* ownerCollection, Args&&... args)
        :
        m_descriptor{ descriptor },
        m_parent{ ownerCollection ? ownerCollection->m_parent : nullptr },
        m_ownerCollection{ ownerCollection },
        m_index{ m_parent ? m_parent->subColumnsCount() : 0 },
        INIT_PROPERTY(calcMode),
        INIT_PROPERTY(showInHeader),
        INIT_PROPERTY(cellHighlightMode),
        INIT_PROPERTY(editorMode),
        INIT_PROPERTY(textAlign),
        INIT_PROPERTY(verticalTextAnchor),
        INIT_PROPERTY(movingText),
        // TODO: a bare float has no property type of its own, so any float in the pack matches.
        m_designWidth{ Props::get(0.0f, args...) },
        m_color{ Props::get<ThemeRule>(ThemeRule{}, args...) },
        INIT_PROPERTY(tag)
    {
        // Copied from Control
        Props::ifThereIs<Text>([&](const auto& p) {
            m_text << p;
            }, std::forward<Args>(args)...);
        Props::ifThereIs<std::wstring_view>([&](const auto& p) {
            m_text << p;
            }, std::forward<Args>(args)...);
        Props::ifThereIs<const wchar_t*>([&](const auto& p) {
            m_text << p;
            }, std::forward<Args>(args)...);
    }

    // ColumnCollection

    template<typename ...Args>
    Column& ColumnCollection::add(Args&&... args)
    {
        Column* result = new Column{ m_descriptor, this, std::forward<Args>(args)... };
        push_back(result);
        if (m_parent)
            m_parent->clearCalculatedWidth();
        // The column set changed, so the cached layout is stale.
        invalidateLayout();
        return *result;
    }

}
