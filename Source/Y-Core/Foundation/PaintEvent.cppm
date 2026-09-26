export module ClaFi.Core.Foundation :PaintEvent;

import :Traversal;

import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.Context.FormContext;

import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;

namespace ClaFi
{
    export class PaintEvent;
    export class AdjustPaintEvent;
    export class ControlTreePainter;

    // A control painting itself, and everything it needs to draw with.
    export class PaintEvent : public ControlEventBase
    {
        friend ControlTreePainter;
        friend AdjustPaintEvent;
    public:
        // How far the keyboard has taken over from the mouse: 0 while the pointer drives, 1
        // once a key has. The focus ring is drawn through it, so it fades rather than snaps.
        inline static float s_keyboardFactor{};
    public:
        explicit PaintEvent(TraversalContext&);
        ~PaintEvent();
    public:
        PaintEvent* parentEvent() const { return m_parentEvent; }
        Control& control() { return m_context.control(); }
        const Control& control() const { return m_context.control(); }
        const StateFactors& factors() const;
        float hoveredFactor() const;
        float selectedFactor() const;
        float pressedFactor() const;
        float enabledFactor() const;
        float focusedFactor() const;
        // How far the window this control is painted in holds the focus. See Control-Foundation
        [[nodiscard]] float windowFocusedFactor() const { return m_windowFocusedFactor; }

        const BakedElement& colorRules() const { return m_colorRules; }

        // TODO: move disabledBlendAmount into ColorRules, so an element states its own.
        float disabledBlendAmount() const { return m_disablingStrength; }

