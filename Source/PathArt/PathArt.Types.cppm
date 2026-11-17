export module ClaFi.PathArt.Types;

import ClaFi.PathArt.GrassPatchSprite;
import ClaFi.PathArt.HareSprite;
import ClaFi.PathArt.SnowEffect;
import ClaFi.PathArt.WolfSprite;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.Graphics.Canvas;

namespace ClaFi::PathArt
{
    using namespace Graphics;

    // The two shades grass blades and tufts are drawn in.
    export struct GrassBladeColors
    {
        Color light{};
        Color dark{};
    };

    // The Chase's own colours in a theme, beside the scenery it shares with the Mill. See PathArt
    export struct ChaseTheme
    {
        Color treeCrown{}; // the branch run's tree
        Color treeCrownLight{};
        Color treeCrownMiddle{};
        Color pebble{};
        WolfCoat ourWolf{}; // the couple the scene follows
        HareCoat ourHare{};
        WolfCoat theirWolf{}; // the couple running the other way
        HareCoat theirHare{};
        Color appleLight{}; // the apple
        Color appleDark{};
        Color leaf{}; // the apple's, and the stump's sprout
        Color appleFlesh{};
        Color appleSeed{};
        Color crowBody{}; // the crow
        Color crowWing{};
        Color crowWingFar{};
        Color crowBeak{};
        Color bootLight{}; // the boot
        Color bootDark{};
        Color bootSole{};
        Color bootPatch{};
        Color lace{};
        Color spinesDark{}; // the hedgehog
        Color spinesLight{};
        Color hedgehogFace{};
        Color hedgehogEar{};
        Color ballPanel1{}; // the ball's three coloured panels, each followed by one in ballBase
        Color ballPanel2{};
        Color ballPanel3{};
        Color ballBase{}; // the ball between its coloured panels and in its middle
        Color peel{}; // the banana peel
        Color peelShade{};
        Color peelInside{};
        Color peelTip{};
        Color rakeHandle{}; // the rake
        Color rakeHandleShade{};
        Color rakeMetal{};
        Color capLight{}; // the toadstool
        Color capDark{};
        Color capSpot{};
        Color stalk{}; // the toadstool's
        Color stalkShade{};
        Color board{};
        Color boardShade{};
        Color stumpTop{}; // the tree stump
        Color stumpRing{};
        Color stumpSprout{};
        Color frogLight{}; // the frog
        Color frogDark{};
        Color frogBelly{};
        Color shellLight{}; // the snail's
        Color shellDark{};
        Color shellLine{};
        Color snailBody{};
        Color fluff{}; // what the wolf's wake tears from the reeds' heads
        Color birdBody{}; // the bird by the trail
        Color birdBelly{};
        Color birdWing{};
        Color birdBeak{};
        Color dust{}; // kicked up where the wolf lands
        Color dazeStar{}; // the stars circling a dazed wolf's head
    public:
        // How many colours it holds, in a row with nothing between them.
        static constexpr std::size_t colorsNum() { return sizeof(ChaseTheme) / sizeof(Color); }
    };

    // The colours of the mill and the parts fixed to it, which nothing else in a theme wears.
    export struct MillTheme
    {
        Color bodyTop{};
        Color bodyBottom{};
        Color roofTop{};
        Color roofBottom{};
        Color timber{}; // the frames, the wings' stocks and hub, the trough and the wheel's axle
        Color windowLight{}; // the lamp behind the windows, as strong as its opacity
        Color wingFabric{};
        Color wheel{};
    public:
        // How many colours it holds, in a row with nothing between them.
        static constexpr std::size_t colorsNum() { return sizeof(MillTheme) / sizeof(Color); }
    };

