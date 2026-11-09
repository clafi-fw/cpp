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
        [[nodiscard]] std::chrono::seconds cycleDuration() const override { return k_cycle; }
        void draw(Graphics::Canvas&, const FloatRect& bounds, SceneTime) override;
    private:
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
        static void drawFrontGrass(const SceneFrame&, float travel);
    private:
        // Three 20-second runs: two over obstacles, then the one into the branch.
        static constexpr std::chrono::seconds k_cycle{ 60 };
    };
}
