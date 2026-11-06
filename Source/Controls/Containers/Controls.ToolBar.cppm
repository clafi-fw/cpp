export module ClaFi.Controls.ToolBar;

import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A strip of tool buttons, a focus scope of its own that the keys walk. See Item-Containers
    export class ToolBar : public StackPanel
    {
    public:
        template<typename... Args>
        explicit ToolBar(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"ToolBar"; }
        // The shape a held bar is laid on: its rect, with every corner rounded.
        [[nodiscard]] RoundedRectangleParts silhouette(const PaintEvent& hostEvent,
            const FloatRect& barRect) const;
    };


    //----------------------------------------------------------------------------


    template<typename... Args>
    ToolBar::ToolBar(const CreateParams& params, Args&&... args)
        :
        StackPanel{
            params,
            Orientation::Horizontal,
            UiElement::ToolBar,
            FocusScope::Yes,
            NavigationReach::Yes,
            MouseFocus::Self,
            std::forward<Args>(args)...
        }
    {
    }
}
