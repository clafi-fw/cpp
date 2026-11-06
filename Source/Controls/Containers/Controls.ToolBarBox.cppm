export module ClaFi.Controls.ToolBarBox;

import ClaFi.Controls.ToolBar;
import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.Base.WithHeldHeader;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    using ToolBarBoxBaseClass = WithHeldHeader<PanelBase, ToolBar>;

    // Props destined for the bar a ToolBarBox builds, given ahead of the box's own.
    export template<typename... Args>
        struct ToolsProps : public Props::PropsRouter<Args...>
    {
        using Props::PropsRouter<Args...>::PropsRouter;
    };
    export template<typename... Args>
        ToolsProps(Args&&...) -> ToolsProps<Args...>;

    // A panel whose tool bar is held over its body as the body scrolls. See Item-Containers
    export class ToolBarBox : public ToolBarBoxBaseClass
    {
    public:
        using PanelBase::createBody;
        using PanelBase::createBottomBar;
        using PanelBase::body;
        using PanelBase::bottomBar;
    public:
        template<typename... Args>
        explicit ToolBarBox(const CreateParams&, Args&&...);
        // The bar's props stand over the ToolBar defaults - a bar built as no focus scope.
        template<typename... ToolsArgs, typename... Args>
        ToolBarBox(const CreateParams&, ToolsProps<ToolsArgs...>&&, Args&&...);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"ToolBarBox"; }
        [[nodiscard]] ToolBar& tools() { return m_tools; }
        [[nodiscard]] const ToolBar& tools() const { return m_tools; }
    protected:
        [[nodiscard]] const Control* headedBody() const override { return body(); }
    private:
        ToolBar& m_tools{ createTopBar<ToolBar>() };
    };

    export template <IsControl BodyType>
        // A tool bar box whose body is a control of the named type.
        class ToolBarBoxWith : public WithBody<ToolBarBox, BodyType>
    {
    public:
        using WithBody<ToolBarBox, BodyType>::WithBody;
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    ToolBarBox::ToolBarBox(const CreateParams& params, Args&&... args)
        :
        ToolBarBoxBaseClass{ params, std::forward<Args>(args)... }
    {
        holdHeader(m_tools);
    }

    template<typename... ToolsArgs, typename... Args>
    ToolBarBox::ToolBarBox(const CreateParams& params, ToolsProps<ToolsArgs...>&& toolsProps,
        Args&&... args)
        :
        ToolBarBoxBaseClass{ params, std::forward<Args>(args)... },
        m_tools{ createTopBar<ToolBar>(std::get<ToolsArgs>(std::move(toolsProps.data))...) }
    {
        holdHeader(m_tools);
    }
}
