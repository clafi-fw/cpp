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
        void paintSurface(PaintEvent&) override;
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
        RowStop navigationStop() override;
        // declines selection
        void getControlState(GetStateEvent&) const override;
    };


    //-------------------------------------------------------------------------


    // RowExpanderHeader

    template<typename ...Args>
    RowExpanderHeader::RowExpanderHeader(const CreateParams& params, Args&&... args)
        :
        ExpanderHeader{ params, std::forward<Args>(args)... }
    {
    }

    void RowExpanderHeader::paintSurface(PaintEvent& event)
    {
        ExpanderHeader::paintSurface(event);
        GridDescriptor& descriptor = parentAs<RowExpander>().descriptor();
        if (!descriptor.drawsHorizontalLines())
            return;
        FloatRect bottomBorderRect = event.controlBounds();
        bottomBorderRect.top = bottomBorderRect.bottom - descriptor.scaledBorderWidth() - event.scale(1.0f - expandedFactor());
        event.canvas().fillRectangle(bottomBorderRect,
            GridDescriptor::gridLineRgb(event.surfaceHsl(), event.bakedColors(),
                event.lightness()));
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
            UiElement::Header,
            // What the expander draws of itself.
            READ_PROPERTY(ExpanderViewMode, ExpanderViewMode::Section)
        );
        Props::ifThereIs<HeaderText>([&](const auto& headerText) {
            header().setHeaderText(headerText);
            }, std::forward<Args>(args)...);

        RowExpanderHeader& hdr = static_cast<RowExpanderHeader&>(*m_controls[k_head]);
        hdr.connectEvent<ToggleExpandedEvent>([this](ToggleExpandedEvent& event)
        {
                m_controls[k_body]->setVisible(event.expanded());
        });
        holdHeader(hdr);
    }

    RowStop RowExpander::navigationStop()
    {
        // The header carries no cells, so a walk over cells alone steps straight over the
        // expander. The button is what a stop here is for - it opens and closes the section -
        // and the header's rect is what places it, so it is met from whichever column the walk
        // is following.
        return { &header().button(), header().boundsInForm() };
    }

}
