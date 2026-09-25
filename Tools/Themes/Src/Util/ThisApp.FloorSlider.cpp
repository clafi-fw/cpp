module ThisApp.FloorSlider;

import ClaFi.Controls.Slider;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.Foundation;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ClaFi;

    void FloorSlider::bind(OnGetFloorBase base)
    {
        m_base = std::move(base);
        invalidate();
    }

    void FloorSlider::paintSlot(PaintEvent& event, const FloatRect& slotRect, SlotSpan span)
    {
        const Hsl base = m_base();
        Graphics::LinearGradient ramp{
            .startPoint = { slotRect.left, slotRect.centerY() },
            .endPoint = { slotRect.right, slotRect.centerY() },
        };
        ramp.stops.reserve(k_stopCount);
        for (std::size_t i = 0ull; i != k_stopCount; ++i)
        {
            const float position = static_cast<float>(i) / static_cast<float>(k_stopCount - 1ull);
            ramp.stops.push_back({
                position,
                event.applyDisabledFactor(sampleAt(span.at(position), base))
            });
        }

        const float radius = slotRect.height() / 2.0f;
        event.canvas().fillRoundedRectangle(slotRect, radius, radius, Graphics::Brush{ ramp });
        paintSlotBorder(event, slotRect, radius);
    }

    // The thumb carries the colour under it, so its hole reads as the slot showing through.
    Color FloorSlider::thumbColor(PaintEvent&, FloatPoint&)
    {
        return sampleAt(relativePosition(), m_base());
    }

    Color FloorSlider::sampleAt(float position, const Hsl& base) const
    {
        Hsl result = base;
        result.luminosity = luminosityOf(0.0f, k_darkLightness, position * maxPosition());
        return result.toColor();
    }
}
