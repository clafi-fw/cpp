module ClaFi.PathArt.CometSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace CometColors
    {
        constexpr Color DustHead{ 0xFFFFF4DC };
        constexpr Color DustMiddle{ 0xFFFFC68A };
        constexpr Color DustEnd{ 0xFFFF9A5C };
        constexpr Color IonTail{ 0xFF9FE6FF };
        constexpr Color ColdSpark{ 0xFFBFE9FF };
        constexpr Color WarmSpark{ 0xFFFFE2B0 };
        constexpr Color Coma{ 0xFFCFEAFF };
        constexpr Color Nucleus{ 0xFFFFFFFF };
    }

    FloatPoint pointOnCourse(const CometCourse& course, const float along)
    {
        const float rest = 1.0f - along;
        return course.start * (rest * rest) + course.control * (2.0f * rest * along)
            + course.end * (along * along);
    }

    void drawComet(const SceneFrame& frame, const CometPose& pose)
    {
        const float unit = frame.unit;
        const float passTime = pose.passTime;
        const float fallShare = pose.fallShare;
        const float tailFade = 1.0f - smoothStep(fallShare, fallShare + 0.08f, passTime);
        if (tailFade <= 0.0f)
            return;
        const bool falling = passTime < fallShare;
        // The comet sets off slowly and gathers speed as it falls.
        const float headAlong = falling ? std::pow(passTime / fallShare, 1.35f) : 1.0f;
        const FloatPoint head = pointOnCourse(pose.course, headAlong);

        // The dust tail follows the curve the comet came along, fanning out as it goes and
        // growing with the comet's speed.
        const float speed = falling
            ? 0.5f + 1.3f * std::sqrt(passTime / fallShare)
            : 1.8f;
        const float span = 0.05f + 0.1f * speed;
        constexpr int k_samples = 16;
        std::array<FloatPoint, k_samples + 1> outerEdge{};
        std::array<FloatPoint, k_samples + 1> innerEdge{};
        FloatPoint tailEnd = head;
        for (int i = 0; i <= k_samples; ++i)
        {
            const float share = static_cast<float>(i) / k_samples;
            const float along = headAlong - span * share;
            const FloatPoint point = pointOnCourse(pose.course, along);
            const FloatPoint ahead = pointOnCourse(pose.course, along + 0.002f);
            const FloatPoint step = ahead - point;
            const float stepLength = std::hypot(step.x, step.y);
            const FloatPoint tangent = stepLength > 0.0f
                ? step * (1.0f / stepLength)
                : FloatPoint{ 1.0f, 0.0f };
            const FloatPoint outward = { -tangent.y, tangent.x };
            const float taper = 15.0f * std::pow(1.0f - share, 1.1f) + 2.0f * share;
            const float halfWidth = taper * unit * (0.7f + 0.3f * speed);
            const float fan = 1.0f + share * 1.6f;
            outerEdge[static_cast<std::size_t>(i)] = point + outward * (halfWidth * fan);
            innerEdge[static_cast<std::size_t>(i)] = point - outward * (halfWidth * 0.5f);
            tailEnd = point;
        }
        PixelPath dustTail;
        dustTail.moveTo(outerEdge[0]);
        for (std::size_t i = 1; i != outerEdge.size(); ++i)
            dustTail.lineTo(outerEdge[i]);
        for (std::size_t i = innerEdge.size(); i != 0; --i)
            dustTail.lineTo(innerEdge[i - 1]);
        dustTail.close();
        const Color headDust = CometColors::DustHead.withOpacity(0.85f * tailFade);
        const Color middleDust = CometColors::DustMiddle.withOpacity(0.45f * tailFade);
        fillShape(frame, dustTail, LinearGradient{
            .startPoint = head,
            .endPoint = tailEnd,
            .stops = {
                { .position = 0.0f, .color = headDust },
                { .position = 0.35f, .color = middleDust },
                { .position = 1.0f, .color = CometColors::DustEnd.withOpacity(0.0f) }
            }
        });

        // The ion tail runs straight back from the head, a little off the line the comet came
        // along.
        const FloatPoint justBehind = pointOnCourse(pose.course, std::max(0.0f, headAlong - 0.01f));
        FloatPoint back = justBehind - head;
        const float backLength = std::hypot(back.x, back.y);
        back = backLength > 0.0f
            ? rotated(back * (1.0f / backLength), 0.16f)
            : FloatPoint{ -1.0f, 0.0f };
        const float ionLength = std::hypot(tailEnd.x - head.x, tailEnd.y - head.y) * 1.25f;
        const FloatPoint ionEnd = head + back * ionLength;
        const FloatPoint across = FloatPoint{ -back.y, back.x } * (3.0f * unit);
        PixelPath ionTail;
        ionTail.moveTo(head + across);
        ionTail.lineTo(ionEnd);
        ionTail.lineTo(head - across);
        ionTail.close();
        const Color ion = CometColors::IonTail;
        fillShape(frame, ionTail, LinearGradient::Simple(head, ionEnd,
            ion.withOpacity(0.7f * tailFade), ion.withOpacity(0.0f)));

        // Sparks shed along the way drift down and fade.
        for (int i = 0; i != 26; ++i)
        {
            const float born = static_cast<float>(i) * 0.021f;
            const float age = (passTime - born) / 0.3f;
            if (age <= 0.0f || age >= 1.0f || born > fallShare)
                continue;
            const FloatPoint origin = pointOnCourse(pose.course, std::pow(born / fallShare, 1.35f));
            const float sinking = 40.0f * unit * age * age;
            const FloatPoint spark = {
                origin.x + (pseudoRandom(i, 41.0f) - 0.5f) * 50.0f * unit * age,
                origin.y + (pseudoRandom(i, 42.0f) - 0.3f) * 30.0f * unit * age + sinking
            };
            const float phase = static_cast<float>(i);
            const float twinkle = 0.6f + 0.4f * std::sin(k_2Pi * 40.0f * pose.cycleTime + phase);
            const Color color = i % 3 == 0 ? CometColors::ColdSpark : CometColors::WarmSpark;
            drawSparkle(frame, spark, (2.0f + 2.0f * pseudoRandom(i, 43.0f)) * unit * twinkle,
                color.withOpacity((1.0f - age) * 0.9f * tailFade));
        }

        if (falling)
        {
            drawGlow(frame, head, 48.0f * unit, CometColors::Coma.withOpacity(0.55f));
            drawGlow(frame, head, 16.0f * unit, CometColors::Nucleus.withOpacity(0.95f));
            fillDisc(frame, head, 4.0f * unit, SolidColor{ CometColors::Nucleus });
        }
    }
}
