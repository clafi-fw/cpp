module ClaFi.PathArt.GearSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace GearColors
    {
        constexpr Color Edge{ 0x8C2A1A0C };
        constexpr Color Axle{ 0xFF2A1D10 };
        constexpr Color AxleGlint{ 0xB3FFF3C0 };
    }

    Gear meshWith(const Gear& driver, const int teeth, const float angle, const int spokes,
        const GearMetal metal, const float toothSize)
    {
        const float pitch = k_2Pi / static_cast<float>(teeth);
        const float pitchRadius = static_cast<float>(teeth) * toothSize * 0.5f;
        const float reach = driver.pitchRadius + pitchRadius;
        // Teeth past the line of centers, counted on each gear, add up to half a tooth: the
        // driver's tooth stands in the driven gear's gap.
        const float driverTeeth = (angle - driver.phase) * static_cast<float>(driver.teeth) / k_2Pi;
        return {
            .teeth = teeth,
            .center = driver.center + FloatPoint{ std::cos(angle), std::sin(angle) } * reach,
            .phase = angle + std::numbers::pi_v<float> - pitch * (0.5f - driverTeeth),
            .spokes = spokes,
            .metal = metal,
            .pitchRadius = pitchRadius
        };
    }

    void drawGear(const SceneFrame& frame, const Gear& gear, const float toothSize)
    {
        const float radius = gear.pitchRadius;
        const float rootRadius = radius - 1.25f * toothSize;
        const float tipRadius = radius + toothSize;
        const float pitch = k_2Pi / static_cast<float>(gear.teeth);
        const float rimInner = gear.spokes > 0 ? rootRadius - 1.6f * toothSize : 0.0f;
        const FloatPoint center = gear.center;
        const auto onCircle = [&](float distance, float angle) {
            return center + FloatPoint{ std::cos(angle), std::sin(angle) } * distance;
        };

        // The teeth outline with the rim's hole cut out of it: the fill is even-odd.
        PixelPath body;
        for (int i = 0; i != gear.teeth; ++i)
        {
            const float toothAngle = gear.phase + static_cast<float>(i) * pitch;
            const FloatPoint rootBefore = onCircle(rootRadius, toothAngle - 0.3f * pitch);
            if (i == 0)
                body.moveTo(rootBefore);
            else
                body.lineTo(rootBefore);
            body.lineTo(onCircle(tipRadius, toothAngle - 0.16f * pitch));
            body.lineTo(onCircle(tipRadius, toothAngle + 0.16f * pitch));
            body.lineTo(onCircle(rootRadius, toothAngle + 0.3f * pitch));
        }
        body.close();
        if (rimInner > 0.0f)
            addEllipse(body, center, rimInner, rimInner);
        const FloatPoint corner = { radius, radius };
        const LinearGradient metal = LinearGradient::Simple(center - corner, center + corner,
            gear.metal.light, gear.metal.shade);
        fillShape(frame, body, metal);
        strokeShape(frame, body, SolidColor{ GearColors::Edge }, frame.unit);

        if (rimInner > 0.0f)
        {
            for (int i = 0; i != gear.spokes; ++i)
            {
                const float spokeTurn = static_cast<float>(i) / static_cast<float>(gear.spokes);
                const float spokeAngle = gear.phase + spokeTurn * k_2Pi + pitch * 0.5f;
                PixelPath spoke;
                spoke.moveTo(0.0f, -radius * 0.07f);
                spoke.lineTo(rimInner + toothSize * 0.4f, -radius * 0.07f);
                spoke.lineTo(rimInner + toothSize * 0.4f, radius * 0.07f);
                spoke.lineTo(0.0f, radius * 0.07f);
                spoke.close();
                const Matrix3x2 place = Matrix3x2::Translation(center)
                    * Matrix3x2::Rotation(spokeAngle);
                fillShape(frame, spoke, LinearGradient::Simple({ 0.0f, -radius }, { 0.0f, radius },
                    gear.metal.light, gear.metal.shade), place);
                strokeShape(frame, spoke, SolidColor{ GearColors::Edge.withOpacity(0.45f) },
                    frame.unit, StrokeCap::Round, place);
            }
        }
        else
        {
            PixelPath ring;
            addEllipse(ring, center, rootRadius * 0.7f, rootRadius * 0.7f);
            strokeShape(frame, ring, SolidColor{ GearColors::Edge.withOpacity(0.35f) },
                1.2f * frame.unit);
        }

        const float hubRadius = std::max(radius * 0.24f, 7.0f * frame.unit);
        const FloatPoint hubCorner = { hubRadius, hubRadius };
        PixelPath hub;
        addEllipse(hub, center, hubRadius, hubRadius);
        fillShape(frame, hub, LinearGradient::Simple(center - hubCorner, center + hubCorner,
            gear.metal.light, gear.metal.shade));
        strokeShape(frame, hub, SolidColor{ GearColors::Edge.withOpacity(0.5f) }, frame.unit);
        fillDisc(frame, center, std::max(radius * 0.08f, 3.0f * frame.unit),
            SolidColor{ GearColors::Axle });
        const float glintRadius = std::max(radius * 0.03f, 1.2f * frame.unit);
        fillDisc(frame, center - FloatPoint{ frame.unit, frame.unit }, glintRadius,
            SolidColor{ GearColors::AxleGlint });
    }
}
