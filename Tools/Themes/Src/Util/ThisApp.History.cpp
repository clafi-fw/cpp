module ThisApp.History;

import ClaFi.Controls.Base.SliderBase;

namespace ThisApp
{
    EditPhase editPhaseOf(const ClaFi::Controls::SliderBase& slider)
    {
        return slider.positionHeldByPointer() ? EditPhase::Held : EditPhase::Settled;
    }
}
