export module ClaFi.Core.Context.ControlContext;

import ClaFi.Core.Context.FormContext;
import ClaFi.Core.AppTheme_Baked;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    export class ControlPaintContext
    {
    public:
        explicit ControlPaintContext(FormContext&, const BakedColors& bakedColors);
        FormContext& formContext() { return m_formContext; }
        const FormContext& formContext() const { return m_formContext; }
        const BakedColors& bakedColors() const { return *m_bakedColors; }
        // THE THEME THIS CONTEXT RESOLVES IN, which a control showing a theme of its own states
        // after the context was built - doAdjustPaint runs after the event's members - so the
        // control that states it resolves its own inks in it and not only its children. See
        // PaintEvent::resetTheme
        void setBakedColors(const BakedColors& value) { m_bakedColors = &value; }
        Graphics::Canvas& canvas() { return m_formContext.canvas(); }
        const Graphics::Canvas& canvas() const { return m_formContext.canvas(); }
        float scaleFactor() const { return m_formContext.scaleFactor(); }
        float scaledStrokeWidth(Thickness thickness) const { return m_formContext.scaledStrokeWidth(thickness); }
        float scaleF(float value) const { return m_formContext.scaleF(value); }
        //
        [[nodiscard]] Color textRgb(InkGrade grade) const { return inkRgb(InkWell::textInk(grade)); }
        [[nodiscard]] Color accentRgb(InkGrade grade) const { return inkRgb(InkWell::accentInk(grade)); }
        [[nodiscard]] Color spotRgb(InkGrade grade) const { return inkRgb(InkWell::spotInk(grade)); }
        [[nodiscard]] Color surfaceRgb() const { return surfaceHsl.toColor(); }
        [[nodiscard]] Hsl inkHsl(const Ink&) const;
        [[nodiscard]] Color inkRgb(const Ink&) const;
        [[nodiscard]] Color fadedInk(Hsl) const;
        [[nodiscard]] Color inkRgb(Pigment, float saturation, float elevation) const;
        [[nodiscard]] Color inkRgb(Pigment, InkTone) const;
        //
        // The band behind selected text, and the colour every check mark, radio mark and caret is
        // filled with. Resolved when they are asked for rather than with everything else: most
        // controls ask for neither.
        [[nodiscard]] Color selectionRgb() const;
        [[nodiscard]] Color indicatorRgb() const;
        // The ink a run of selected text is drawn in: the colour that run already carries, with
        // the band's own text rule over it. Asked for per span, where the run's colour is known.
        [[nodiscard]] Hsl selectionInkHsl(Hsl runInk) const;
        // The mode an element applied by hand stands in: this control's, crossed once if the
        // element states a flip. PaintEvent makes that crossing for the element a control wears,
        // and the band behind selected text is not that element - it is raised over whatever
        // surface the text sits on - so the places applying it by hand make the crossing here. See
        // BakedElement::flip.
        [[nodiscard]] Lightness elementLightness(const BakedElement& element) const
        {
            return element.lightnessIn(lightness);
        }
        // What a colour becomes on a control that cannot be used: itself, moved toward the surface
        // it is drawn on. The alpha is the caller's and is put back untouched - Color::blend
        // carries alpha with it and the surface is opaque, so left to itself the blend would drag
        // a faint mark up toward solid, and a fade is a colour and never an opacity.
        [[nodiscard]] Color disabledRgb(Color color) const
        {
            if (!disabledAmount)
                return color;

            Color result = color;
            result.blend(surfaceHsl.toColor(), disabledAmount);
            result.alpha = color.alpha;
            return result;
        }
        //
        Lightness lightness{ k_darkLightness };
        Hsl surfaceHsl{};
        Hsl textHsl{};
        float disabledAmount{ 0.0f };
        // How far the control drawing this holds the keyboard focus. A selection stays behind when
        // the focus leaves, so the band has to say whether the keys still act on it, and the
        // factor is animated so the two bands crossfade rather than swap.
        float focusedFactor{ 0.0f };
        //
        // TODO: nothing gives this a colour, so a found range would draw in the value below. It
        // belongs with the selection band - the same rules at a different strength, or its own -
        // and the decision is open. Nothing populates hitRanges yet, which is the only reason it
        // does not show.
        Color hit{ 0xff00FFff };
        Color surface;
    private:
        FormContext& m_formContext;
        const BakedColors* m_bakedColors;
    };


    //-------------------------------------------------------------------------


    ControlPaintContext::ControlPaintContext(FormContext& formContext, const BakedColors& bakedColors)
        :
        m_formContext{ formContext },
        m_bakedColors{ &bakedColors }
    {
        
    }

    Hsl ControlPaintContext::inkHsl(const Ink& ink) const
    {
        // The far end of the mix: a semantic colour's hue, pure black or white, the theme's accent
        // or spot rule over the chain's ink, or the chain's ink itself. The rule goes on before the
        // mix, since one that states where it lands would overwrite the grade.
        Hsl inkEnd = textHsl;
        const std::optional<Pigment> pigment = pigmentOf(ink.color);
        if (pigment)
            inkEnd = m_bakedColors->pigmentHsl(*pigment, lightness);
        else if (ink.color == InkColor::Black)
            inkEnd = { 0.0f, 0.0f, 0.0f };
        else if (ink.color == InkColor::White)
            inkEnd = { 0.0f, 0.0f, 1.0f };
        else if (ink.color == InkColor::Accent or ink.color == InkColor::Spot)
            m_bakedColors->ruleOf(ink.color).applyTo(inkEnd, 1.0f, lightness);

        // Both ends of the ladder are held here, so they are answered rather than mixed to. A mix
        // at either end returns what is already in hand, and returns it through a pair of trig
        // conversions that do not come back bit for bit - and the renderer compares the ink at
        // the far end for equality to decide whether a run needs a drawing effect at all. The two
        // constants are exact: gradeOf(Strongest) is 1.0f and surfaceInk() is 0.0f, so a grade
        // that means an end lands on it.
        Hsl result;
        if (ink.grade == 1.0f)
            result = inkEnd;
        else if (ink.grade == 0.0f)
            result = surfaceHsl;
        else
            result = Hsl{ surfaceHsl, inkEnd, ink.grade };

        // The fade a control that cannot be used calls for, taken in the colour model like every
        // other move here: the ink slides toward the surface it stands on.
        if (disabledAmount)
            return Hsl{ result, surfaceHsl, disabledAmount };

        return result;
    }

    Color ControlPaintContext::inkRgb(const Ink& ink) const
    {
        return inkHsl(ink).toColor();
    }

    Color ControlPaintContext::fadedInk(Hsl hsl) const
    {
        if (disabledAmount)
            return Hsl{ hsl, surfaceHsl, disabledAmount }.toColor();
        return hsl.toColor();
    }

    Color ControlPaintContext::inkRgb(Pigment pigment, float saturation, float elevation) const
    {
        const InkTone tone{
            saturation,
            luminosityOf(elevation, lightness)
        };
        return fadedInk(m_bakedColors->pigmentHsl(pigment, tone));
    }

    Color ControlPaintContext::inkRgb(Pigment pigment, InkTone tone) const
    {
        return fadedInk(m_bakedColors->pigmentHsl(pigment, tone));
    }

    // Raised off the control's own surface, in the direction the element stands in - the
    // control's own, crossed once where the theme states a flip on the band.
    Color ControlPaintContext::selectionRgb() const
    {
        const BakedElement& band = m_bakedColors->element(UiElement::SelectedText);
        const Lightness elementLightness = this->elementLightness(band);
        Hsl result = surfaceHsl;
        band.surface.applyTo(result, 1.0f, elementLightness);
        band.active.applyTo(result, focusedFactor, elementLightness);
        return disabledRgb(result.toColor());
    }

    // The resting band with the accent over it, so a caret, every check and radio mark, the ring
    // on the control the user is on and a tab's indicator are drawn from one statement. The two
    // rules are read in different directions on purpose: the band is an element and is read in the
    // direction that element stands in - reading it at this control's side would put a band drawn
    // here on the far side from one drawn by a control wearing it - while the accent belongs to no
    // element and is read in this control's own.
    Color ControlPaintContext::indicatorRgb() const
    {
        const BakedElement& band = m_bakedColors->element(UiElement::SelectedText);
        Hsl result = surfaceHsl;
        band.surface.applyTo(result, 1.0f, elementLightness(band));
        m_bakedColors->rule(UiElement::Accent).applyTo(result, 1.0f, lightness);
        return disabledRgb(result.toColor());
    }

    // The ink over the band. A run carries the ink of the side the control stands on, so on a
    // flipped band it crosses with the element before the band's own text rule is applied - the
    // order PaintEvent takes for a control wearing a flipped element. Without the crossing a run
    // whose text rule says nothing keeps the ink of the side the band has just left, and no rule
    // in the theme says anything is wrong.
    Hsl ControlPaintContext::selectionInkHsl(Hsl runInk) const
    {
        const BakedElement& band = m_bakedColors->element(UiElement::SelectedText);
        runInk.luminosity += band.flip * (1.0f - 2.0f * runInk.luminosity);
        band.text.applyTo(runInk, 1.0f, elementLightness(band));
        return runInk;
    }


}
