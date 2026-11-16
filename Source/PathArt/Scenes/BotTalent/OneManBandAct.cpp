module ClaFi.PathArt.OneManBandAct;

import ClaFi.PathArt.AndroidSprite;
import ClaFi.PathArt.BotTalentAct;
import ClaFi.PathArt.MusicNotesEffect;
import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.PathArt.SparkleBurstEffect;
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

    namespace DrumColors
    {
        constexpr Color Shell{ 0xFFB8233A };
        constexpr Color ShellLight{ 0x2EFFFFFF };
        constexpr Color ShellShade{ 0x40000000 };
        constexpr Color Lacing{ 0xBFF3EFE6 };
        constexpr Color Hoop{ 0xFFD9A845 };
        constexpr Color Underside{ 0xFF8A1A2C };
        constexpr Color Skin{ 0xFFF3EFE6 };
        constexpr Color SkinStruck{ 0xFFFFFFFF };
        constexpr Color Stick{ 0xFFE8C9A0 };
        constexpr Color StickTip{ 0xFFF3E3C8 };
        constexpr Color Metal{ 0xFF9AA3AD };
        constexpr Color MetalDark{ 0xFF5E6873 };
        constexpr Color Felt{ 0xFFF3EFE6 };
        constexpr Color Cymbal{ 0xFFD9A845 };
        constexpr Color CymbalShine{ 0xFFF0CF72 };
        constexpr Color CymbalBell{ 0xFF9A7A2C };
        constexpr Color Ring{ 0xB3FFF6DC };
    }

    namespace
    {
        constexpr float k_pi = std::numbers::pi_v<float>;
        // The middle of the guitar's lower bout, in units about the pelvis.
        constexpr FloatPoint k_bodyMiddle{ -14.0f, -44.0f };
        // A chord a bar of 1.25 seconds, as a shift of the fretting hand along the neck.
        constexpr std::array<float, 4> k_chordShifts{ 0.0f, 1.0f, -0.6f, 0.5f };
        constexpr float k_strokeSeconds{ 0.3125f };

        // The drum set stands this far to the android's left, from its pelvis to the batter head.
        constexpr float k_kitToAndroid{ 92.0f };
        // In the kit's units, about where the batter head meets the floor.
        constexpr FloatPoint k_bassDrumMiddle{ -35.0f, -52.0f };
        constexpr FloatPoint k_beaterAxle{ 8.0f, -34.0f };
        constexpr FloatPoint k_footboardHeel{ 62.0f, -3.0f }; // the hinge the footboard turns on
        constexpr FloatPoint k_cymbal{ -54.0f, -236.0f };
        constexpr float k_beaterLength{ 44.0f };
        // A kick every 0.625 seconds, every 0.3125 in the fill, the last with the crash.
        constexpr float k_kickSeconds{ 0.625f };
        constexpr float k_fillFrom{ 7.5f };
        constexpr float k_crashAt{ 8.4375f };
        // A drumstick's length from its tail to its tip, and where a hand grips it to play.
        constexpr float k_stickLength{ 66.0f };
        constexpr float k_stickGrip{ 10.0f };
        // Behind the left ear, in the head's units: a short end out above the ear, the rest hidden.
        constexpr FloatPoint k_stickTailBehindEar{ -44.8f, -46.2f };
        constexpr float k_stickBehindEarAngle{ 0.75f };
        constexpr float k_stickTakenAt{ 4.0f }; // from the tail, where the hand takes it
        // The stick's angles: raised over the cymbal, striking down onto it, up for the ta-da.
        constexpr float k_stickRaisedAngle{ k_pi * 1.33f };
        constexpr float k_stickStrikeAngle{ k_pi * 0.76f };
        constexpr float k_stickTadaAngle{ k_pi * 1.4f };

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

        // A tom in units about the middle of its head.
        void drawTom(const SceneFrame& drum)
        {
            fillBox(drum, { -42.0f, 0.0f, 42.0f, 30.0f }, SolidColor{ DrumColors::Shell });
            fillBox(drum, { -42.0f, 0.0f, 42.0f, 30.0f }, LinearGradient::Simple({ -42.0f, 0.0f },
                { 42.0f, 0.0f }, DrumColors::ShellLight, DrumColors::ShellShade));
            PixelPath lacing;
            for (int i = 0; i != 8; ++i)
            {
                const float x = -38.0f + static_cast<float>(i) * 10.8f;
                lacing.moveTo(x, 4.0f);
                lacing.lineTo(x + 5.4f, 26.0f);
            }
            strokeShape(drum, lacing, SolidColor{ DrumColors::Lacing }, 1.1f);
            const SolidColor hoop{ DrumColors::Hoop };
            fillBox(drum, { -43.0f, -1.0f, 43.0f, 4.0f }, hoop);
            fillBox(drum, { -43.0f, 27.0f, 43.0f, 32.0f }, hoop);
            fillOval(drum, { 0.0f, 31.0f }, 43.0f, 6.0f, SolidColor{ DrumColors::Underside });
            fillOval(drum, { 0.0f, 0.0f }, 43.0f, 8.5f, hoop);
            fillOval(drum, { 0.0f, -0.6f }, 40.0f, 7.0f, SolidColor{ DrumColors::Skin });
        }

        [[nodiscard]] FloatPoint directionOf(const float angle)
        {
            return { std::cos(angle), std::sin(angle) };
        }

        void drawStick(const SceneFrame& upper, const FloatPoint tail, const float angle)
        {
            const FloatPoint tip = tail + directionOf(angle) * k_stickLength;
            drawSegment(upper, tail, tip, SolidColor{ DrumColors::Stick }, 3.6f);
            fillDisc(upper, tip, 3.0f, SolidColor{ DrumColors::StickTip });
        }

        // The bass drum lying on its side, its batter head turned a little to the audience.
        void drawBassDrum(const SceneFrame& kit, const float kicked)
        {
            const FloatPoint middle = k_bassDrumMiddle;
            const SolidColor metal{ DrumColors::Metal };
            drawSegment(kit, { -60.0f, -24.0f }, { -84.0f, 0.0f }, metal, 3.0f);
            const FloatRect shell = {
                -70.0f,
                middle.y - 48.0f,
                0.0f,
                middle.y + 48.0f
            };
            fillBox(kit, shell, SolidColor{ DrumColors::Shell });
            fillBox(kit, shell, LinearGradient::Simple({ 0.0f, shell.top }, { 0.0f, shell.bottom },
                DrumColors::ShellLight, DrumColors::ShellShade));
            PixelPath rods;
            for (const float angle : { -1.05f, -0.52f, 0.0f, 0.52f, 1.05f })
            {
                const float y = middle.y + 46.0f * std::sin(angle);
                rods.moveTo(-66.0f, y);
                rods.lineTo(-4.0f, y);
            }
            strokeShape(kit, rods, SolidColor{ DrumColors::Lacing }, 1.1f);
            const SolidColor hoop{ DrumColors::Hoop };
            fillRoundedBox(kit, { -73.0f, shell.top - 2.0f, -66.0f, shell.bottom + 2.0f }, 3.0f,
                hoop);
            fillOval(kit, { 0.0f, middle.y }, 11.0f, 50.0f, hoop);
            fillOval(kit, { 0.0f, middle.y }, 8.0f, 47.0f,
                SolidColor{ colorBetween(DrumColors::Skin, DrumColors::SkinStruck, kicked) });
        }

        // The footboard's toe end comes down with the press, about its heel.
        [[nodiscard]] FloatPoint footboardToeAt(const float press)
        {
            return { 12.0f, std::lerp(-17.0f, -7.0f, press) };
        }

        // The pedal at the batter head: a footboard hinged at its heel, a chain from its toe up
        // to the axle, the beater on the axle.
        void drawPedal(const SceneFrame& kit, const float press)
        {
            const FloatPoint boardHeel = k_footboardHeel;
            const FloatPoint boardToe = footboardToeAt(press);
            const SolidColor metal{ DrumColors::Metal };
            const SolidColor dark{ DrumColors::MetalDark };
            fillBox(kit, { 2.0f, -3.0f, 16.0f, 0.0f }, dark);
            drawSegment(kit, { 8.0f, 0.0f }, k_beaterAxle, metal, 3.0f, StrokeCap::Butt);
            const float beaterAngle = std::lerp(0.5f, -0.1f, press);
            const FloatPoint ball = k_beaterAxle
                + FloatPoint{ std::sin(beaterAngle), -std::cos(beaterAngle) } * k_beaterLength;
            drawSegment(kit, k_beaterAxle, ball, metal, 3.0f);
            fillOval(kit, ball, 6.5f, 7.5f, SolidColor{ DrumColors::Felt });
            fillDisc(kit, k_beaterAxle, 3.2f, dark);
            drawSegment(kit, boardToe, k_beaterAxle + FloatPoint{ 2.0f, 2.0f }, dark, 1.6f);
            fillBox(kit, { boardHeel.x - 6.0f, -3.0f, boardHeel.x + 4.0f, 0.0f }, dark);
            drawSegment(kit, boardHeel, boardToe, metal, 5.0f);
        }

        // The footboard pressed, 0 to 1, and the bass drum's head struck, 1 at each kick.
        struct Pedal
        {
            float press;
            float kicked;
        };

        [[nodiscard]] Pedal pedalAt(const float actTime)
        {
            Pedal pedal = { 0.0f, 0.0f };
            const auto kick = [&](const float at) {
                const float since = actTime - at;
                const float press = since < 0.0f
                    ? smoothStep(-0.07f, 0.0f, since)
                    : 1.0f - smoothStep(0.0f, 0.22f, since);
                pedal.press = std::max(pedal.press, press);
                if (since >= 0.0f && since < 0.25f)
                    pedal.kicked = std::max(pedal.kicked, 1.0f - since / 0.25f);
            };
            for (int beat = 0; beat != 12; ++beat)
                kick(static_cast<float>(beat) * k_kickSeconds);
            for (int beat = 0; beat != 4; ++beat)
                kick(k_fillFrom + static_cast<float>(beat) * k_kickSeconds * 0.5f);
            return pedal;
        }
    }

    // Seconds: four chords a bar of 1.25, a kick every 0.625; the neck raised from 6.6; a fill on
    // the bass drum from 7.5; from 7.8 the strumming hand reaches for the stick behind the ear,
    // takes it at 8, raises it over the cymbal by 8.3, and strikes it at 8.44 with the last kick;
    // the ta-da from 8.6.
    void OneManBandAct::pose(const ActStage& stage, const float actTime, const SceneTime time)
    {
        const SceneFrame& frame = stage.frame;
        const float unit = frame.unit;
        const float x = stage.centerX + 50.0f * unit * stage.closeness;
        const float dip = 6.0f * std::pow(std::cos(k_pi * actTime / 0.625f), 2.0f);
        m_android = standingAndroid(frame, { x, stage.floorY }, time.cycleTime, dip, 26.0f);
        m_kitFloor = { x - k_kitToAndroid * unit, stage.floorY };
        const auto kitToFrame = [&](const FloatPoint kitPoint) {
            return m_kitFloor + kitPoint * unit;
        };

        // the left foot on the footboard, its sole along the board
        const Pedal pedal = pedalAt(actTime);
        const FloatPoint toe = footboardToeAt(pedal.press);
        const FloatPoint board = toe - k_footboardHeel;
        const FloatPoint along = board * (1.0f / std::hypot(board.x, board.y));
        const FloatPoint ankle = k_footboardHeel + along * 14.0f
            + FloatPoint{ -along.y, along.x } * AndroidBody::AnkleHeight;
        plantAndroidFeet(frame, m_android, kitToFrame(ankle),
            { x + 26.0f * unit, stage.floorY - AndroidBody::AnkleHeight * unit });
        m_android.left.footTurn = std::atan2(-along.y, -along.x);
        m_pedalPress = pedal.press;
        m_kicked = pedal.kicked;

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

        const float tada = smoothStep(8.6f, 9.0f, actTime);
        const float toCymbal = smoothStep(8.05f, 8.3f, actTime) * (1.0f - tada);
        m_android.headTurn = std::lerp(0.1f * raised + 0.06f * std::sin(k_2Pi * actTime / 1.25f),
            -0.12f, toCymbal);
        m_android.look = std::lerp(4.0f, -6.0f, toCymbal);

        // the strumming hand takes the stick by its end from behind the ear, draws it out along
        // itself, raises it over the cymbal as its grip slides up, strikes, and holds it up for
        // the ta-da; the stick stays behind the head until it points away from it
        const float toEar = smoothStep(7.8f, 8.0f, actTime);
        const float pulled = smoothStep(8.0f, 8.14f, actTime);
        const float raise = smoothStep(8.1f, 8.3f, actTime);
        const float strike = smoothStep(8.32f, k_crashAt, actTime);
        const FloatPoint earTail = androidHeadToUpper(m_android, k_stickTailBehindEar);
        const float earAngle = k_stickBehindEarAngle + m_android.headTurn;
        const FloatPoint takenAt = earTail + directionOf(earAngle) * k_stickTakenAt;
        const FloatPoint struckPoint = androidFrameToUpper(frame, m_android,
            kitToFrame(k_cymbal + FloatPoint{ 20.0f, -4.0f }));
        const FloatPoint strikeGrip = struckPoint
            - directionOf(k_stickStrikeAngle) * (k_stickLength - k_stickGrip);
        FloatPoint hand = pointBetween(onGuitar({ 28.0f, 15.0f * stroke }), takenAt, toEar);
        hand = pointBetween(hand, takenAt - directionOf(earAngle) * 44.0f, pulled);
        hand = pointBetween(hand, strikeGrip + FloatPoint{ 4.0f, -20.0f }, raise);
        hand = pointBetween(hand, strikeGrip, strike);
        hand = pointBetween(hand, ActHands::LeftRaised, tada);
        float stickAngle = std::lerp(earAngle, k_stickRaisedAngle, raise);
        stickAngle = std::lerp(stickAngle, k_stickStrikeAngle, strike);
        stickAngle = std::lerp(stickAngle, k_stickTadaAngle, tada);
        const float gripAlong = std::lerp(k_stickTakenAt, k_stickGrip, raise);
        const bool taken = actTime >= 8.0f;
        m_stickAngle = taken ? stickAngle : earAngle;
        m_stickTail = taken ? hand - directionOf(stickAngle) * gripAlong : earTail;
        m_stickInFront = raise > 0.7f;
        m_android.left.hand = hand;
        m_android.left.handShape = actTime >= 7.95f ? HandShape::Fist : HandShape::Open;
        m_android.left.forearmOnTop = true;
        m_android.right.hand = onGuitar({ fret, 10.0f });
        m_android.right.elbow = ElbowBend::Down;
        m_android.right.handShape = HandShape::Fist;
        m_android.right.forearmOnTop = true;
        if (actTime > 9.0f)
            m_android.lights = flashingLights(actTime);
        m_ringing = (1.0f - fractionalPart(actTime / k_strokeSeconds)) * (1.0f - toEar);
        m_crashAge = actTime > k_crashAt ? actTime - k_crashAt : -1.0f;
        m_spotlightX = x - 40.0f * unit;
        m_actTime = actTime;
    }

    // The drum set stands behind the android's leg, the cymbal's stand behind the bass drum.
    void OneManBandAct::draw(const SceneFrame& frame) const
    {
        const float unit = frame.unit;
        const SceneFrame kit = frame.within(Matrix3x2::Translation(m_kitFloor)
            * Matrix3x2::Scale(unit));
        const SolidColor metal{ DrumColors::Metal };

        // the cymbal on its stand, shaking after the crash
        drawSegment(kit, { -96.0f, 0.0f }, k_cymbal, metal, 3.0f, StrokeCap::Butt);
        drawSegment(kit, { -112.0f, 0.0f }, { -80.0f, 0.0f }, SolidColor{ DrumColors::MetalDark },
            3.0f);
        const float shaken = std::clamp(m_crashAge / 1.4f, 0.0f, 1.0f);
        const float wobble = m_crashAge >= 0.0f
            ? 0.35f * std::exp(-shaken * 3.0f) * std::sin(k_2Pi * shaken * 4.0f)
            : 0.0f;
        const SceneFrame plate = kit.within(Matrix3x2::Translation(k_cymbal)
            * Matrix3x2::Rotation(0.12f + wobble));
        fillOval(plate, { 0.0f, 0.0f }, 34.0f, 5.5f, SolidColor{ DrumColors::Cymbal });
        fillOval(plate, { 0.0f, -1.0f }, 30.0f, 3.4f, SolidColor{ DrumColors::CymbalShine });
        fillDisc(plate, { 0.0f, -2.0f }, 3.0f, SolidColor{ DrumColors::CymbalBell });

        drawBassDrum(kit, m_kicked);
        drawSegment(kit, { -34.0f, -98.0f }, { -34.0f, -114.0f }, metal, 3.0f, StrokeCap::Butt);
        drawTom(kit.within(Matrix3x2::Translation(-32.0f, -126.0f) * Matrix3x2::Rotation(0.2f)
            * Matrix3x2::Scale(0.6f)));
        drawPedal(kit, m_pedalPress);

        const SceneFrame upper = androidUpperFrame(frame, m_android);
        if (!m_stickInFront)
            drawStick(upper, m_stickTail, m_stickAngle);
        drawAndroid(frame, m_android);
        drawGuitar(upper.within(Matrix3x2::Translation(k_bodyMiddle)
            * Matrix3x2::Rotation(m_neckAngle)), m_ringing);
        if (m_stickInFront)
            drawStick(upper, m_stickTail, m_stickAngle);
        drawAndroidForearm(frame, m_android, AudienceSide::Left);
        drawAndroidForearm(frame, m_android, AudienceSide::Right);

        // a thump off the bass drum's top with every kick, and sparkles off the crash
        if (m_kicked > 0.2f)
        {
            PixelPath lines;
            for (const float across : { -50.0f, -22.0f })
            {
                lines.moveTo(across, -108.0f);
                lines.lineTo(across + 4.0f, -120.0f);
            }
            strokeShape(kit, lines, SolidColor{ DrumColors::Ring.faded(m_kicked) }, 1.6f);
        }
        if (m_crashAge >= 0.0f)
            drawSparkleBurst(frame, SparkleBurst{
                .center = m_kitFloor + k_cymbal * unit,
                .age = shaken,
                .count = 9,
                .seed = 81.0f,
                .size = 0.9f,
                .colors = { MagicColors::Gold, MagicColors::White, MagicColors::Gold }
            });
        drawMusicNotes(frame, MusicNotes{
            .origin = androidUpperToFrame(frame, m_android, { 70.0f, -120.0f }),
            .spread = 160.0f,
            .time = m_actTime,
            .seed = 7.0f
        });
    }
}
