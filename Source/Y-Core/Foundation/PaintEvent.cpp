module ClaFi.Core.Foundation;

import :PaintEvent;
import :Form;
import :Control;
import :Traversal;

import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Metrics;
import ClaFi.Core.AppTheme_Theme;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.Utils;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.InkWell;
import ClaFi.StdLib;

namespace ClaFi
{
    // How far the shadow under a held header reaches out from each of its edges. It doubles as
    // the distance the header has to be carried before the shadow is at full strength, so one
    // number says how deep it is and how long it takes to arrive.
    static constexpr float k_heldShadowReach = 8.0f;
    // The shadow at full strength. A drop shadow is the absence of light rather than a colour the
    // theme names, so it is black in every theme and only its opacity is in play.
    static constexpr float k_heldShadowOpacity = 0.28f;
    // PaintEvent

    PaintEvent::PaintEvent(TraversalContext& context)
        :
        ControlEventBase{
            context.parent() ?
                static_cast<PaintEvent*>(context.parent()->visitorData())->formContext()
                :
                context.formContext()
        },
        m_context{ context },
        m_enteredTheme{ formContext().themePtrRef() },
        m_enteredBakedColors{ formContext().bakedColorsPtrRef() },
        m_parentEvent{
            context.parent() ? static_cast<PaintEvent*>(context.parent()->visitorData()) : nullptr
        },
        m_interactivity{ context.control().interactivity() },
        m_strokeOverContent{ m_parentEvent == nullptr }
    {
        if (m_parentEvent)
        {
            m_overlayHost = m_parentEvent->m_overlayHost;
            m_overlayStage = m_parentEvent->m_overlayStage;
            m_pressScaleApplied = m_parentEvent->m_pressScaleApplied;
        }

        context.setVisitorData(this);

        Control& control = context.control();
        control.doPaintInitialization();
        // After the initialization, which is where a control first asks for its own state:
        // until then every factor reads as its default, and enabled defaults to on. Taken
        // before it, a control that has never been enabled paints its first frame live and
        // only settles when something else invalidates it - a pointer crossing the window.
        m_enabledFactor = control.enabledFactor() * (m_parentEvent ? m_parentEvent->m_enabledFactor : 1.0f);
        if (m_parentEvent)
            m_windowFocusedFactor = m_parentEvent->m_windowFocusedFactor;
        else if (const FormBase* form = control.getForm())
            m_windowFocusedFactor = form->windowFocusedFactor();

        // Metrics
        {
            ControlMetrics metrics;
            AdjustMetricsEvent event{ formContext(), control, metrics };
            control.doAdjustMetrics(event);
            m_borderWidth = scaler().scaledStrokeWidth(metrics.border);
            m_radius = scaleF(metrics.radius);
            m_spacing = scale(metrics.spacing);
            // Metrics say how much depth this control would take; the control says whether it takes
            // any. Read here rather than in pressScale(), so that everything downstream of the
            // scale - the transform, the text raster mode - sees one answer.
            m_zDepthFactor = control.allowZAnimation() ? metrics.zDepthFactor : 0.0f;
        }

        // Before the adjustment, so what the control or its parent states there has the last word.
        inheritCornerRadii();

        // The lightness this control stands at, inherited before it adjusts anything, so a
        // control reading it during the adjustment is answered with what the chain handed it.
        m_lightness = m_parentEvent ? m_parentEvent->m_lightness : bakedColors().lightness;

        // Adjust Colors
        {
            AdjustPaintEvent adjustEvent{ *this };
            control.doAdjustPaint(adjustEvent);
        }

        Hsl ink{ inheritedTextHsl() };

        // Read once the rules are settled: doAdjustPaint is where a control picks the element it
        // wears. A control that named its lightness itself has already answered the question.
        if (!m_lightnessStated and m_colorRules.flip != 0.0f)
        {
            m_lightness = m_colorRules.lightnessIn(m_lightness);
            // The ink crosses with the element. What was inherited stands on the side this
            // element has just left - a light theme's black ink over a strip that is now dark -
            // and an element whose text rule says nothing would be left holding it.
            //
            // Carried across rather than re-seeded from the theme, so how far an ancestor had
            // already moved the ink off its own limit is kept, and so a second flip further down
            // puts the ink back exactly where it started.
            ink.luminosity += m_colorRules.flip * (1.0f - 2.0f * ink.luminosity);
        }

        // Calculate final colors
        {
            StateFactors factors = control.factors();
            const Control* lender = lendingParent();

            // HOW FAR THE ENABLED FACTOR GATES A STATE. Hovered and pressed are things a pointer
            // is doing to a control, and a control that cannot be acted on is having none of them
            // done to it. Selected is what the data says, and goes on being true of a control that
            // cannot be acted on - which is how the check and the dot have always read it, so the
            // fill reads it the same way. What disabled still takes off it is the blend towards
            // the surface behind, which every colour here goes through.
            auto stateEnabledFactor = [](const VisualStateIndex index, const float enabledFactor){
                return index == VisualStateIndex::Selected ? 1.0f : enabledFactor;
            };

            auto calcEffectiveFactor = [&](const BakedRule&, VisualStateIndex index, float parentAmount)->float
                {
                    float effectiveFactor = factors[index] * stateEnabledFactor(index, m_enabledFactor);
                    if (parentAmount && lender)
                    {
                        const StateFactors& parentFactors = lender->factors();

                        float pf = parentFactors[index] * stateEnabledFactor(index, parentFactors.enabled());
                        effectiveFactor = effectiveFactor * (1.0f - parentAmount) + (pf * parentAmount);
                    }
                    return effectiveFactor;
                };

            float hoverEffectiveFactor = calcEffectiveFactor(m_colorRules.hovered, VisualStateIndex::Hovered, m_parentHoverAmount);
            // How far this control is the one in effect: its own selected state, or its window's
            // focus as far as setWindowSelectedAmount asked. The ink reads it whatever the surface
            // does with it: an element that paints no selected fill still states its ink through
            // activeText.
            const float selectedFactor = std::lerp(
                calcEffectiveFactor(m_colorRules.active, VisualStateIndex::Selected,
                    m_parentSelectedAmount),
                m_windowFocusedFactor,
                m_windowSelectedAmount);
            // At zero the active rule reaches neither the fill, nor the grow-in that
            // m_surfaceVisibility drives, which is the whole of what a surface says.
            float selectedEffectiveFactor = m_showSelectionOnSurface ? selectedFactor : 0.0f;

            float baseFactor = m_showSurfaceAtRest
                ? 1.0f
                : StateFactors::compose(
                    hoverEffectiveFactor,
                    selectedEffectiveFactor,
                    m_parentHoverAmount * (lender ? lender->factors().hoveredOrSelected() : 0.0f));

            // WHAT THE CONTROL STANDS ON. The host's answer where it gave one, because the
            // parent chain cannot: a grid cell is filled by the row, so a control hosted in a
            // coloured cell stands on the cell and not on the row - see
            // AdjustPaintEvent::setSurfaceHsl.
            //
            // Kept, because it is needed twice and stops being available in between: it is where
            // this control's own surface starts, and it is what a disabled colour fades toward.
            // Neither the parent's surface nor m_controlContext.surfaceHsl can be read for the
            // second - the parent's is the row rather than the cell, and the context's is written
            // over with this control's own surface before the border is resolved.
            const Hsl standsOn{ inheritedSurfaceHsl() };
            Hsl surface{ standsOn };

            float bgChanged2{ 0.0f };
            if (m_showSurfaceAtRest)
                bgChanged2 = m_colorRules.surface.applyTo(surface, baseFactor, m_lightness);

            // Every rule this control applies runs in the one direction the control stands in,
            // the surface and text rules that establish the element included. An element that
            // states a flip is read wholly from the far side of the theme - its surface, its ink,
            // its states, its border and everything it contains - so the gap the theme author
            // wrote between a surface and the ink on it survives the crossing.

            // Active
            bgChanged2 = StateFactors::compose(bgChanged2,
                m_colorRules.active.applyTo(surface, selectedEffectiveFactor, m_lightness));
            // Hover
            bgChanged2 = StateFactors::compose(bgChanged2,
                m_colorRules.hovered.applyTo(surface, hoverEffectiveFactor, m_lightness));
            // Pressed
            float effectivePressedFactor = calcEffectiveFactor(m_colorRules.pressed, VisualStateIndex::Pressed, m_parentPressedAmount);
            bgChanged2 = StateFactors::compose(bgChanged2,
                m_colorRules.pressed.applyTo(surface, effectivePressedFactor, m_lightness));

            // Kept from the same three numbers the colours were mixed from, so a surface cannot
            // be at full opacity while still at rest size, whatever combination of its own state
            // and its parent's put it there.
            m_surfaceVisibility = StateFactors::compose(
                hoverEffectiveFactor,
                selectedEffectiveFactor,
                effectivePressedFactor
            );

            m_surfaceRgb = surface.toColor();
            m_surfaceRgb = disabledFormOf(m_surfaceRgb, standsOn, disabledAmount());
            // Taken before the alpha is applied, so it is the colour that actually shows here -
            // the surface has already accumulated down the parent chain, and a control with no
            // surface at rest contributes nothing to it at rest.
            m_controlContext.surface = m_surfaceRgb;
            m_surfaceRgb.setOpacity(bgChanged2);

            // Text Color
            {
                // The control drawing a selection is the one that owns it, so the band is taken
                // by this control's own focused factor, as far as its window holds the focus.
                // Handed over rather than resolved: the band and the caret are two colours out of
                // the many a context can answer with, and almost nothing asks for either.
                m_controlContext.focusedFactor = factors.focused() * m_windowFocusedFactor;

                // The ink arrives from the control above, or from a host that named one, so only
                // this control's own rule is applied to it - at full strength, with no state term
                // and no part in the grow-in. What a control states as its text is what everything
                // inside it starts from, whatever state either of them is in.
                m_colorRules.text.applyTo(ink, 1.0f, m_lightness);

                // What being in effect does to that ink. Applied after the text rule because it
                // modifies what that rule established, and at the same factor the active rule is
                // taken at, so the two move together.
                m_colorRules.activeText.applyTo(ink, selectedFactor, m_lightness);

                // The pair the resolver measures between: an elevation of 0 lands on the surface
                // and 1 lands as far off as the ink itself. Set before the first ink is asked
                // for, since asking reads them.
                m_controlContext.surfaceHsl = surface;
                m_controlContext.textHsl = ink;
                m_controlContext.lightness = m_lightness;
            }


            if (m_strokeRule)
            {
                // Applied to this control's own surface, which is what setStrokeRule promises.
                Hsl newStrokeHsl = surface;
                float borderChanged = m_strokeRule->applyTo(newStrokeHsl, baseFactor, m_lightness);

                m_strokeRgb = newStrokeHsl.toColor();
                m_strokeRgb = disabledFormOf(m_strokeRgb, standsOn, disabledAmount());
                m_strokeRgb.setOpacity(borderChanged);
            }
            else if (!m_colorRules.stroke.changesNothing())
            {
                Hsl newStrokeHsl = surface;
                // Which of the two surfaces stands higher is asked in the direction this control
                // stands in, because that is the direction the border is about to be raised in.
                // The two surfaces can lie on opposite sides of the theme - a card that states a
                // flip against the page it is on - and higher then means higher to this control.
                const float sign = 1.0f - 2.0f * m_lightness;
                if (m_parentEvent && m_parentEvent->surfaceHsl().luminosity * sign > surface.luminosity * sign)
                    newStrokeHsl = m_parentEvent->surfaceHsl();

                // Raised along this control's own contrast: a border has to be seen against the
                // surface it is drawn on, which is the same thing the ink answers to.
                float borderChanged = m_colorRules.stroke.applyTo(newStrokeHsl, baseFactor,
                    m_lightness);

                m_strokeRgb = newStrokeHsl.toColor();
                m_strokeRgb = disabledFormOf(m_strokeRgb, standsOn, disabledAmount());
                m_strokeRgb.setOpacity(borderChanged);
            }

            // The colours held as colours rather than resolved on demand. What the context
            // resolves for itself carries the same fade, toward the same surface, as it is built -
            // see ControlPaintContext::disabledRgb - so an ink asked for while painting matches
            // one that was ready.
            m_controlContext.hit = disabledFormOf(m_controlContext.hit);

            // Recorded after the fade rather than before it, so the inks resolved above are faded
            // once. What is resolved from here on has it applied as it is built, toward the same
            // surface, so an ink asked for while painting matches one that was ready.
            m_controlContext.disabledAmount = disabledAmount();

            //bool alwaysShowFocus = false;
            // The current item of a container wears the same ring, but it is not gated on
            // the keyboard: it marks where a Shift range would grow from, and that has to
            // stay readable while the mouse is what drives the selection.
            //
            // Whether there is a ring and what colour it is are two questions, so two terms.
            const float focusHolderFactor = factors.focused() * s_keyboardFactor;
            const float activeFactor = activeFactorOf(factors.hovered(), factors.focused())
                * m_windowFocusedFactor;
            float effectiveFocusFactor = std::max(focusHolderFactor, factors.current());
                switch (m_interactivity)
                {
                case Interactivity::MouseOnly:
                case Interactivity::Focusable:
                case Interactivity::ActiveContainer:
                    if (effectiveFocusFactor)
                    {
                        m_focusRingFactor = effectiveFocusFactor;
                        applyFocus2(m_strokeRgb, effectiveFocusFactor, activeFactor);
                        if (effectiveFocusFactor)
                        {
                            m_borderWidth = effectiveFocusFactor
                                * (scaledStrokeWidth(ThemeMetrics::focusedBorder) - m_borderWidth) + m_borderWidth;
                        }
                    }
                    break;
                case Interactivity::None:
                    break;
                }
        }
    }

