module ClaFi.PathArt.ClockworkScene;

import ClaFi.PathArt.EscapementSprite;
import ClaFi.PathArt.GearSprite;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SteamEffect;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace ClockworkColors
    {
        constexpr Color BackTop{ 0xFF20160F };
        constexpr Color BackBottom{ 0xFF2E2017 };
        constexpr Color PlateLight{ 0xFF3E2D1E };
        constexpr Color PlateShade{ 0xFF22170F };
        constexpr Color PlateRim{ 0x8CA5793A };
        constexpr Color Rivet{ 0xFFB88A42 };
        constexpr Color Lamp{ 0xFFFFB85C };
        constexpr Color Shadow{ 0xFF0A0603 };
        constexpr Color BobShade{ 0xFF7D5418 };
        constexpr Color BobFaceLight{ 0xFFE9BF5C };
        constexpr Color Pallet{ 0xFFC63B52 };
        constexpr Color SteelLight{ 0xFFDFE5EA };
        constexpr Color SteelShade{ 0xFF6B7580 };
        constexpr Color PipeLight{ 0xFFF0A273 };
        constexpr Color PipeShade{ 0xFF7A3618 };
        constexpr Color FlangeLight{ 0xFFE5B25A };
        constexpr Color Valve{ 0xFFB8433A };
        constexpr Color Steam{ 0xFFECE6DA };
        constexpr Color GaugeRimLight{ 0xFFF0C766 };
        constexpr Color GaugeFace{ 0xFFF3EAD6 };
        constexpr Color GaugeInk{ 0xFF3A2A1C };
        constexpr Color Needle{ 0xFF1D1A17 };
    }

    namespace
    {
        constexpr float k_degree{ std::numbers::pi_v<float> / 180.0f };
        // Swings of the pendulum per cycle; each crossing of the middle releases one tooth.
        constexpr float k_swingsPerCycle{ 20.0f };
    }

    // Every gear turns a whole number of teeth and of spokes in a cycle, so the train ends the
    // cycle where it began. See PathArt#clockworkscene
    void ClockworkScene::draw(Canvas& canvas, const FloatRect& bounds, const float cycleTime)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        // A narrow frame draws the movement smaller rather than cut it.
        const float size = frame.unit * std::min(1.0f, (frame.width / frame.unit) / 800.0f);
        const FloatPoint center = { frame.width * 0.5f - 20.0f * size, frame.height * 0.5f };

        drawPlate(frame, center - FloatPoint{ 260.0f * size, 260.0f * size });

        const float toothSize = 6.2f * size;
        const FloatPoint escapeCenter = center + FloatPoint{ -230.0f * size, -60.0f * size };
        const EscapementPose escapement = escapementAt(escapeCenter, size,
            k_swingsPerCycle * cycleTime);
        drawPendulum(frame, escapement);

        constexpr GearMetal k_brass{ Color{ 0xFFF0C766 }, Color{ 0xFF8C6223 } };
        constexpr GearMetal k_brassBehind{ Color{ 0xFFC4943F }, Color{ 0xFF5E4015 } };
        constexpr GearMetal k_copper{ Color{ 0xFFE39A6A }, Color{ 0xFF7D3B1C } };
        constexpr GearMetal k_steel{ ClockworkColors::SteelLight, ClockworkColors::SteelShade };
        const Gear escapeGear = {
            .teeth = 30,
            .center = escapeCenter,
            .phase = escapement.wheelPhase,
            .spokes = 5,
            .metal = k_brass,
            .pitchRadius = 30.0f * toothSize * 0.5f
        };
        const Gear second = meshWith(escapeGear, 12, 20.0f * k_degree, 0, k_copper, toothSize);
        const Gear third = meshWith(second, 36, -32.0f * k_degree, 6, k_brass, toothSize);
        const Gear pinion = {
            .teeth = 12,
            .center = third.center,
            .phase = third.phase,
            .spokes = 0,
            .metal = k_steel,
            .pitchRadius = 12.0f * toothSize * 0.5f
        };
        const Gear great = meshWith(pinion, 40, 80.0f * k_degree, 8, k_brassBehind, toothSize);
        const Gear fifth = meshWith(great, 20, 12.0f * k_degree, 5, k_copper, toothSize);
        const Gear sixth = meshWith(fifth, 15, -62.0f * k_degree, 3, k_brass, toothSize);

        // Each gear casts a soft shadow on whatever stands behind it.
        for (const Gear* gear : { &great, &third, &escapeGear, &second, &fifth, &sixth })
        {
            drawGlow(frame, gear->center + FloatPoint{ 7.0f * size, 9.0f * size },
                gear->pitchRadius * 1.12f, ClockworkColors::Shadow.withOpacity(0.55f));
            drawGear(frame, *gear, toothSize);
        }
        drawGlow(frame, pinion.center + FloatPoint{ 5.0f * size, 6.0f * size },
            pinion.pitchRadius * 1.2f, ClockworkColors::Shadow.withOpacity(0.6f));
        drawGear(frame, pinion, toothSize);
        drawGlow(frame, escapeCenter + FloatPoint{ 5.0f * size, 6.0f * size }, 66.0f * size,
            ClockworkColors::Shadow.withOpacity(0.5f));
        drawEscapement(frame, escapement);
        drawPipes(frame, center, size, cycleTime);
    }

    void ClockworkScene::drawPlate(const SceneFrame& frame, const FloatPoint lampAt)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        fillBox(frame, { 0.0f, 0.0f, width, height },
            LinearGradient::Simple({ 0.0f, 0.0f }, { width, height }, ClockworkColors::BackTop,
                ClockworkColors::BackBottom));
        const float inset = 14.0f * unit;
        PixelPath plate;
        const float corner = 14.0f * unit;
        plate.drawRoundedRect(FloatRect{ inset, inset, width - inset, height - inset }, corner,
            corner);
        fillShape(frame, plate, LinearGradient::Simple({ 0.0f, 0.0f }, { width * 0.3f, height },
            ClockworkColors::PlateLight, ClockworkColors::PlateShade));
        strokeShape(frame, plate, SolidColor{ ClockworkColors::PlateRim }, 2.0f * unit);

        ScatteredShapes rivets{ frame, SolidColor{ ClockworkColors::Rivet } };
        const float rivetRadius = 3.4f * unit;
        for (float x = inset + 22.0f * unit; x < width - inset - 10.0f * unit; x += 52.0f * unit)
        {
            addEllipse(rivets.next(), { x, inset + 10.0f * unit }, rivetRadius, rivetRadius);
            addEllipse(rivets.next(), { x, height - inset - 10.0f * unit }, rivetRadius,
                rivetRadius);
        }
        for (float y = inset + 62.0f * unit; y < height - inset - 40.0f * unit; y += 52.0f * unit)
        {
            addEllipse(rivets.next(), { inset + 10.0f * unit, y }, rivetRadius, rivetRadius);
            addEllipse(rivets.next(), { width - inset - 10.0f * unit, y }, rivetRadius,
                rivetRadius);
        }
        rivets.draw();
        drawGlow(frame, lampAt, 420.0f * unit, ClockworkColors::Lamp.withOpacity(0.16f));
    }

    void ClockworkScene::drawPipes(const SceneFrame& frame, const FloatPoint center,
        const float size, const float cycleTime)
    {
        const float inset = 14.0f * frame.unit;
        const float pipeY = center.y + 262.0f * size;
        const float pipeX = center.x + 372.0f * size;
        // A pipe is lit across its width, from `lit` to `shaded`.
        const auto pipeMetal = [](FloatPoint lit, FloatPoint shaded) {
            return LinearGradient::Simple(lit, shaded, ClockworkColors::PipeLight,
                ClockworkColors::PipeShade);
        };
        const float pipeHalf = 9.0f * size;
        drawSegment(frame, { inset, pipeY }, { pipeX, pipeY },
            pipeMetal({ 0.0f, pipeY - pipeHalf }, { 0.0f, pipeY + pipeHalf }), 2.0f * pipeHalf,
            StrokeCap::Butt);
        drawSegment(frame, { pipeX, pipeY + pipeHalf }, { pipeX, inset },
            pipeMetal({ pipeX - pipeHalf, 0.0f }, { pipeX + pipeHalf, 0.0f }), 2.0f * pipeHalf,
            StrokeCap::Butt);
        const FloatPoint elbow = { pipeX, pipeY };
        const FloatPoint elbowCorner = { 14.0f * size, 14.0f * size };
        fillDisc(frame, elbow, 14.0f * size, pipeMetal(elbow - elbowCorner, elbow + elbowCorner));
        const float flangeHalf = 13.0f * size;
        const LinearGradient flangeMetal = LinearGradient::Simple({ 0.0f, pipeY - flangeHalf },
            { 0.0f, pipeY + flangeHalf }, ClockworkColors::FlangeLight, ClockworkColors::BobShade);
        for (const float flangeAt : { -330.0f, -60.0f, 230.0f })
        {
            const float flangeX = center.x + flangeAt * size;
            const FloatRect flange = {
                flangeX - 4.0f * size,
                pipeY - flangeHalf,
                flangeX + 4.0f * size,
                pipeY + flangeHalf
            };
            fillBox(frame, flange, flangeMetal);
        }

        // The valve lets off steam four times a cycle, each burst a dozen puffs.
        const float valveX = center.x + 130.0f * size;
        const FloatRect valve = {
            valveX - 7.0f * size,
            pipeY - 26.0f * size,
            valveX + 7.0f * size,
            pipeY - 8.0f * size
        };
        fillBox(frame, valve, LinearGradient::Simple({ 0.0f, pipeY - 28.0f * size },
            { 0.0f, pipeY }, ClockworkColors::BobFaceLight, ClockworkColors::BobShade));
        drawSegment(frame, { valveX - 16.0f * size, pipeY - 30.0f * size },
            { valveX + 16.0f * size, pipeY - 30.0f * size },
            SolidColor{ ClockworkColors::Valve }, 5.0f * size);
        drawSteam(frame, SteamBurst{
            .vent = { valveX, pipeY - 34.0f * size },
            .size = size,
            .time = fractionalPart(cycleTime * 4.0f),
            .color = ClockworkColors::Steam
        });

        // the pressure gauge on its branch of the pipe
        const FloatPoint gauge = { pipeX - 72.0f * size, center.y - 196.0f * size };
        const float gaugeRadius = 40.0f * size;
        const float branchHalf = 6.0f * size;
        drawSegment(frame, gauge + FloatPoint{ gaugeRadius, 0.0f }, { pipeX, gauge.y },
            pipeMetal({ 0.0f, gauge.y - branchHalf }, { 0.0f, gauge.y + branchHalf }),
            2.0f * branchHalf, StrokeCap::Butt);
        const FloatPoint gaugeCorner = { gaugeRadius, gaugeRadius };
        fillDisc(frame, gauge, gaugeRadius + 6.0f * size,
            LinearGradient::Simple(gauge - gaugeCorner, gauge + gaugeCorner,
            ClockworkColors::GaugeRimLight, ClockworkColors::BobShade));
        fillDisc(frame, gauge, gaugeRadius, SolidColor{ ClockworkColors::GaugeFace });
        const auto onDial = [&](float degrees, float distance) {
            const float angle = degrees * k_degree;
            return gauge + FloatPoint{ std::cos(angle), std::sin(angle) } * distance;
        };
        PixelPath ticks;
        for (int i = 0; i <= 10; ++i)
        {
            const float degrees = -215.0f + static_cast<float>(i) * 25.0f;
            ticks.moveTo(onDial(degrees, gaugeRadius * (i % 5 == 0 ? 0.7f : 0.8f)));
            ticks.lineTo(onDial(degrees, gaugeRadius * 0.9f));
        }
        strokeShape(frame, ticks, SolidColor{ ClockworkColors::GaugeInk }, 1.5f * size,
            StrokeCap::Butt);
        const float redRadius = gaugeRadius * 0.86f;
        PixelPath redZone;
        redZone.moveTo(onDial(10.0f, redRadius));
        redZone.arcTo({ redRadius, redRadius }, 0.0f, ArcSize::Small, ArcSweep::Clockwise,
            onDial(35.0f, redRadius));
        strokeShape(frame, redZone, SolidColor{ ClockworkColors::Pallet }, 4.0f * size,
            StrokeCap::Butt);
        // The needle wanders three times a cycle and trembles as it goes.
        const float wander = 40.0f * std::sin(k_2Pi * 3.0f * cycleTime);
        const float tremble = 4.0f * std::sin(k_2Pi * 31.0f * cycleTime);
        drawSegment(frame, gauge, onDial(-120.0f + wander + tremble, gaugeRadius * 0.82f),
            SolidColor{ ClockworkColors::Needle }, 2.4f * size);
        fillDisc(frame, gauge, 4.0f * size, SolidColor{ ClockworkColors::GaugeInk });
    }
}
