export module ClaFi.Controls.Button;

export import ClaFi.Controls.Base.ButtonBase;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A button: a face, a click, and whatever icon it is given.
    export class Button : public ButtonBase
    {
    public:
        template <typename... Args>
        explicit Button(const CreateParams&, Args&&...);
        // A button is the thing a press is about, so it is one of the few controls that takes the
        // depth. Inherited by PrimaryButton and ToolButton below, which are a button's colours
        // rather than a different kind of thing.
        [[nodiscard]] bool allowZAnimation() const override { return true; }
    };

    // The button a dialog leads with, drawn as the answer expected.
    export class PrimaryButton : public Button
    {
    public:
        using Button::Button;
    };

    // A button with no surface at rest, for a toolbar.
    export class ToolButton : public Button
    {
    public:
        template<typename... Args>
        explicit ToolButton(const CreateParams&, Args&&...);
    };


    //----------------------------------------------------------------------------


    // Button

    template<typename ...Args>
    Button::Button(const CreateParams& params, Args && ...args)
        :
        ButtonBase{
            params,
            Interactivity::Focusable,
            params.themeMetrics().button,
            UiElement::Button,
            std::forward<Args>(args)...
        }
    {
    }

    // ToolButton

    template<typename ...Args>
    ToolButton::ToolButton(const CreateParams& params, Args && ...args)
        :
        // The surface arrives with the pointer and nothing else moves. Growing it from the
        // inside on hover would read as a double move against PaintEvent::pressScale(), which
        // already scales the whole control.
        //
        // Stated ahead of the caller's own properties, so a tool button asked for a surface at
        // rest gets one: Props::get takes the last of the matching arguments.
        Button{ params, ShowSurfaceAtRest::No, std::forward<Args>(args)... }
    {
    }

}
