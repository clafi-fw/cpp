export module ClaFi.PathArt.ChaseScene;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    // A wolf chases a hare across a scrolling landscape, and keeps falling over. See PathArt
    export class ChaseScene : public Scene
    {
    public:
        [[nodiscard]] std::wstring_view name() const override { return L"Chase"; }
        void draw(Graphics::Canvas&, const FloatRect& bounds, float cycleTime) override;
    private:
        // How the wolf stands, runs or lies at one moment.
        struct WolfPose
        {
            float turn;
            float stride; // where the gallop is, in radians; the legs and the tail follow it
            float sprawl; // 0 on its feet, 1 sprawled on its belly
            float onBack; // 0 on its feet, 1 on its back with its legs in the air
            float dazed;
            float headTurn;
            float tongueTurn;
            float glance;
        };
        // How the hare runs or sits at one moment.
        struct HarePose
        {
            float facing; // 1 facing right, -1 facing left; between them it turns
            float air; // 0 on the ground, 1 at the top of a hop
            float stretch;
            float bodyTurn;
            float sitting;
            float earTurn;
            float earWiggle;
            float footLift;
        };
        using TileDrawer = std::function<void(float tileLeft, float tileWidth)>;
    private:
        // The camera's travel by `episodeTime` of an episode, as a share of a full-speed episode's.
        [[nodiscard]] static float cameraShare(float episodeTime, int episode);
        [[nodiscard]] static float travelAt(float cycleTime);
        // A period that the whole cycle's travel divides, near the one wanted. See PathArt
        [[nodiscard]] static float periodNear(float parallax, float wanted);
        static void forEachTile(const SceneFrame&, float parallax, float period, float travel,
            const TileDrawer&);
        static void drawBackdrop(const SceneFrame&, float travel, float groundY);
        static void drawGround(const SceneFrame&, float travel, float groundY);
        static void drawObstacle(const SceneFrame&, int episode, float x, float groundY,
            float footY);
        static void drawWolf(const SceneFrame&, FloatPoint center, float size, const WolfPose&,
            float cycleTime);
        static void drawHare(const SceneFrame&, FloatPoint foot, float size, const HarePose&);
        static void drawFrontGrass(const SceneFrame&, float travel);
    };
}
