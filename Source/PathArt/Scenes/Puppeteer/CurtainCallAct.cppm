export module ClaFi.PathArt.CurtainCallAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BouquetSprite;
import ClaFi.PathArt.GatorSprite;
import ClaFi.PathArt.MarionetteSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.RabbitSprite;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.TopHatSprite;
import ClaFi.PathArt.WolfSprite;

namespace ClaFi::PathArt
{
    // The whole cast takes its bows as bouquets fly onto the stage. See PathArt
    export class CurtainCallAct : public PuppeteerAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_android.pelvis.x; }
        [[nodiscard]] bool usesMarionette() const override { return true; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        bool m_holdingHat{ false };
        TopHatPose m_hat{};
        RabbitPose m_rabbit{};
        bool m_catchThrown{ false };
        BouquetPose m_caughtBouquet{};
        GatorPose m_gator{};
        MarionetteJoints m_marionette{};
        WolfPose m_wolf{};
        float m_actTime{ 0.0f };
        float m_middleX{ 0.0f };
        float m_floorY{ 0.0f };
        float m_bottomY{ 0.0f }; // the frame's bottom edge, below which the audience throw from
        float m_spread{ 1.0f }; // how far apart the cast stand, 1 in a frame 900 units wide
    };
}
