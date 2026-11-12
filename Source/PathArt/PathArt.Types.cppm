export module ClaFi.PathArt.Types;

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

    // The Chase's own colours in a theme, beside the sky, meadow and trees it shares. See PathArt
    export struct ChaseTheme
    {
        Color mountainTop{}; // the mountains' clear colours, which the scene hazes
        Color mountainFoot{};
        Color crownLight{}; // the far hills' trees, beside treeLeavesBase
        Color crownMiddle{};
        Color treeCrown{}; // the branch run's tree
        Color treeCrownLight{};
        Color treeCrownMiddle{};
        Color trail{};
        Color pebble{};
        Color grass{}; // the flowers' stems, the tufts and the foreground grass
        Color bladeDark{}; // the patch of grass on the trail
        Color bladeMiddle{};
        Color bladeLight{};
        Color daisyLeaf{};
        Color reedStem{};
        Color frontGrassDark{}; // every third blade of the foreground grass
        Color shadow{}; // what the figures and props cast
        WolfCoat greyWolf{}; // ours
        HareCoat brownHare{};
        WolfCoat blackWolf{}; // the other couple's
        HareCoat greyHare{};
        Color flowerHeart{}; // a daisy's, and the meadow's flowers'
        Color bough{}; // the signpost, the stumps, every trunk and the Mill's wing frames, in wood
        Color bark{};
        Color barkShade{};
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
        Color duckLight{}; // the rubber duck
        Color duckShade{};
        Color duckBeak{};
        Color duckBeakShade{};
        Color ballRed{}; // the ball
        Color ballWhite{};
        Color ballBlue{};
        Color ballYellow{};
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
        Color wing{}; // the butterfly's
        Color wingEdge{};
        Color stoneLight{}; // the boulder
        Color stoneDark{};
        Color stoneCrack{};
        Color reedHead{};
        Color fluff{};
        Color birdBody{}; // the bird by the trail
        Color birdBelly{};
        Color birdWing{};
        Color birdBeak{};
        Color dust{}; // kicked up where the wolf lands
    public:
        // How many colours it holds, in a row with nothing between them.
        static constexpr std::size_t colorsNum() { return sizeof(ChaseTheme) / sizeof(Color); }
    };

    export struct SceneTheme
    {
        std::wstring_view name;
        Color skyTop, skyBottom; // 1
        Color sunDisk, sunStroke, sunCorona1, sunCorona2, sunGlow; // 3
        Color cloud; // 8
        Color hillTop, hillBottom; // 9
        Color riverTop, riverBottom, riverWave, riverReflect, foam; // 11
        Color millBodyTop, millBodyBottom, millRoofTop, millRoofBottom; // 16,
        Color millWingFabric, millWheel; // 20
        Color treeLeavesBase, reedStem, reedHead, lilyPad, lilyFlower, shadow; // 22
        Color cowBody, cowSpots, cowSnout, cowDetail; // 28
        Color wolfBody, wolfEar, wolfEye; // 32

        Color effectColor1{ 255, 255, 255 }; // 35
        Color effectColor2{ 255, 255, 255 }; // 36
        Color petal1, petal2, petal3; // 37, a flower wears one, falling petals the first two
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
        float airHaze = 0.25f; // skyTop's share at a scene's reference distance, from 0 to 1
        // make sure to update effectsNum() when adding new effects
    public:
        Color* asArray() { return &skyTop; }
        const Color* asArray() const { return &skyTop; }
        const void* colorsEnd() const { return &hasRain; }
        static constexpr std::size_t colorsNum() { return 39 + ChaseTheme::colorsNum(); }
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
