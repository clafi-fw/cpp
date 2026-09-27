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
        // The band behind selected text, resolved when asked for: most controls never ask.
        [[nodiscard]] Color selectionRgb() const;
        // The resting band with the accent over it - a caret, a focus ring, a tab's indicator.
        [[nodiscard]] Color indicatorRgb() const;
        // A run's own ink with the band's text rules over it, asked for per span.
        [[nodiscard]] Hsl selectionInkHsl(Hsl runInk) const;
        // Whether a rule reaching the band writes the ink, so a run under it has to be re-inked.
        [[nodiscard]] bool selectionChangesInk() const;
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
        // The inputs of the control drawing this, as its ink reads them. The band reads them.
        RuleInputLevels ruleInputLevels{};
        //
        // TODO: nothing gives this a colour, so a found range would draw in the value below. It
        // belongs with the selection band - the same rules at a different strength, or its own -
        // and the decision is open. Nothing populates hitRanges yet, which is the only reason it
        // does not show.
        Color hit{ 0xff00FFff };
        Color surface;
    private:
        // The band's rules on one channel: its own list, then Any element's.
        void applySelectionRules(PaintChannel, Hsl& color, const RuleInputLevels&) const;
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

    // Raised by the inputs of the control drawing it, which owns the selection. A selection stays
    // behind when the focus leaves, so the band says whether the keys still act on it.
    Color ControlPaintContext::selectionRgb() const
    {
        Hsl result = surfaceHsl;
        applySelectionRules(PaintChannel::Surface, result, ruleInputLevels);
        return disabledRgb(result.toColor());
    }

    // Levels of zero leave only the resting rules, so the control's own state does not reach the
    // caret, the focus ring or a tab's indicator.
    Color ControlPaintContext::indicatorRgb() const
    {
        Hsl result = surfaceHsl;
        applySelectionRules(PaintChannel::Surface, result, RuleInputLevels{});
        m_bakedColors->rule(UiElement::Accent).applyTo(result, 1.0f, lightness);
        return disabledRgb(result.toColor());
    }

    // The rules go over whatever the run already carries, so a grey run stays grey under the band
    // and an accent run stays accent.
    Hsl ControlPaintContext::selectionInkHsl(Hsl runInk) const
    {
        applySelectionRules(PaintChannel::Text, runInk, ruleInputLevels);
        return runInk;
    }

    bool ControlPaintContext::selectionChangesInk() const
    {
        const BakedRules2& rules = m_bakedColors->rules2;
        auto writesInk = [](const BakedColorRule2& rule){
            return rule.output == PaintChannel::Text;
        };
        return std::ranges::any_of(rules.of(UiElement::SelectedText), writesInk)
            or std::ranges::any_of(rules.shared, writesInk);
    }

    // The order PaintEvent takes for the element a control wears. The band's rest is its own, so
    // it is raised in full whether or not the control shows a surface at rest.
    void ControlPaintContext::applySelectionRules(const PaintChannel channel, Hsl& color,
        const RuleInputLevels& levels) const
    {
        const BakedRules2& rules = m_bakedColors->rules2;
        for (const BakedColorRules2* list : { &rules.of(UiElement::SelectedText), &rules.shared })
        {
            for (const BakedColorRule2& rule : *list)
            {
                if (rule.output == channel)
                    rule.effect.applyTo(color, rule.levelIn(levels), lightness);
            }
        }
    }


}
