module ClaFi.PathArt.TutuSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace TutuColors
    {
        constexpr Color Shade{ 0xFFE48AAC };
        constexpr Color Frill{ 0xFFF6ABC5 };
        constexpr Color FrillShade{ 0xFFEF9BBA };
        constexpr Color Top{ 0xFFFFE3EC };
        constexpr Color TopShade{ 0xFFF7B5CC };
        constexpr Color Pleat{ 0x73FFFFFF };
        constexpr Color Rim{ 0x99FFFFFF };
        constexpr Color Band{ 0xFFE9799F };
        constexpr Color Bow{ 0xFFD9648C };
        constexpr Color BowKnot{ 0xFFF6A2BF };
    }

    namespace
    {
        // The middle of the top layer of tulle, below the middle of the hips.
        constexpr float k_topY{ 3.0f };
        constexpr float k_topRadius{ 71.0f };
        constexpr float k_topDepth{ 13.0f };
        constexpr int k_frillCount{ 36 };
        constexpr int k_pleatCount{ 20 };
    }

    // A disc of stiff tulle seen a little from above: a shaded lower layer, a frilled rim, and
    // a top layer lit from above with pleats running out to its edge; over it all, the waistband
    // with a bow at the front.
    void drawTutu(const SceneFrame& hips, const float turn)
    {
        fillOval(hips, { 0.0f, k_topY + 7.0f }, 76.0f, 16.0f, SolidColor{ TutuColors::Shade });
        // Neighbouring frills overlap, so each is filled on its own.
        for (int i = 0; i != k_frillCount; ++i)
        {
            const float angle = k_2Pi * static_cast<float>(i) / k_frillCount + turn;
            const FloatPoint at = {
                75.0f * std::cos(angle),
                k_topY + 4.0f + 15.0f * std::sin(angle),
            };
            fillDisc(hips, at, 4.5f,
                SolidColor{ i % 2 == 1 ? TutuColors::Frill : TutuColors::FrillShade });
        }
        PixelPath top;
        addEllipse(top, { 0.0f, k_topY }, k_topRadius, k_topDepth);
        fillShape(hips, top, LinearGradient::Simple({ 0.0f, k_topY - k_topDepth },
            { 0.0f, k_topY + k_topDepth }, TutuColors::Top, TutuColors::TopShade));
        PixelPath pleats;
        for (int i = 0; i != k_pleatCount; ++i)
        {
            const float angle = k_2Pi * static_cast<float>(i) / k_pleatCount + turn;
            const FloatPoint toward = { std::cos(angle), std::sin(angle) };
            pleats.moveTo(30.0f * toward.x, k_topY + 5.5f * toward.y);
            pleats.lineTo(69.0f * toward.x, k_topY + 12.6f * toward.y);
        }
        strokeShape(hips, pleats, SolidColor{ TutuColors::Pleat }, 1.0f);
        strokeShape(hips, top, SolidColor{ TutuColors::Rim }, 1.2f);

        fillRoundedBox(hips, { -32.0f, -9.0f, 32.0f, 2.0f }, 5.0f, SolidColor{ TutuColors::Band });
        PixelPath bow;
        for (const float side : { -1.0f, 1.0f })
        {
            bow.moveTo(0.0f, -3.5f);
            bow.lineTo(side * 10.0f, -9.0f);
            bow.lineTo(side * 10.0f, 2.0f);
            bow.close();
        }
        fillShape(hips, bow, SolidColor{ TutuColors::Bow });
        fillDisc(hips, { 0.0f, -3.5f }, 2.8f, SolidColor{ TutuColors::BowKnot });
    }
}
