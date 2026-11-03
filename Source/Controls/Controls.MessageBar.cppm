export module ClaFi.Controls.MessageBar;

import ClaFi.Controls.Base.SplitButtonBase;
import ClaFi.Controls.Button;
import ClaFi.Controls.Stack;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.Context.DrawIconEvent;
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
        // The message, beside the icon named, the bar wearing the callout of the icon's kind.
        void setMessage(MessageIcon, const Text& message);
        // Adds an answer after those standing, its button taking the pack after the band's own.
        template<typename Handler, typename... Args>
        Button& addAnswer(std::wstring_view caption, Handler&& onClick, Args&&... args);
    protected:
        void secondaryClicked(NestedClickEvent&) override {} // an answer acts on its own click
        void alignContent(AlignEvent&, ScaledPosition, ScaledSize&) override;
        void drawIcon(DrawIconEvent&) override;
        void adjustPaint(AdjustPaintEvent&) override;
        void paintFrame(PaintEvent&) override;
    private:
        static constexpr float k_iconSize = 20.0f;
        static constexpr float k_answerSpacing = 8.0f;
    private:
        StackPanel& m_answers;
        MessageIcon m_icon{ MessageIcon::None };
    };


//-----------------------------------------------------------------------------


    template<typename... Args>
    MessageBar::MessageBar(const CreateParams& params, Args&&... args)
        :
        SplitButtonBase{
            params,
            MetricsSet::Callout,
            WordWrap::Yes,
            IconSize{ k_iconSize },
            HorizontalTextAnchor::Left,
            VerticalTextAnchor::Center,
            std::forward<Args>(args)...
        },
        m_answers{ createSecondaryPart<StackPanel>(
            Orientation::Horizontal,
            Spacing{ k_answerSpacing },
            VerticalAlign::Center,
            ItemSizing::Equal
        ) }
    {
    }

    template<typename Handler, typename... Args>
    Button& MessageBar::addAnswer(const std::wstring_view caption, Handler&& onClick,
        Args&&... args)
    {
        return m_answers.add<Button>(
            caption,
            VerticalTextAnchor::Center,
            HorizontalTextAnchor::Center,
            OnEvent{ std::forward<Handler>(onClick) },
            std::forward<Args>(args)...
        );
    }
}
