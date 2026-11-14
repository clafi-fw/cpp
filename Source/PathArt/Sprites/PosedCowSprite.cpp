module ClaFi.PathArt.PosedCowSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        constexpr float k_bodyLift{ 2.0f }; // how far the body stands up off the legs' round tops
        constexpr float k_headUp{ 0.1f }; // the head drop, in radians, of a cow looking ahead
        constexpr float k_headGrazing{ 1.15f };
        constexpr float k_headReach{ 16.0f }; // how far ahead of its hinge the head hangs

        // A leg hanging from the body at `hip`, swung by `swing` radians, its round foot the hoof.
        void drawLeg(const SceneFrame& cow, FloatPoint hip, float swing, Color leg, Color hoof);
    }

    // Drawn far to near: the far legs and the tail, the body and its spots, the near legs, and
    // the head apart in front. A cow facing left is the same cow mirrored.
    void drawPosedCow(const SceneFrame& frame, const PosedCowPose& pose,
        const PosedCowColors& colors)
    {
        const SceneFrame cow = frame.within(Matrix3x2::Translation(pose.foot)
            * Matrix3x2::Scale(pose.size * pose.facing, pose.size));
        const Color belly = colorBetween(colors.body, colors.detail.fullyOpaque(), 0.16f);
        const Color far = colorBetween(colors.body, colors.detail.fullyOpaque(), 0.3f);
        const float stepAngle = pose.stride ? *pose.stride * k_2Pi : 0.0f;
        const float bob = pose.stride ? std::abs(std::sin(stepAngle)) * 1.5f : 0.0f;
        const float legSwing = pose.stride ? std::sin(stepAngle) * 0.32f : 0.0f;
        const float hipY = -30.0f - bob;

        fillOval(cow, { -2.0f, 1.0f }, 52.0f, 6.0f, SolidColor{ faded(colors.shadow, 0.9f) });
        drawLeg(cow, { -34.0f, hipY }, -legSwing, far, colors.detail);
        drawLeg(cow, { 26.0f, hipY }, legSwing, far, colors.detail);

        const float bodyRise = bob + k_bodyLift;
        const float tailTip = -55.0f + pose.tailSwing * 24.0f;
        PixelPath tail;
        tail.moveTo(-48.0f, -58.0f - bodyRise);
        tail.quadTo({ -58.0f + pose.tailSwing * 10.0f, -44.0f - k_bodyLift },
            { tailTip, -24.0f - k_bodyLift });
        strokeShape(cow, tail, SolidColor{ far }, 2.6f);
        fillOval(cow, { tailTip, -22.0f - k_bodyLift }, 3.0f, 6.0f, SolidColor{ colors.detail },
            pose.tailSwing);

        const SceneFrame body = cow.within(Matrix3x2::Translation(0.0f, -bodyRise));
        PixelPath outline;
        outline.moveTo(-50.0f, -58.0f);
        outline.cubicTo({ -30.0f, -66.0f }, { 10.0f, -60.0f }, { 34.0f, -63.0f });
        outline.cubicTo({ 44.0f, -62.0f }, { 48.0f, -54.0f }, { 46.0f, -44.0f });
        outline.cubicTo({ 44.0f, -34.0f }, { 36.0f, -30.0f }, { 24.0f, -30.0f });
        outline.cubicTo({ 4.0f, -26.0f }, { -24.0f, -28.0f }, { -42.0f, -30.0f });
        outline.cubicTo({ -52.0f, -32.0f }, { -56.0f, -48.0f }, { -50.0f, -58.0f });
        outline.close();
        fillShape(body, outline, LinearGradient{
            .startPoint = { 0.0f, -64.0f },
            .endPoint = { 0.0f, -28.0f },
            .stops = {
                { .position = 0.0f, .color = colors.body },
                { .position = 0.6f, .color = colors.body },
                { .position = 1.0f, .color = belly }
            }
        });
        const Matrix3x2 bodyToCanvas = body.toCanvas;
        frame.canvas.pushClip(outline, &bodyToCanvas);
        const SolidColor spots{ colors.spots };
        fillOval(body, { -28.0f, -54.0f }, 14.0f, 10.0f, spots, 0.3f);
        fillOval(body, { 8.0f, -44.0f }, 11.0f, 9.0f, spots, -0.4f);
        fillOval(body, { 34.0f, -60.0f }, 8.0f, 6.0f, spots, 0.2f);
        frame.canvas.popClip();
        fillOval(body, { -16.0f, -28.0f }, 7.0f, 4.0f, SolidColor{ colors.snout });

        drawLeg(cow, { -40.0f, hipY }, legSwing, colors.body, colors.detail);
        drawLeg(cow, { 32.0f, hipY }, -legSwing, colors.body, colors.detail);

        // The head hangs apart from the body; its hinge rises with the body as the head comes up,
        // and stays down so the muzzle reaches.
        const float hingeLift = k_bodyLift
            * (1.0f - smoothStep(k_headUp, k_headGrazing, pose.headDrop));
        const SceneFrame hinge = cow.within(Matrix3x2::Translation(40.0f, -52.0f - bob - hingeLift)
            * Matrix3x2::Rotation(pose.headDrop));
        const SceneFrame head = hinge.within(Matrix3x2::Translation(k_headReach, 0.0f));
        PixelPath shape;
        shape.moveTo(-2.0f, -12.0f);
        shape.cubicTo({ 14.0f, -14.0f }, { 26.0f, -8.0f }, { 30.0f, 2.0f });
        shape.lineTo(30.0f, 10.0f + pose.chew);
        shape.cubicTo({ 20.0f, 14.0f }, { 6.0f, 12.0f }, { -2.0f, 10.0f });
        shape.arcTo({ 11.0f, 11.0f }, 0.0f, ArcSize::Small, ArcSweep::Clockwise, { -2.0f, -12.0f });
        shape.close();
        fillShape(head, shape, SolidColor{ colors.body });
        fillOval(head, { 29.0f, 5.0f + pose.chew * 0.5f }, 8.0f, 7.5f,
            SolidColor{ colors.snout });
        fillDisc(head, { 32.0f, 3.0f }, 1.3f, SolidColor{ colors.detail });
        fillDisc(head, { 12.0f, -4.0f }, 2.2f, SolidColor{ colors.detail });
        fillOval(head, { 4.0f, -6.0f }, 6.0f, 5.0f, spots);
        fillOval(head, { 0.0f, -10.0f }, 8.0f, 3.5f, SolidColor{ far }, -0.6f);
        PixelPath horn;
        horn.moveTo(6.0f, -12.0f);
        horn.quadTo({ 4.0f, -22.0f }, { 10.0f, -24.0f });
        strokeShape(head, horn, SolidColor{
            colorBetween(colors.body, colors.detail.fullyOpaque(), 0.35f) }, 3.0f);
    }

    namespace
    {
        void drawLeg(const SceneFrame& cow, const FloatPoint hip, const float swing,
            const Color leg, const Color hoof)
        {
            const SceneFrame hanging = cow.within(Matrix3x2::Translation(hip)
                * Matrix3x2::Rotation(swing));
            constexpr float k_halfWidth{ 4.75f };
            constexpr float k_endRound{ k_halfWidth }; // both ends round
            constexpr float k_top{ -5.0f }; // above the hip, so a near leg's top shows on the belly
            constexpr float k_hoofTop{ 26.0f };
            constexpr float k_sole{ 31.0f };
            PixelPath shape;
            shape.drawRoundedRect({ -k_halfWidth, k_top, k_halfWidth, k_sole }, k_endRound,
                k_endRound);
            fillShape(hanging, shape, SolidColor{ leg });
            PixelPath foot;
            foot.moveTo(-k_halfWidth, k_hoofTop);
            foot.lineTo(k_halfWidth, k_hoofTop);
            foot.lineTo(k_halfWidth, k_sole);
            foot.lineTo(-k_halfWidth, k_sole);
            foot.close();
            const Matrix3x2 legToCanvas = hanging.toCanvas;
            hanging.canvas.pushClip(foot, &legToCanvas);
            fillShape(hanging, shape, SolidColor{ hoof });
            hanging.canvas.popClip();
        }
    }
}