        FloatRect viewport() const { return m_context.viewport(); }
        FloatRect controlBounds() const { return m_context.controlBounds(); }
        float left() const { return m_context.left(); }
        float top() const { return m_context.top(); }
        float right() const { return m_context.right(); }
        float bottom() const { return m_context.bottom(); }
        float width() const { return m_context.width(); }
        float height() const { return m_context.height(); }
        FloatPoint topLeft() const { return m_context.topLeft(); }
        FloatPoint center() const { return m_context.center(); }
        FloatRect centerRect(float x, float y) const { return m_context.centerRect(x, y); }
        FloatRect centerRect(float size) const { return m_context.centerRect(size); }
        float borderWidth() const { return m_borderWidth; }
        // The control's own radius, as a length. The corners it paints are cornerRadii(): a
        // corner a parent's arc reaches wears a radius concentric with the parent's, and a corner
        // the viewport cuts off is square - see inheritCornerRadii.
        float radius() const { return m_radius; }
        const CornerRadii& cornerRadii() const { return m_cornerRadii; }
        ScaledPadding padding() const { return m_context.padding(); }
        // Where this control's children start, which is what their own coordinates are relative to.
        FloatPoint contentPosition() const { return m_context.contentPosition(); }
        ScaledSpacing spacing() const { return m_spacing; }
        // The single factor everything this control draws is scaled by, about the control's own
        // centre. Surface, text, icon, indicator and any child all inherit it from one
        // transform, so no element animates its size on its own account.
        [[nodiscard]] float pressScale() const;
        // The hover and press that drive the depth, this control's own blended with its parent's by
        // the amount the parent asked for. See AdjustPaintEvent::setParentZAmount.
        [[nodiscard]] float zHoveredFactor() const;
        [[nodiscard]] float zPressedFactor() const;
        // The lightness this control stands at: the theme's own at the form root, inherited by
        // every control from the one above it, and carried across where the element it wears
        // states that it stands on the far side of the theme - see BakedElement::flip. A control
        // that knows better states it outright through AdjustPaintEvent::setLightness, and a
        // stated lightness is not crossed again by the rule set that control happens to wear.
        //
        // It is an input to the rules and never a result of them: what the theme says about an
        // element cannot move the direction that element is read in.
        [[nodiscard]] Lightness lightness() const { return m_lightness; }
        [[nodiscard]] bool pressScaleApplied() const { return m_pressScaleApplied; }
        void resetTheme();
        Hsl surfaceHsl() const { return m_controlContext.surfaceHsl; }
        // The ink this control carries, before any of the derived tones in textColor(). It is
        // inherited from the control above and changed only by this control's own text rule.
        Hsl textHsl() const { return m_controlContext.textHsl; }
        Color surfaceRgb() const { return m_surfaceRgb; }
        Color strokeRgb() const { return m_strokeRgb; }
        // What every check mark, radio mark and caret is filled with.
        Color indicatorRgb() const { return m_controlContext.indicatorRgb(); }
        ControlPaintContext& controlContext() { return m_controlContext; }
        const ControlPaintContext& controlContext() const { return m_controlContext; }
        Color textRgb(InkGrade grade) const { return inkRgb(InkWell::textInk(grade)); }
        Color accentRgb(InkGrade grade) const { return inkRgb(InkWell::accentInk(grade)); }
        Color spotRgb(InkGrade grade) const { return inkRgb(InkWell::spotInk(grade)); }
        // The colour a pigment names at an elevation, on this control's surface. Every ink this
        // control draws text or an icon in comes from here: a pigment says which of the theme's
        // colours, an elevation says how far it stands from the surface it is drawn on.
        [[nodiscard]] Color inkRgb(const Ink& ink) const { return m_controlContext.inkRgb(ink); }
        [[nodiscard]] Color inkRgb(Pigment pigment, InkTone tone) const { return m_controlContext.inkRgb(pigment, tone); }
        [[nodiscard]] Color inkRgb(Pigment pigment, float saturation, float elevation) const { return m_controlContext.inkRgb(pigment, saturation, elevation); }
        // How much of a ring is drawn live rather than in the inactive grey: the focused
        // factor once the keyboard has taken over, the hovered one while the pointer drives,
        // crossfaded between them.
        [[nodiscard]] static float activeFactorOf(float hoveredFactor, float focusedFactor);
        // ringFactor is how much ring there is; activeFactor how much of it is live. See the
        // definition.
        void applyFocus2(Color& targetColor, float ringFactor, float activeFactor) const;
        void applyFocus2(Color& targetColor) const;
        // How far every ink is blended toward the backdrop because the control is not fully
        // enabled. Exposed because a PaintIconEvent has to be given it: the colours it carries
        // have already had it applied, so an icon drawing in colours of its own cannot work it
        // out from them.
        [[nodiscard]] float disabledAmount() const;
        // What a colour becomes on a control that cannot be used: see the definition.
        [[nodiscard]] Color disabledFormOf(Color) const;
        [[nodiscard]] Color disabledFormOf(Color, float amount) const;
        [[nodiscard]] Color disabledFormOf(Color, const Hsl& surface, float amount) const;
        void applyDisabledFactorTo(Color&) const;
        Color applyDisabledFactor(Color) const;
        void paintChildren() const;
        void paintChild(Control&) const;
        // The inset is stated here rather than left on the event for this call to find. It is a
        // design value and is scaled on the way in.
        void defaultPaintSurface(FloatPoint inset = {});
        bool overlayStage() const { return m_overlayStage; }
        const Control* overlayHost() const { return m_overlayHost; }
        // Whether this is the stage that paints the control itself. The surface, the text and
        // painted() are already held to it, so only a paintChildren override drawing content of
        // its own has to ask: paintChildren is called in every stage, because an overlay control
        // nested under one that paints in neither still has to be reached.
        //
        // A control that is its own overlay host needs overlayStage() as well. Its children are
        // painted twice inside one paint(), and this answer is the same for both of those passes.
        [[nodiscard]] bool paintsSelf() const { return m_paintsSelf; }
        // How much of the top of the view a control has taken and holds against the scroll - a
        // grid keeping its header at the viewport edge states the part of that edge the header
        // covers. Raised while the strip is painted and read once the pass over the content is
        // over, so the control stating it may sit at any depth under the one reading it.
        //
        // What it is for: a fade that says there is more content above marks where the content
        // passes out of sight, and under a held strip that is the strip's inner edge rather than
        // the view's own. Drawn over the strip it would fade the strip instead.
        void raisePinnedTop(float value);
        [[nodiscard]] float pinnedTop() const { return m_pinnedTop; }
        // WHAT GOES UNDER A HEADER HELD OVER THE CONTENT: its shadow, and an opaque backdrop of
        // this control's surface. Drawn by the container holding the header, from its own event,
        // because the header is clipped to its own rect and neither of the two stays within it:
        // the shadow rings the header and falls outside, and the backdrop covers what the
        // header's own paint leaves open - the notch outside a rounded corner, or the whole rect
        // of a header that paints no surface at all - which would otherwise show the content
        // passing underneath.
        //
        // `travel` is how far the header has been carried past where it was laid out, and sets
        // the shadow's strength the way the distance a bar has travelled sets a scroll box's
        // edge fade: the shadow arrives over the first stretch of the lift rather than switching
        // on the moment the header leaves its place. See Control::heldHeader.
        void paintHeldBackdrop(const RoundedRectangleParts& silhouette, float travel);
    private:
        // The surface as it is painted: the part of the control in view, the corners it turns and
        // the sides that are its own edges, at the grow-in scale.
        struct SurfaceShape
        {
            FloatRect rect{};
            CornerRadii radii{};
            RectSidesBoolArray sides{};
        };
    private:
        [[nodiscard]] float zAnimationFactor(VisualStateIndex) const;
        // The parent this control may take state from, which is not always the one above it.
        // Control::isHovered walks up from whatever the pointer is on, so every ancestor reports
        // hovered - a scroll box is hovered by its own body, a form by anything at all. A control
        // that is not interactive has no state of its own in the first place, so there is nothing
        // there for a child to blend with or animate toward. Returns null in that case.
        [[nodiscard]] const Control* lendingParent() const;
        // The surface this control stands on before anything of its own is applied to it: what a
        // host has stated for it, or what the chain hands down. The layer carries only what a
        // host stated, and the colour calculation is what puts the finished surface in it - so
        // this is the one answer for both the adjustment, which runs before that calculation, and
        // the calculation itself.
        [[nodiscard]] Hsl inheritedSurfaceHsl() const;
        // The ink this control starts from, before its own text rule is applied to it: what a
        // host has stated for it, or what the chain hands down. Held the way the surface is, and
        // answered for both phases for the same reason.
        [[nodiscard]] Hsl inheritedTextHsl() const;
        // The event of the control this one stands in: the parent's, except for an overlay
        // control, which stands on its host - see the definition.
        [[nodiscard]] const PaintEvent* containerEvent() const;
        // Whether a host further out, in its standard stage at the moment, lists this control - a
        // control that host's overlay stage is going to paint. See paint.
        [[nodiscard]] bool isListedByEnclosingHost(const Control&) const;
        void inheritCornerRadii();
        void paint();
        [[nodiscard]] SurfaceShape surfaceShape(FloatPoint inset);
        void paintFill(const SurfaceShape&);
        void paintStroke(const SurfaceShape&);
    private:
        TraversalContext& m_context;
        // The theme and the baked set the walk arrived at this control in, and what the
        // destructor puts back.
        const AppTheme* m_enteredTheme;
        const BakedColors* m_enteredBakedColors;
        // Built from this event's own theme, not the traversal's: a control showing another theme
        // swaps the one its event carries, and everything inside it copies the swapped one. A rule
        // ink and a pigment ink are read straight off this theme, so bound to the traversal's it
        // would draw the application's spot and accent inside a preview of another theme.
        ControlPaintContext m_controlContext{ formContext(), bakedColors() };
        PaintEvent* m_parentEvent;
        const Control* m_overlayHost{};
        bool m_overlayStage{};
        bool m_paintsSelf{};
        float m_pinnedTop{ 0.0f };
    private:
        BakedElement m_colorRules{};
        float m_parentHoverAmount{ 0.0f };
        float m_parentSelectedAmount{ 0.0f };
        float m_parentPressedAmount{ 0.0f };
        float m_windowSelectedAmount{ 0.0f };
        // Zero by default: a control's depth answers to its own state unless its host says
        // otherwise, so nothing starts out moving on someone else's behalf.
        float m_parentZAmount{ 0.0f };
        float m_disablingStrength{ 0.77f };
        float m_enabledFactor{ 1.0f };
        float m_windowFocusedFactor{ 1.0f };
        bool m_showSurfaceAtRest{ true };
        bool m_showSelectionOnSurface{ true };
        float m_surfaceVisibility{ 0.0f };
        Interactivity m_interactivity;
        // Set for the control that owns the press animation, and inherited by everything it
        // contains, so a child does not scale a second time about its own centre.
        bool m_pressScaleApplied{ false };
        // Metrics
        float m_zDepthFactor{ 0.0f };
        // How much of the stroke is a focus ring rather than the surface's own border.
        // A ring marks the control's frame, so it does not take part in the grow-in.
        float m_focusRingFactor{ 0.0f };
        float m_borderWidth;
        // Null unless setStrokeRule named one, in which case it replaces m_colorRules.stroke.
        const BakedRule* m_strokeRule{};
        float m_radius{};
        CornerRadii m_cornerRadii{};
        ScaledSpacing m_spacing;
        // A ROOT'S RING IS DRAWN OVER ITS CONTENT. Its children stand right inside the ring and
        // turn its corners, so a stroke drawn under them would have its inner edge painted over
        // along every arc. The stroke is held until the content is painted, and the content is
        // painted under the clip a corner pass hands the root, so that clip cuts what stands
        // inside the ring and never the ring's own edge.
        const Graphics::PixelPath* m_contentClip{};
        bool m_strokeOverContent{};
        bool m_strokeHeld{};
        FloatPoint m_heldStrokeInset{};
        // Colors
        // The lightness every rule and every ink on this control is read at. Filled from the
        // parent event before the control adjusts its paint, so a control asking for it during
        // that adjustment is answered with what it inherited.
        Lightness m_lightness{ k_darkLightness };
        // Whether the control named the lightness itself. A control that did is not carried across
        // a second time by the element it wears: a selected tab standing at its page's lightness
        // wears the button's rules, and the button's flip is not the tab's business.
        bool m_lightnessStated{ false };
        // Whether the host named the surface this control stands on. What it named is held in
        // the layer's surfaceHsl until the colours are calculated, which is where the parent
        // chain would otherwise seed it from.
        bool m_surfaceHslStated{ false };
        // Whether the host named the ink this control starts from. Held the way the surface is,
        // and read by the same seed.
        bool m_textHslStated{ false };
        //
        Color m_surfaceRgb;
        Color m_strokeRgb{};
    };

