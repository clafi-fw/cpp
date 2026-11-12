module ClaFi.PathArt.GatorSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace GatorColors
    {
        constexpr Color Body{ 0xFF4F7D3C };
        constexpr Color Dark{ 0xFF36592A };
        constexpr Color Belly{ 0xCCB3C07C };
        constexpr Color JawUnderside{ 0xB3B3C07C };
        constexpr Color Mouth{ 0xFFC25067 };
        constexpr Color Throat{ 0xCC6E2236 };
        constexpr Color Tongue{ 0xFFE07A8C };
        constexpr Color Tooth{ 0xFFFBF6E8 };
        constexpr Color Eye{ 0xFFFFD34D };
        constexpr Color Pupil{ 0xFF1B1D22 };
        constexpr Color Pore{ 0xE636592A };
    }

    namespace
    {
        // The head turns up about the neck, the upper jaw about the back of the mouth.
        constexpr FloatPoint k_neck{ 0.0f, -30.0f };
        constexpr FloatPoint k_mouthBack{ 34.0f, 0.0f };
        constexpr float k_jawTip{ 150.0f };
        // Where the body gives way to the tail, and the tail's tip.
        constexpr float k_tailRoot{ -175.0f };
        constexpr float k_tailTip{ -305.0f };

        [[nodiscard]] float topAt(float x, float tailDrop);
        [[nodiscard]] float underAt(float x, float tailDrop);
        [[nodiscard]] SceneFrame bodyFrameOf(const SceneFrame&, const GatorPose&);
        [[nodiscard]] SceneFrame headFrameOf(const SceneFrame&, const GatorPose&);
        void drawLeg(const SceneFrame& body, float x, bool far);
    }

    FloatPoint gatorHeadToFrame(const GatorPose& pose, const FloatPoint headPoint)
    {
        const FloatPoint lifted = k_neck + rotated(headPoint - k_neck, -pose.headLift);
        return pose.neck + lifted * pose.size;
    }

    FloatPoint gatorUpperJawToFrame(const GatorPose& pose, const FloatPoint jawPoint)
    {
        return gatorHeadToFrame(pose, k_mouthBack + rotated(jawPoint - k_mouthBack, -pose.gape));
    }

    FloatPoint gatorMouthPoint(const GatorPose& pose, const float depth)
    {
        const float midway = -pose.gape * 0.5f + 0.01f;
        return gatorHeadToFrame(pose,
            k_mouthBack + FloatPoint{ std::cos(midway), std::sin(midway) } * depth);
    }

    void drawGator(const SceneFrame& frame, const GatorPose& pose)
    {
        const SceneFrame body = bodyFrameOf(frame, pose);
        drawLeg(body, -40.0f, true);
        drawLeg(body, -150.0f, true);
        PixelPath outline;
        outline.moveTo(12.0f, topAt(12.0f, pose.tailDrop));
        for (float x = 12.0f; x >= k_tailTip; x -= 6.0f)
            outline.lineTo(x, topAt(x, pose.tailDrop));
        for (float x = k_tailTip; x <= 12.0f; x += 6.0f)
            outline.lineTo(x, underAt(x, pose.tailDrop));
        outline.close();
        fillShape(body, outline, LinearGradient::Simple({ 0.0f, -48.0f }, { 0.0f, 0.0f },
            GatorColors::Body, GatorColors::Dark));
        PixelPath belly;
        belly.moveTo(6.0f, -10.0f);
        belly.quadTo({ -80.0f, -16.0f }, { -170.0f, -9.0f });
        belly.lineTo(-170.0f, 0.0f);
        belly.lineTo(6.0f, 0.0f);
        belly.close();
        fillShape(body, belly, SolidColor{ GatorColors::Belly });
        PixelPath ridges;
        for (float x = -14.0f; x > k_tailTip + 10.0f; x -= 15.0f)
        {
            const float y = topAt(x, pose.tailDrop);
            ridges.moveTo(x - 5.0f, y + 2.0f);
            ridges.lineTo(x, y - 6.0f);
            ridges.lineTo(x + 5.0f, y + 2.0f);
            ridges.close();
        }
        fillShape(body, ridges, SolidColor{ GatorColors::Dark });
        drawLeg(body, -30.0f, false);
        drawLeg(body, -140.0f, false);

        const SceneFrame head = headFrameOf(frame, pose);
        PixelPath neck;
        neck.moveTo(-20.0f, -18.0f);
        neck.lineTo(k_mouthBack.x, -6.0f);
        neck.lineTo(k_mouthBack.x + 4.0f, 20.0f);
        neck.lineTo(-20.0f, 30.0f);
        neck.close();
        fillShape(head, neck, SolidColor{ GatorColors::Body });
        const FloatPoint upperTip = k_mouthBack + rotated({ 116.0f, 0.0f }, -pose.gape);
        PixelPath mouth;
        mouth.moveTo(k_mouthBack.x - 4.0f, 0.0f);
        mouth.lineTo(upperTip);
        mouth.lineTo(k_jawTip, 2.0f);
        mouth.close();
        fillShape(head, mouth, SolidColor{ GatorColors::Mouth });
        drawGlow(head, k_mouthBack + FloatPoint{ 6.0f, -4.0f },
            36.0f * std::min(1.0f, pose.gape * 2.0f), GatorColors::Throat);
        fillOval(head, { 78.0f, 1.0f }, 30.0f, 3.5f, SolidColor{ GatorColors::Tongue });
        PixelPath jaw;
        jaw.moveTo(k_mouthBack.x - 6.0f, 0.0f);
        jaw.lineTo(k_jawTip, 2.0f);
        jaw.lineTo(k_jawTip + 4.0f, 7.0f);
        jaw.lineTo(k_jawTip - 2.0f, 14.0f);
        jaw.lineTo(k_mouthBack.x - 6.0f, 18.0f);
        jaw.close();
        fillShape(head, jaw, SolidColor{ GatorColors::Body });
        PixelPath underside;
        underside.moveTo(k_mouthBack.x, 12.0f);
        underside.lineTo(k_jawTip - 4.0f, 11.0f);
        underside.lineTo(k_jawTip - 2.0f, 14.0f);
        underside.lineTo(k_mouthBack.x, 18.0f);
        underside.close();
        fillShape(head, underside, SolidColor{ GatorColors::JawUnderside });
    }

    // The teeth come with the upper jaw, so a head between the jaws shows them over it.
    void drawGatorUpperJaw(const SceneFrame& frame, const GatorPose& pose)
    {
        const SceneFrame head = headFrameOf(frame, pose);
        const SolidColor tooth{ GatorColors::Tooth };
        PixelPath lowerTeeth;
        for (float x = 52.0f; x < 146.0f; x += 12.0f)
        {
            const float y = 2.0f * (x - k_mouthBack.x) / (k_jawTip - k_mouthBack.x);
            lowerTeeth.moveTo(x, y + 1.0f);
            lowerTeeth.lineTo(x + 3.5f, y - 8.0f);
            lowerTeeth.lineTo(x + 7.0f, y + 1.0f);
            lowerTeeth.close();
        }
        fillShape(head, lowerTeeth, tooth);

        const SceneFrame upper = head.within(Matrix3x2::Translation(k_mouthBack)
            * Matrix3x2::Rotation(-pose.gape)
            * Matrix3x2::Translation(-k_mouthBack.x, -k_mouthBack.y));
        PixelPath upperTeeth;
        for (float x = 48.0f; x < 146.0f; x += 12.0f)
        {
            upperTeeth.moveTo(x, -1.0f);
            upperTeeth.lineTo(x + 3.5f, 9.0f);
            upperTeeth.lineTo(x + 7.0f, -1.0f);
            upperTeeth.close();
        }
        fillShape(upper, upperTeeth, tooth);
        PixelPath snout;
        snout.moveTo(-10.0f, -4.0f);
        snout.lineTo(k_mouthBack);
        snout.lineTo(k_jawTip, 0.0f);
        snout.lineTo(k_jawTip + 5.0f, -7.0f);
        snout.lineTo(k_jawTip, -15.0f);
        snout.lineTo(70.0f, -19.0f);
        snout.lineTo(48.0f, -26.0f);
        snout.lineTo(10.0f, -30.0f);
        snout.lineTo(-14.0f, -24.0f);
        snout.close();
        fillShape(upper, snout, LinearGradient::Simple({ 0.0f, -30.0f }, { 0.0f, 0.0f },
            GatorColors::Body, GatorColors::Dark));
        fillDisc(upper, { 145.0f, -15.0f }, 5.5f, SolidColor{ GatorColors::Body });
        fillDisc(upper, { 147.0f, -17.0f }, 1.6f, SolidColor{ GatorColors::Pupil });
        PixelPath pores;
        for (float x = 64.0f; x < 140.0f; x += 16.0f)
            addEllipse(pores, { x, -12.0f }, 1.4f, 1.4f);
        fillShape(upper, pores, SolidColor{ GatorColors::Pore });

        // the eye on its bump, slanted as the mill gator's
        PixelPath bump;
        bump.moveTo(18.0f, -28.0f);
        bump.quadTo({ 32.0f, -50.0f }, { 50.0f, -26.0f });
        bump.close();
        fillShape(upper, bump, SolidColor{ GatorColors::Body });
        const FloatPoint eye = { 34.0f, -33.0f };
        fillOval(upper, eye, 7.5f, 5.0f, SolidColor{ GatorColors::Eye }, -0.2f);
        fillOval(upper, eye + FloatPoint{ pose.glance, 0.0f }, 1.6f, 4.3f,
            SolidColor{ GatorColors::Pupil });
        if (pose.lid > 0.0f)
        {
            PixelPath eyeShape;
            addEllipse(eyeShape, eye, 8.0f, 5.4f);
            upper.canvas.pushClip(eyeShape, &upper.toCanvas);
            fillBox(upper, { 24.0f, -40.0f, 44.0f, -38.5f + 10.5f * pose.lid },
                SolidColor{ GatorColors::Dark });
            upper.canvas.popClip();
        }
    }

    namespace
    {
        // The back rises in a hump from the neck to the hips and runs down the tail to its tip.
        float topAt(const float x, const float tailDrop)
        {
            if (x > k_tailRoot)
                return -36.0f - 12.0f * std::sin(std::numbers::pi_v<float>
                    * std::clamp((12.0f - x) / (12.0f - k_tailRoot), 0.0f, 1.0f));
            const float share = (k_tailRoot - x) / (k_tailRoot - k_tailTip);
            return std::lerp(-36.0f, -9.0f, share) + tailDrop * share * share;
        }

        float underAt(const float x, const float tailDrop)
        {
            if (x > k_tailRoot)
                return 0.0f;
            const float share = (k_tailRoot - x) / (k_tailRoot - k_tailTip);
            return std::lerp(0.0f, -3.0f, share) + tailDrop * share * share;
        }

        SceneFrame bodyFrameOf(const SceneFrame& frame, const GatorPose& pose)
        {
            return frame.within(Matrix3x2::Translation(pose.neck) * Matrix3x2::Scale(pose.size));
        }

        SceneFrame headFrameOf(const SceneFrame& frame, const GatorPose& pose)
        {
            return bodyFrameOf(frame, pose)
                .within(Matrix3x2::Translation(k_neck) * Matrix3x2::Rotation(-pose.headLift)
                    * Matrix3x2::Translation(-k_neck.x, -k_neck.y));
        }

        void drawLeg(const SceneFrame& body, const float x, const bool far)
        {
            const SolidColor color{ far ? GatorColors::Dark : GatorColors::Body };
            drawSegment(body, { x, -14.0f }, { x + 8.0f, -2.0f }, color, 14.0f);
            for (int toe = 0; toe != 3; ++toe)
                drawSegment(body, { x + 8.0f, -2.0f },
                    { x + 14.0f + static_cast<float>(toe) * 5.0f, 2.0f }, color, 4.0f);
        }
    }
}
