export module ClaFi.Controls.CodeBlock;

import ClaFi.Controls.Base.PanelBase;
import ClaFi.Controls.CodeBox;
import ClaFi.Controls.TextBox;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Code set apart from the prose around it, in a box of its own. See Controls#codeblock
    export class CodeBlock : public WithBody<PanelBase, CodeBox>
    {
    public:
        template<typename... Args>
        explicit CodeBlock(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"CodeBlock"; }
    protected:
        void paintFrame(PaintEvent&) override;
    private:
        using Base = WithBody<PanelBase, CodeBox>;
    };


    //-------------------------------------------------------------------------


    // **The block is the theme's and the box is the code's.** The box stands on the block's
    // surface with no surface of its own, and shows the code until it is told it may be edited.
    template<typename... Args>
    CodeBlock::CodeBlock(const CreateParams& params, Args&&... args)
        :
        Base{
            params,
            HostProps{
                UiElement::CodeBlock,
                std::forward<Args>(args)...
            },
            BodyProps{ ReadOnly::Yes }
        }
    {
    }
}
