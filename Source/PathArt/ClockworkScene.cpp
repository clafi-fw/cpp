module ClaFi.PathArt.ClockworkScene;

import ClaFi.PathArt.SceneHelpers;
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
        constexpr Color Edge{ 0x8C2A1A0C };
        constexpr Color Axle{ 0xFF2A1D10 };
        constexpr Color AxleGlint{ 0xB3FFF3C0 };
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
        constexpr int k_escapeTeeth{ 20 };
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

        const float swing = 0.2f * std::sin(k_2Pi * k_swingsPerCycle * cycleTime);
        const float ticks = 2.0f * k_swingsPerCycle * cycleTime;
        const float tickCount = std::floor(ticks);
        const float progress = tickCount + smoothStep(0.0f, 0.12f, ticks - tickCount);
        const float escapePhase = -(k_2Pi / k_escapeTeeth) * progress + 0.05f;

        const float toothSize = 6.2f * size;
        const FloatPoint escapeCenter = center + FloatPoint{ -230.0f * size, -60.0f * size };
        const FloatPoint pivot = escapeCenter - FloatPoint{ 0.0f, 76.0f * size };
        const FloatPoint bob = pivot + rotated({ 0.0f, 320.0f * size }, -swing);
        const FloatPoint rodHalf = { 4.0f, 0.0f };
        drawSegment(frame, pivot, bob, LinearGradient::Simple(pivot - rodHalf, pivot + rodHalf,
            ClockworkColors::RodLight, ClockworkColors::RodShade), 5.0f * size);
        const float bobRadius = 36.0f * size;
        const FloatPoint bobCorner = { bobRadius, bobRadius };
        fillDisc(frame, bob, bobRadius, LinearGradient::Simple(bob - bobCorner, bob + bobCorner,
            ClockworkColors::BobLight, ClockworkColors::BobShade));
        const FloatPoint faceCorner = { 26.0f * size, 26.0f * size };
        fillDisc(frame, bob, 26.0f * size,
            LinearGradient::Simple(bob + faceCorner, bob - faceCorner,
            ClockworkColors::BobFaceLight, ClockworkColors::BobFaceShade));
        drawGlow(frame, bob - FloatPoint{ 12.0f * size, 12.0f * size }, 14.0f * size,
            ClockworkColors::BobGlint.withOpacity(0.55f));

        constexpr Metal k_brass{ Color{ 0xFFF0C766 }, Color{ 0xFF8C6223 } };
        constexpr Metal k_brassBehind{ Color{ 0xFFC4943F }, Color{ 0xFF5E4015 } };
        constexpr Metal k_copper{ Color{ 0xFFE39A6A }, Color{ 0xFF7D3B1C } };
        constexpr Metal k_steel{ ClockworkColors::SteelLight, ClockworkColors::SteelShade };
        const Gear escapeGear = {
            .teeth = 30,
            .center = escapeCenter,
            .phase = escapePhase,
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
        drawEscapeWheel(frame, escapeCenter, size, escapePhase);
        drawAnchor(frame, pivot, swing, size);
        drawPipes(frame, center, size, cycleTime);
    }

    ClockworkScene::Gear ClockworkScene::meshWith(const Gear& driver, const int teeth,
        const float angle, const int spokes, const Metal metal, const float toothSize)
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

        PixelPath rivets;
        const float rivetRadius = 3.4f * unit;
        for (float x = inset + 22.0f * unit; x < width - inset - 10.0f * unit; x += 52.0f * unit)
        {
            addEllipse(rivets, { x, inset + 10.0f * unit }, rivetRadius, rivetRadius);
            addEllipse(rivets, { x, height - inset - 10.0f * unit }, rivetRadius, rivetRadius);
        }
        for (float y = inset + 62.0f * unit; y < height - inset - 40.0f * unit; y += 52.0f * unit)
        {
            addEllipse(rivets, { inset + 10.0f * unit, y }, rivetRadius, rivetRadius);
            addEllipse(rivets, { width - inset - 10.0f * unit, y }, rivetRadius, rivetRadius);
        }
        fillShape(frame, rivets, SolidColor{ ClockworkColors::Rivet });
        drawGlow(frame, lampAt, 420.0f * unit, ClockworkColors::Lamp.withOpacity(0.16f));
    }

    void ClockworkScene::drawGear(const SceneFrame& frame, const Gear& gear, const float toothSize)
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
        strokeShape(frame, body, SolidColor{ ClockworkColors::Edge }, frame.unit);

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
                strokeShape(frame, spoke, SolidColor{ ClockworkColors::Edge.withOpacity(0.45f) },
                    frame.unit, StrokeCap::Round, place);
            }
        }
        else
        {
            PixelPath ring;
            addEllipse(ring, center, rootRadius * 0.7f, rootRadius * 0.7f);
            strokeShape(frame, ring, SolidColor{ ClockworkColors::Edge.withOpacity(0.35f) },
                1.2f * frame.unit);
        }

        const float hubRadius = std::max(radius * 0.24f, 7.0f * frame.unit);
        const FloatPoint hubCorner = { hubRadius, hubRadius };
        PixelPath hub;
        addEllipse(hub, center, hubRadius, hubRadius);
        fillShape(frame, hub, LinearGradient::Simple(center - hubCorner, center + hubCorner,
            gear.metal.light, gear.metal.shade));
        strokeShape(frame, hub, SolidColor{ ClockworkColors::Edge.withOpacity(0.5f) }, frame.unit);
        fillDisc(frame, center, std::max(radius * 0.08f, 3.0f * frame.unit),
            SolidColor{ ClockworkColors::Axle });
        const float glintRadius = std::max(radius * 0.03f, 1.2f * frame.unit);
        fillDisc(frame, center - FloatPoint{ frame.unit, frame.unit }, glintRadius,
            SolidColor{ ClockworkColors::AxleGlint });
    }

    // The escape wheel's teeth are hooks: a long slope up to the tip and a short drop back.
    void ClockworkScene::drawEscapeWheel(const SceneFrame& frame, const FloatPoint center,
        const float size, const float phase)
    {
        const float radius = 62.0f * size;
        const float pitch = k_2Pi / k_escapeTeeth;
        const float rootRadius = radius * 0.82f;
        const auto onCircle = [&](float distance, float angle) {
            return center + FloatPoint{ std::cos(angle), std::sin(angle) } * distance;
        };
        PixelPath wheel;
        for (int i = 0; i != k_escapeTeeth; ++i)
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
            ClockworkColors::SteelLight, ClockworkColors::SteelShade);
        fillShape(frame, wheel, steel);
        strokeShape(frame, wheel, SolidColor{ ClockworkColors::SteelEdge }, size);
        PixelPath spokes;
        for (int i = 0; i != 4; ++i)
        {
            spokes.moveTo(center);
            spokes.lineTo(onCircle(rootRadius * 0.86f,
                phase + static_cast<float>(i) * k_2Pi / 4.0f));
        }
        strokeShape(frame, spokes, steel, 4.0f * size);
        fillDisc(frame, center, 8.0f * size, SolidColor{ ClockworkColors::EscapeHub });
        fillDisc(frame, center, 3.5f * size, SolidColor{ ClockworkColors::Pivot });
    }

    void ClockworkScene::drawAnchor(const SceneFrame& frame, const FloatPoint pivot,
        const float swing, const float size)
    {
        const SceneFrame anchor = frame.within(Matrix3x2::Translation(pivot)
            * Matrix3x2::Rotation(-swing) * Matrix3x2::Scale(size));
        PixelPath arms;
        arms.moveTo(-52.0f, 46.0f);
        arms.quadTo({ -36.0f, -8.0f }, { 0.0f, 0.0f });
        arms.quadTo({ 36.0f, -8.0f }, { 52.0f, 46.0f });
        strokeShape(anchor, arms, LinearGradient::Simple({ -40.0f, 0.0f }, { 40.0f, 40.0f },
            ClockworkColors::SteelLight, ClockworkColors::SteelShade), 7.0f);
        PixelPath pallets;
        for (const float side : { -1.0f, 1.0f })
        {
            pallets.moveTo(side * 47.0f, 42.0f);
            pallets.lineTo(side * 56.0f, 44.0f);
            pallets.lineTo(side * 50.0f, 58.0f);
            pallets.close();
        }
        fillShape(anchor, pallets, SolidColor{ ClockworkColors::Pallet });
        fillDisc(anchor, {}, 7.0f, SolidColor{ ClockworkColors::RodLight });
        fillDisc(anchor, {}, 2.8f, SolidColor{ ClockworkColors::Pivot });
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
        const float burst = fractionalPart(cycleTime * 4.0f);
        for (int i = 0; i != 12; ++i)
        {
            const float age = (burst - static_cast<float>(i) * 0.018f) / 0.5f;
            if (age <= 0.0f || age >= 1.0f)
                continue;
            const FloatPoint puff = {
                valveX + (pseudoRandom(i, 31.0f) - 0.5f) * 40.0f * size * age + 30.0f * size * age,
                pipeY - 34.0f * size - 150.0f * size * std::pow(age, 0.8f)
            };
            const float fade = 1.0f - age;
            fillDisc(frame, puff, (6.0f + 26.0f * age) * size,
                SolidColor{ ClockworkColors::Steam.withOpacity(0.42f * fade * fade) });
        }

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
