module;
#include "../Y-Core/System/EventBindings.h"
export module ClaFi.Controls.ColorSlider;

import ClaFi.Controls.Slider;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.Foundation;
import ClaFi.Core.System.Props;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Controls.Base.SliderBase;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.System.Events;
import ClaFi.Core.TextEngine.Types;
import ClaFi.StdLib;

namespace ClaFi::Controls
{
    // Which channel of a colour the slider moves.
    export enum class ColorAttribute
    {
        Hue,
        Saturation,
        Luminosity
    };

    // A slider that moves one channel of a colour and paints its range.
    export class ColorSlider : public Slider
    {
    public:
        template<typename... Args>
        ColorSlider(const CreateParams&, Args&&...);
    public:
        const float trackingValue() const { return *m_trackingValue; }
        void invalidateSlot();
        void trackingValueChanged(bool propagateChanges = false);
        void paintValue(Text&, EventPhase);
        void setTrackingAttribute(ColorAttribute);
        void setEditedColor(Hsl& value);
        // A slider over a bare hue rather than over a colour. What it edits is one number, so the
        // saturation and luminosity its ramp and its thumb are drawn at are the palette's display
        // pair - the same pair a palette swatch carries, so a ramp and a swatch beside it agree.
        void setEditedHue(float& value);
    protected:
        void paintSlot(PaintEvent&, const FloatRect&, SlotSpan) override;
        Color thumbColor(PaintEvent&, FloatPoint&) override;
        void getThumbTooltip(GetTooltipEvent&) override;
        void thumbPressDown() override;
        void keyDown(KeyDownEvent&) override;
        void changed(SliderChangeEvent&) override;
        void sizeChanged() override;
    private:
        // A slot bitmap and what it was written for.
        struct SlotSpectrum
        {
            Graphics::Bitmap bitmap{};
            bool valid{ false };
            Lightness lightness{};   // what it was written at, which sets the luminosity floor
            SlotSpan span{};
        };
    private:
        void relinkTrackingValue();
        void rebuildSlotBmp(Graphics::Bitmap&, IntSize bmpBounds, SlotSpan, const PaintEvent&);
        void paintNotch(PaintEvent&, const FloatRect& slotRect, SlotSpan);
    private:
        inline static Hsl s_nullColor{ 0.6f, 0.5f, 0.5f };
        // The notch marking the value the slider was given, in design units.
        static constexpr float k_notchHalfWidth = 3.0f;
        static constexpr float k_notchHeight = 3.0f;
        static constexpr float k_notchGap = 1.0f;
        std::function<void(Hsl&)> onAdjustSlotColor;
        SlotSpectrum m_slot{};       // the slider's own slot, over the whole range
        SlotSpectrum m_spanSlot{};   // a slot over part of the range, drawn for a finer slider
        Hsl* m_editedColor{ &s_nullColor };
        // Set by setEditedHue: the number the caller owns, and the colour this shows it as. The
        // pair below is what m_editedColor points at while a hue is bound, so everything the
        // slider draws goes on working from a whole colour.
        float* m_boundHue{ nullptr };
        Hsl m_displayColor{};
        ColorAttribute m_trackingAttribute{};
        float* m_trackingValue{ nullptr };
        float m_originalValue{};
    };

    export class ColorSlidersLinker : private std::vector<ColorSlider*>, public EventComponent
    {
    public:
        ColorSlidersLinker(std::initializer_list<ColorSlider*>);
        EventConnection onChange(auto&& callback) {
            return connectEvent<SliderChangeEvent>(std::forward<decltype(callback)>(callback));
        }
    };


    //----------------------------------------------------------------------------


    // ColorSlider

    template<typename ...Args>
    ColorSlider::ColorSlider(const CreateParams& params, Args&&... args)
        :
        Slider{ params, std::forward<Args>(args)... }
    {
        // Which channel of the colour the slider moves.
        setTrackingAttribute(READ_PROPERTY(ColorAttribute, ColorAttribute::Hue));
        // TODO: a bare const Color* has no property type of its own, so any pointer to a colour
        // in the pack matches. It wants a named type before it can be declared.
        setEditedColor(*Props::get(&s_nullColor, std::forward<Args>(args)...));
    }

    void ColorSlider::invalidateSlot()
    {
        m_slot.valid = false;
        m_spanSlot.valid = false;
        invalidate();
    }

    void ColorSlider::trackingValueChanged(bool propagateChanges)
    {
        // The one place the slider is told its value has moved under it, so it is where a bound
        // hue is read back in. Nothing else copies it, so nothing else can be holding a stale one.
        if (m_boundHue)
            m_displayColor.hue = *m_boundHue;

        float maxPos = maxPosition();
        setPosition(std::round(*m_trackingValue * maxPos), propagateChanges);
        m_originalValue = *m_trackingValue;
    }

