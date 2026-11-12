export module ClaFi.PathArt.PuppeteerAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // How long an act runs before the curtain closes on it, in seconds.
    export constexpr float k_actSeconds{ 10.0f };

    // The stage an act plays on, as PuppeteerScene lays it out in the frame. See PathArt
    export struct ActStage
    {
        SceneFrame frame;
        float centerX;
        float floorY; // the line the feet stand on
        float closeness; // 1, or less in a narrow frame, which draws figures closer together
        float drapeWidth; // the side drapes' reach in from each edge
    };

    // One of the Puppeteer's acts: posed at a time into it, then drawn. See PathArt
    export class PuppeteerAct
    {
    public:
        virtual ~PuppeteerAct() = default;
    public:
        // Poses the act `actTime` seconds into it, the scene at `time`.
        virtual void pose(const ActStage&, float actTime, SceneTime time) = 0;
        // Where the spotlight falls across the frame, as the act was last posed.
        [[nodiscard]] virtual float spotlightX() const = 0;
        // Whether the marionette plays in the act, rather than hanging on its stand.
        [[nodiscard]] virtual bool usesMarionette() const { return false; }
        // Seconds since something landed hard enough to jolt the stage, below 0 if nothing has.
        [[nodiscard]] virtual float joltAge() const { return -1.0f; }
        // Draws the act as it was last posed.
        virtual void draw(const SceneFrame&) const = 0;
    };

    // The colours the magic in the acts sparkles in.
    export namespace MagicColors
    {
        constexpr Color Gold{ 0xFFFFD34D };
        constexpr Color Cyan{ 0xFF7AF4FF };
        constexpr Color White{ 0xFFFFF6DC };
    }

    // Where the hands go in the poses several acts strike, in units about the pelvis.
    export namespace ActHands
    {
        constexpr FloatPoint LeftOnHip{ -44.0f, -19.0f };
        constexpr FloatPoint RightOnHip{ 44.0f, -19.0f };
        constexpr FloatPoint LeftRaised{ -101.0f, -195.0f }; // the arms in a V for the ta-da
        constexpr FloatPoint RightRaised{ 101.0f, -195.0f };
    }

    // The marionette's dance, in radians of its sway, at a time of the scene. See PathArt
    export [[nodiscard]] float danceSway(SceneTime);
    // Where the dancing marionette's feet are across the frame, `sway` radians into its dance.
    export [[nodiscard]] float marionetteX(const ActStage&, float sway);
    // Where the android stands while it works the marionette.
    export [[nodiscard]] float puppeteerX(const ActStage&);
    // The chest lights flashing all together, three times a second.
    export [[nodiscard]] ChestLights flashingLights(float actTime);
    // A magician's wand from the hand that holds it to its white tip.
    export void drawWand(const SceneFrame&, FloatPoint hand, FloatPoint tip);
}