    // A CONTROL SHOWING A THEME OF ITS OWN SHOWS IT TO WHAT IT CONTAINS AND TO NOTHING ELSE.
    // The context that carries the theme belongs to the FORM, and this event's life is exactly
    // this control's subtree - its children are painted inside its own paint() - so putting the
    // theme back here is what bounds the reset. Left standing, it would carry that theme to every
    // control painted after this one, and to every frame after this one: nothing else in the tree
    // ever writes that pointer, so the form would stop reading the application's theme at all.
    PaintEvent::~PaintEvent()
    {
        formContext().themePtrRef() = m_enteredTheme;
        formContext().bakedColorsPtrRef() = m_enteredBakedColors;
    }

    const StateFactors& PaintEvent::factors() const
    {
        return m_context.control().factors();
    }

    float PaintEvent::hoveredFactor() const
    {
        return m_context.control().hoveredFactor();
    }

    float PaintEvent::selectedFactor() const
    {
        return m_context.control().selectedFactor();
    }

    float PaintEvent::pressedFactor() const
    {
        return m_context.control().pressedFactor();
    }

    float PaintEvent::enabledFactor() const
    {
        return m_enabledFactor;
    }

    float PaintEvent::focusedFactor() const
    {
        return m_context.control().focusedFactor();
    }

