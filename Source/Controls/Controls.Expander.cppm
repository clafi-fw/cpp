module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.Expander;

export import ClaFi.Controls.Base.ExpanderBase;

import ClaFi.Controls.Panel;
import ClaFi.Controls.Button;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Animation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A panel whose body is shown or hidden by its header.
    export class Expander : public WithHeldHeader<PanelBase>
    {
    public:
        using PanelBase::createBody;
        using PanelBase::createBottomBar;
        using PanelBase::topBar;
        using PanelBase::body;
        using PanelBase::bottomBar;
    public:
        template<typename... Args>
        explicit Expander(const CreateParams& params, Args&&...);
        void setHeaderText(const Text& value);
        // The header carries the chevron button and the expanded state, and is what a host
        // reaches for to say anything about either.
        [[nodiscard]] ExpanderHeader& header() { return m_header; }
        [[nodiscard]] const ExpanderHeader& header() const { return m_header; }
    protected:
        [[nodiscard]] const Control* headedBody() const override { return body(); }
    private:
        ExpanderHeader& m_header;
    };

    export template <IsControl BodyType>
        // An expander whose body is a control of the named type.
        class ExpanderWith : public WithBody<Expander, BodyType>
    {
    public:
        using WithBody<Expander, BodyType>::WithBody;
    };


    //-------------------------------------------------------------------------


    // Expander

    template<typename... Args>
    Expander::Expander(const CreateParams& params, Args&&... args)
        :
        WithHeldHeader<PanelBase>{
            params,
            params.themeMetrics().page,
            expanderColorRules(UiElement::Section,
                // What the expander draws of itself, which is what picks its colour rules.
                READ_PROPERTY(ExpanderViewMode, ExpanderViewMode::Section)),
            std::forward<Args>(args)...
        },
        // Only the look is handed on. The header is a control of its own, so a props bag meant
        // for the expander would set its text and its metrics a second time.
        m_header{ createTopBar<ExpanderHeader>(
            VerticalTextAnchor::Center,
            // The same, handed on to the header the expander builds.
            READ_PROPERTY(ExpanderViewMode, ExpanderViewMode::Section)) }
    {
        // The header owns whether the section is open; the body is the host's to show. One
        // answer to the toggle keeps the two in step however the header was worked - the
        // button, a double click on the header, or setExpanded from code.
        m_header.connectEvent<ToggleExpandedEvent>([this](ToggleExpandedEvent& event) {
            body()->setVisible(event.expanded());
        });
        m_header.setRadius(params.themeMetrics().page.radius - padding().x);
        holdHeader(m_header);
        Props::ifThereIs<HeaderText>([&](const auto& headerText) {
            setHeaderText(headerText);
            }, std::forward<Args>(args)...);
    }

    void Expander::setHeaderText(const Text& value)
    {
        m_header.setHeaderText(value);
    }

}
