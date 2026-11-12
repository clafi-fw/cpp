module ClaFi.PathArt.BalletAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    namespace
    {
        using HandOrFootPair = std::array<FloatPoint, 2>;

        // A foot's turn to stand on the tip of its shoe, in radians.
        constexpr float k_pointe{ 1.45f };
        // On pointe, the ankle stands this much higher than on a flat foot, and the pelvis with it.
        constexpr float k_pointeRise{ 6.0f };
        constexpr float k_legLength{ 94.0f };
        constexpr HandOrFootPair k_fifthFeet{ FloatPoint{ -7.0f, 103.0f },
            FloatPoint{ 7.0f, 103.0f } };

        // Where the hands go in each position of the arms, in units about the pelvis.
        namespace BalletArms
        {
            constexpr HandOrFootPair Low{ FloatPoint{ -24.0f, -2.0f }, FloatPoint{ 24.0f, -2.0f } };
            constexpr HandOrFootPair First{ FloatPoint{ -22.0f, -58.0f },
                FloatPoint{ 22.0f, -58.0f } };
            constexpr HandOrFootPair Second{ FloatPoint{ -128.0f, -82.0f },
                FloatPoint{ 128.0f, -82.0f } };
            constexpr HandOrFootPair Fifth{ FloatPoint{ -20.0f, -196.0f },
                FloatPoint{ 20.0f, -196.0f } };
            constexpr HandOrFootPair Preparation{ FloatPoint{ -128.0f, -82.0f },
                FloatPoint{ 22.0f, -58.0f } };
            constexpr HandOrFootPair Penche{ FloatPoint{ -132.0f, -46.0f },
                FloatPoint{ 92.0f, -194.0f } };
            constexpr HandOrFootPair Leap{ FloatPoint{ -140.0f, -116.0f },
                FloatPoint{ 100.0f, -188.0f } };
            constexpr HandOrFootPair Bow{ FloatPoint{ -116.0f, -22.0f },
                FloatPoint{ 116.0f, -22.0f } };
        }

        [[nodiscard]] HandOrFootPair pairBetween(const HandOrFootPair& from,
            const HandOrFootPair& to, const float share)
        {
            return { pointBetween(from[0], to[0], share), pointBetween(from[1], to[1], share) };
        }
    }

    // Seconds: fifth position, rising onto pointe as the arms go up to 2.1; a bourree to the
    // left to 3.1; a plie in fourth, then three pirouettes to 5.4; a penche to 7.1; a grand jete
    // to the right, landing at 8.05; the reverence.
    void BalletAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float floorY = stage.floorY;
        const float startX = stage.centerX + 30.0f * unit * stage.closeness;
        const float leftX = stage.centerX - 50.0f * unit * stage.closeness;
        const float rightX = stage.centerX + 40.0f * unit * stage.closeness;
        const auto between = [&](float from, float to) {
            return smoothStep(from, to, actTime);
        };
        m_android = {
            .lights = ChestLights::Pink,
            .wearsTutu = true,
            .wearsPointeShoes = true,
            .cycleTime = time.cycleTime
        };
        float pelvisX = startX;
        float pelvisHeight = AndroidBody::StandingHeight;
        HandOrFootPair feet = k_fifthFeet;
        HandOrFootPair hands = BalletArms::Low;
        bool placed = false;

        if (actTime < 3.1f)
        {
            // the arms rise through first to fifth as it goes up onto pointe; then a bourree,
            // the feet trembling on their points as it travels, the arms opening to second
            const float up = between(1.5f, 2.0f);
            pelvisX = std::lerp(startX, leftX, between(2.1f, 3.1f));
            const float step = std::sin(k_2Pi * actTime * 9.0f) * between(2.1f, 2.25f)
                * (1.0f - between(2.95f, 3.1f));
            feet = {
                FloatPoint{ -7.0f, 103.0f - 4.0f * std::max(0.0f, step) },
                FloatPoint{ 7.0f, 103.0f - 4.0f * std::max(0.0f, -step) },
            };
            pelvisHeight += k_pointeRise * up;
            m_android.left.footTurn = -k_pointe * up;
            m_android.right.footTurn = k_pointe * up;
            hands = pairBetween(BalletArms::Low, BalletArms::First, between(1.2f, 1.6f));
            hands = pairBetween(hands, BalletArms::Fifth, between(1.6f, 2.1f));
            hands = pairBetween(hands, BalletArms::Second, between(2.6f, 3.1f));
            m_android.headTurn = -0.07f + 0.05f * between(1.6f, 2.1f);
            m_android.look = actTime < 2.1f ? -1.0f : -5.0f;
            if (actTime < 1.3f)
                m_android.eyeOpening = 0.55f;
        }
        else if (actTime < 3.5f)
        {
            // down off pointe into fourth, in plie, ready to turn
            pelvisX = leftX;
            const float down = between(3.1f, 3.35f);
            const float dip = 14.0f * down;
            pelvisHeight += k_pointeRise * (1.0f - down) - dip;
            const HandOrFootPair fourth = {
                FloatPoint{ -24.0f, 103.0f },
                FloatPoint{ 10.0f, 103.0f },
            };
            feet = pairBetween(k_fifthFeet, fourth, down);
            feet[0].y = 103.0f - dip;
            feet[1].y = 103.0f - dip;
            m_android.left.footTurn = -k_pointe * (1.0f - down);
            m_android.right.footTurn = k_pointe * (1.0f - down);
            hands = pairBetween(BalletArms::Second, BalletArms::Preparation, down);
            m_android.look = -3.0f;
        }
        else if (actTime < 5.5f)
        {
            // three pirouettes on the right pointe, the left foot drawn up to the right knee
            pelvisX = leftX;
            const float go = between(3.5f, 3.62f);
            const float land = between(5.22f, 5.42f);
            const float spin = k_2Pi * 3.0f * std::clamp((actTime - 3.62f) / 1.6f, 0.0f, 1.0f);
            m_android.facing = std::cos(spin);
            m_android.tutuTurn = spin;
            const float up = go * (1.0f - land);
            const float dip = 14.0f * (1.0f - go);
            pelvisHeight += k_pointeRise * up - dip;
            const HandOrFootPair preparation = {
                FloatPoint{ -24.0f, 103.0f - dip },
                FloatPoint{ 10.0f, 103.0f - dip },
            };
            const HandOrFootPair passe = { FloatPoint{ -2.0f, 50.0f }, FloatPoint{ 2.0f, 103.0f } };
            feet = pairBetween(pairBetween(preparation, passe, go), k_fifthFeet, land);
            m_android.left.footTurn = std::lerp(-2.36f * go, 0.0f, land);
            m_android.right.footTurn = k_pointe * up;
            hands = pairBetween(BalletArms::Preparation, BalletArms::First, go);
            hands = pairBetween(hands, BalletArms::Second, land);
            m_android.look = -2.0f * land;
        }
        else if (actTime < 7.1f)
        {
            // a penche: the left leg stays upright on pointe while the figure tips to the left
            // and the right leg rises straight out to the right
            const float tipped = between(5.6f, 6.15f) * (1.0f - between(6.7f, 7.05f));
            const float up = between(5.5f, 5.7f) * (1.0f - between(6.85f, 7.08f));
            const float wobble = 0.025f * std::sin(k_2Pi * actTime * 1.8f) * between(6.0f, 6.2f)
                * (1.0f - between(6.55f, 6.7f));
            m_android.tilt = -0.42f * tipped + wobble;
            const FloatPoint ankle = {
                leftX - 7.0f * unit,
                floorY - (AndroidBody::AnkleHeight + k_pointeRise * up) * unit,
            };
            const FloatPoint leftHip = ankle - FloatPoint{ 10.0f, k_legLength } * unit;
            const FloatPoint leftHipFromPelvis = {
                -AndroidBody::RightHip.x * unit,
                AndroidBody::RightHip.y * unit,
            };
            m_android.pelvis = leftHip - rotated(leftHipFromPelvis, m_android.tilt);
            const FloatPoint rightHip = androidBodyToFrame(frame, m_android, AndroidBody::RightHip);
            const float raise = std::lerp(1.677f, -0.32f, tipped);
            const FloatPoint toe = rightHip
                + FloatPoint{ std::cos(raise), std::sin(raise) } * (k_legLength * unit);
            feet = {
                androidFrameToBody(frame, m_android, ankle),
                androidFrameToBody(frame, m_android, toe),
            };
            m_android.left.footTurn = -k_pointe * up - m_android.tilt;
            m_android.right.footTurn = std::lerp(k_pointe, raise - m_android.tilt, tipped);
            hands = pairBetween(BalletArms::Second, BalletArms::Penche, tipped);
            m_android.headTurn = 0.12f * tipped;
            m_android.look = 3.0f * tipped;
            placed = true;
        }
        else if (actTime < 8.45f)
        {
            // a grand jete to the right: a plie, the legs flung apart in the air a little below
            // level, so they show beneath the tutu, and a plie to land
            const float preparing = between(7.1f, 7.38f);
            const float flight = std::clamp((actTime - 7.45f) / 0.6f, 0.0f, 1.0f);
            const bool flying = actTime >= 7.45f && actTime < 8.05f;
            const float split = smoothStep(0.0f, 0.3f, flight)
                * (1.0f - smoothStep(0.72f, 1.0f, flight));
            const float landing = between(8.05f, 8.12f) * (1.0f - between(8.18f, 8.45f));
            pelvisX = std::lerp(leftX, rightX, flight);
            float dip = 0.0f;
            if (actTime < 7.45f)
                dip = 16.0f * preparing;
            else if (!flying)
                dip = 4.0f + 14.0f * landing;
            pelvisHeight -= dip;
            if (flying)
                pelvisHeight += 78.0f * 4.0f * flight * (1.0f - flight);
            const HandOrFootPair ground = {
                FloatPoint{ -14.0f, 103.0f - dip },
                FloatPoint{ 14.0f, 103.0f - dip },
            };
            const HandOrFootPair air = {
                FloatPoint{ -106.0f, 26.0f },
                FloatPoint{ 108.0f, 33.0f },
            };
            feet = pairBetween(ground, air, split);
            m_android.left.footTurn = -0.3f * split;
            m_android.right.footTurn = 0.22f * split;
            m_android.tilt = 0.06f * split;
            hands = pairBetween(BalletArms::Second, BalletArms::Low, preparing);
            hands = pairBetween(hands, BalletArms::Leap, smoothStep(0.0f, 0.25f, flight));
            hands = pairBetween(hands, BalletArms::Second, between(8.05f, 8.4f));
            m_android.look = 4.0f * (1.0f - between(8.1f, 8.4f));
        }
        else
        {
            // the reverence: the knees bent, one foot behind, the arms opened low, the head bowed
            pelvisX = rightX;
            const float bow = between(8.5f, 8.9f) * (1.0f - between(9.05f, 9.4f));
            const float dip = 4.0f * (1.0f - between(8.45f, 8.6f)) + 22.0f * bow;
            pelvisHeight -= dip;
            feet = {
                FloatPoint{ -12.0f, 103.0f - dip },
                FloatPoint{ 14.0f, 99.0f - dip - 2.0f * bow },
            };
            hands = pairBetween(BalletArms::Second, BalletArms::Bow, between(8.45f, 8.8f));
            m_android.left.elbow = ElbowBend::Down;
            m_android.right.elbow = ElbowBend::Down;
            m_android.headTurn = 0.15f * bow;
            m_android.look = -2.0f * bow;
            if (bow > 0.5f)
                m_android.eyeOpening = 0.2f;
            if (actTime > 9.0f)
                m_android.lights = flashingLights(actTime);
        }

        if (!placed)
            m_android.pelvis = { pelvisX, floorY - pelvisHeight * unit };
        m_android.left.foot = feet[0];
        m_android.right.foot = feet[1];
        m_android.left.hand = hands[0];
        m_android.right.hand = hands[1];
    }

    void BalletAct::draw(const SceneFrame& frame) const
    {
        drawAndroid(frame, m_android);
    }
}
