module ClaFi.PathArt.SatelliteSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace SatelliteColors
    {
        constexpr Color InShadow{ 0xFF0B1020 };
        constexpr Color Boom{ 0xFF9AA3AD };
        constexpr Color PanelLight{ 0xFF3460C4 };
        constexpr Color PanelDeep{ 0xFF16307A };
        constexpr Color PanelGrid{ 0xFF8FB0EE };
        constexpr Color PanelBackLight{ 0xFFB4BAC4 };
        constexpr Color PanelBackShade{ 0xFF7A808B };
        constexpr Color PanelRib{ 0xFF4E5560 };
        constexpr Color PanelSpar{ 0xFF5C636E };
        constexpr Color PanelFrame{ 0xFFC9CED6 };
        constexpr Color PanelGlint{ 0xFFE8F2FF };
        constexpr Color ModuleLight{ 0xFFEEF1F5 };
        constexpr Color ModuleShade{ 0xFFA8B1BD };
        constexpr Color FoilLight{ 0xFFFFE08A };
        constexpr Color FoilMiddle{ 0xFFC99232 };
        constexpr Color FoilBright{ 0xFFF2C75A };
        constexpr Color FoilDark{ 0xFF8A5D14 };
        constexpr Color CreaseLight{ 0xFFFFF3C0 };
        constexpr Color CreaseDark{ 0xFF6E4A10 };
        constexpr Color DishLight{ 0xFFFFFFFF };
        constexpr Color DishShade{ 0xFFAEB6C1 };
        constexpr Color Feed{ 0xFF7B8794 };
        constexpr Color RedLight{ 0xFFFF4D4D };
        constexpr Color RedLamp{ 0xFFFFB3B3 };
        constexpr Color GreenLight{ 0xFF4DFF88 };
        constexpr Color GreenLamp{ 0xFFC8FFD9 };
        constexpr Color Strobe{ 0xFFFFFFFF };
    }

    void drawSatellite(const SceneFrame& frame, const SatellitePose& pose)
    {
        const FloatPoint center = pose.center;
        const float size = pose.size;
        const float sunlit = pose.sunlit;
        const float orbitTime = pose.orbitTime;
        const float blinkTime = pose.blinkTime;
        // In the Earth's shadow every part goes dark toward the same deep blue.
        const auto lit = [&](Color color) {
            return colorBetween(SatelliteColors::InShadow, color, 0.35f + 0.65f * sunlit);
        };
        const float sway = -0.16f + 0.05f * std::sin(k_2Pi * orbitTime);
        const SceneFrame craft = frame.within(Matrix3x2::Translation(center)
            * Matrix3x2::Rotation(sway) * Matrix3x2::Scale(size));
        const SolidColor frameMetal{ lit(SatelliteColors::PanelFrame) };

        // The panels turn once a cycle about their booms; past edge-on they show their backs.
        const float panelTurn = std::cos(k_2Pi * orbitTime + 0.4f);
        const float panelHalfHeight = std::max(std::abs(30.0f * panelTurn), 1.2f);
        for (const float side : { -1.0f, 1.0f })
        {
            drawSegment(craft, { side * 36.0f, 0.0f }, { side * 74.0f, 0.0f },
                SolidColor{ lit(SatelliteColors::Boom) }, 4.0f, StrokeCap::Butt);
            const float left = side > 0.0f ? 74.0f : -222.0f;
            const FloatRect panel = { left, -panelHalfHeight, left + 148.0f, panelHalfHeight };
            if (panelTurn > 0.0f)
            {
                fillBox(craft, panel, LinearGradient::Simple(panel.topLeft(), panel.bottomRight(),
                    lit(SatelliteColors::PanelLight), lit(SatelliteColors::PanelDeep)));
                PixelPath grid;
                for (int i = 1; i != 6; ++i)
                {
                    const float x = left + 148.0f * static_cast<float>(i) / 6.0f;
                    grid.moveTo(x, -panelHalfHeight);
                    grid.lineTo(x, panelHalfHeight);
                }
                grid.moveTo(left, 0.0f);
                grid.lineTo(left + 148.0f, 0.0f);
                const float gridOpacity = 0.5f * (0.3f + 0.7f * sunlit);
                const SolidColor gridLine{ SatelliteColors::PanelGrid.withOpacity(gridOpacity) };
                strokeShape(craft, grid, gridLine, 0.8f, StrokeCap::Butt);
                if (sunlit > 0.05f)
                {
                    const float sweeps = blinkTime * 2.0f + (side > 0.0f ? 0.5f : 0.0f);
                    const float glintX = left + 148.0f * fractionalPart(sweeps);
                    drawGlow(craft, { glintX, 0.0f }, 22.0f,
                        SatelliteColors::PanelGlint.withOpacity(0.45f * sunlit * panelTurn), 0.6f);
                }
            }
            else
            {
                // the back: the cells' ribs and the spar along the boom
                fillBox(craft, panel, LinearGradient::Simple(panel.topLeft(), panel.bottomRight(),
                    lit(SatelliteColors::PanelBackLight), lit(SatelliteColors::PanelBackShade)));
                PixelPath ribs;
                for (int i = 1; i != 6; ++i)
                {
                    const float x = left + 148.0f * static_cast<float>(i) / 6.0f;
                    ribs.moveTo(x, -panelHalfHeight);
                    ribs.lineTo(x, panelHalfHeight);
                }
                strokeShape(craft, ribs, SolidColor{ lit(SatelliteColors::PanelRib) }, 1.6f,
                    StrokeCap::Butt);
                drawSegment(craft, { left, 0.0f }, { left + 148.0f, 0.0f },
                    SolidColor{ lit(SatelliteColors::PanelSpar) }, 3.0f, StrokeCap::Butt);
            }
            PixelPath frameLine;
            frameLine.moveTo(panel.left, panel.top);
            frameLine.lineTo(panel.right, panel.top);
            frameLine.lineTo(panel.right, panel.bottom);
            frameLine.lineTo(panel.left, panel.bottom);
            frameLine.close();
            strokeShape(craft, frameLine, frameMetal, 1.2f);
        }

        // the instrument module, its whips, the gold-foiled body and the dish under it
        PixelPath module;
        module.drawRoundedRect(FloatRect{ -22.0f, -62.0f, 22.0f, -38.0f }, 5.0f, 5.0f);
        fillShape(craft, module, LinearGradient::Simple({ -22.0f, 0.0f }, { 22.0f, 0.0f },
            lit(SatelliteColors::ModuleLight), lit(SatelliteColors::ModuleShade)));
        drawSegment(craft, { 10.0f, -62.0f }, { 22.0f, -92.0f }, frameMetal, 1.4f);
        drawSegment(craft, { -8.0f, -62.0f }, { -14.0f, -84.0f }, frameMetal, 1.4f);
        PixelPath body;
        body.drawRoundedRect(FloatRect{ -36.0f, -40.0f, 36.0f, 40.0f }, 4.0f, 4.0f);
        fillShape(craft, body, LinearGradient{
            .startPoint = { -36.0f, -40.0f },
            .endPoint = { 36.0f, 40.0f },
            .stops = {
                { .position = 0.0f, .color = lit(SatelliteColors::FoilLight) },
                { .position = 0.45f, .color = lit(SatelliteColors::FoilMiddle) },
                { .position = 0.7f, .color = lit(SatelliteColors::FoilBright) },
                { .position = 1.0f, .color = lit(SatelliteColors::FoilDark) }
            }
        });
        PixelPath lightCreases;
        PixelPath darkCreases;
        for (int i = 0; i != 7; ++i)
        {
            const float y = -34.0f + static_cast<float>(i) * 11.0f;
            PixelPath& creases = i % 2 == 1 ? lightCreases : darkCreases;
            creases.moveTo(-34.0f, y + pseudoRandom(i, 91.0f) * 6.0f);
            creases.lineTo(34.0f, y - pseudoRandom(i, 92.0f) * 6.0f);
        }
        const float creaseOpacity = 0.35f * (0.3f + 0.7f * sunlit);
        const SolidColor lightCrease{ SatelliteColors::CreaseLight.withOpacity(creaseOpacity) };
        const SolidColor darkCrease{ SatelliteColors::CreaseDark.withOpacity(creaseOpacity) };
        strokeShape(craft, lightCreases, lightCrease, 1.0f, StrokeCap::Butt);
        strokeShape(craft, darkCreases, darkCrease, 1.0f, StrokeCap::Butt);
        drawSegment(craft, { 0.0f, 40.0f }, { 0.0f, 56.0f }, frameMetal, 3.0f);
        PixelPath dish;
        dish.moveTo(-30.0f, 58.0f);
        dish.quadTo({ 0.0f, 86.0f }, { 30.0f, 58.0f });
        dish.close();
        fillShape(craft, dish, LinearGradient::Simple({ -30.0f, 58.0f }, { 30.0f, 72.0f },
            lit(SatelliteColors::DishLight), lit(SatelliteColors::DishShade)));
        const SolidColor feed{ lit(SatelliteColors::Feed) };
        drawSegment(craft, { 0.0f, 66.0f }, { 0.0f, 78.0f }, feed, 2.0f);

        // The navigation lights blink in turn, red and green on the panel tips, the strobe on top.
        const float unit = frame.unit;
        const auto lampAt = [&](FloatPoint point) {
            return center + rotated(point * size, sway);
        };
        if (fractionalPart(blinkTime * 16.0f) < 0.3f)
        {
            drawGlow(frame, lampAt({ -224.0f, 0.0f }), 12.0f * unit,
                SatelliteColors::RedLight.withOpacity(0.8f));
            fillDisc(frame, lampAt({ -224.0f, 0.0f }), 2.4f * unit,
                SolidColor{ SatelliteColors::RedLamp });
        }
        if (fractionalPart(blinkTime * 16.0f + 0.5f) < 0.3f)
        {
            drawGlow(frame, lampAt({ 224.0f, 0.0f }), 12.0f * unit,
                SatelliteColors::GreenLight.withOpacity(0.8f));
            fillDisc(frame, lampAt({ 224.0f, 0.0f }), 2.4f * unit,
                SolidColor{ SatelliteColors::GreenLamp });
        }
        if (fractionalPart(blinkTime * 20.0f) < 0.05f)
        {
            drawGlow(frame, lampAt({ 0.0f, -64.0f }), 16.0f * unit,
                SatelliteColors::Strobe.withOpacity(0.9f));
            fillDisc(frame, lampAt({ 0.0f, -64.0f }), 2.6f * unit,
                SolidColor{ SatelliteColors::Strobe });
        }
    }
}
