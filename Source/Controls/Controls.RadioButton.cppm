export module ClaFi.Controls.RadioButton;

import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.Foundation;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A button drawn with a radio mark, one of a set.
    export class RadioButton : public ButtonBase
    {
    public:
        template<typename... Args>
        RadioButton(const CreateParams&, Args&&...);
    };


    //-------------------------------------------------------------------------


    template<typename ...Args>
    RadioButton::RadioButton(const CreateParams& params, Args&&... args)
        :
        ButtonBase{
            params,
            Interactivity::Focusable,
            IndicatorVisibility::Always,
            IndicatorStyle::Radio,
            params.themeMetrics().listItem, std::forward<Args>(args)... }
    {
    }
}
