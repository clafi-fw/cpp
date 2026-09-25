export module ClaFi.Controls.Panel;

export import ClaFi.Controls.Base.PanelBase;

namespace ClaFi::Controls
{
    // A container with bars around a body.
    export class Panel : public PanelBase
    {
    public:
        using PanelBase::PanelBase;
        using PanelBase::createTopBar;
        using PanelBase::createLeftBar;
        using PanelBase::createBody;
        using PanelBase::createRightBar;
        using PanelBase::createBottomBar;
        using PanelBase::topBar;
        using PanelBase::leftBar;
        using PanelBase::body;
        using PanelBase::bodyAs;
        using PanelBase::rightBar;
        using PanelBase::bottomBar;
    };
}
