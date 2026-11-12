module ClaFi.PathArt.AccordionAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.MusicNotesEffect;
import ClaFi.PathArt.PuppeteerAct;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace AccordionColors
    {
        constexpr Color FoldShade{ 0xFF6A1022 };
        constexpr Color FoldLight{ 0xFFC7334A };
        constexpr Color Trim{ 0xFFD9A845 };
        constexpr Color TrimLine{ 0xB3D9A845 };
        constexpr Color End{ 0xFF1F1D27 };
        constexpr Color Button{ 0xFFF3EFE6 };
        constexpr Color Keys{ 0xFFF6F2E8 };
        constexpr Color KeyLine{ 0xCC8C8379 };
        constexpr Color BlackKey{ 0xFF1B1A22 };
    }

    namespace
    {
        // The middle of the bellows, in units about the pelvis, before its tilt.
        constexpr FloatPoint k_bellowsMiddle{ 0.0f, -58.0f };

        // An accordion in its own units about the middle of the bellows: the buttons at the left
        // end, the keys at the right; `halfWidth` is half the bellows' width.
        void drawAccordion(const SceneFrame& accordion, const float halfWidth)
        {
            constexpr float k_top{ -36.0f };
            constexpr float k_bottom{ 36.0f };
            constexpr int k_folds{ 8 };
            const float foldWidth = 2.0f * halfWidth / static_cast<float>(k_folds);
            for (int i = 0; i != k_folds; ++i)
            {
                const float left = -halfWidth + foldWidth * static_cast<float>(i);
                const float right = left + foldWidth;
                const float middle = (left + right) * 0.5f;
                PixelPath fold;
                fold.moveTo(left, k_top + 3.0f);
                fold.lineTo(middle, k_top - 2.0f);
                fold.lineTo(right, k_top + 3.0f);
                fold.lineTo(right, k_bottom - 3.0f);
                fold.lineTo(middle, k_bottom + 2.0f);
                fold.lineTo(left, k_bottom - 3.0f);
                fold.close();
                fillShape(accordion, fold, LinearGradient{
                    .startPoint = { left, 0.0f },
                    .endPoint = { right, 0.0f },
                    .stops = {
                        { .position = 0.0f, .color = AccordionColors::FoldShade },
                        { .position = 0.5f, .color = AccordionColors::FoldLight },
                        { .position = 1.0f, .color = AccordionColors::FoldShade }
                    }
                });
            }
            const SolidColor trimLine{ AccordionColors::TrimLine };
            drawSegment(accordion, { -halfWidth, k_top + 6.0f }, { halfWidth, k_top + 6.0f },
                trimLine, 1.2f, StrokeCap::Butt);
            drawSegment(accordion, { -halfWidth, k_bottom - 6.0f }, { halfWidth, k_bottom - 6.0f },
                trimLine, 1.2f, StrokeCap::Butt);

            const FloatRect buttonEnd = {
                -halfWidth - 18.0f,
                k_top - 4.0f,
                -halfWidth,
                k_bottom + 4.0f,
            };
            const FloatRect keyEnd = {
                halfWidth,
                k_top - 4.0f,
                halfWidth + 22.0f,
                k_bottom + 4.0f,
            };
            const SolidColor end{ AccordionColors::End };
            fillRoundedBox(accordion, buttonEnd, 4.0f, end);
            PixelPath buttons;
            for (int row = 0; row != 6; ++row)
            {
                for (int column = 0; column != 2; ++column)
                    addEllipse(buttons, { -halfWidth - 12.0f + static_cast<float>(column) * 6.0f,
                        k_top + 6.0f + static_cast<float>(row) * 11.0f }, 1.9f, 1.9f);
            }
            fillShape(accordion, buttons, SolidColor{ AccordionColors::Button });
            fillRoundedBox(accordion, keyEnd, 4.0f, end);
            fillBox(accordion, { halfWidth + 4.0f, k_top, halfWidth + 18.0f, k_bottom },
                SolidColor{ AccordionColors::Keys });
            PixelPath keyLines;
            PixelPath blackKeys;
            for (int key = 0; key != 9; ++key)
            {
                const float y = k_top + 4.0f + static_cast<float>(key) * 8.0f;
                keyLines.moveTo(halfWidth + 4.0f, y + 4.0f);
                keyLines.lineTo(halfWidth + 18.0f, y + 4.0f);
                // Two white keys in seven stand side by side with no black key between them.
                if (key % 7 != 2 && key % 7 != 6)
                    blackKeys.drawRoundedRect(FloatRect{ halfWidth + 4.0f, y + 2.0f,
                        halfWidth + 12.0f, y + 6.0f }, 0.0f, 0.0f);
            }
            strokeShape(accordion, keyLines, SolidColor{ AccordionColors::KeyLine }, 0.6f,
                StrokeCap::Butt);
            fillShape(accordion, blackKeys, SolidColor{ AccordionColors::BlackKey });
            PixelPath ends;
            ends.drawRoundedRect(buttonEnd, 4.0f, 4.0f);
            strokeShape(accordion, ends, SolidColor{ AccordionColors::Trim }, 1.2f);
            ends.clear();
            ends.drawRoundedRect(keyEnd, 4.0f, 4.0f);
            strokeShape(accordion, ends, SolidColor{ AccordionColors::Trim }, 1.2f);
        }
    }

    // The bellows open and close every 1.25 seconds and spread wide for the last chord.
    void AccordionAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float pi = std::numbers::pi_v<float>;
        const float x = stage.centerX + 30.0f * unit * stage.closeness;
        const float sway = 10.0f * std::sin(k_2Pi * actTime / 2.5f);
        const float dip = 4.0f * std::pow(std::cos(pi * actTime / 0.625f), 2.0f);
        m_android = {
            .pelvis = {
                x + sway * unit,
                stage.floorY - (AndroidBody::StandingHeight - dip) * unit
            },
            .cycleTime = time.cycleTime
        };
        const float tap = std::max(0.0f, std::sin(k_2Pi * actTime / 0.625f)) * 7.0f;
        const float ankleY = stage.floorY - AndroidBody::AnkleHeight * unit;
        plantAndroidFeet(frame, m_android, { x - 22.0f * unit, ankleY },
            { x + 22.0f * unit, ankleY - tap * unit });
        m_android.right.footTurn = -0.04f * tap;

        const float finale = smoothStep(8.4f, 8.9f, actTime);
        m_bellowsHalfWidth = std::lerp(30.0f + 16.0f * std::sin(k_2Pi * actTime / 1.25f), 50.0f,
            finale);
        m_tilt = 0.07f * std::sin(k_2Pi * actTime / 2.5f);
        const auto onAccordion = [&](FloatPoint point) {
            return k_bellowsMiddle + rotated(point, m_tilt);
        };
        const float fingering = -8.0f + 7.0f * std::sin(k_2Pi * actTime * 3.1f);
        m_android.left.hand = onAccordion({ -m_bellowsHalfWidth - 10.0f, fingering });
        m_android.left.forearmOnTop = true;
        m_android.right.hand = onAccordion({ m_bellowsHalfWidth + 22.0f, 2.0f });
        m_android.right.forearmOnTop = true;
        m_android.headTurn = 0.08f * std::sin(k_2Pi * actTime / 1.25f) - m_tilt;
        m_android.look = 3.0f * std::sin(k_2Pi * actTime / 2.5f);
        if (actTime > 3.0f && actTime < 7.0f)
            m_android.eyeOpening = 0.35f;
        if (actTime > 8.9f)
            m_android.lights = flashingLights(actTime);
        m_spotlightX = x;
        m_actTime = actTime;
    }

    // The keys face the audience on the left, under the android's right hand.
    void AccordionAct::draw(const SceneFrame& frame) const
    {
        drawAndroid(frame, m_android);
        const SceneFrame accordion = androidUpperFrame(frame, m_android).within(
            Matrix3x2::Translation(k_bellowsMiddle) * Matrix3x2::Rotation(m_tilt)
            * Matrix3x2::Scale(-1.0f, 1.0f));
        drawAccordion(accordion, m_bellowsHalfWidth);
        drawAndroidForearm(frame, m_android, AudienceSide::Left);
        drawAndroidForearm(frame, m_android, AudienceSide::Right);
        drawMusicNotes(frame, MusicNotes{
            .origin = androidUpperToFrame(frame, m_android, { 0.0f, -120.0f }),
            .spread = 210.0f,
            .time = m_actTime,
            .seed = 3.0f
        });
    }
}