    export struct SceneTheme
    {
        std::wstring_view name;
        Color skyTop, skyBottom; // 1
        Color sunDisk, sunGlow; // 3
        Color cloud; // 5
        Color mountainTop, mountainFoot; // 6, the mountains' clear colours, which the scenes haze
        Color hillTop, hillBottom; // 8
        Color riverTop, riverBottom, riverWave, foam; // 10
        Color treeLeavesBase, crownLight, crownMiddle; // 14, the crowns are the far hills' trees
        Color bark, barkShade; // 17, every trunk, and the mill's door
        Color bough; // 19, the signpost and the Mill's footbridge, in wood
        Color trail; // 20
        GrassBladeColors grassBlade; // 21, blades and tufts, and the flowers' stems and leaves
        GrassPatchColors grassPatch; // 23, the lit patch on the trail, and the Mill cow's tuft
        Color reedStem, reedHead; // 26
        Color stoneLight, stoneDark, stoneCrack; // 28, the boulder, the mill's plinth and chimney
        Color shadow; // 31, what the figures and props cast
        Color cowBody, cowSpots, cowSnout, cowDetail; // 32
        Color wolfBody, wolfEye; // 36

        Color effectColor1{ 255, 255, 255 }; // 38
        Color effectColor2{ 255, 255, 255 }; // 39
        Color petal1, petal2, petal3; // 40, a flower wears one, falling petals the first two
        Color flowerHeart; // 43, a daisy's, and the meadow's flowers'
        Color wing, wingEdge; // 44, the butterfly's
        Color flock; // 46, the birds flying over the mill
        Color motherDuckBody, motherDuckWing, motherDuckEye; // 47, the duck leading the ducklings
        Color duckLight, duckShade, duckBeak, duckBeakShade; // 50, ducklings and the rubber duck
        Color fish; // 54, the fish leaping from the pond
        Color hedge; // 55, the hedge rows, hazed by each layer's distance
        MillTheme mill{};
        ChaseTheme chase{};
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
        float airHaze = 0.10f; // the air's share at a scene's reference distance, from 0 to 1
        // make sure to update effectsNum() when adding new effects
    public:
        Color* asArray() { return &skyTop; }
        const Color* asArray() const { return &skyTop; }
        const void* colorsEnd() const { return &hasRain; }
        static constexpr std::size_t colorsNum()
        {
            return 55 + MillTheme::colorsNum() + ChaseTheme::colorsNum();
        }
        float* effectsArray() { return &hasRain; }
        const float* effectsArray() const { return &hasRain; }
        static constexpr std::size_t effectsNum() { return 9; }
        void drawIcon(Canvas& canvas, const FloatRect& bounds, float cornersRadius, Opacity opacity) const;
    };


    //-------------------------------------------------------------------------


    void SceneTheme::drawIcon(Canvas& canvas, const FloatRect& bounds, float cornersRadius, Opacity opacity) const
    {
        float w = bounds.width();
        float h = bounds.height();
        float objectScale = std::min(w, h) / 96.0f;

        // 1. Define the "Global" Clip (Rounded Rectangle centered at bounds)
        PixelPath path{};
        path.drawRoundedRect(w, h, cornersRadius);
        Matrix3x2 transform = Matrix3x2::Translation(bounds.center());

        // Push the geometric clip mask onto the canvas [CP]
        canvas.pushClip(path, &transform);

        // 2. Sky Gradient (Optimized to draw a simple rectangle directly clipped by the Canvas)
        canvas.fillRectangle(bounds, LinearGradient::Simple(
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

        transform = Matrix3x2::Translation({ bounds.left + w * 0.75f, bounds.top + h * 0.25f });
        canvas.fillPath(path, SolidColor{ sunDisk.withOpacity(opacity) }, &transform);

        // 4. Hill
        transform = Matrix3x2::Translation(bounds.topLeft());
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
            transform = Matrix3x2::Translation(
                { bounds.left + w * 0.3f, bounds.top + h * 0.4f }
            ) * Matrix3x2::Scale(objectScale);
            canvas.drawPath(path, SolidColor{ effectColor1.withOpacity(opacity) }, 1.0f, &transform);
        }

        // 6. Pop the clip geometry to clean up the canvas stack [CP]
        canvas.popClip();
    }
}
