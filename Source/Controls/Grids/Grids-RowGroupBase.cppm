export module ClaFi.Controls.Grids :RowGroupBase;

import :RowBase;
import :Row;
import ClaFi.Core.Foundation;
import :Columns;
import :Descriptor;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{
    export class SubGrid;

    // A row holding a grid of rows, under an expander's header or beside a group's span. See Grids
    export class RowGroupBase : public RowBase
    {
    public:
        template<typename... Args>
        RowGroupBase(const CreateParams&, GridDescriptor&, Args&&...);
        ~RowGroupBase() override;
    public:
        SubGrid& body();
    protected:
        Interactivity interactivity() const override { return Interactivity::None; }
        ControlSpan controls() override { return { m_controls }; }
        bool hasContent(const Column&) const override { return false; }
        void preCalcColumn(Column& column) override;
        GridBase* subGrid() override;
        ScaledDimensions calculateContent(AlignEvent& event) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
    protected:
        // Head first, body second, in one array so that controls() can hand them out as a range.
        static constexpr std::size_t k_head{ 0 };
        static constexpr std::size_t k_body{ 1 };
        std::array<ControlPtr, 2> m_controls{};
    private:
        void createSubGrid(GridDescriptor&);
    };

    // to be deleted
    export template<IsControl HeaderClass>
        // A row group whose header is of the named type.
        class RowGroupBaseWith : public RowGroupBase
    {
    public:
        using RowGroupBase::RowGroupBase;
    public:
        HeaderClass& header() { return static_cast<HeaderClass&>(*m_controls[k_head]); }
        const HeaderClass& header() const { return static_cast<HeaderClass&>(*m_controls[k_head]); }
    };


    //-------------------------------------------------------------------------


    // RowGroupBase

    template<typename ...Args>
    RowGroupBase::RowGroupBase(const CreateParams& params, GridDescriptor& descriptor, Args&&... args)
        :
        RowBase{ params, descriptor, std::forward<Args>(args)... }
    {
        createSubGrid(descriptor);
    }

}
