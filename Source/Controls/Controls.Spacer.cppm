export module ClaFi.Controls.Spacer;

import ClaFi.Controls.Base.SpacerBase;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // "Do you see the gopher?" "No." "Neither do I. But it's there!"
    export class Spacer : public SpacerBase
    {
    public:
        Spacer(const CreateParams& params, MinSize);
        explicit Spacer(const CreateParams& params, float = 4.0);
        Spacer(const CreateParams& params, float, float);
    };

    // THE ROOM NOBODY ELSE CLAIMED. It measures the size it is given and takes what its lane
    // has over, so what stands after it sits at the lane's end - the commands under a strip, the
    // answer at the foot of a page. Two of them either side of an item centre it, and two at one
    // end share what is left evenly. A lane is only as long as its stack was granted, so one in a
    // stack that wraps, or in a stack nothing stretched, has nothing to take - and the size it
    // was given is the gap that is left there. See Control::fillsLane
    export class FlexSpacer : public SpacerBase
    {
    public:
        explicit FlexSpacer(const CreateParams&, float minSize = 0.0f);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"FlexSpacer"; }
        [[nodiscard]] bool fillsLane() const override { return true; }
    };

    //-------------------------------------------------------------------------

    // IT KEEPS THE BOX IT IS GIVEN, which is the whole of the difference from Spacer above and
    // the one thing to leave alone here. Control::align hands a control the room its lane granted
    // and then TAKES IT BACK where the control aligns to an edge rather than filling - see
    // needSecondAlignY - so a spacer stated Top would be handed the surplus, shrink to the
    // nothing it measured, and leave everything after it exactly where it was. Fill is the
    // default on both axes, so this constructor states no alignment at all.
    FlexSpacer::FlexSpacer(const CreateParams& params, const float minSize)
        :
        SpacerBase{ params, MinSize{ minSize, minSize } }
    {
    }

    Spacer::Spacer(const CreateParams& params, MinSize size)
        :
        SpacerBase{ params, size }
    {
        setHorizontalAlign(HorizontalAlign::Left);
        setVerticalAlign(VerticalAlign::Top);
    }

    Spacer::Spacer(const CreateParams& params, float size)
        :
        Spacer{ params, { size, size } }
    {
    }

    Spacer::Spacer(const CreateParams& params, float sizeX, float sizeY)
        :
        Spacer{ params,  { sizeX, sizeY } }
    {
    }
}
