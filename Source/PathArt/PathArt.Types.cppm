export module ClaFi.PathArt.Types;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.Graphics.Canvas;

namespace ClaFi::PathArt
{
    using namespace Graphics;

    export struct SceneTheme
    {
        std::wstring_view name;
        Color skyTop, skyBottom; // 1
        Color sunDisk, sunStroke, sunCorona1, sunCorona2, sunGlow; // 3
        Color cloud; // 8
        Color hillTop, hillBottom; // 9
        Color riverTop, riverBottom, riverWave, riverReflect, foam; // 11
        Color millBodyTop, millBodyBottom, millRoofTop, millRoofBottom; // 16,
        Color millWingWood, millWingWoodStroke, millWingFabric, millWingFabricStroke, millWheel; // 20
        Color treeTrunk, treeLeavesBase, reedStem, reedHead, lilyPad, lilyFlower, shadow; // 25
        Color cowBody, cowSpots, cowSnout, cowDetail; // 32
        Color wolfBody, wolfEar, wolfEye; // 36

        Color effectColor1{ 255, 255, 255 }; // 39
        Color effectColor2{ 255, 255, 255 }; // 40
        // make sure to update colorsNum() when adding new colors

        // Effect toggles
        float hasRain = false;
        float hasFireflies = false;
        float hasLeaves = false;
        float hasSnow = false;
        float hasPetals = false;
        float hasDust = false;
        // Predator toggle
        float isAfrica = false;
        float hasStars = false;
        // make sure to update effectsNum() when adding new effects
    public:
        Color* asArray() { return &skyTop; }
        const Color* asArray() const { return &skyTop; }
        const void* colorsEnd() const { return &hasRain; }
        static constexpr std::size_t colorsNum() { return 40; }
        float* effectsArray() { return &hasRain; }
        const float* effectsArray() const { return &hasRain; }
        static constexpr std::size_t effectsNum() { return 8; }
        void paintIcon(Canvas& canvas, const FloatRect& bounds, float cornersRadius, Opacity opacity) const;
    };

    export void createSnowFlakePath(PixelPath& p);


    //-------------------------------------------------------------------------


    void SceneTheme::paintIcon(Canvas& canvas, const FloatRect& bounds, float cornersRadius, Opacity opacity) const
    {
        float w = bounds.width();
        float h = bounds.height();
        float objectScale = std::min(w, h) / 96.0f;

        // 1. Define the "Global" Clip (Rounded Rectangle centered at bounds)
        PixelPath path{};
        path.drawRoundedRect(w, h, cornersRadius);
        Matrix3x2 transform = Matrix3x2::translation(bounds.center());

        // Push the geometric clip mask onto the canvas [CP]
        canvas.pushClip(path, &transform);

        // 2. Sky Gradient (Optimized to draw a simple rectangle directly clipped by the Canvas)
        canvas.fillRectangle(bounds, LinearGradient::simple(
            bounds.topLeft(),
            bounds.bottomLeft(),
            skyTop.withOpacity(opacity),
            skyBottom.withOpacity(opacity)
        ));

        // 3. Sun/Moon
        path.clear();
        float sunR = w * 0.15f;
        const float ctrl = sunR * 0.4142f;
        path.moveTo(0, -sunR);
        path.quadTo({ ctrl, -sunR }, { sunR * 0.707f, -sunR * 0.707f });
        path.quadTo({ sunR, -ctrl }, { sunR, 0 });
        path.quadTo({ sunR, ctrl }, { sunR * 0.707f, sunR * 0.707f });
        path.quadTo({ ctrl, sunR }, { 0, sunR });
        path.quadTo({ -ctrl, sunR }, { -sunR * 0.707f, sunR * 0.707f });
        path.quadTo({ -sunR, ctrl }, { -sunR, 0 });
        path.quadTo({ -sunR, -ctrl }, { -sunR * 0.707f, -sunR * 0.707f });
        path.quadTo({ -ctrl, -sunR }, { 0, -sunR });
        path.close();

        transform = Matrix3x2::translation({ bounds.left + w * 0.75f, bounds.top + h * 0.25f });
        canvas.fillPath(path, SolidColor{ sunDisk.withOpacity(opacity) }, &transform);

        // 4. Hill
        transform = Matrix3x2::translation(bounds.topLeft());
        path.clear();
        path.moveTo(0.0f, h * 0.9f);
        path.quadTo({ w * 0.5f, h * 0.5f }, { w, h * 0.8f });
        path.lineTo(w, h);
        path.lineTo(0.0f, h);
        path.close();
        canvas.fillPath(path, SolidColor{ hillTop.withOpacity(opacity) }, &transform);

        // 5. Subtle effect indicators
        if (hasRain) {
            path.clear();
            path.moveTo(w * 0.2f, h * 0.2f);
            path.lineTo(w * 0.15f, h * 0.4f);
            path.moveTo(w * 0.5f, h * 0.1f);
            path.lineTo(w * 0.45f, h * 0.3f);
            canvas.drawPath(path, SolidColor{ effectColor1.withOpacity(opacity) }, 1.0f, &transform);
        }
        if (hasSnow) {
            createSnowFlakePath(path);
            transform = Matrix3x2::translation(
                { bounds.left + w * 0.3f, bounds.top + h * 0.4f }
            ) * Matrix3x2::scale(objectScale);
            canvas.drawPath(path, SolidColor{ effectColor1.withOpacity(opacity) }, 1.0f, &transform);
        }

        // 6. Pop the clip geometry to clean up the canvas stack [CP]
        canvas.popClip();
    }

    // createSnowFlakePath

    export void createSnowFlakePath(PixelPath& p)
    {
        p.clear();
        for (int i = 0; i < 6; ++i) {
            float angle = i * k_2Pi / 6.0f;
            float sa = std::sin(angle), ca = std::cos(angle);
            p.moveTo(0.0f, 0.0f); p.lineTo(12.0f * sa, 12.0f * ca);
            float sideAngle = angle + 0.8f;
            p.moveTo(6.0f * sa, 6.0f * ca);
            p.lineTo(
                6.0f * sa + 5.0f * std::sin(sideAngle),
                6.0f * ca + 5.0f * std::cos(sideAngle)
            );
            sideAngle = angle - 0.8f;
            p.moveTo(6.0f * sa, 6.0f * ca);
            p.lineTo(
                6.0f * sa + 5.0f * std::sin(sideAngle),
                6.0f * ca + 5.0f * std::cos(sideAngle)
            );
        }
    }


}
