module ClaFi.PathArt.MarionetteSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace MarionetteColors
    {
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
    }

    namespace
    {
        void drawStrings(const SceneFrame& frame, const MarionetteJoints& puppet,
            const MarionetteControl& control)
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
            strokeShape(frame, strings, SolidColor{ MarionetteColors::String }, frame.unit,
                StrokeCap::Butt);
        }

        void drawControl(const SceneFrame& frame, const MarionetteControl& control)
        {
            const float unit = frame.unit;
            drawSegment(frame, control.center, control.legBarCenter,
                SolidColor{ MarionetteColors::BarPeg }, 3.0f * unit);
            drawSegment(frame, control.kneeEndLeft, control.kneeEndRight,
                SolidColor{ MarionetteColors::LegBar }, 4.5f * unit);
            drawSegment(frame, control.endLeft, control.endRight,
                SolidColor{ MarionetteColors::BarDark }, 7.5f * unit);
            drawSegment(frame, control.endLeft, control.endRight,
                SolidColor{ MarionetteColors::Bar }, 5.0f * unit);
        }

        void drawPuppet(const SceneFrame& frame, const MarionetteJoints& puppet)
        {
            const float unit = frame.unit;
            const SolidColor wood{ MarionetteColors::Wood };
            const SolidColor woodJoint{ MarionetteColors::WoodJoint };

            for (const bool left : { true, false })
            {
                const FloatPoint hip = left ? puppet.hipLeft : puppet.hipRight;
                const Limb& leg = left ? puppet.legLeft : puppet.legRight;
                drawSegment(frame, hip, leg.joint, wood, 6.5f * unit);
                drawSegment(frame, leg.joint, leg.end, wood, 6.0f * unit);
                fillDisc(frame, leg.joint, 4.0f * unit, woodJoint);
                fillOval(frame, leg.end + FloatPoint{ (left ? -3.0f : 3.0f) * unit, 2.0f * unit },
                    8.0f * unit, 4.5f * unit,
                    SolidColor{ MarionetteColors::Shoe });
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
            fillShape(frame, shorts, SolidColor{ MarionetteColors::Shorts });
            PixelPath shirt;
            shirt.moveTo(shoulderLeft);
            shirt.lineTo(shoulderRight);
            shirt.lineTo(waistRight);
            shirt.lineTo(waistLeft);
            shirt.close();
            fillShape(frame, shirt,
                LinearGradient::Simple(shoulderLeft, waistRight, MarionetteColors::ShirtLight,
                MarionetteColors::ShirtShade));

            for (const bool left : { true, false })
            {
                const FloatPoint shoulder = left ? puppet.shoulderLeft : puppet.shoulderRight;
                const Limb& arm = left ? puppet.armLeft : puppet.armRight;
                drawSegment(frame, shoulder, arm.joint, SolidColor{ MarionetteColors::Sleeve },
                    6.0f * unit);
                drawSegment(frame, arm.joint, arm.end, wood, 5.0f * unit);
                fillDisc(frame, arm.joint, 3.6f * unit, woodJoint);
                fillDisc(frame, arm.end, 4.2f * unit, SolidColor{ MarionetteColors::Skin });
            }

            fillDisc(frame, puppet.head, 15.0f * unit, SolidColor{ MarionetteColors::Skin });
            const SceneFrame face = frame.within(Matrix3x2::Translation(puppet.head)
                * Matrix3x2::Rotation(puppet.lean * 1.6f));
            fillOval(face, { -8.0f * unit, 4.0f * unit }, 4.0f * unit, 2.6f * unit,
                SolidColor{ MarionetteColors::Cheek });
            fillOval(face, { 8.0f * unit, 4.0f * unit }, 4.0f * unit, 2.6f * unit,
                SolidColor{ MarionetteColors::Cheek });
            fillDisc(face, { -5.5f * unit, -2.0f * unit }, 1.9f * unit,
                SolidColor{ MarionetteColors::Ink });
            fillDisc(face, { 5.5f * unit, -2.0f * unit }, 1.9f * unit,
                SolidColor{ MarionetteColors::Ink });
            fillDisc(face, { 0.0f, 2.5f * unit }, 2.4f * unit,
                SolidColor{ MarionetteColors::Nose });
            PixelPath smile;
            smile.moveTo(-4.85f * unit, 7.5f * unit);
            smile.quadTo({ 0.0f, 12.5f * unit }, { 4.85f * unit, 7.5f * unit });
            strokeShape(face, smile, SolidColor{ MarionetteColors::Smile }, 1.4f * unit);
            PixelPath cap;
            cap.moveTo(-14.0f * unit, -7.0f * unit);
            cap.quadTo({ 0.0f, -13.0f * unit }, { 14.0f * unit, -7.0f * unit });
            cap.lineTo(5.0f * unit, -34.0f * unit);
            cap.close();
            fillShape(face, cap, SolidColor{ MarionetteColors::Cap });
            fillDisc(face, { 5.0f * unit, -35.0f * unit }, 3.5f * unit,
                SolidColor{ MarionetteColors::Pompom });
        }
    }

    MarionettePose danceMarionette(const float unit, const FloatPoint feet, const float sway)
    {
        const float step = 2.0f * sway;
        const float puppetX = feet.x;
        const float floorY = feet.y;

        MarionetteJoints puppet{};
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
        MarionetteControl control{};
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
        return {
            .joints = puppet,
            .control = control
        };
    }

    void drawMarionette(const SceneFrame& frame, const MarionettePose& pose)
    {
        drawStrings(frame, pose.joints, pose.control);
        drawControl(frame, pose.control);
        drawPuppet(frame, pose.joints);
    }
}