    // The paint being set up, before anything is drawn.
    export class AdjustPaintEvent : public ControlEventBase
    {
    public:
        AdjustPaintEvent(PaintEvent& target);
        Control& control() { return m_target.control(); }
    public:
        void resetTheme(const AppTheme& theme, const BakedColors& bakedColors);
        BakedElement& colorRules() { return m_target.m_colorRules; }
        // The element this control paints from, resolved against the set the pass carries.
        void setColorRules(UiElement);
        void setColorRules(const BakedElement& value) { m_target.m_colorRules = value; }
        // Keeps the control off its own surface until its state calls for one - the surface rule
        // states the colour it arrives at, and nothing states one at rest. See
        // ButtonBase's ShowSurfaceAtRest.
        void dropSurfaceAtRest() { m_target.m_showSurfaceAtRest = false; }
        // Keeps the selected state off the surface - the control is still selected, and its
        // indicator, text and focus ring still say so. See ButtonBase's ShowSelectionOnSurface.
        void dropSelectedSurface() { m_target.m_showSelectionOnSurface = false; }
        // The control's own enabled factor is the whole answer, with nothing taken from the
        // control around it. For a part that stands for a command other than its owner's -
        // see DropdownPart, which makes this same division for the state walk, and whose own
        // factor already carries what the owner stands on.
        void dropInheritedEnabled();
        void setParentHoverAmount(float value) { m_target.m_parentHoverAmount = value; }
        void setParentSelectedAmount(float value) { m_target.m_parentSelectedAmount = value; }
        void setParentPressedAmount(float value) { m_target.m_parentPressedAmount = value; }
        // How much of the parent's hover and press drive this child's depth. Separate from the three
        // above, which weight colour: a mark can be lit partly on its parent's account and still
        // have to move entirely on its own, or the other way about.
        //
        // 1 is a child that stands for its parent - a check box mark, where the pointer is over the
        // row and the click is forwarded on, so the child's own factors barely move. Anything less
        // is a child that is also clickable in its own right, which has its own hover to show and
        // should keep a share of the movement for it.
        void setParentZAmount(float value) { m_target.m_parentZAmount = value; }
        // How much of the selected factor is the window's focus rather than the control's own.
        void setWindowSelectedAmount(float value) { m_target.m_windowSelectedAmount = value; }
        void setDisabledBlendAmount(float value) { m_target.m_disablingStrength = value; }
        // The ink this control has inherited, before its own text rule is applied to it: what a
        // host has named for it, or what the chain hands down. Read and written where a control's
        // ink has to follow its own state, which no rule can say: a rule is applied at full
        // strength so that what a panel states reaches everything inside it. A tab taking on its
        // page's ink as it is selected is the case this exists for, and it does the blend itself,
        // by the same factor its surface takes the page's colour.
        [[nodiscard]] Hsl textHsl() const { return m_target.inheritedTextHsl(); }
        void setTextHsl(Hsl value);
        // The lightness inherited from the control above, before this control has said anything
        // about it. Read by a control that has to state its rules in the direction they will be
        // applied in - see Tab::adjustPaint, which spells an exact colour into two of them.
        [[nodiscard]] Lightness lightness() const { return m_target.m_lightness; }
        // The lightness this control stands at, where the control knows it and the parent chain
        // does not. A selected tab wears the page it opens, so it takes the page's lightness
        // rather than the strip's. Stating it also settles it: the element this control wears
        // cannot carry it across afterwards.
        void setLightness(Lightness value);
        // The surface this control stands on, as it reaches the control: what a host has named
        // for it, or what the chain hands down. A host mixing a tint of its own applies it to
        // this and states the result through setSurfaceHsl.
        [[nodiscard]] Hsl surfaceHsl() const { return m_target.inheritedSurfaceHsl(); }
        // The surface this control stands on, where its host knows better than the parent
        // chain does. A grid cell is filled by the row rather than by a control of its own,
        // so a control hosted in a coloured cell would otherwise stand on the row's colour -
        // see RowBase::adjustChildPaint. Everything downstream answers to this: the ink is
        // read against it, the border is drawn on it, and every fade converges on it.
        void setSurfaceHsl(Hsl value);
        // The event being adjusted, for a caller that has to hand it on to something taking a
        // finished one - a Column naming its colour rule. Const on purpose: the colours have
        // not been calculated yet, and what is adjusted here is adjusted through the setters.
        [[nodiscard]] const PaintEvent& paintEvent() const { return m_target; }
        // The radii of the corners the control paints, in Corner order. A corner a parent's arc
        // reaches has taken the parent's radius by the time this is called, and what is set here
        // has the last word.
        [[nodiscard]] const CornerRadii& cornerRadii() const { return m_target.m_cornerRadii; }
        void setCornerRadii(const CornerRadii& value) { m_target.m_cornerRadii = value; }
        void setCornerRadius(Corner corner, float value) { m_target.m_cornerRadii[cornerIndex(corner)] = value; }
        void setInteractivity(Interactivity value) { m_target.m_interactivity = value; }
        void setBorderWidth(float value) { m_target.m_borderWidth = value; }
        // Draws the border from this rule applied to the control's own surface, in place of the
        // element's stroke, which is applied to the higher of that surface and the one the
        // control stands on. The rule is held by address and must outlive the paint; a member of
        // BakedColors does.
        void setStrokeRule(const BakedRule& value) { m_target.m_strokeRule = &value; }
    private:
        PaintEvent& m_target;
    };

    export class ControlTreePainter
    {
    public:
        // contentClip, where given, is the clip the root's content is painted under; the root's
        // own surface and its ring are painted outside it.
        static void paint(Control&, const FloatRect* systemClipRect, const Graphics::PixelPath* contentClip = nullptr);
    };

    //-------------------------------------------------------------------------

}
