module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Grids :RowExpander;

import :Columns;
import :Descriptor;
import :RowBase;
import :RowGroupBase;

import ClaFi.Controls.Base.ExpanderBase;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls::Grids
{

    class RowExpanderHeader : public ExpanderHeader
    {
    public:
        using ExpanderHeader::ExpanderHeader;
    public:
        template <typename... Args>
        RowExpanderHeader(const CreateParams&, Args&&... args);
    protected:
        // A corner standing at the grid's turns the grid's corner, as a cell's does there.
        void adjustPaint(AdjustPaintEvent&) override;
        void paintSurface(PaintEvent&) override;
    private:
        // The sides standing on the grid's frame, measured the way a cell's are.
        [[nodiscard]] RectSides sidesOnGridFrame(const PaintEvent&) const;
    };

    // A row whose header is held at the top of the view while its body scrolls under. See Grids
    export class RowExpander : public WithHeldHeader<RowGroupBaseWith<RowExpanderHeader>>
    {
    public:
        template<typename ...Args>
        RowExpander(const CreateParams& params, GridDescriptor& descriptor, Args&&... args);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"RowExpander"; }
    protected:
        [[nodiscard]] const Control* headedBody() const override { return m_controls[k_body].get(); }
        RowExpander* asRowExpander() override { return this; }
        RowStop navigationStop() override;
        // From above at the header, from below in the body, from a side where the line runs.
        [[nodiscard]] Control* navigationEntry(const NavigationStep&) override;
        // Down from the header goes on into the body, Up from the body back to the header.
        [[nodiscard]] Control* navigationNeighbour(const Control& child,
            const NavigationStep&) override;
        // declines selection
        void getControlState(GetStateEvent&) const override;
        // The body is read shut or open, and opened to show a match in it.
        [[nodiscard]] ChildSearch childSearch(const Control&) const override;
        void revealChild(Control&) override;
    private:
        void bodyToggled(ToggleExpandedEvent&);
    };


    //-------------------------------------------------------------------------


    // RowExpanderHeader

    template<typename ...Args>
    RowExpanderHeader::RowExpanderHeader(const CreateParams& params, Args&&... args)
        :
        ExpanderHeader{ params, std::forward<Args>(args)... }
    {
    }


    // RowExpander

    template<typename ...Args>
    RowExpander::RowExpander(const CreateParams& params, GridDescriptor& descriptor, Args && ...args)
        :
        WithHeldHeader{ params, descriptor, Interactivity::None, std::forward<Args>(args)...}
    {
        m_controls[k_head] = std::make_unique<RowExpanderHeader>(
            CreateParams{ *this },
            descriptor.designCellMetrics().padding,
            UiElement::SectionHeader,
            // What the expander draws of itself.
            READ_PROPERTY(ExpanderViewMode, ExpanderViewMode::Section)
        );
        Props::ifThereIs<HeaderText>([&](const auto& headerText) {
            header().setHeaderText(headerText);
            }, std::forward<Args>(args)...);

        RowExpanderHeader& hdr = static_cast<RowExpanderHeader&>(*m_controls[k_head]);
        hdr.connectEvent<ToggleExpandedEvent>([this](ToggleExpandedEvent& event) {
            bodyToggled(event);
        });
        holdHeader(hdr);
    }

    RowStop RowExpander::navigationStop()
    {
        // The header carries no cells, so a walk over cells alone steps straight over the
        // expander. The header is the stop, and its rect spans every column, so it is met from
        // whichever column the walk is following.
        return { &header(), header().boundsInForm() };
    }

}
