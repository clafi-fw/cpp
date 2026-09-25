export module ClaFi.Controls.Base.SpacerBase;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;


namespace ClaFi::Controls
{
    // A control that takes room and draws nothing.
    export class SpacerBase : public Control
    {
    public:
        SpacerBase(const CreateParams&, MinSize size);
    public:
        void setMinSize(MinSize value) { m_minSize = value; }
        void setMinSize(float value) { m_minSize = { value, value }; }
    protected:
        void adjustMetrics(AdjustMetricsEvent&) const override;
        void hitTest(HitTestEvent& event) const override { event.zone = HitTest::Transparent; }
    private:
        MinSize m_minSize{ 4.0, 4.0 };
    };


    //-------------------------------------------------------------------------


    SpacerBase::SpacerBase(const CreateParams& params, MinSize size)
        :
        Control{ params },
        m_minSize{ size }
    {
    }

    void SpacerBase::adjustMetrics(AdjustMetricsEvent& event) const
    {
        event.metrics.minSize = m_minSize;
    }
}
