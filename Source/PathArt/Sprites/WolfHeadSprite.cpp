module ClaFi.PathArt.WolfHeadSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawWolfHead(const SceneFrame& frame, const WolfHeadPose& pose,
        const WolfHeadColors& colors)
    {
        if (pose.opacity <= 0.0f)
            return;

        const SceneFrame head = frame.within(Matrix3x2::Translation(pose.at)
            * Matrix3x2::Scale(pose.size));
        PixelPath shape;
        shape.moveTo(-31.0f, 3.0f);
        shape.quadTo({ -24.0f, -3.0f }, { -12.0f, -6.0f });
        shape.quadTo({ -2.0f, -14.0f }, { 10.0f, -13.0f });
        shape.quadTo({ 20.0f, -10.0f }, { 22.0f, 0.0f });
        shape.lineTo(22.0f, 16.0f);
        shape.quadTo({ 8.0f, 14.0f }, { -4.0f, 11.0f });
        shape.quadTo({ -18.0f, 10.0f }, { -28.0f, 8.0f });
        shape.quadTo({ -33.0f, 6.0f }, { -31.0f, 3.0f });
        shape.close();
        fillShape(head, shape, SolidColor{ colors.head.fullyOpaque().faded(pose.opacity) });

        const SolidColor ears{ colors.ears.faded(pose.opacity) };
        shape.clear();
        shape.moveTo(6.0f, -12.0f);
        shape.lineTo(14.0f, -27.0f);
        shape.lineTo(18.0f, -8.0f);
        shape.close();
        fillShape(head, shape, ears);
        const SceneFrame frontEar = head.within(Matrix3x2::Translation(2.0f, -11.0f)
            * Matrix3x2::Rotation(pose.frontEarTurn));
        shape.clear();
        shape.moveTo(-6.0f, 1.0f);
        shape.lineTo(-2.0f, -17.0f);
        shape.lineTo(6.0f, -1.0f);
        shape.close();
        fillShape(frontEar, shape, ears);
        fillDisc(head, { -30.0f, 3.5f }, 2.6f, ears);

        if (!pose.eyesOpen)
            return;
        constexpr std::array<float, 2> k_eyeXs{ -11.0f, 1.0f };
        constexpr float k_eyeY{ -5.0f };
        const SolidColor eyes{ colors.eyes.faded(pose.opacity) };
        for (const float x : k_eyeXs)
        {
            drawGlow(head, { x, k_eyeY }, 9.0f, colors.eyes.faded(0.55f * pose.opacity));
            shape.clear();
            shape.moveTo(x + 5.0f, k_eyeY);
            shape.lineTo(x - 2.0f, k_eyeY + 2.5f);
            shape.lineTo(x - 1.5f, k_eyeY - 1.0f);
            shape.close();
            fillShape(head, shape, eyes);
        }
    }
}
