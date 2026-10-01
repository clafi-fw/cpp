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
        // The band behind text a search has found, resolved when asked for.
        [[nodiscard]] Color foundTextRgb() const;
        // The selection band raised over the found one, where a match is selected.
        [[nodiscard]] Color selectedFoundRgb() const;
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
        Color surface;
    private:
        // A band's rules on one channel: each element's own list in turn, then Any element's.
        void applyBandRules(std::initializer_list<UiElement>, PaintChannel, Hsl& color,
            const RuleInputLevels&) const;
        // The surface in FoundText's seed hue, where the found band starts.
        [[nodiscard]] Hsl foundSeedHsl() const;
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
            m_bakedColors->effectOf(ink.color).applyTo(inkEnd, 1.0f, lightness);

        // A light surface reads the steps from the ink's end - see lightGradeOf - and an element
        // standing between the sides stands between the two readings.
        const float grade = std::lerp(ink.grade,
            lightGradeOf(ink.grade, m_bakedColors->darkModeFloor), lightness);

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
            result = Hsl{ surfaceHsl, inkEnd, grade };

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
        applyBandRules({ UiElement::SelectedText }, PaintChannel::Surface, result, ruleInputLevels);
        return disabledRgb(result.toColor());
    }

    // Levels of zero leave only the resting rules, so the control's own state does not reach the
    // caret, the focus ring or a tab's indicator.
    Color ControlPaintContext::indicatorRgb() const
    {
        Hsl result = surfaceHsl;
        applyBandRules({ UiElement::SelectedText }, PaintChannel::Surface, result,
            RuleInputLevels{});
        m_bakedColors->effect(UiElement::Accent).applyTo(result, 1.0f, lightness);
        return disabledRgb(result.toColor());
    }

    // The rules go over whatever the run already carries, so a grey run stays grey under the band
    // and an accent run stays accent.
    Hsl ControlPaintContext::selectionInkHsl(Hsl runInk) const
    {
        applyBandRules({ UiElement::SelectedText }, PaintChannel::Text, runInk, ruleInputLevels);
        return runInk;
    }

    bool ControlPaintContext::selectionChangesInk() const
    {
        const BakedRules& rules = m_bakedColors->rules;
        auto writesInk = [](const BakedColorRule& rule){
            return rule.output == PaintChannel::Text;
        };
        return std::ranges::any_of(rules.of(UiElement::SelectedText), writesInk)
            or std::ranges::any_of(rules.shared, writesInk);
    }

    // Seeded with its pigment's hue over the surface it is laid on, the way the Themes app shows
    // it, and raised by the inputs of the control drawing it, as the selection band is.
    Color ControlPaintContext::foundTextRgb() const
    {
        Hsl result = foundSeedHsl();
        applyBandRules({ UiElement::FoundText }, PaintChannel::Surface, result, ruleInputLevels);
        return disabledRgb(result.toColor());
    }

    // The selection is raised over the found band as over any surface text sits on, so the rules
    // of both elements count, and the ones every element shares count once.
    Color ControlPaintContext::selectedFoundRgb() const
    {
        Hsl result = foundSeedHsl();
        applyBandRules({ UiElement::FoundText, UiElement::SelectedText }, PaintChannel::Surface,
            result, ruleInputLevels);
        return disabledRgb(result.toColor());
    }

    Hsl ControlPaintContext::foundSeedHsl() const
    {
        Hsl result = surfaceHsl;
        if (const std::optional<Pigment> pigment = seedPigmentOf(UiElement::FoundText))
            result.hue = m_bakedColors->pigmentHues[static_cast<std::size_t>(*pigment)];
        return result;
    }

    // The order PaintEvent takes for the element a control wears. A band's rest is its own, so
    // it is raised in full whether or not the control shows a surface at rest.
    void ControlPaintContext::applyBandRules(const std::initializer_list<UiElement> elements,
        const PaintChannel channel, Hsl& color, const RuleInputLevels& levels) const
    {
        const BakedRules& rules = m_bakedColors->rules;
        auto apply = [&](const BakedColorRules& list){
            for (const BakedColorRule& rule : list)
            {
                if (rule.output == channel)
                    rule.effect.applyTo(color, rule.levelIn(levels), lightness);
            }
        };
        for (const UiElement element : elements)
            apply(rules.of(element));
        apply(rules.shared);
    }


}
