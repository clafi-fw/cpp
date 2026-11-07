module ClaFi.PathArt.EscapementSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace EscapementColors
    {
        constexpr Color RodLight{ 0xFFB9C1C9 };
        constexpr Color RodShade{ 0xFF6B7580 };
        constexpr Color BobLight{ 0xFFF5D27A };
        constexpr Color BobShade{ 0xFF7D5418 };
        constexpr Color BobFaceLight{ 0xFFE9BF5C };
        constexpr Color BobFaceShade{ 0xFF8C6223 };
        constexpr Color BobGlint{ 0xFFFFF3C0 };
        constexpr Color Pallet{ 0xFFC63B52 };
        constexpr Color Pivot{ 0xFF22272D };
        constexpr Color EscapeHub{ 0xFF9AA4AE };
        constexpr Color SteelLight{ 0xFFDFE5EA };
        constexpr Color SteelShade{ 0xFF6B7580 };
        constexpr Color SteelEdge{ 0x991D2228 };
    }

    namespace
    {
        // The escape wheel's teeth are hooks: a long slope up to the tip and a short drop back.
        void drawEscapeWheel(const SceneFrame& frame, const FloatPoint center, const float size,
            const float phase)
        {
            const float radius = 62.0f * size;
            const float pitch = k_2Pi / k_escapeWheelTeeth;
            const float rootRadius = radius * 0.82f;
            const auto onCircle = [&](float distance, float angle) {
                return center + FloatPoint{ std::cos(angle), std::sin(angle) } * distance;
            };
            PixelPath wheel;
            for (int i = 0; i != k_escapeWheelTeeth; ++i)
            {
                const float toothAngle = phase + static_cast<float>(i) * pitch;
                const FloatPoint slopeFoot = onCircle(rootRadius, toothAngle - 0.75f * pitch);
                if (i == 0)
                    wheel.moveTo(slopeFoot);
                else
                    wheel.lineTo(slopeFoot);
                wheel.lineTo(onCircle(radius, toothAngle));
                wheel.lineTo(onCircle(rootRadius, toothAngle + 0.08f * pitch));
            }
            wheel.close();
            addEllipse(wheel, center, rootRadius * 0.84f, rootRadius * 0.84f);
            const FloatPoint corner = { radius, radius };
            const LinearGradient steel = LinearGradient::Simple(center - corner, center + corner,
                EscapementColors::SteelLight, EscapementColors::SteelShade);
            fillShape(frame, wheel, steel);
            strokeShape(frame, wheel, SolidColor{ EscapementColors::SteelEdge }, size);
            PixelPath spokes;
            for (int i = 0; i != 4; ++i)
            {
                spokes.moveTo(center);
                spokes.lineTo(onCircle(rootRadius * 0.86f,
                    phase + static_cast<float>(i) * k_2Pi / 4.0f));
            }
            strokeShape(frame, spokes, steel, 4.0f * size);
            fillDisc(frame, center, 8.0f * size, SolidColor{ EscapementColors::EscapeHub });
            fillDisc(frame, center, 3.5f * size, SolidColor{ EscapementColors::Pivot });
        }

        void drawAnchor(const SceneFrame& frame, const FloatPoint pivot, const float swing,
            const float size)
        {
            const SceneFrame anchor = frame.within(Matrix3x2::Translation(pivot)
                * Matrix3x2::Rotation(-swing) * Matrix3x2::Scale(size));
            PixelPath arms;
            arms.moveTo(-52.0f, 46.0f);
            arms.quadTo({ -36.0f, -8.0f }, { 0.0f, 0.0f });
            arms.quadTo({ 36.0f, -8.0f }, { 52.0f, 46.0f });
            strokeShape(anchor, arms, LinearGradient::Simple({ -40.0f, 0.0f }, { 40.0f, 40.0f },
                EscapementColors::SteelLight, EscapementColors::SteelShade), 7.0f);
            PixelPath pallets;
            for (const float side : { -1.0f, 1.0f })
            {
                pallets.moveTo(side * 47.0f, 42.0f);
                pallets.lineTo(side * 56.0f, 44.0f);
                pallets.lineTo(side * 50.0f, 58.0f);
                pallets.close();
            }
            fillShape(anchor, pallets, SolidColor{ EscapementColors::Pallet });
            fillDisc(anchor, {}, 7.0f, SolidColor{ EscapementColors::RodLight });
            fillDisc(anchor, {}, 2.8f, SolidColor{ EscapementColors::Pivot });
        }
    }

    EscapementPose escapementAt(const FloatPoint wheelCenter, const float size, const float swings)
    {
        const float ticks = 2.0f * swings;
        const float tickCount = std::floor(ticks);
        const float progress = tickCount + smoothStep(0.0f, 0.12f, ticks - tickCount);
        return {
            .wheelCenter = wheelCenter,
            .size = size,
            .wheelPhase = -(k_2Pi / k_escapeWheelTeeth) * progress + 0.05f,
            .swing = 0.2f * std::sin(k_2Pi * swings)
        };
    }

    void drawPendulum(const SceneFrame& frame, const EscapementPose& pose)
    {
        const float size = pose.size;
        const FloatPoint pivot = pose.wheelCenter - FloatPoint{ 0.0f, 76.0f * size };
        const FloatPoint bob = pivot + rotated({ 0.0f, 320.0f * size }, -pose.swing);
        const FloatPoint rodHalf = { 4.0f, 0.0f };
        drawSegment(frame, pivot, bob, LinearGradient::Simple(pivot - rodHalf, pivot + rodHalf,
            EscapementColors::RodLight, EscapementColors::RodShade), 5.0f * size);
        const float bobRadius = 36.0f * size;
        const FloatPoint bobCorner = { bobRadius, bobRadius };
        fillDisc(frame, bob, bobRadius, LinearGradient::Simple(bob - bobCorner, bob + bobCorner,
            EscapementColors::BobLight, EscapementColors::BobShade));
        const FloatPoint faceCorner = { 26.0f * size, 26.0f * size };
        fillDisc(frame, bob, 26.0f * size,
            LinearGradient::Simple(bob + faceCorner, bob - faceCorner,
            EscapementColors::BobFaceLight, EscapementColors::BobFaceShade));
        drawGlow(frame, bob - FloatPoint{ 12.0f * size, 12.0f * size }, 14.0f * size,
            EscapementColors::BobGlint.withOpacity(0.55f));
    }

    void drawEscapement(const SceneFrame& frame, const EscapementPose& pose)
    {
        const FloatPoint pivot = pose.wheelCenter - FloatPoint{ 0.0f, 76.0f * pose.size };
        drawEscapeWheel(frame, pose.wheelCenter, pose.size, pose.wheelPhase);
        drawAnchor(frame, pivot, pose.swing, pose.size);
    }
}
