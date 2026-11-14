export module ClaFi.PathArt.OneManBandAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    // The android strums a guitar and works a drum set's pedal with its foot. See PathArt
    export class OneManBandAct : public BotTalentAct
    {
    public:
        void pose(const ActStage&, float actTime, SceneTime time) override;
        [[nodiscard]] float spotlightX() const override { return m_spotlightX; }
        void draw(const SceneFrame&) const override;
    private:
        AndroidPose m_android{};
        float m_neckAngle{ 0.0f };
        float m_ringing{ 0.0f }; // how far the strings still shake from the last stroke, 0 to 1
        float m_pedalPress{ 0.0f }; // 0 the footboard up to 1 pressed, the beater on the head
        float m_kicked{ 0.0f }; // 1 the moment the beater strikes the bass drum, falling to 0
        float m_crashAge{ -1.0f }; // seconds since the cymbal was struck, below 0 before
        FloatPoint m_stickTail{}; // the drumstick's tail end, in units of the upper body
        float m_stickAngle{ 0.0f }; // from its tail to its tip, in the upper body's frame
        bool m_stickInFront{ false }; // out in front of the android, or else behind its head
        FloatPoint m_kitFloor{}; // where the bass drum's batter head meets the floor, in the frame
        float m_spotlightX{ 0.0f };
        float m_actTime{ 0.0f };
    };
}