    float PaintEvent::pressScale() const
    {
        if (!m_zDepthFactor)
            return 1.0f;
        const ThemeMetrics& metrics = themeMetrics();
        // Rest to full size as the pointer arrives, then in again while it is held. Reading the
        // two steps in that order is what keeps a keyboard press - held but never hovered - from
        // growing the control on its way down.
        float result = std::lerp(metrics.pressRestScale, 1.0f, zHoveredFactor());
        result = std::lerp(result, metrics.pressHeldScale, zPressedFactor());
        return std::lerp(1.0f, result, m_zDepthFactor);
    }

    float PaintEvent::zHoveredFactor() const
    {
        return zAnimationFactor(VisualStateIndex::Hovered);
    }

    float PaintEvent::zPressedFactor() const
    {
        return zAnimationFactor(VisualStateIndex::Pressed);
    }

    // A control showing another theme starts that theme afresh, so the surface, the ink and the
    // mode all come from the theme now in force rather than from the chain above. The surface and
    // the ink are stated, which is what carries them past the seed the calculation takes from the
    // parent; the mode is a field of its own and no seed reaches it.
    void PaintEvent::resetTheme()
    {
        // Stated on the context as well as on the chain. The context was built with the theme the
        // walk arrived in, and this runs during doAdjustPaint, which is after - so without this
        // the control's surface and ink would come from the theme it named while every ink it
        // resolved while painting came from the one it left.
        m_controlContext.setBakedColors(bakedColors());
        m_controlContext.surfaceHsl = bakedColors().formSurface();
        m_surfaceHslStated = true;
        m_controlContext.textHsl = bakedColors().formText();
        m_textHslStated = true;
        m_lightness = bakedColors().lightness;
    }

