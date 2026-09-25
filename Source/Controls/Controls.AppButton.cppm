export module ClaFi.Controls.AppButton;

import ClaFi.Controls.Button;
import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export constexpr float k_appIconSize{ 28.0f };

    // The button an application puts its own mark on. See Controls
    export class AppButton : public ToolButton
    {
    public:
        template <typename... Args>
        explicit AppButton(const CreateParams&, Args&&...);
    };
}


//-----------------------------------------------------------------------------


namespace ClaFi::Controls
{
    // PRESENTS appMenuAction, so every application has its menu under this button with nothing
    // written: ApplicationBase connects the action to the menu once, and a click here runs it.
    template <typename... Args>
    AppButton::AppButton(const CreateParams& params, Args&&... args)
        :
        ToolButton{
            params,
            ButtonViewMode::IconOnly,
            IconSize{ k_appIconSize },
            ShowSelectionOnSurface::Yes, // Shows when the backstage is dropped down
            appMenuAction(),
            std::forward<Args>(args)...
        }
    {
    }
}
