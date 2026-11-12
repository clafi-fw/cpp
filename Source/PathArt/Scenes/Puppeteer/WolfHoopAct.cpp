module ClaFi.PathArt.WolfHoopAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.DazeStarsEffect;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.WolfSprite;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace HoopColors
    {
        constexpr Color Red{ 0xFFC2412D };
        constexpr Color Cream{ 0xFFF6E7C8 };
    }

    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        // The hand grips the top of the hoop, held out at the android's side.
        constexpr FloatPoint k_hoopGrip{ -150.0f, -55.0f };
        constexpr float k_hoopRadius{ 60.0f };
        // Seen almost edge on, the hoop is a narrow ellipse.
        constexpr float k_hoopDepth{ 15.0f };
        constexpr float k_wolfSize{ 1.2f };
        // The wolf's middle above his feet, in his units, and lying on his back.
        constexpr float k_standingHeight{ 54.0f };
        constexpr float k_lyingHeight{ 26.0f };
        constexpr float k_runSpeed{ 480.0f };
        // A leap starts and ends this far from the hoop.
        constexpr float k_leapReach{ 120.0f };
        // A stride of the gallop covers 84 of the wolf's units.
        constexpr float k_strideLength{ 84.0f };
        constexpr float k_backInAt{ 4.2f };

        // Half the hoop as red and cream bands; the far half is drawn under what goes through it.
        void drawHoopHalf(const SceneFrame& frame, const FloatPoint middle, const bool nearHalf)
        {
            const float unit = frame.unit;
            const FloatPoint radii = { k_hoopDepth * unit, k_hoopRadius * unit };
            const float start = nearHalf ? k_pi * 0.5f : -k_pi * 0.5f;
            constexpr int k_bands{ 10 };
            PixelPath red;
            PixelPath cream;
            for (int band = 0; band != k_bands; ++band)
            {
                const float from = start + k_pi * static_cast<float>(band) / k_bands;
                const float to = start + k_pi * static_cast<float>(band + 1) / k_bands + 0.02f;
                PixelPath& path = band % 2 == 0 ? red : cream;
                path.moveTo(
                    middle + FloatPoint{ std::cos(from) * radii.x, std::sin(from) * radii.y });
                path.arcTo(radii, 0.0f, ArcSize::Small, ArcSweep::Clockwise,
                    middle + FloatPoint{ std::cos(to) * radii.x, std::sin(to) * radii.y });
            }
            strokeShape(frame, red, SolidColor{ HoopColors::Red }, 6.0f * unit, StrokeCap::Butt);
            strokeShape(frame, cream, SolidColor{ HoopColors::Cream }, 6.0f * unit,
                StrokeCap::Butt);
        }
    }

    // Seconds: the wolf waits by the stand, panting, to 1.4; runs and leaps clean through; runs
    // off past the android; comes back in from the right at 4.2 facing left, hops too low, catches
    // the rim and goes over, lies dazed, and is up again by 8.6.
    void WolfHoopAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float x = stage.centerX + 150.0f * unit * stage.closeness;
        const float wolfUnit = k_wolfSize * unit;
        const float runY = stage.floorY + 8.0f * unit;
        const float standingY = runY - k_standingHeight * wolfUnit;
        const float speed = k_runSpeed * unit;
        const float leapReach = k_leapReach * unit;
        const float startX = stage.drapeWidth + 95.0f * unit;
        const float backInX = frame.width + 140.0f * unit;

        // the hoop, which swings in the hand once the wolf has caught it
        m_android = standingAndroid(frame, { x, stage.floorY }, time.cycleTime);
        const float restingHoopX = androidUpperToFrame(frame, m_android,
            k_hoopGrip + FloatPoint{ 0.0f, k_hoopRadius }).x;
        const float caughtAt = k_backInAt + (backInX - (restingHoopX + 22.0f * unit)) / speed;
        const float sinceCaught = actTime - caughtAt;
        const float swing = sinceCaught > 0.0f
            ? 0.3f * std::exp(-sinceCaught / 0.7f) * std::sin(k_2Pi * sinceCaught * 1.8f)
            : 0.0f;
        m_hoop = androidUpperToFrame(frame, m_android,
            k_hoopGrip + rotated({ 0.0f, k_hoopRadius }, swing));
        const float hoopX = m_hoop.x;
        const float leapLeftX = hoopX - leapReach;
        const float leapRightX = hoopX + leapReach;
        const float leapWidth = 2.0f * leapReach;

        m_wolf = {
            .center = { startX, standingY },
            .size = wolfUnit,
            .turn = 0.0f,
            .stride = 0.0f,
            .leap = 0.0f,
            .sprawl = 0.0f,
            .onBack = 0.0f,
            .dangle = 0.0f,
            .dazed = 0.0f,
            .headTurn = 0.0f,
            .tongueTurn = 0.1f,
            .jawOpen = 0.35f,
            .glance = 0.0f,
            .cycleTime = time.cycleTime
        };
        m_wolfFacesLeft = false;
        m_dazeTurns = -1.0f;
        const auto lowHop = [&](float share) {
            return standingY - 4.0f * 14.0f * unit * share * (1.0f - share);
        };
        if (actTime < 1.4f)
        {
            m_wolf.stride = 0.2f * std::sin(k_2Pi * actTime * 2.0f);
            m_wolf.tongueTurn = 0.2f * std::sin(k_2Pi * actTime * 3.0f);
            m_wolf.glance = 2.0f;
        }
        else if (actTime < k_backInAt)
        {
            const float running = actTime - 1.4f;
            const float run = running < 0.3f ? running * running / 0.6f : running - 0.15f;
            m_wolf.center.x = startX + speed * run;
            m_wolf.stride = k_2Pi * (m_wolf.center.x / wolfUnit) / k_strideLength;
            const float share = std::clamp((m_wolf.center.x - leapLeftX) / leapWidth, 0.0f, 1.0f);
            if (share > 0.0f && share < 1.0f)
            {
                m_wolf.leap = std::min(1.0f, 2.2f * std::sin(k_pi * share));
                m_wolf.center.y = standingY - 4.0f * 62.0f * unit * share * (1.0f - share);
                m_wolf.turn = -0.3f * std::cos(k_pi * share);
            }
            m_wolf.jawOpen = 0.25f;
        }
        else if (actTime < caughtAt)
        {
            m_wolfFacesLeft = true;
            m_wolf.center.x = backInX - speed * (actTime - k_backInAt);
            m_wolf.stride = k_2Pi * (-m_wolf.center.x / wolfUnit) / k_strideLength;
            const float share = std::clamp((leapRightX - m_wolf.center.x) / leapWidth, 0.0f, 1.0f);
            if (share > 0.0f)
            {
                m_wolf.leap = std::min(1.0f, 2.2f * std::sin(k_pi * std::min(share, 0.5f)));
                m_wolf.center.y = lowHop(share);
                m_wolf.turn = -0.3f * std::cos(k_pi * share);
            }
            m_wolf.jawOpen = 0.25f;
        }
        else
        {
            // caught on the rim: over onto his back, dazed, and up again
            const float caughtX = backInX - speed * (caughtAt - k_backInAt);
            const float caughtShare = std::clamp((leapRightX - caughtX) / leapWidth, 0.0f, 1.0f);
            const float fall = smoothStep(0.0f, 1.0f, (actTime - caughtAt) / 0.55f);
            const float up = smoothStep(8.1f, 8.6f, actTime);
            const float lyingY = runY - k_lyingHeight * wolfUnit;
            m_wolfFacesLeft = true;
            m_wolf.center.x = caughtX - 110.0f * unit * fall;
            m_wolf.center.y = std::lerp(lowHop(caughtShare), lyingY, fall)
                - 36.0f * unit * std::sin(k_pi * fall)
                + (standingY - lyingY) * up - 30.0f * unit * std::sin(k_pi * up);
            m_wolf.turn = std::lerp(-0.3f * std::cos(k_pi * caughtShare), k_pi, fall) + k_pi * up;
            m_wolf.onBack = fall * (1.0f - up);
            m_wolf.leap = std::max(0.0f, 1.0f - 2.0f * fall) * (1.0f - up);
            const bool dazed = actTime < 8.3f;
            m_wolf.dazed = dazed ? 1.0f : 0.0f;
            m_wolf.jawOpen = dazed ? 0.5f : 0.35f;
            m_wolf.tongueTurn = dazed ? 0.6f : 0.2f * std::sin(k_2Pi * actTime * 3.0f);
            m_wolf.glance = actTime > 8.8f ? 3.0f : 0.0f;
            if (actTime > caughtAt + 0.4f && actTime < 8.2f)
                m_dazeTurns = actTime * 1.4f;
        }

        // the android: holds the hoop out, sends him, cheers, holds its head, ta-da anyway
        const float sending = smoothStep(1.0f, 1.3f, actTime)
            * (1.0f - smoothStep(1.8f, 2.1f, actTime));
        const float cheering = smoothStep(2.6f, 2.9f, actTime)
            * (1.0f - smoothStep(3.4f, 3.7f, actTime));
        const float facepalm = smoothStep(5.9f, 6.3f, actTime)
            * (1.0f - smoothStep(7.6f, 8.0f, actTime));
        const float tada = smoothStep(8.6f, 9.0f, actTime);
        FloatPoint rightHand = pointBetween(ActHands::RightOnHip, { -40.0f, -120.0f }, sending);
        rightHand = pointBetween(rightHand, { 110.0f, -190.0f }, cheering);
        rightHand = pointBetween(rightHand, { 8.0f, -158.0f }, facepalm);
        m_android.right.hand = pointBetween(rightHand, ActHands::RightRaised, tada);
        m_android.right.elbow = facepalm > 0.5f ? ElbowBend::Down : ElbowBend::Out;
        m_android.left.hand = k_hoopGrip;
        m_android.left.handShape = HandShape::Fist;
        m_android.left.forearmOnTop = true;
        const float watching = std::clamp((m_wolf.center.x - x) / (200.0f * unit), -1.0f, 1.0f);
        m_android.look = facepalm > 0.5f ? 0.0f : 6.0f * watching;
        m_android.headTurn = facepalm > 0.5f ? 0.12f : 0.06f * watching;
        if (cheering > 0.5f || actTime > 9.0f)
            m_android.lights = flashingLights(actTime);
        m_spotlightX = std::lerp(hoopX, x, 0.4f);
    }

    // He runs downstage of the android, so in front of it, and through the hoop between its halves.
    void WolfHoopAct::draw(const SceneFrame& frame) const
    {
        drawAndroid(frame, m_android);
        drawHoopHalf(frame, m_hoop, false);
        const SceneFrame wolfFrame = m_wolfFacesLeft
            ? frame.within(Matrix3x2::Translation(2.0f * m_wolf.center.x, 0.0f)
                * Matrix3x2::Scale(-1.0f, 1.0f))
            : frame;
        drawWolf(wolfFrame, m_wolf, WolfCoats::Grey);
        drawHoopHalf(frame, m_hoop, true);
        drawAndroidForearm(frame, m_android, AudienceSide::Left);
        if (m_dazeTurns < 0.0f)
            return;
        const float unit = frame.unit;
        const float facing = m_wolfFacesLeft ? -1.0f : 1.0f;
        const FloatPoint head = m_wolf.center + FloatPoint{ facing * 20.0f, -16.0f } * m_wolf.size;
        drawDazeStars(frame, DazeStars{
            .center = head - FloatPoint{ 0.0f, 34.0f * unit },
            .turns = m_dazeTurns
        });
    }
}
