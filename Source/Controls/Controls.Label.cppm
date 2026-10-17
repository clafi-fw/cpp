export module ClaFi.Controls.Label;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A text label.
    export class Label : public ButtonBase
    {
    public:
        template<typename... Args>
        Label(const CreateParams& params, Args&&...);
    };

    export class LabelWithTextLayout : public WithTextLayout<Label>
    {
        using WithTextLayout::WithTextLayout;
    };

    //-------------------------------------------------------------------------


    template<typename ...Args>
    Label::Label(const CreateParams& params, Args&&... args)
        :
        ButtonBase{ params, std::forward<Args>(args)...}
    {
    }
}