    // The ring's colour says the control is the one the user is on, and which control that is
    // depends on the device: the focus under the keyboard, the pointer under the mouse. So the
    // live share of the ring is taken from whichever factor the device in use speaks through,
    // crossfaded by s_keyboardFactor along with everything else the device switch moves.
    //
    // At either end the other term is weighted out entirely, which is what keeps the old rule:
    // a ring the keyboard put on a control does not change colour because the pointer moved off
    // it or left the window. What changed is the mouse end - a container's current item used to
    // stay the inactive grey no matter what the pointer did, so under the mouse nothing said
    // where the user was.
    float PaintEvent::activeFactorOf(float hoveredFactor, float focusedFactor)
    {
        return std::lerp(hoveredFactor, focusedFactor, s_keyboardFactor);
    }

    // ringFactor says there is a ring at all; activeFactor how much of it is drawn live, in
    // indicatorRgb(), rather than in the inactive grey.
    void PaintEvent::applyFocus2(Color& targetColor, float ringFactor, float activeFactor) const
    {
        if (ringFactor <= 0.0f)
            return;
        Color inactiveRingColor = targetColor;
        inactiveRingColor.alpha = 255;
        inactiveRingColor.blend(m_controlContext.textRgb(InkGrade::Strongest), ringFactor);
        targetColor.blend(inactiveRingColor, ringFactor);
        targetColor.blend(indicatorRgb(), activeFactor);
    }

    void PaintEvent::applyFocus2(Color& targetColor) const
    {
        const Control& target = control();
        const float focusHolderFactor = target.focusedFactor() * s_keyboardFactor;
        applyFocus2(
            targetColor,
            std::max(focusHolderFactor, target.currentFactor()),
            activeFactorOf(target.hoveredFactor(), target.focusedFactor()) * m_windowFocusedFactor
        );
    }

    float PaintEvent::disabledAmount() const
    {
        // with m_disabledBlendAmount = 1.0 a disabled control disappears completely
        return (1.0f - m_enabledFactor) * m_disablingStrength;
    }