    void ColorSlider::paintValue(Text& text, EventPhase phase)
    {
        bool calcOnly = phase == EventPhase::Calculate;
        switch (m_trackingAttribute)
        {
        case ColorAttribute::Hue:
        {
            if (calcOnly)
                text << 360;
            else
                text << static_cast<int>(std::round(*m_trackingValue * 360.0f));
            text << L'\xb0';
            break;
        }
        default:
            float value = calcOnly ? 100.0f : *m_trackingValue * 100.0f;
            text << std::format(L"{0:.2f}", value) << L'%';
            break;
        }
    }

    void ColorSlider::setTrackingAttribute(ColorAttribute value)
    {
        m_trackingAttribute = value;
        static constexpr float k_maxPositions[]{ 360.0f, 300.0f, 500.0f };
        setMaxPosition(k_maxPositions[static_cast<std::size_t>(m_trackingAttribute)]);
        relinkTrackingValue();
    }

    void ColorSlider::setEditedColor(Hsl& value)
    {
        m_editedColor = &value;
        relinkTrackingValue();
    }

    void ColorSlider::setEditedHue(float& value)
    {
        m_boundHue = &value;
        m_displayColor = paletteDisplayColor(value);
        setTrackingAttribute(ColorAttribute::Hue);
        setEditedColor(m_displayColor);
    }

    // THE LIGHTNESS IS AN INPUT TO THE SPECTRUM - the luminosity floor is read from it - so a
    // theme crossing under a slot that is still valid would leave the bitmap stating the side of
    // the theme it was left on. The size is one too: the owner's own size is followed through
    // sizeChanged, and a finer slider's is not.
    void ColorSlider::paintSlot(PaintEvent& event, const FloatRect& slotRect, SlotSpan span)
    {
        float radius = slotRect.height() / 2.0f;
        // The rounded ends are painted as geometry over the caps, so the bitmap carries the
        // straight middle only. Rounding the rect out rather than rounding its dimensions
        // keeps the pixel that a fractional top and a fractional bottom both touch.
        IntRect bmpRect = slotRect.inflated(-radius, 0.0f).roundedOut();
        const IntSize bmpSize = bmpRect.dimensions();
        SlotSpectrum& spectrum = span == SlotSpan{} ? m_slot : m_spanSlot;
        if (!spectrum.valid
            || spectrum.lightness != event.lightness()
            || spectrum.span != span
            || spectrum.bitmap.width() != bmpSize.x
            || spectrum.bitmap.height() != bmpSize.y)
        {
            spectrum.valid = true;
            spectrum.lightness = event.lightness();
            spectrum.span = span;
            rebuildSlotBmp(spectrum.bitmap, bmpSize, span, event);
        }
        Graphics::Bitmap& bitmap = spectrum.bitmap;
        if (bitmap.empty())
            return;

        // The bitmap holds whole pixels and drawBitmap copies them without resampling, so its
        // origin is the whole pixel it was sized from. A fractional origin would leave the copy
        // half a pixel off the caps drawn against the same edges.
        event.canvas().drawBitmap(bitmap, bmpRect.topLeft().toFloat());

        // Left slot cap
        RoundedRectangleParts halfSphere{
            .bounds = slotRect,
            .radii = { radius, 0.0f, 0.0f, radius },
            .sides = k_allRectSidesTrue
        };
        halfSphere.bounds.right = slotRect.left + radius;
        event.canvas().fillPartialRoundedRectangle(halfSphere, *bitmap.pixel(0, 0));
        // Right slot cap
        halfSphere.bounds.right = slotRect.right;
        halfSphere.bounds.left = slotRect.right - radius;
        halfSphere.radii = { 0.0f, radius, radius, 0.0f };
        event.canvas().fillPartialRoundedRectangle(
            halfSphere,
            *bitmap.pixel(bitmap.width() - 1, 0)
        );

        // Border
        paintSlotBorder(event, slotRect, radius);

        paintNotch(event, slotRect, span);
    }

    Color ColorSlider::thumbColor(PaintEvent&, FloatPoint&)
    {
        Hsl tmp = *m_editedColor;
        if (onAdjustSlotColor)
            onAdjustSlotColor(tmp);
        return tmp.toColor();
    }

    void ColorSlider::getThumbTooltip(GetTooltipEvent& event)
    {
        event.placement = FormPlacement::Top;
        event.hideOnUserInput = false;
        event.text << TextAlign::Center;
        paintValue(event.text, event.phase);
    }

    void ColorSlider::thumbPressDown()
    {
        form().tooltip().showRightNow(thumb());
    }

