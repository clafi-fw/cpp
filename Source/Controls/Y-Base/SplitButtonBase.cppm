export module ClaFi.Controls.Base.SplitButtonBase;

export import ClaFi.Controls.Base.ButtonBase;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // SplitButtonBase

    // A button carrying a second target that is pressed on its own account. See Controls-Base
    export class SplitButtonBase : public ButtonBase
    {
    public:
        template <typename... Args>
        explicit SplitButtonBase(const CreateParams&, Args&&...);
    public:
        /// Null on a control that never made one - a tab outside a browser has no close button.
        /// The part whatever its state, which is what a control that shows and hides one holds it
        /// by.
        [[nodiscard]] Control* secondaryPart() const { return m_secondaryPart; }
        /// The part, when there is one and it is shown. A PART THAT IS NOT SHOWN IS NOT THERE: it
        /// takes no room on either axis, nothing is laid out against it, and no press lands on it.
        /// Every reader that asks what the control's face is made of asks this one.
        [[nodiscard]] Control* shownSecondaryPart() const;
    protected:
        /// Creates the part and hands it to ButtonBase to own. A member initializer is the usual
        /// place to call it from: the base subobject is complete by then.
        ///
        /// The part's Interactivity is the caller's to pick, and it settles both how the keyboard
        /// reaches the part and what a press on it does to the container above the host.
        ///
        /// MouseOnly keeps the part off the focus walk in Input::setMouseDown, so a press on it
        /// focuses the host, and the keyboard reaches what the part does through a key on the host
        /// instead - DropdownControlBase answers F4 and Alt+Down that way. That suits a host
        /// standing on its own.
        ///
        /// Focusable makes the part a focus stop in its own right, which is what a host inside an
        /// ActiveContainer needs. The walk stops at the first control that can take focus, so a
        /// part that cannot take it lets the walk climb to the host, and the container picks the
        /// host as its current item. A tab's close button takes focus for exactly that reason: the
        /// strip is then asked about the button rather than the tab, and declines it.
        template <typename PartClass, typename... Args>
            requires std::derived_from<PartClass, Control>
        PartClass& createSecondaryPart(Args&&...);

        [[nodiscard]] virtual SecondaryEdge secondaryEdge() const { return SecondaryEdge::Right; }
        /// Whether the part runs out to the control's own edge, taking over the content padding on
        /// the side it sits on, or stays inside that padding like any other content.
        ///
        /// It also says whether the part is structure or content. A part on the edge is one face
        /// of the control and holds its axis even at zero size; one inside the padding is content,
        /// and content shrunk to nothing takes its spacing with it.
        [[nodiscard]] virtual bool secondaryReachesEdge() const { return false; }
        /// Whether a press on the part goes on to the control and its ancestors. A container above
        /// reads a nested press as "make this item current", which is right for a strip that is
        /// half of the control's face and wrong for a button that removes the control outright.
        [[nodiscard]] virtual bool secondaryPressPropagates() const { return true; }
        /// Puts the part in the box the content has left it, at its own size against the edge it
        /// belongs to and centred on the other axis. Overridden by a control whose part is a strip
        /// rather than a mark, which spans the whole of the cross axis instead.
        ///
        /// The box is in child coordinates, which begin at the child inset rather than at the
        /// control's top left.
        ///
        /// Called only where there is a part and it is shown, so an override may take
        /// shownSecondaryPart() as the part it places.
        virtual void placeSecondaryPart(ScaledDimensions childArea);
        /// What a press on the part does. Called only when there is a part, so a control that
        /// creates one has to answer here.
        virtual void secondaryClicked(ClickEvent&);

        /// Whether the event landed on the part rather than on the control itself.
        [[nodiscard]] bool pressedSecondary(const ClickEventBase&) const;
        /// How much of the content box the part takes, on the axis it sits on.
        [[nodiscard]] ScaledDimensions secondaryExtent(const FormContext&, ScaledPadding, ScaledSpacing) const;

        void calculateChildren(FormBase&) override;
        ScaledDimensions calculateContent(AlignEvent&) override;
        void alignContent(AlignEvent&, ScaledPosition, ScaledDimensions&) override;
        void adjustTextRect(AdjustTextRectEvent&) const override;
        FloatRect iconRect(const PaintEvent&) const override;
        void adjustPaint(AdjustPaintEvent&) override;
        void pressDown(PressDownEvent&) override;
        void click(ClickEvent&) override;
    private:
        // Owned by ButtonBase, which holds the one child slot a button has.
        Control* m_secondaryPart{ nullptr };
    };


    //-------------------------------------------------------------------------


    // SplitButtonBase
    //
    // The constructor and createSecondaryPart() are the definitions that stay here: they are
    // templates, so every caller instantiates them from this interface. Every other body lives in
    // SplitButtonBase.cpp.

    template<typename ...Args>
    SplitButtonBase::SplitButtonBase(const CreateParams& params, Args && ...args)
        :
        // A SPLIT BUTTON IS TWO TARGETS ON ONE FACE, and a surface at rest would draw one box
        // around both of them - the seam between the halves is what has to read, and it reads
        // against the page rather than against a fill. So the surface arrives with the pointer,
        // which is also when there is a half to tell apart.
        //
        // Stated ahead of the caller's own properties, so a split button asked for a surface at
        // rest gets one: Props::get takes the last of the matching arguments.
        ButtonBase{ params, ShowSurfaceAtRest::No, std::forward<Args>(args)... }
    {
    }

    template<typename PartClass, typename ...Args>
        requires std::derived_from<PartClass, Control>
    PartClass& SplitButtonBase::createSecondaryPart(Args&&... args)
    {
        if (m_secondaryPart)
            unreachable("SplitButtonBase carries one secondary part, and this control already made one");

        PartClass& result = static_cast<PartClass&>(
            addChild(std::make_unique<PartClass>(CreateParams{ *this }, std::forward<Args>(args)...))
        );
        m_secondaryPart = &result;
        return result;
    }

} // of namespace ClaFi