    // An ink this control draws, on its own surface.
    Color PaintEvent::disabledFormOf(Color color) const
    {
        return disabledFormOf(color, surfaceHsl(), disabledAmount());
    }

    // The same, at an amount the caller states. For a colour that is not being painted now:
    // a cached bitmap is written once and kept until it is dropped, so it takes the settled
    // answer rather than whatever the fade is passing through this frame.
    Color PaintEvent::disabledFormOf(Color color, float amount) const
    {
        return disabledFormOf(color, surfaceHsl(), amount);
    }

    // What a colour becomes on a control that cannot be used: itself, moved toward the surface
    // it is drawn on by the amount given. Which surface that is, is the whole of it - a control's
    // own surface answers to what is behind it, and its inks to the surface. Measured
    // against itself a colour has nowhere to go, and the fade comes out as a change of nothing.
    //
    // The alpha is the caller's and is put back untouched. Color::blend carries alpha with it,
    // and the surface is opaque, so left to itself the blend would drag a faint ink up toward
    // solid - and a fade is a colour, never an opacity.
    Color PaintEvent::disabledFormOf(Color color, const Hsl& surface, float amount) const
    {
        if (!amount)
            return color;

        Color result = color;
        result.blend(surface.toColor(), amount);
        result.alpha = color.alpha;
        return result;
    }

    void PaintEvent::applyDisabledFactorTo(Color& color) const
    {
        color = disabledFormOf(color);
    }

    Color PaintEvent::applyDisabledFactor(Color c) const
    {
        applyDisabledFactorTo(c);
        return c;
    }

    void PaintEvent::paintChildren() const
    {
        m_context.traverseChildren();
    }

    // A hidden control is not painted, wherever the call comes from. traverseChildren tests this
    // on its own walk, and a hand-written paint order - a tab strip's hover stack, an expander's
    // held header - REPLACES that walk rather than extending it, so the test belongs here, which
    // is the one point all of them pass through. Layout already skips a hidden control, so one
    // painted anyway is drawn over whatever took its place.
    void PaintEvent::paintChild(Control& control) const
    {
        if (!control.visible())
            return;

        TraversalContext childContext{ m_context, control };
        childContext.traverse();
    }

    void PaintEvent::defaultPaintSurface(FloatPoint inset)
    {
        const SurfaceShape shape = surfaceShape(inset);
        if (shape.rect.empty())
            return;
        paintFill(shape);
        if (m_strokeOverContent)
        {
            m_strokeHeld = true;
            m_heldStrokeInset = inset;
            return;
        }
        paintStroke(shape);
    }

    // Every event on the way to the root carries it, because which one reads it is not the
    // raiser's to know: the reader is whichever ancestor's pass ends first with a use for it, and
    // a control's own event is gone by then. An event is built fresh for each control of each
    // pass, so nothing here outlives the frame that raised it.
    void PaintEvent::raisePinnedTop(float value)
    {
        for (PaintEvent* event = this; event; event = event->m_parentEvent)
            event->m_pinnedTop = std::max(event->m_pinnedTop, value);
    }

    // The shadow first and the backdrop over it: a shadow is measured from the shape's edge and
    // reaches inward as much as out, and the backdrop is what covers the half that falls inside.
    void PaintEvent::paintHeldBackdrop(const RoundedRectangleParts& silhouette, float travel)
    {
        const float reach = scaleF(k_heldShadowReach);
        if (reach > 0.0f)
        {
            const float strength = std::min(travel / reach, 1.0f);
            const Color black = { 0, 0, 0 };
            const Graphics::ShadowParams shadow = {
                .color = black.withOpacity(k_heldShadowOpacity * strength),
                .blur = reach,
                .falloff = Graphics::GlowFalloff::Smooth
            };
            canvas().castShadow(silhouette, shadow);
        }
        canvas().fillPartialRoundedRectangle(silhouette, surfaceRgb().withOpacity(1.0f));
    }

    // Blended against the parent's own factors, not against the amounts adjustChildPaint weights
    // colour by. Those are chosen to keep a child legible against a parent that is merely lit, which
    // is a different question from how far it should travel.
    float PaintEvent::zAnimationFactor(VisualStateIndex index) const
    {
        const StateFactors& own = factors();
        float result = own[index] * m_enabledFactor;
        if (m_parentZAmount)
        {
            if (const Control* lender = lendingParent())
            {
                const StateFactors& parentFactors = lender->factors();
                float parentValue = parentFactors[index] * parentFactors.enabled();
                result = std::lerp(result, parentValue, m_parentZAmount);
            }
        }
        return result;
    }

    const Control* PaintEvent::lendingParent() const
    {
        const TraversalContext* parent = m_context.parent();
        if (!parent || parent->control().interactivity() == Interactivity::None)
            return nullptr;
        return &parent->control();
    }

