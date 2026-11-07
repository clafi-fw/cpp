module ClaFi.PathArt.PuppeteerScene;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace PuppeteerColors
    {
        constexpr Color WallTop{ 0xFF2C1832 };
        constexpr Color WallBottom{ 0xFF170C1B };
        constexpr Color WallGlow{ 0xFF6B2C4F };
        constexpr Color FloorBack{ 0xFF3D2515 };
        constexpr Color FloorFront{ 0xFF7C5434 };
        constexpr Color FloorSeam{ 0xFF24150B };
        constexpr Color FloorEdge{ 0xFF0D070B };
        constexpr Color Spotlight{ 0xFFFFF1C9 };
        constexpr Color SpotlightPool{ 0xFFFFE9B0 };
        constexpr Color MetalLight{ 0xFFEEF2F5 };
        constexpr Color MetalShade{ 0xFF97A3AF };
        constexpr Color MetalEdge{ 0xFF3B4450 };
        constexpr Color Limb{ 0xFF8793A0 };
        constexpr Color Joint{ 0xFF4B5563 };
        constexpr Color Foot{ 0xFF5B6573 };
        constexpr Color Pelvis{ 0xFF7D8995 };
        constexpr Color Panel{ 0xFF1D2430 };
        constexpr Color Neck{ 0xFF6B7684 };
        constexpr Color HeadShade{ 0xFFA3AEB9 };
        constexpr Color HeadLight{ 0xFFF3F6F8 };
        constexpr Color Visor{ 0xFF0F1420 };
        constexpr Color Eye{ 0xFF7AF4FF };
        constexpr Color EyeGlow{ 0xFF5EF0FF };
        constexpr Color AntennaOn{ 0xFFFF5A5A };
        constexpr Color AntennaOff{ 0xFF7A2A2A };
        constexpr Color AntennaGlow{ 0xFFFF4D4D };
        constexpr Color Hand{ 0xFFA9B3BD };
        constexpr Color String{ 0xB8F6E7C8 };
        constexpr Color BarDark{ 0xFF5B3A20 };
        constexpr Color Bar{ 0xFF8A5A33 };
        constexpr Color LegBar{ 0xFF9B6A3E };
        constexpr Color BarPeg{ 0xFF6E4526 };
        constexpr Color Wood{ 0xFFE0AE74 };
        constexpr Color WoodJoint{ 0xFFB9824A };
        constexpr Color Skin{ 0xFFEFC48D };
        constexpr Color Shoe{ 0xFF5A2E1A };
        constexpr Color Shorts{ 0xFFC2412D };
        constexpr Color ShirtLight{ 0xFF3D7FCB };
        constexpr Color ShirtShade{ 0xFF285C99 };
        constexpr Color Sleeve{ 0xFF2F6DB5 };
        constexpr Color Cheek{ 0x99E8846B };
        constexpr Color Ink{ 0xFF2B1D14 };
        constexpr Color Nose{ 0xFFE0A070 };
        constexpr Color Smile{ 0xFF7A3A24 };
        constexpr Color Cap{ 0xFFC2412D };
        constexpr Color Pompom{ 0xFFFFD34D };
        constexpr Color Dust{ 0xFFFFF1C9 };
        constexpr Color CurtainShade{ 0xFF4F0B18 };
        constexpr Color CurtainLight{ 0xFFAB2740 };
        constexpr Color ValanceTop{ 0xFF9A1D33 };
        constexpr Color ValanceBottom{ 0xFF6A1022 };
        constexpr Color Gold{ 0xFFD9A845 };
        constexpr std::array<Color, 3> ChestLights{ Color{ 0xFF5EF0FF }, Color{ 0xFFFFD166 },
            Color{ 0xFFFF6B8B } };
    }

    namespace
    {
        // The floor's top edge and the line the feet stand on, as shares of the frame's height.
        constexpr float k_floorTopShare{ 0.64f };
        constexpr float k_floorLineShare{ 0.85f };
        // Sways of the dance per cycle; a step falls on every half sway.
        constexpr float k_swaysPerCycle{ 16.0f };
    }

    void PuppeteerScene::draw(Canvas& canvas, const FloatRect& bounds, const float cycleTime)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const float centerX = frame.width * 0.5f;
        const float floorY = frame.height * k_floorLineShare;
        // A narrow frame draws the puppet and the android closer together.
        const float closeness = std::min(1.0f, (frame.width / unit) / 640.0f);
        const float sway = k_2Pi * k_swaysPerCycle * cycleTime;
        const float step = 2.0f * sway;

        // The puppet's skeleton
        Puppet puppet{};
        const float puppetX = centerX - 78.0f * unit * closeness + 34.0f * unit * std::sin(sway);
        const float liftLeft = std::max(0.0f, std::sin(step));
        const float liftRight = std::max(0.0f, -std::sin(step));
        const FloatPoint footLeft = {
            puppetX - 12.0f * unit - 7.0f * unit * liftLeft,
            floorY - 24.0f * unit * liftLeft
        };
        const FloatPoint footRight = {
            puppetX + 12.0f * unit + 7.0f * unit * liftRight,
            floorY - 24.0f * unit * liftRight
        };
        puppet.pelvis = { puppetX, floorY - 50.0f * unit - 5.0f * unit * std::abs(std::sin(step)) };
        puppet.lean = 0.12f * std::sin(sway + 0.6f);
        puppet.neck = puppet.pelvis + rotated({ 0.0f, -40.0f * unit }, puppet.lean);
        puppet.head = puppet.neck + rotated({ 0.0f, -17.0f * unit }, puppet.lean * 1.6f);
        puppet.shoulderLeft = puppet.neck + rotated({ -14.0f * unit, 5.0f * unit }, puppet.lean);
        puppet.shoulderRight = puppet.neck + rotated({ 14.0f * unit, 5.0f * unit }, puppet.lean);
        puppet.hipLeft = puppet.pelvis + rotated({ -8.0f * unit, 0.0f }, puppet.lean * 0.5f);
        puppet.hipRight = puppet.pelvis + rotated({ 8.0f * unit, 0.0f }, puppet.lean * 0.5f);
        puppet.legLeft = reachToward(puppet.hipLeft, footLeft, 27.0f * unit, 27.0f * unit,
            BendToward::Left);
        puppet.legRight = reachToward(puppet.hipRight, footRight, 27.0f * unit, 27.0f * unit,
            BendToward::Right);

        // The control: its tilt lifts a hand, its leg bar's rock lifts a knee.
        Control control{};
        control.center = {
            puppet.head.x + 6.0f * unit * std::sin(sway - 0.5f),
            floorY - 284.0f * unit + 7.0f * unit * std::sin(step)
        };
        const float tilt = 0.42f * std::sin(sway);
        control.endLeft = control.center + rotated({ -38.0f * unit, 0.0f }, tilt);
        control.endRight = control.center + rotated({ 38.0f * unit, 0.0f }, tilt);
        control.legBarCenter = control.center + FloatPoint{ 0.0f, 16.0f * unit };
        const float legTilt = 0.5f * std::sin(step);
        control.kneeEndLeft = control.legBarCenter + rotated({ -24.0f * unit, 0.0f }, legTilt);
        control.kneeEndRight = control.legBarCenter + rotated({ 24.0f * unit, 0.0f }, legTilt);
        const FloatPoint handDrop = { 0.0f, 170.0f * unit };
        puppet.armLeft = reachToward(puppet.shoulderLeft, control.endLeft + handDrop, 19.0f * unit,
            19.0f * unit, BendToward::Left);
        puppet.armRight = reachToward(puppet.shoulderRight, control.endRight + handDrop,
            19.0f * unit, 19.0f * unit, BendToward::Right);

        drawStage(frame);
        const float lampX = centerX - 56.0f * unit * closeness;
        drawSpotlight(frame, lampX, puppetX, floorY);

        const float androidX = centerX + 80.0f * unit * closeness;
        drawAndroid(frame, androidX, floorY, cycleTime);

        // The android's left arm holds the control - the upper arm here, the forearm over the bar.
        const float bob = -2.5f * unit * std::abs(std::sin(step));
        const FloatPoint shoulder = { androidX - 47.0f * unit, floorY - 218.0f * unit + bob };
        const FloatPoint grip = control.center - FloatPoint{ 0.0f, 9.0f * unit };
        const Limb arm = reachToward(shoulder, grip, 87.0f * unit, 87.0f * unit, BendToward::Down);
        drawSegment(frame, shoulder, arm.joint, SolidColor{ PuppeteerColors::Limb }, 13.0f * unit);
        fillDisc(frame, shoulder, 11.0f * unit, SolidColor{ PuppeteerColors::Joint });

        drawStrings(frame, puppet, control);
        drawControl(frame, control);
        drawPuppet(frame, puppet);

        drawSegment(frame, arm.joint, arm.end, SolidColor{ PuppeteerColors::Limb }, 12.0f * unit);
        fillDisc(frame, arm.joint, 7.5f * unit, SolidColor{ PuppeteerColors::Joint });
        const FloatRect hand = {
            grip.x - 10.0f * unit,
            grip.y - 6.0f * unit,
            grip.x + 10.0f * unit,
            grip.y + 10.0f * unit
        };
        PixelPath handPath;
        handPath.drawRoundedRect(hand, 6.0f * unit, 6.0f * unit);
        fillShape(frame, handPath, SolidColor{ PuppeteerColors::Hand });
        strokeShape(frame, handPath, SolidColor{ PuppeteerColors::MetalEdge }, 1.2f * unit);

        drawDust(frame, lampX, puppetX, floorY, cycleTime);
        drawCurtains(frame, cycleTime);
    }

    void PuppeteerScene::drawStage(const SceneFrame& frame)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const float centerX = width * 0.5f;
        const float floorTop = height * k_floorTopShare;

        fillBox(frame, { 0.0f, 0.0f, width, floorTop },
            LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, floorTop }, PuppeteerColors::WallTop,
                PuppeteerColors::WallBottom));
        drawGlow(frame, { centerX - 40.0f * unit, floorTop - 30.0f * unit }, 300.0f * unit,
            PuppeteerColors::WallGlow.withOpacity(0.35f), 0.55f);

        fillBox(frame, { 0.0f, floorTop, width, height },
            LinearGradient::Simple({ 0.0f, floorTop }, { 0.0f, height }, PuppeteerColors::FloorBack,
                PuppeteerColors::FloorFront));

        // The planks run toward a point high above the stage: the floor as seen from the stalls.
        const float vanishingY = floorTop - height * 0.9f;
        const float share = (floorTop - vanishingY) / (height - vanishingY);
        PixelPath planks;
        for (int i = -18; i <= 18; ++i)
        {
            const float bottomX = centerX + static_cast<float>(i) * 54.0f * unit;
            planks.moveTo(centerX + (bottomX - centerX) * share, floorTop);
            planks.lineTo(bottomX, height);
        }
        strokeShape(frame, planks, SolidColor{ PuppeteerColors::FloorSeam.withOpacity(0.5f) },
            1.2f * unit, StrokeCap::Butt);

        PixelPath seams;
        for (const float depth : { 0.1f, 0.25f, 0.44f, 0.68f, 0.96f })
        {
            const float y = floorTop + (height - floorTop) * depth;
            seams.moveTo(0.0f, y);
            seams.lineTo(width, y);
        }
        strokeShape(frame, seams, SolidColor{ PuppeteerColors::FloorSeam.withOpacity(0.32f) }, unit,
            StrokeCap::Butt);
        fillBox(frame, { 0.0f, floorTop - 4.0f * unit, width, floorTop + 3.0f * unit },
            SolidColor{ PuppeteerColors::FloorEdge.withOpacity(0.85f) });
    }

    void PuppeteerScene::drawSpotlight(const SceneFrame& frame, const float lampX,
        const float puppetX, const float floorY)
    {
        const float unit = frame.unit;
        PixelPath cone;
        cone.moveTo(lampX - 26.0f * unit, 0.0f);
        cone.lineTo(lampX + 26.0f * unit, 0.0f);
        cone.lineTo(puppetX + 112.0f * unit, floorY + 14.0f * unit);
        cone.lineTo(puppetX - 112.0f * unit, floorY + 14.0f * unit);
        cone.close();
        const Color beam = PuppeteerColors::Spotlight;
        fillShape(frame, cone, LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, floorY },
            beam.withOpacity(0.02f), beam.withOpacity(0.17f)));
        drawGlow(frame, { puppetX, floorY + 6.0f * unit }, 50.0f * unit,
            PuppeteerColors::SpotlightPool.withOpacity(0.5f), 0.42f);
    }

    void PuppeteerScene::drawAndroid(const SceneFrame& frame, const float androidX,
        const float floorY, const float cycleTime)
    {
        const float unit = frame.unit;
        const float sway = k_2Pi * k_swaysPerCycle * cycleTime;
        const float step = 2.0f * sway;
        const float bob = -2.5f * unit * std::abs(std::sin(step));
        // Places on the body, in units from its middle and from the floor; only the feet keep
        // still.
        const auto bodyX = [&](float units) {
            return androidX + units * unit;
        };
        const auto bodyY = [&](float units) {
            return floorY + units * unit + bob;
        };
        const auto footY = [&](float units) {
            return floorY + units * unit;
        };
        const SolidColor limb{ PuppeteerColors::Limb };
        const SolidColor joint{ PuppeteerColors::Joint };

        // legs and feet
        drawSegment(frame, { bodyX(-17.0f), bodyY(-108.0f) }, { bodyX(-19.0f), footY(-14.0f) },
            limb, 16.0f * unit);
        drawSegment(frame, { bodyX(17.0f), bodyY(-108.0f) }, { bodyX(19.0f), footY(-14.0f) }, limb,
            16.0f * unit);
        fillDisc(frame, { bodyX(-18.0f), footY(-60.0f) + bob * 0.5f }, 9.0f * unit, joint);
        fillDisc(frame, { bodyX(18.0f), footY(-60.0f) + bob * 0.5f }, 9.0f * unit, joint);
        const SolidColor foot{ PuppeteerColors::Foot };
        fillRoundedBox(frame, { bodyX(-38.0f), footY(-17.0f), bodyX(-6.0f), floorY }, 6.0f * unit,
            foot);
        fillRoundedBox(frame, { bodyX(6.0f), footY(-17.0f), bodyX(38.0f), floorY }, 6.0f * unit,
            foot);

        // pelvis, torso and the chest panel
        fillRoundedBox(frame, { bodyX(-30.0f), bodyY(-130.0f), bodyX(30.0f), bodyY(-104.0f) },
            8.0f * unit, SolidColor{ PuppeteerColors::Pelvis });
        PixelPath torso;
        const FloatRect torsoBox = { bodyX(-42.0f), bodyY(-234.0f), bodyX(42.0f), bodyY(-126.0f) };
        torso.drawRoundedRect(torsoBox, 18.0f * unit, 18.0f * unit);
        fillShape(frame, torso, LinearGradient::Simple({ bodyX(-40.0f), bodyY(-232.0f) },
            { bodyX(40.0f), bodyY(-120.0f) }, PuppeteerColors::MetalLight,
            PuppeteerColors::MetalShade));
        strokeShape(frame, torso, SolidColor{ PuppeteerColors::MetalEdge }, 1.6f * unit);
        fillRoundedBox(frame, { bodyX(-24.0f), bodyY(-206.0f), bodyX(24.0f), bodyY(-166.0f) },
            7.0f * unit, SolidColor{ PuppeteerColors::Panel });
        // The chest lights chase the beat, one lit at a time.
        const int litLight = static_cast<int>(fractionalPart(cycleTime * 32.0f) * 3.0f);
        for (int i = 0; i != 3; ++i)
        {
            const FloatPoint light = { bodyX(static_cast<float>(i - 1) * 13.0f), bodyY(-186.0f) };
            const Color color = PuppeteerColors::ChestLights[static_cast<std::size_t>(i)];
            const bool lit = i == litLight;
            if (lit)
                drawGlow(frame, light, 12.0f * unit, color.withOpacity(0.6f));
            fillDisc(frame, light, 4.2f * unit,
                SolidColor{ color.withOpacity(lit ? 1.0f : 0.28f) });
        }

        // neck and the head, turned toward the puppet
        fillBox(frame, { bodyX(-8.0f), bodyY(-250.0f), bodyX(8.0f), bodyY(-232.0f) },
            SolidColor{ PuppeteerColors::Neck });
        const SceneFrame head = frame.within(Matrix3x2::Translation(androidX, bodyY(-246.0f))
            * Matrix3x2::Rotation(-0.1f + 0.06f * std::sin(sway)));
        fillDisc(head, { -38.0f * unit, -28.0f * unit }, 8.0f * unit, joint);
        fillDisc(head, { 38.0f * unit, -28.0f * unit }, 8.0f * unit, joint);
        PixelPath skull;
        skull.drawRoundedRect(FloatRect{ -36.0f * unit, -58.0f * unit, 36.0f * unit, 0.0f },
            22.0f * unit, 22.0f * unit);
        fillShape(head, skull, LinearGradient::Simple({ -30.0f * unit, -58.0f * unit },
            { 30.0f * unit, 0.0f }, PuppeteerColors::HeadLight, PuppeteerColors::HeadShade));
        strokeShape(head, skull, SolidColor{ PuppeteerColors::MetalEdge }, 1.6f * unit);
        fillRoundedBox(head, { -28.0f * unit, -46.0f * unit, 28.0f * unit, -21.0f * unit },
            10.0f * unit, SolidColor{ PuppeteerColors::Visor });
        const float blink = fractionalPart(cycleTime * 4.0f + 0.2f) > 0.975f ? 0.15f : 1.0f;
        const float look = -5.0f * unit + 2.0f * unit * std::sin(sway);
        const float eyeY = -34.0f * unit;
        const float eyeHeight = 12.0f * unit * blink;
        for (const float eyeLeft : { -19.0f, 5.0f })
        {
            const float left = eyeLeft * unit + look;
            drawGlow(head, { left + 7.0f * unit, eyeY }, 13.0f * unit,
                PuppeteerColors::EyeGlow.withOpacity(0.35f));
            const FloatRect eye = {
                left,
                eyeY - eyeHeight * 0.5f,
                left + 14.0f * unit,
                eyeY + eyeHeight * 0.5f
            };
            fillRoundedBox(head, eye, 4.0f * unit * blink, SolidColor{ PuppeteerColors::Eye });
        }
        PixelPath grille;
        for (int i = 0; i != 3; ++i)
        {
            const float x = static_cast<float>(-9 + i * 9) * unit;
            grille.moveTo(x, -12.0f * unit);
            grille.lineTo(x, -6.0f * unit);
        }
        strokeShape(head, grille, SolidColor{ PuppeteerColors::Neck }, 2.4f * unit);
        drawSegment(head, { 0.0f, -58.0f * unit }, { 0.0f, -78.0f * unit },
            SolidColor{ PuppeteerColors::Neck }, 3.0f * unit);
        const bool antennaLit = fractionalPart(cycleTime * 16.0f) < 0.5f;
        if (antennaLit)
            drawGlow(head, { 0.0f, -83.0f * unit }, 14.0f * unit,
                PuppeteerColors::AntennaGlow.withOpacity(0.6f));
        fillDisc(head, { 0.0f, -83.0f * unit }, 5.0f * unit,
            SolidColor{ antennaLit ? PuppeteerColors::AntennaOn : PuppeteerColors::AntennaOff });

        // the right arm, its hand on the hip
        const FloatPoint shoulder = { bodyX(47.0f), bodyY(-218.0f) };
        const FloatPoint elbow = { bodyX(80.0f), bodyY(-172.0f) };
        const FloatPoint hand = { bodyX(44.0f), bodyY(-136.0f) };
        drawSegment(frame, shoulder, elbow, limb, 13.0f * unit);
        drawSegment(frame, elbow, hand, limb, 12.0f * unit);
        fillDisc(frame, elbow, 7.5f * unit, joint);
        fillDisc(frame, hand, 9.0f * unit, SolidColor{ PuppeteerColors::Hand });
        fillDisc(frame, shoulder, 11.0f * unit, joint);
    }

    void PuppeteerScene::drawStrings(const SceneFrame& frame, const Puppet& puppet,
        const Control& control)
    {
        const FloatPoint headTop = puppet.head
            + rotated({ 0.0f, -15.0f * frame.unit }, puppet.lean * 1.6f);
        PixelPath strings;
        strings.moveTo(control.center);
        strings.lineTo(headTop);
        strings.moveTo(control.endLeft);
        strings.lineTo(puppet.armLeft.end);
        strings.moveTo(control.endRight);
        strings.lineTo(puppet.armRight.end);
        strings.moveTo(control.kneeEndLeft);
        strings.lineTo(puppet.legLeft.joint);
        strings.moveTo(control.kneeEndRight);
        strings.lineTo(puppet.legRight.joint);
        strokeShape(frame, strings, SolidColor{ PuppeteerColors::String }, frame.unit,
            StrokeCap::Butt);
    }

    void PuppeteerScene::drawControl(const SceneFrame& frame, const Control& control)
    {
        const float unit = frame.unit;
        drawSegment(frame, control.center, control.legBarCenter,
            SolidColor{ PuppeteerColors::BarPeg }, 3.0f * unit);
        drawSegment(frame, control.kneeEndLeft, control.kneeEndRight,
            SolidColor{ PuppeteerColors::LegBar }, 4.5f * unit);
        drawSegment(frame, control.endLeft, control.endRight,
            SolidColor{ PuppeteerColors::BarDark }, 7.5f * unit);
        drawSegment(frame, control.endLeft, control.endRight, SolidColor{ PuppeteerColors::Bar },
            5.0f * unit);
    }

    void PuppeteerScene::drawPuppet(const SceneFrame& frame, const Puppet& puppet)
    {
        const float unit = frame.unit;
        const SolidColor wood{ PuppeteerColors::Wood };
        const SolidColor woodJoint{ PuppeteerColors::WoodJoint };

        for (const bool left : { true, false })
        {
            const FloatPoint hip = left ? puppet.hipLeft : puppet.hipRight;
            const Limb& leg = left ? puppet.legLeft : puppet.legRight;
            drawSegment(frame, hip, leg.joint, wood, 6.5f * unit);
            drawSegment(frame, leg.joint, leg.end, wood, 6.0f * unit);
            fillDisc(frame, leg.joint, 4.0f * unit, woodJoint);
            fillOval(frame, leg.end + FloatPoint{ (left ? -3.0f : 3.0f) * unit, 2.0f * unit },
                8.0f * unit, 4.5f * unit,
                SolidColor{ PuppeteerColors::Shoe });
        }

        const auto atNeck = [&](float x, float y) {
            return puppet.neck + rotated({ x * unit, y * unit }, puppet.lean);
        };
        const auto atPelvis = [&](float x, float y, float leanShare) {
            return puppet.pelvis + rotated({ x * unit, y * unit }, puppet.lean * leanShare);
        };
        const FloatPoint shoulderLeft = atNeck(-13.0f, 4.0f);
        const FloatPoint shoulderRight = atNeck(13.0f, 4.0f);
        const FloatPoint waistLeft = atPelvis(-10.0f, -8.0f, 0.6f);
        const FloatPoint waistRight = atPelvis(10.0f, -8.0f, 0.6f);
        const FloatPoint hipLeft = atPelvis(-11.0f, 6.0f, 0.5f);
        const FloatPoint hipRight = atPelvis(11.0f, 6.0f, 0.5f);
        PixelPath shorts;
        shorts.moveTo(waistLeft);
        shorts.lineTo(waistRight);
        shorts.lineTo(hipRight);
        shorts.lineTo(hipLeft);
        shorts.close();
        fillShape(frame, shorts, SolidColor{ PuppeteerColors::Shorts });
        PixelPath shirt;
        shirt.moveTo(shoulderLeft);
        shirt.lineTo(shoulderRight);
        shirt.lineTo(waistRight);
        shirt.lineTo(waistLeft);
        shirt.close();
        fillShape(frame, shirt,
            LinearGradient::Simple(shoulderLeft, waistRight, PuppeteerColors::ShirtLight,
            PuppeteerColors::ShirtShade));

        for (const bool left : { true, false })
        {
            const FloatPoint shoulder = left ? puppet.shoulderLeft : puppet.shoulderRight;
            const Limb& arm = left ? puppet.armLeft : puppet.armRight;
            drawSegment(frame, shoulder, arm.joint, SolidColor{ PuppeteerColors::Sleeve },
                6.0f * unit);
            drawSegment(frame, arm.joint, arm.end, wood, 5.0f * unit);
            fillDisc(frame, arm.joint, 3.6f * unit, woodJoint);
            fillDisc(frame, arm.end, 4.2f * unit, SolidColor{ PuppeteerColors::Skin });
        }

        fillDisc(frame, puppet.head, 15.0f * unit, SolidColor{ PuppeteerColors::Skin });
        const SceneFrame face = frame.within(Matrix3x2::Translation(puppet.head)
            * Matrix3x2::Rotation(puppet.lean * 1.6f));
        fillOval(face, { -8.0f * unit, 4.0f * unit }, 4.0f * unit, 2.6f * unit,
            SolidColor{ PuppeteerColors::Cheek });
        fillOval(face, { 8.0f * unit, 4.0f * unit }, 4.0f * unit, 2.6f * unit,
            SolidColor{ PuppeteerColors::Cheek });
        fillDisc(face, { -5.5f * unit, -2.0f * unit }, 1.9f * unit,
            SolidColor{ PuppeteerColors::Ink });
        fillDisc(face, { 5.5f * unit, -2.0f * unit }, 1.9f * unit,
            SolidColor{ PuppeteerColors::Ink });
        fillDisc(face, { 0.0f, 2.5f * unit }, 2.4f * unit, SolidColor{ PuppeteerColors::Nose });
        PixelPath smile;
        smile.moveTo(-4.85f * unit, 7.5f * unit);
        smile.quadTo({ 0.0f, 12.5f * unit }, { 4.85f * unit, 7.5f * unit });
        strokeShape(face, smile, SolidColor{ PuppeteerColors::Smile }, 1.4f * unit);
        PixelPath cap;
        cap.moveTo(-14.0f * unit, -7.0f * unit);
        cap.quadTo({ 0.0f, -13.0f * unit }, { 14.0f * unit, -7.0f * unit });
        cap.lineTo(5.0f * unit, -34.0f * unit);
        cap.close();
        fillShape(face, cap, SolidColor{ PuppeteerColors::Cap });
        fillDisc(face, { 5.0f * unit, -35.0f * unit }, 3.5f * unit,
            SolidColor{ PuppeteerColors::Pompom });
    }

    void PuppeteerScene::drawDust(const SceneFrame& frame, const float lampX, const float puppetX,
        const float floorY, const float cycleTime)
    {
        const float unit = frame.unit;
        for (int i = 0; i != 24; ++i)
        {
            const float rise = fractionalPart(pseudoRandom(i, 1.0f) - cycleTime * 3.0f);
            const float y = std::lerp(floorY, frame.height * 0.04f, rise);
            const float depth = y / floorY;
            const float scatter = (pseudoRandom(i) - 0.5f) * 180.0f * unit;
            const float x = std::lerp(lampX, puppetX, depth) + scatter * depth;
            const float radius = (0.7f + 1.1f * pseudoRandom(i, 2.0f)) * unit;
            const float opacity = 0.45f * std::sin(std::numbers::pi_v<float> * rise);
            fillDisc(frame, { x, y }, radius,
                SolidColor{ PuppeteerColors::Dust.withOpacity(opacity) });
        }
    }

    void PuppeteerScene::drawCurtains(const SceneFrame& frame, const float cycleTime)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        const float drapeWidth = std::max(width * 0.11f, 62.0f * unit);
        constexpr int k_folds = 5;
        const float foldWidth = drapeWidth / k_folds;
        for (const bool left : { true, false })
        {
            for (int i = 0; i != k_folds; ++i)
            {
                const float foldLeft = left
                    ? static_cast<float>(i) * foldWidth
                    : width - static_cast<float>(i + 1) * foldWidth;
                const float foldRight = foldLeft + foldWidth;
                const float wavePhase = static_cast<float>(i) * 0.9f + (left ? 0.0f : 2.0f);
                const float swing = 4.0f * unit * std::sin(k_2Pi * 2.0f * cycleTime + wavePhase);
                PixelPath fold;
                fold.moveTo(foldLeft, 0.0f);
                fold.lineTo(foldRight, 0.0f);
                fold.lineTo(foldRight + swing, height);
                fold.lineTo(foldLeft + swing, height);
                fold.close();
                fillShape(frame, fold, LinearGradient{
                    .startPoint = { foldLeft, 0.0f },
                    .endPoint = { foldRight, 0.0f },
                    .stops = {
                        { .position = 0.0f, .color = PuppeteerColors::CurtainShade },
                        { .position = 0.5f, .color = PuppeteerColors::CurtainLight },
                        { .position = 1.0f, .color = PuppeteerColors::CurtainShade }
                    }
                });
            }
        }

        const float valanceHeight = std::max(height * 0.085f, 40.0f * unit);
        const float scallop = 64.0f * unit;
        PixelPath valance;
        valance.moveTo(0.0f, 0.0f);
        valance.lineTo(width, 0.0f);
        valance.lineTo(width, valanceHeight);
        PixelPath trim;
        trim.moveTo(width, valanceHeight);
        for (float x = width; x > -scallop; x -= scallop)
        {
            valance.quadTo({ x - scallop * 0.5f, valanceHeight + 20.0f * unit },
                { x - scallop, valanceHeight });
            trim.quadTo({ x - scallop * 0.5f, valanceHeight + 20.0f * unit },
                { x - scallop, valanceHeight });
        }
        valance.lineTo(-scallop, 0.0f);
        valance.close();
        fillShape(frame, valance,
            LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, valanceHeight + 10.0f * unit },
            PuppeteerColors::ValanceTop, PuppeteerColors::ValanceBottom));
        strokeShape(frame, trim, SolidColor{ PuppeteerColors::Gold }, 3.0f * unit);
    }
}
