export module ClaFi.Controls.MessageBar;

import ClaFi.Controls.Base.SplitButtonBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // A question about what a page shows, standing in the page as a band. See Controls#messagebar
    export class MessageBar : public SplitButtonBase
    {
    public:
        template<typename... Args>
        explicit MessageBar(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] std::wstring_view diagnosticText() const override { return L"MessageBar"; }
        // The message, beside the icon named and a stripe in that icon's colour.
        void setMessage(MessageIcon, const Text& message);
        // Adds an answer under the caller's own caption, after those already standing.
        template<typename Handler>
        Button& addAnswer(std::wstring_view caption, Handler&& onClick);
    protected:
        void secondaryClicked(ClickEvent&) override {} // an answer acts on its own click
        void paintIcon(PaintIconEvent&) override;
        void adjustPaint(AdjustPaintEvent&) override;
        void paintSurface(PaintEvent&) override;
    private:
        static constexpr float k_paddingX = 16.0f;
        static constexpr float k_paddingY = 12.0f;
        static constexpr float k_spacing = 12.0f; // between the icon, the message and the answers
        static constexpr float k_iconSize = 20.0f;
        static constexpr float k_stripeWidth = 4.0f;
        static constexpr float k_answerSpacing = 8.0f;
    private:
        Stack& m_answers;
        MessageIcon m_icon{ MessageIcon::None };
    };


//-----------------------------------------------------------------------------


    template<typename... Args>
    MessageBar::MessageBar(const CreateParams& params, Args&&... args)
        :
        SplitButtonBase{
            params,
            params.themeMetrics().page,
            Interactivity::None,
            WordWrap::Yes,
            IconSize{ k_iconSize },
            HorizontalTextAnchor::Left,
            VerticalTextAnchor::Center,
            Padding{ k_paddingX, k_paddingY },
            Spacing{ k_spacing },
            std::forward<Args>(args)...
        },
        m_answers{ createSecondaryPart<Stack>(
            Orientation::Horizontal,
            Spacing{ k_answerSpacing },
            VerticalAlign::Center
        ) }
    {
    }

    template<typename Handler>
    Button& MessageBar::addAnswer(const std::wstring_view caption, Handler&& onClick)
    {
        return m_answers.add<Button>(
            caption,
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            OnEvent{ std::forward<Handler>(onClick) }
        );
    }
}
