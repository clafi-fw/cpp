module ClaFi.PathArt.GuitarAct;

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

    namespace GuitarColors
    {
        constexpr Color Rim{ 0xFF5D3214 };
        constexpr Color Wood{ 0xFFD08A43 };
        constexpr Color WoodShade{ 0x597A3E14 };
        constexpr Color Neck{ 0xFF3A2414 };
        constexpr Color Head{ 0xFF5B3A20 };
        constexpr Color Peg{ 0xFFD9A845 };
        constexpr Color SoundHole{ 0xFF2A1A10 };
        constexpr Color Rosette{ 0xCC3A2414 };
        constexpr Color Fret{ 0xB3C9C2B0 };
        constexpr Color String{ 0xE6EFE6D0 };
    }

    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        // The middle of the lower bout, in units about the pelvis.
        constexpr FloatPoint k_bodyMiddle{ -14.0f, -44.0f };
        // A chord a bar of 1.25 seconds, as a shift of the fretting hand along the neck.
        constexpr std::array<float, 4> k_chordShifts{ 0.0f, 1.0f, -0.6f, 0.5f };
        constexpr float k_strokeSeconds{ 0.3125f };

        // A guitar in its own units about the middle of its lower bout, the neck along +x.
        void drawGuitar(const SceneFrame& guitar, const float ringing)
        {
            const SolidColor rim{ GuitarColors::Rim };
            const SolidColor wood{ GuitarColors::Wood };
            fillDisc(guitar, { 0.0f, 0.0f }, 42.0f, rim);
            fillDisc(guitar, { 46.0f, 0.0f }, 32.0f, rim);
            fillBox(guitar, { 0.0f, -30.0f, 46.0f, 30.0f }, rim);
            fillBox(guitar, { 60.0f, -7.0f, 176.0f, 7.0f }, SolidColor{ GuitarColors::Neck });
            PixelPath head;
            head.moveTo(174.0f, -9.0f);
            head.lineTo(198.0f, -12.0f);
            head.lineTo(198.0f, 12.0f);
            head.lineTo(174.0f, 9.0f);
            head.close();
            fillShape(guitar, head, SolidColor{ GuitarColors::Head });
            PixelPath pegs;
            for (const float pegX : { 181.0f, 189.0f })
            {
                addEllipse(pegs, { pegX, -14.0f }, 2.6f, 2.6f);
                addEllipse(pegs, { pegX, 14.0f }, 2.6f, 2.6f);
            }
            fillShape(guitar, pegs, SolidColor{ GuitarColors::Peg });
            fillDisc(guitar, { 0.0f, 0.0f }, 39.5f, wood);
            fillDisc(guitar, { 46.0f, 0.0f }, 29.5f, wood);
            fillBox(guitar, { 0.0f, -27.5f, 46.0f, 27.5f }, wood);
            drawGlow(guitar, { -10.0f, 10.0f }, 40.0f, GuitarColors::WoodShade);
            fillDisc(guitar, { 30.0f, 0.0f }, 12.0f, SolidColor{ GuitarColors::SoundHole });
            PixelPath rosette;
            addEllipse(rosette, { 30.0f, 0.0f }, 14.5f, 14.5f);
            strokeShape(guitar, rosette, SolidColor{ GuitarColors::Rosette }, 1.6f);
            fillBox(guitar, { -22.0f, -12.0f, -16.0f, 12.0f }, SolidColor{ GuitarColors::Neck });
            PixelPath frets;
            for (int fret = 0; fret != 9; ++fret)
            {
                const float order = static_cast<float>(fret);
                const float x = 70.0f + order * 11.5f - order * order * 0.25f;
                frets.moveTo(x, -7.0f);
                frets.lineTo(x, 7.0f);
            }
            strokeShape(guitar, frets, SolidColor{ GuitarColors::Fret }, 0.8f, StrokeCap::Butt);
            PixelPath strings;
            for (int i = 0; i != 4; ++i)
            {
                const float y = -4.5f + static_cast<float>(i) * 3.0f;
                const float shake = ringing * 0.9f
                    * std::sin(static_cast<float>(i) * 1.7f + ringing * 40.0f);
                strings.moveTo(-19.0f, y);
                strings.quadTo({ 30.0f, y + shake }, { 176.0f, y });
            }
            strokeShape(guitar, strings, SolidColor{ GuitarColors::String }, 0.7f);
        }
    }

    // Held right-handed: the neck goes off to the audience's right, fretted by the hand there.
    void GuitarAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float x = stage.centerX + 30.0f * unit * stage.closeness;
        const float dip = 6.0f * std::pow(std::cos(k_pi * actTime / 0.625f), 2.0f);
        m_android = standingAndroid(frame, { x, stage.floorY }, time.cycleTime, dip, 26.0f);

        const float raised = smoothStep(6.6f, 7.0f, actTime)
            * (1.0f - smoothStep(8.2f, 8.6f, actTime));
        m_neckAngle = -0.4f - 0.05f * std::sin(k_2Pi * actTime / 2.5f) - 0.5f * raised;
        const auto onGuitar = [&](FloatPoint point) {
            return k_bodyMiddle + rotated(point, m_neckAngle);
        };
        const int bar = static_cast<int>(std::floor(actTime / 1.25f));
        const float chord = k_chordShifts[static_cast<std::size_t>(bar % 4)];
        const float lastChord = k_chordShifts[static_cast<std::size_t>((bar + 3) % 4)];
        const float slide = smoothStep(0.0f, 0.12f, fractionalPart(actTime / 1.25f));
        const float fret = 132.0f + 14.0f * std::lerp(lastChord, chord, slide);
        const float stroke = std::cos(k_pi * actTime / k_strokeSeconds);
        const float tada = smoothStep(8.4f, 8.9f, actTime);

        m_android.left.hand = pointBetween(onGuitar({ 28.0f, 15.0f * stroke }),
            ActHands::LeftRaised, tada);
        m_android.left.forearmOnTop = true;
        m_android.right.hand = onGuitar({ fret, 10.0f });
        m_android.right.elbow = ElbowBend::Down;
        m_android.right.handShape = HandShape::Fist;
        m_android.right.forearmOnTop = true;
        m_android.headTurn = 0.1f * raised + 0.06f * std::sin(k_2Pi * actTime / 1.25f);
        m_android.look = 4.0f;
        if (actTime > 8.9f)
            m_android.lights = flashingLights(actTime);
        m_ringing = (1.0f - fractionalPart(actTime / k_strokeSeconds)) * (1.0f - tada);
        m_spotlightX = x;
        m_actTime = actTime;
    }

    void GuitarAct::draw(const SceneFrame& frame) const
    {
        drawAndroid(frame, m_android);
        drawGuitar(androidUpperFrame(frame, m_android).within(
            Matrix3x2::Translation(k_bodyMiddle) * Matrix3x2::Rotation(m_neckAngle)), m_ringing);
        drawAndroidForearm(frame, m_android, AudienceSide::Left);
        drawAndroidForearm(frame, m_android, AudienceSide::Right);
        drawMusicNotes(frame, MusicNotes{
            .origin = androidUpperToFrame(frame, m_android, { 70.0f, -120.0f }),
            .spread = 160.0f,
            .time = m_actTime,
            .seed = 7.0f
        });
    }
}