    Hsl PaintEvent::inheritedSurfaceHsl() const
    {
        if (m_surfaceHslStated)
            return m_controlContext.surfaceHsl;
        if (m_parentEvent)
            return m_parentEvent->surfaceHsl();
        return bakedColors().formSurface();
    }

    Hsl PaintEvent::inheritedTextHsl() const
    {
        if (m_textHslStated)
            return m_controlContext.textHsl;
        if (m_parentEvent)
            return m_parentEvent->textHsl();
        return bakedColors().formText();
    }

    // An overlay control is held against its host rather than its parent: the host is what clips
    // it and what it is drawn over, so a grid's header held at the top of the view takes the
    // view's corner and not the grid's.
    const PaintEvent* PaintEvent::containerEvent() const
    {
        if (m_overlayHost && m_overlayHost->hasInOverlayControls(m_context.control()))
        {
            for (const PaintEvent* event = m_parentEvent; event; event = event->m_parentEvent)
            {
                if (&event->m_context.control() == m_overlayHost)
                    return event;
            }
        }
        return m_parentEvent;
    }

    bool PaintEvent::isListedByEnclosingHost(const Control& control) const
    {
        for (const PaintEvent* event = m_parentEvent; event; event = event->m_parentEvent)
        {
            if (!event->m_overlayStage && event->m_overlayHost && event->m_overlayHost->hasInOverlayControls(control))
                return true;
        }
        return false;
    }

    // WHICH CORNERS A CONTROL ROUNDS. Its own radius goes on the corners of its bounds, and a
    // corner the viewport cuts off is not one of them, so it is square. Each corner is then held
    // against the corner of its container it stands at: where this control's arc - a square
    // corner at any inset under 0.29 of the container's radius, a rounder one at less - would
    // reach past the container's inner arc, the corner takes the radius concentric with the
    // container's, which is the container's radius less this control's inset. Concentric is the
    // smallest radius that stays inside for a control standing right at the container's border,
    // and the natural one further in. A corner that stays inside keeps its own radius.
    //
    // Viewports rather than bounds, so a control the scroll has cut at the container's corner
    // takes the arc on its cut edge, and a control whose corner is scrolled out of the container's
    // takes nothing there. A container's radius is read off its own event, so what it took from
    // its own container reaches down through it.
    void PaintEvent::inheritCornerRadii()
    {
        const FloatRect bounds = controlBounds();
        const FloatRect visible = viewport();
        using CornerPoints = std::array<FloatPoint, k_cornersNum>;
        const CornerPoints ownCorners = { bounds.topLeft(), bounds.topRight(), bounds.bottomRight(), bounds.bottomLeft() };
        const CornerPoints corners = { visible.topLeft(), visible.topRight(), visible.bottomRight(), visible.bottomLeft() };
        for (std::size_t i = 0; i < k_cornersNum; ++i)
            m_cornerRadii[i] = corners[i] == ownCorners[i] ? m_radius : 0.0f;

        const PaintEvent* container = containerEvent();
        if (!container)
            return;

        const FloatRect containerRect = container->viewport();
        const CornerPoints containerCorners = { containerRect.topLeft(), containerRect.topRight(), containerRect.bottomRight(), containerRect.bottomLeft() };
        const float containerStroke = container->m_borderWidth;

        // The direction each corner's interior lies in, so every corner is worked in one frame:
        // both axes running inward from the container's corner point.
        struct Inward
        {
            float x;
            float y;
        };
        constexpr std::array<Inward, k_cornersNum> inward = { { { 1.0f, 1.0f }, { -1.0f, 1.0f }, { -1.0f, -1.0f }, { 1.0f, -1.0f } } };

        for (std::size_t i = 0; i < k_cornersNum; ++i)
        {
            const float containerRadius = container->m_cornerRadii[i];
            if (containerRadius <= 0.0f)
                continue;
            const float insetX = (corners[i].x - containerCorners[i].x) * inward[i].x;
            const float insetY = (corners[i].y - containerCorners[i].y) * inward[i].y;
            if (insetX < 0.0f || insetY < 0.0f)
                continue;
            const float concentric = containerRadius - std::max(insetX, insetY);
            if (concentric <= 0.0f)
                continue;

            // This corner's arc runs between (insetX, insetY + own) and (insetX + own, insetY),
            // bulging toward the container's corner point. Its farthest point from the
            // container's arc centre lies along the line through the two centres while this
            // centre is nearer the corner on both axes, and at one of the two ends otherwise.
            const float own = m_cornerRadii[i];
            const float centreX = insetX + own - containerRadius;
            const float centreY = insetY + own - containerRadius;
            float reach = 0.0f;
            if (centreX <= 0.0f && centreY <= 0.0f)
            {
                reach = std::sqrt(centreX * centreX + centreY * centreY) + own;
            }
            else
            {
                const float startX = insetX - containerRadius;
                const float startY = centreY;
                const float endX = centreX;
                const float endY = insetY - containerRadius;
                reach = std::sqrt(std::max(startX * startX + startY * startY, endX * endX + endY * endY));
            }
            if (reach <= containerRadius - containerStroke)
                continue;
            m_cornerRadii[i] = std::max(own, concentric);
        }
    }

