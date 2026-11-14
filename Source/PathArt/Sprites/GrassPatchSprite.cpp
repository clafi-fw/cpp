module ClaFi.PathArt.GrassPatchSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    // Each blade bends from its root, its tip leaning furthest.
    void drawGrassPatch(const SceneFrame& frame, const GrassPatchColors& colors,
        const float bladeLean)
    {
        struct Blade
        {
            float root;
            float height;
            float tilt;
            Color color;
        };
        const std::array<Blade, 11> blades{
            Blade{ -12.0f, 22.0f, -0.35f, colors.dark },
            Blade{ -4.0f, 30.0f, -0.1f, colors.dark },
            Blade{ 5.0f, 27.0f, 0.15f, colors.dark },
            Blade{ 13.0f, 21.0f, 0.4f, colors.dark },
            Blade{ -15.0f, 17.0f, -0.55f, colors.middle },
            Blade{ -8.0f, 25.0f, -0.25f, colors.middle },
            Blade{ 1.0f, 33.0f, 0.0f, colors.middle },
            Blade{ 9.0f, 26.0f, 0.25f, colors.middle },
            Blade{ -10.0f, 18.0f, -0.4f, colors.light },
            Blade{ -1.0f, 23.0f, -0.05f, colors.light },
            Blade{ 7.0f, 19.0f, 0.3f, colors.light }
        };
        for (const Blade& blade : blades)
        {
            // the taller a blade, the further the lean bends it
            const float lean = blade.tilt + bladeLean * (0.7f + blade.height / 100.0f);
            const FloatPoint up = { std::sin(lean), -std::cos(lean) };
            const FloatPoint halfway = { std::sin(0.5f * lean), -std::cos(0.5f * lean) };
            const FloatPoint root = { blade.root, 1.0f };
            const FloatPoint tip = root + up * blade.height;
            PixelPath shape;
            shape.moveTo(root - FloatPoint{ 2.2f, 0.0f });
            shape.quadTo(root + halfway * (0.6f * blade.height) - FloatPoint{ 0.6f, 0.0f }, tip);
            shape.quadTo(root + halfway * (0.55f * blade.height) + FloatPoint{ 1.2f, 0.0f },
                root + FloatPoint{ 2.2f, 0.0f });
            shape.close();
            fillShape(frame, shape, SolidColor{ blade.color });
        }
    }
}