    void ColorSlider::keyDown(KeyDownEvent& key)
    {
        Slider::keyDown(key);
        if (key.handled)
            form().tooltip().showRightNow(thumb());
    }

    void ColorSlider::changed(SliderChangeEvent& event)
    {
        *m_trackingValue = event.newPosition / maxPosition();
        // The display colour is what the slider edits; the number the caller owns is what that
        // edit means, so it is written through here rather than left to be collected.
        if (m_boundHue)
            *m_boundHue = m_displayColor.hue;

        form().tooltip().showRightNow(thumb());
        Slider::changed(event);
    }

    void ColorSlider::sizeChanged()
    {
        Slider::sizeChanged();
        invalidateSlot();
    }

    void ColorSlider::relinkTrackingValue()
    {
        m_trackingValue = &reinterpret_cast<float*>(m_editedColor)[static_cast<std::size_t>(m_trackingAttribute)];
        trackingValueChanged();
        invalidateSlot();
    }

    void ColorSlider::rebuildSlotBmp(Graphics::Bitmap& bitmap, IntSize bmpBounds, SlotSpan span,
        const PaintEvent& event)
    {
        bitmap.resize(bmpBounds);

        float maxLum = 0.9f;
        float minLum = std::lerp(0.08f, 0.2f, event.lightness());

        Hsl slotColor = *m_editedColor;
        slotColor.luminosity = (std::min)((std::max)(slotColor.luminosity, minLum), maxLum);

        std::size_t valueOffset = ((std::size_t)m_trackingValue - (std::size_t)m_editedColor);

        valueOffset /= sizeof(float);
        if (!valueOffset)
            // it's hue
    //{
            slotColor.saturation = std::min(std::max(slotColor.saturation, 0.2f), 1.0f);
        //  tempHsl.lum = 0.5;
        //  tempHsl.sat = 0.5;
        //}

        float* trackingValue = reinterpret_cast<float*>(&slotColor) + valueOffset;

        bool deepEnabled = enabled(true);
        for (int x = 0; x < bmpBounds.x; ++x)
        {
            // assuming left is 0 here
            if (deepEnabled || valueOffset != 1ull)
                *trackingValue = span.at(static_cast<float>(x) / static_cast<float>(bmpBounds.x));

            Hsl tmp = slotColor;
            if (onAdjustSlotColor)
                onAdjustSlotColor(tmp);
            Color colColor = tmp.toColor();
            // The fade takes the colour and leaves the alpha at the 255 the spectrum was built
            // with, which is what the staging bitmap requires: it is premultiplied, so a pixel
            // holding straight colour at partial alpha is added over the surface rather than
            // replacing it - a plausible dim spectrum on a dark form, and every channel clipped
            // to white on a light one.
            //
            // The amount is stated rather than taken from the event: this bitmap is written once
            // and kept until invalidateSlot(), so it takes where the fade settles and not where
            // it is passing through.
            colColor = event.disabledFormOf(colColor,
                (1.0f - static_cast<float>(deepEnabled)) * event.disabledBlendAmount());
            for (int y = 0; y < bmpBounds.y; ++y)
            {
                *bitmap.pixel(x, y) = colColor;
            }
        }
    }

    // THE VALUE THE SLIDER WAS GIVEN, marked above the slot: what a drag has moved away from,
    // and where it stands while nothing has moved. A triangle pointing at the slot, in the room
    // the thumb leaves above it. A slot over a span the value lies outside has nothing to mark.
    void ColorSlider::paintNotch(PaintEvent& event, const FloatRect& slotRect, SlotSpan span)
    {
        const float fraction = span.fractionOf(m_originalValue);
        if (fraction < 0.0f || fraction > 1.0f)
            return;
        const float x = fraction * slotRect.width() + slotRect.left;
        const float half = event.scaleF(k_notchHalfWidth);
        const float bottom = slotRect.top - event.scaleF(k_notchGap);
        const float top = bottom - event.scaleF(k_notchHeight);

        Graphics::PixelPath notch{};
        notch.moveTo(x - half, top);
        notch.lineTo(x + half, top);
        notch.lineTo(x, bottom);
        notch.close();
        event.canvas().fillPath(notch, event.textRgb(InkGrade::Muted));
    }

    ColorSlidersLinker::ColorSlidersLinker(std::initializer_list<ColorSlider*> list)
        :
        std::vector<ColorSlider*>{ list }
    {
        for (ColorSlider* slider : list)
            slider->onChange([slider, this](SliderChangeEvent& event)
                {
                    for (ColorSlider* otherSlider : *this)
                        if (otherSlider != slider)
                        {
                            otherSlider->invalidateSlot();
                            if (otherSlider->trackingValue() == slider->trackingValue())
                                otherSlider->trackingValueChanged();
                        }
                    emitEvent(event);
                });
    }
}
