export module ClaFi.Controls.Label;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Text drawn where it stands, taking no click.
    export class Label : public ButtonBase
    {
    public:
        template<typename... Args>
        Label(const CreateParams& params, Args&&...);
    };


    //-------------------------------------------------------------------------


    template<typename ...Args>
    Label::Label(const CreateParams& params, Args&&... args)
        :
        ButtonBase{ params, std::forward<Args>(args)...}
    {
    }
}
