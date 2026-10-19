export module ClaFi.Controls.Base.SplitButtonBase;

export import ClaFi.Controls.Base.ButtonBase;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    export enum class SecondaryPartPlacement
    {
        Right,
        Bottom,
        Left
    };

    export class SplitButtonBase : public ButtonBase
    {
    public:
        template <typename... Args>
        explicit SplitButtonBase(const CreateParams&, Args&&...);
    public:
        [[nodiscard]] Control* secondaryPart() const { return m_secondaryPart; }
        [[nodiscard]] Control* shownSecondaryPart() const;
    protected:
        template <typename PartClass, typename... Args>
            requires std::derived_from<PartClass, Control>
        PartClass& createSecondaryPart(Args&&...);

        [[nodiscard]] virtual SecondaryPartPlacement secondaryPartPlacement() const;
        [[nodiscard]] virtual bool secondaryReachesEdge() const { return false; }
        [[nodiscard]] virtual bool secondaryPressPropagates() const { return true; }
        virtual void placeSecondaryPart(ScaledSize childArea);
        virtual void secondaryClicked(NestedClickEvent&);
        [[nodiscard]] bool pressedSecondary(const Control* pressed) const;
        [[nodiscard]] ScaledSize secondaryExtent(const FormContext&, ScaledPadding, ScaledSpacing) const;

        ScaledSize measureText(AlignEvent&, ScaledSize asked, const Text&) override;
        void calculateChildren(FormBase&) override;
        ScaledSize calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledSize&) override;
        void adjustTextRect(AdjustTextRectEvent&) const override;
        FloatRect iconRect(const PaintEvent&) const override;
        void nestedPressDown(PressDownEvent&) override;
        void nestedClick(NestedClickEvent&) override;
    private:
        // Owned by ButtonBase, which holds the one child slot a button has.
        Control* m_secondaryPart{ nullptr };
    };


    //-------------------------------------------------------------------------


    template<typename ...Args>
    SplitButtonBase::SplitButtonBase(const CreateParams& params, Args && ...args)
        :
        ButtonBase{ params, std::forward<Args>(args)... }
    {
    }

    template<typename PartClass, typename ...Args>
        requires std::derived_from<PartClass, Control>
    PartClass& SplitButtonBase::createSecondaryPart(Args&&... args)
    {
        if (m_secondaryPart)
            noReach("SplitButtonBase carries one secondary part, and this control already made one");

        PartClass& result = static_cast<PartClass&>(
            addChild(std::make_unique<PartClass>(CreateParams{ *this }, std::forward<Args>(args)...))
        );
        m_secondaryPart = &result;
        return result;
    }

}
