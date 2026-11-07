module ClaFi.PathArt.GhostSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace GhostColors
    {
        constexpr Color GhostTop{ 0xF0FFFFFF };
        constexpr Color GhostBottom{ 0x80CFE0FF };
        constexpr Color GhostHalo{ 0xD9BFE3FF };
        constexpr Color Blush{ 0x80FFB3C7 };
        constexpr Color Face{ 0xFF1A1F33 };
        constexpr Color White{ 0xFFFFFFFF };
    }

    // The sheet leans into its path and its hem trails, so the ghost reads as moving through air.
    void drawGhost(const SceneFrame& frame, const GhostPose& pose)
    {
        const float cycleTime = pose.cycleTime;
        const float lean = 0.16f * pose.heading;
        const float trail = -6.0f * pose.heading;
        const SceneFrame ghost = frame.within(Matrix3x2::Translation(pose.center)
            * Matrix3x2::Rotation(lean) * Matrix3x2::Scale(pose.size));
        const LinearGradient sheet = LinearGradient::Simple({ 0.0f, -60.0f }, { 0.0f, 72.0f },
            GhostColors::GhostTop, GhostColors::GhostBottom);
        const PathDrawLayer halo = PathDrawLayer::OuterGlow(GhostColors::GhostHalo, 22.0f);
        const PathDrawLayer cloth{ .geometry{}, .brush = sheet };

        for (const float side : { -1.0f, 1.0f })
        {
            const float wave = std::sin(k_2Pi * 12.0f * cycleTime + (side > 0.0f ? 1.4f : 0.0f));
            const Matrix3x2 shoulder = Matrix3x2::Translation(side * 36.0f, 4.0f)
                * Matrix3x2::Rotation(side * (0.45f + 0.35f * wave));
            PixelPath arm;
            arm.moveTo(0.0f, -7.0f);
            arm.quadTo({ side * 26.0f, -11.0f }, { side * 25.0f, 0.0f });
            arm.quadTo({ side * 23.0f, 9.0f }, { 0.0f, 8.0f });
            arm.close();
            const Matrix3x2 toCanvas = ghost.toCanvas * shoulder;
            ghost.canvas.drawPath(arm, { halo, cloth }, &toCanvas);
        }

        PixelPath body;
        body.moveTo(-40.0f, 0.0f);
        body.cubicTo({ -40.0f, -72.0f }, { 40.0f, -72.0f }, { 40.0f, 0.0f });
        body.quadTo({ 42.0f, 28.0f }, { 46.0f + trail, 50.0f });
        constexpr std::array<float, 5> k_hemPoints{ 46.0f, 23.0f, 0.0f, -23.0f, -46.0f };
        for (std::size_t i = 0; i != 4; ++i)
        {
            const float from = k_hemPoints[i] + trail;
            const float to = k_hemPoints[i + 1] + trail;
            const float ripple = std::sin(k_2Pi * 24.0f * cycleTime - static_cast<float>(i) * 1.3f);
            const float depth = 15.0f + 7.0f * ripple;
            body.quadTo({ (from + to) * 0.5f, 50.0f + depth * 2.0f }, { to, 50.0f });
        }
        body.quadTo({ -42.0f, 28.0f }, { -40.0f, 0.0f });
        body.close();
        ghost.canvas.drawPath(body, { halo, cloth }, &ghost.toCanvas);

        const float look = 5.0f * pose.heading;
        const float blink = fractionalPart(cycleTime * 6.0f + 0.3f) > 0.97f ? 0.12f : 1.0f;
        const SolidColor blush{ GhostColors::Blush };
        fillOval(ghost, { -24.0f + look, -8.0f }, 6.0f, 3.5f, blush);
        fillOval(ghost, { 24.0f + look, -8.0f }, 6.0f, 3.5f, blush);
        fillOval(ghost, { -13.0f + look, -24.0f }, 6.0f, 9.0f * blink,
            SolidColor{ GhostColors::Face });
        fillOval(ghost, { 13.0f + look, -24.0f }, 6.0f, 9.0f * blink,
            SolidColor{ GhostColors::Face });
        if (blink > 0.5f)
        {
            const SolidColor glint{ GhostColors::White };
            fillDisc(ghost, { -15.0f + look, -28.0f }, 2.0f, glint);
            fillDisc(ghost, { 11.0f + look, -28.0f }, 2.0f, glint);
        }
        // The mouth opens and closes eight times a cycle.
        const float open = 0.5f + 0.5f * std::sin(k_2Pi * 8.0f * cycleTime);
        fillOval(ghost, { look * 0.6f, -2.0f }, 5.0f + 2.0f * open, 4.0f + 7.0f * open,
            SolidColor{ GhostColors::Face });
    }
}