    void PaintEvent::paint()
    {
        FloatRect clip = m_context.controlClipRect();
        if (clip.empty())
            return;

        Control& control = m_context.control();

        bool isOverlayControl = m_overlayHost && m_overlayHost->hasInOverlayControls(control);
        // Held on the event rather than kept here, so a paintChildren override can ask it. The
        // stages below run paintChildren whichever way this lands.
        //
        // The overlay stage paints what its host lists and nothing else. The standard stage
        // paints what no host lists - the host whose stage this is, and any host further out
        // whose standard stage this one runs inside: a grid hosting its expanders' headers runs
        // both of its passes inside its parent's standard pass, and the grid's own header, which
        // that parent lists, waits for the parent's overlay pass.
        m_paintsSelf = m_overlayStage ?
            isOverlayControl
            :
            !isOverlayControl && !isListedByEnclosingHost(control);

        // --- RESOLVE CLIPPING MODE ---
        bool shouldClip = false;
        if (!m_overlayStage)
        {
            shouldClip = true;
        }
        else if (isOverlayControl)
        {
            ClippingMode clipMode = m_overlayHost->getOverlayClippingMode(control);
            if (clipMode == ClippingMode::Standard)
            {
                shouldClip = true;
            }
        }

        if (shouldClip)
            canvas().pushClip(clip);

        {
            // The one place the press animation is applied. It goes on after the clip, so the
            // control stays clipped to the bounds it actually occupies, and it wraps the
            // surface, the children and the text alike - which is what makes the whole button
            // move as one piece about a single origin. A control inside one that is already
            // scaled leaves it alone, so the origin is always the outermost animating control.
            Graphics::Matrix3x2 pressTransform = Graphics::Matrix3x2::identity();
            if (!m_pressScaleApplied)
            {
                if (float scale = pressScale(); scale != 1.0f)
                {
                    FloatPoint origin = control.pressOrigin(*this);
                    pressTransform = Graphics::Matrix3x2::translation(origin)
                        * Graphics::Matrix3x2::scale(scale)
                        * Graphics::Matrix3x2::translation(-origin.x, -origin.y);
                    m_pressScaleApplied = true;
                }
            }
            Graphics::ScopedCanvasTransform pressScaleTransform{ canvas(), pressTransform };

            if (m_paintsSelf)
                control.doPaintSurface(*this);
            if (m_contentClip)
                canvas().pushClip(*m_contentClip);
            {
                // --- ROBUST STAGE ENGINE ---
                // If this control has overlays and we are not currently inside an active overlay stage,
                // manage the standard and overlay loops directly in the paint loop.
                bool hasOverlays = control.hasOverlayControls();


                const Control* savedHost = m_overlayHost;
                bool savedStage = m_overlayStage;

                if (hasOverlays && !m_overlayStage)
                {
                    // 1. Run Standard Pass
                    m_overlayHost = &control;
                    m_overlayStage = false;
                    control.paintChildren(*this);
                    // 2. Run Overlay Pass
                    m_overlayStage = true;
                }
                else
                {
                    if (isOverlayControl && m_overlayStage)
                    {
                        m_overlayHost = nullptr;
                        m_overlayStage = false;
                    }
                }
                control.paintChildren(*this);
                m_overlayHost = savedHost;
                m_overlayStage = savedStage;
            }
            if (m_contentClip)
                canvas().popClip();
            if (m_paintsSelf)
            {
                control.doPaintTextInitialization();
                control.paintText(*this);
                control.doPainted(*this);
                if (m_strokeHeld)
                    paintStroke(surfaceShape(m_heldStrokeInset));
            }
        }

        if (shouldClip)
            canvas().popClip();
    }

