export module ClaFi.Controls.LabeledDivider;

import ClaFi.Controls.Panel;
import ClaFi.Controls.Divider;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A line with a word in it, across whatever it stands in.
    export class LabeledDivider : public Panel
    {
    public:
        template<typename... Args>
        LabeledDivider(const CreateParams&, Args&&...);
    private:
        Divider& m_separator{ createBody<Divider>(
            VerticalAlign::Center
        ) };
    };


    //-------------------------------------------------------------------------


    template<typename... Args>
    LabeledDivider::LabeledDivider(const CreateParams& params, Args&&... args)
        :
        Panel{ params, TextPlacement::Left, Spacing{ 4.0f }, std::forward<Args>(args)... }
    {
    }
}