    PaintEvent::SurfaceShape PaintEvent::surfaceShape(FloatPoint inset)
    {
        FloatRect bounds = controlBounds();
        bounds.inflate(-scaleF(inset));
        // The surface is the part of the control in view, and the outline runs along the edges
        // that are the control's own: a cut edge carries no line.
        FloatRect rect = FloatRect::intersection(bounds, viewport());
        if (rect.empty())
            return {};
        const RectSidesBoolArray sides = {
            rect.top == bounds.top,
            rect.right == bounds.right,
            rect.bottom == bounds.bottom,
            rect.left == bounds.left
        };
        CornerRadii radii = m_cornerRadii;

        // Only the fill and its stroke grow in - the icon and the caption are drawn elsewhere and
        // stay where they are, so this reads as a surface arriving behind the content rather
        // than as the control itself resizing. That is also why it is here and not in pressScale():
        // the press animation deliberately moves the whole control as one piece.
        //
        // Driven by m_surfaceVisibility rather than by this control's own factors, because a
        // child with no surface at rest can be lit entirely by its parent: an indicator set to
        // appear on hover is shown by the button's hover, never by its own, and reading its raw
        // factors would leave it stuck at rest size for as long as it was on screen.
        if (!m_showSurfaceAtRest)
        {
            float scale = std::lerp(themeMetrics().surfaceGrowInScale, 1.0f, m_surfaceVisibility);
            // The focus ring is drawn with this same rect, and it marks the control's frame
            // rather than its surface, so it pulls the shape back out to full size. A current
            // item that is not hovered has next to no surface to grow in, and without this the
            // ring lands at the rest scale - shrunken, and gathered toward whatever pressOrigin()
            // anchors on instead of sitting on the bounds. The fill follows along, which shows
            // nowhere: at rest it has no alpha, and once it does the grow-in has finished anyway.
            scale = std::lerp(scale, 1.0f, m_focusRingFactor);
            if (scale != 1.0f)
            {
                // The same origin the press animation uses, so a control that anchors on its icon
                // has its surface come out of that icon rather than out of its own middle.
                FloatPoint origin = m_context.control().pressOrigin(*this);
                rect = {
                    std::lerp(origin.x, rect.left, scale),
                    std::lerp(origin.y, rect.top, scale),
                    std::lerp(origin.x, rect.right, scale),
                    std::lerp(origin.y, rect.bottom, scale)
                };
                // Kept proportional, so the shape that grows in is the shape that settles.
                for (float& radius : radii)
                    radius *= scale;
            }
        }
        return { rect, radii, sides };
    }

    void PaintEvent::paintFill(const SurfaceShape& shape)
    {
        const Color color = surfaceRgb();
        if (!color.alpha)
            return;
        const RoundedRectangleParts parts{
            .bounds = shape.rect,
            .radii = shape.radii,
            .sides = k_allRectSidesTrue
        };
        canvas().fillPartialRoundedRectangle(parts, color);
    }

    // No inset here. A stroke lies inside the bounds it is given - see IBackend - so the border's
    // outer edge is the same edge the fill covers, and nothing of the fill shows past it.
    void PaintEvent::paintStroke(const SurfaceShape& shape)
    {
        const Color color = strokeRgb();
        const float strokeWidth = borderWidth();
        if (!color.alpha || !strokeWidth)
            return;
        const RoundedRectangleParts parts{
            .bounds = shape.rect,
            .radii = shape.radii,
            .sides = shape.sides
        };
        canvas().drawPartialRoundedRectangle(parts, color, strokeWidth);
    }

    // AdjustPaintEvent

    AdjustPaintEvent::AdjustPaintEvent(PaintEvent& target)
        :
        ControlEventBase{ target.formContext() },
        m_target{ target }
    {
    }

    void AdjustPaintEvent::resetTheme(const AppTheme& theme, const BakedColors& bakedColors)
    {
        m_target.formContext().themePtrRef() = &theme;
        m_target.formContext().bakedColorsPtrRef() = &bakedColors;
        m_target.resetTheme();
    }

    void AdjustPaintEvent::setColorRules(UiElement value)
    {
        m_target.m_colorRules = m_target.bakedColors().element(value);
    }

    // Out of line: the control is only declared where this event is, and reading a factor
    // off it needs the whole class.
    void AdjustPaintEvent::dropInheritedEnabled()
    {
        m_target.m_enabledFactor = m_target.control().enabledFactor();
    }

    void AdjustPaintEvent::setTextHsl(Hsl value)
    {
        m_target.m_controlContext.textHsl = value;
        m_target.m_textHslStated = true;
    }

    void AdjustPaintEvent::setLightness(Lightness value)
    {
        m_target.m_lightness = value;
        m_target.m_lightnessStated = true;
    }

    void AdjustPaintEvent::setSurfaceHsl(Hsl value)
    {
        m_target.m_controlContext.surfaceHsl = value;
        m_target.m_surfaceHslStated = true;
    }

    // ControlTreePainter

    void ControlTreePainter::paint(Control& rootControl, const FloatRect* systemClipRect, const Graphics::PixelPath* contentClip)
    {
        ControlTreeWalker treeWalker{ rootControl, TraversalMode::Manual, systemClipRect };
        treeWalker.traverse([contentClip](TraversalContext& context) {
            PaintEvent paintEvent{ context };
            if (!context.parent())
                paintEvent.m_contentClip = contentClip;
            paintEvent.paint();
            });
    }
}
