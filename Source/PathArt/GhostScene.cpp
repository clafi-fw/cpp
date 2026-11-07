module ClaFi.PathArt.GhostScene;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace GhostColors
    {
        constexpr Color SkyTop{ 0xFF090E24 };
        constexpr Color SkyBottom{ 0xFF24365F };
        constexpr Color Star{ 0xFFE8EEFF };
        constexpr Color MoonHalo{ 0xFFCFD9FF };
        constexpr Color MoonGlow{ 0xFFEEF2FF };
        constexpr Color MoonLight{ 0xFFFFFCEA };
        constexpr Color MoonShade{ 0xFFDED8B4 };
        constexpr Color Crater{ 0xFFCFC89F };
        constexpr Color CloudTop{ 0xC756699F };
        constexpr Color CloudBottom{ 0xD922325A };
        constexpr Color Silhouette{ 0xFF06070D };
        constexpr Color FarHill{ 0xFF18233F };
        constexpr Color Tree{ 0xFF090C16 };
        constexpr Color Fence{ 0xFF0B0F1C };
        constexpr Color GroundTop{ 0xFF1F2D4B };
        constexpr Color GroundBottom{ 0xFF0A1121 };
        constexpr Color GroundGlow{ 0xFFA8C8FF };
        constexpr Color StoneShade{ 0xFF3B4459 };
        constexpr Color StoneLight{ 0xFF6C7894 };
        constexpr Color Engraving{ 0xCC262D3D };
        constexpr Color PumpkinGlow{ 0xFFFF9F43 };
        constexpr Color PumpkinShade{ 0xFFC9561A };
        constexpr Color Pumpkin{ 0xFFE8762B };
        constexpr Color Stem{ 0xFF4E6B2A };
        constexpr Color Candle{ 0xFFFFD27A };
        constexpr Color GhostTop{ 0xF0FFFFFF };
        constexpr Color GhostBottom{ 0x80CFE0FF };
        constexpr Color GhostHalo{ 0xD9BFE3FF };
        constexpr Color Blush{ 0x80FFB3C7 };
        constexpr Color Face{ 0xFF1A1F33 };
        constexpr Color White{ 0xFFFFFFFF };
        constexpr Color Fog{ 0xFF9FB4E8 };
    }

    namespace
    {
        // The tombstones along the near ground: where, how big, which shape and how far they lean.
        enum class StoneShape
        {
            Rounded,
            Cross,
            Slab
        };

        struct Tombstone
        {
            float share;
            float width;
            float height;
            StoneShape shape;
            float lean;
        };

        constexpr std::array<Tombstone, 5> k_tombstones{
            Tombstone{ 0.22f, 30.0f, 50.0f, StoneShape::Rounded, -0.08f },
            Tombstone{ 0.36f, 26.0f, 58.0f, StoneShape::Cross, 0.04f },
            Tombstone{ 0.55f, 36.0f, 46.0f, StoneShape::Rounded, 0.07f },
            Tombstone{ 0.71f, 42.0f, 32.0f, StoneShape::Slab, -0.03f },
            Tombstone{ 0.87f, 28.0f, 44.0f, StoneShape::Rounded, -0.13f }
        };
    }

    void GhostScene::draw(Canvas& canvas, const FloatRect& bounds, const float cycleTime)
    {
        const SceneFrame frame = SceneFrame::FromBounds(canvas, bounds);
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;

        drawSky(frame, cycleTime);

        // the far hill, the bare tree and the fence
        PixelPath farHill;
        farHill.moveTo(0.0f, height * 0.69f);
        farHill.quadTo({ width * 0.3f, height * 0.6f }, { width * 0.62f, height * 0.67f });
        farHill.quadTo({ width * 0.85f, height * 0.72f }, { width, height * 0.66f });
        farHill.lineTo(width, height);
        farHill.lineTo(0.0f, height);
        farHill.close();
        fillShape(frame, farHill, SolidColor{ GhostColors::FarHill });
        drawBareTree(frame, { width * 0.13f, height * 0.74f });
        drawFence(frame);

        PixelPath ground;
        ground.moveTo(0.0f, height * 0.78f);
        ground.quadTo({ width * 0.5f, height * 0.70f }, { width, height * 0.80f });
        ground.lineTo(width, height);
        ground.lineTo(0.0f, height);
        ground.close();
        fillShape(frame, ground, LinearGradient::Simple({ 0.0f, height * 0.7f }, { 0.0f, height },
            GhostColors::GroundTop, GhostColors::GroundBottom));

        // Two figure eights a cycle, with a small bob on top.
        const float bob = 5.0f * unit * std::sin(k_2Pi * 16.0f * cycleTime);
        const FloatPoint ghost = {
            width * 0.5f + width * 0.24f * std::sin(k_2Pi * 2.0f * cycleTime),
            height * 0.45f + 42.0f * unit * std::sin(k_2Pi * 4.0f * cycleTime) + bob
        };
        const float groundUnder = groundY(frame, ghost.x);
        const float closeness = 1.0f - (groundUnder - ghost.y) / (height * 0.6f);
        drawGlow(frame, { ghost.x, groundUnder + 10.0f * unit }, 60.0f * unit,
            GhostColors::GroundGlow.withOpacity(std::clamp(0.22f * closeness, 0.0f, 1.0f)), 0.35f);

        drawTombstones(frame);
        drawPumpkin(frame, cycleTime);
        drawGhost(frame, ghost, 1.05f * unit, cycleTime);

        // two fog banks drifting in front
        for (int bank = 0; bank != 2; ++bank)
        {
            for (int i = 0; i != 3; ++i)
            {
                const float start = static_cast<float>(i) / 3.0f + static_cast<float>(bank) * 0.17f;
                const float laps = static_cast<float>(1 + bank);
                const float drift = fractionalPart(cycleTime * laps + start);
                const float x = drift * (width + 600.0f * unit) - 300.0f * unit;
                const float y = height * (0.83f + 0.08f * static_cast<float>(bank));
                drawGlow(frame, { x, y }, 70.0f * unit, GhostColors::Fog.withOpacity(0.16f), 0.22f);
            }
        }
    }

    float GhostScene::groundY(const SceneFrame& frame, const float x)
    {
        const float share = std::clamp(x / frame.width, 0.0f, 1.0f);
        const float rest = 1.0f - share;
        // The ground line draw fills under: a quadratic Bezier from 0.78 to 0.80 of the height.
        const float pull = 2.0f * share * rest;
        return frame.height * (rest * rest * 0.78f + pull * 0.70f + share * share * 0.80f);
    }

    void GhostScene::drawSky(const SceneFrame& frame, const float cycleTime)
    {
        const float unit = frame.unit;
        const float width = frame.width;
        const float height = frame.height;
        fillBox(frame, { 0.0f, 0.0f, width, height },
            LinearGradient::Simple({ 0.0f, 0.0f }, { 0.0f, height * 0.8f }, GhostColors::SkyTop,
                GhostColors::SkyBottom));
        drawStars(frame, cycleTime, StarField{
            .count = 70,
            .top = 0.0f,
            .bottom = height * 0.55f,
            .seed = 30.0f,
            .color = GhostColors::Star
        });

        const FloatPoint moon = { width * 0.74f, height * 0.22f };
        const float moonRadius = 46.0f * unit;
        drawGlow(frame, moon, 190.0f * unit, GhostColors::MoonHalo.withOpacity(0.28f));
        drawGlow(frame, moon, 80.0f * unit, GhostColors::MoonGlow.withOpacity(0.35f));
        const FloatPoint moonCorner = { moonRadius, moonRadius };
        fillDisc(frame, moon, moonRadius,
            LinearGradient::Simple(moon - moonCorner, moon + moonCorner,
            GhostColors::MoonLight, GhostColors::MoonShade));
        fillOval(frame, moon + FloatPoint{ -14.0f * unit, -10.0f * unit }, 9.0f * unit, 7.0f * unit,
            SolidColor{ GhostColors::Crater.withOpacity(0.55f) });
        fillOval(frame, moon + FloatPoint{ 16.0f * unit, 12.0f * unit }, 12.0f * unit, 9.0f * unit,
            SolidColor{ GhostColors::Crater.withOpacity(0.45f) });
        fillOval(frame, moon + FloatPoint{ 4.0f * unit, -22.0f * unit }, 5.0f * unit, 4.0f * unit,
            SolidColor{ GhostColors::Crater.withOpacity(0.5f) });

        // clouds crossing the moon
        for (int i = 0; i != 3; ++i)
        {
            const float span = width + 520.0f * unit;
            const float laps = static_cast<float>(1 + i % 2);
            const float x = fractionalPart(pseudoRandom(i, 21.0f) + cycleTime * laps) * span
                - 260.0f * unit;
            const float y = height * (0.13f + 0.09f * static_cast<float>(i));
            const float halfWidth = (150.0f + 60.0f * pseudoRandom(i, 22.0f)) * unit;
            PixelPath cloud;
            cloud.moveTo(x - halfWidth, y);
            cloud.quadTo({ x - halfWidth * 0.85f, y - 26.0f * unit },
                { x - halfWidth * 0.55f, y - 20.0f * unit });
            cloud.quadTo({ x - halfWidth * 0.4f, y - 48.0f * unit },
                { x - halfWidth * 0.1f, y - 30.0f * unit });
            cloud.quadTo({ x + halfWidth * 0.15f, y - 56.0f * unit },
                { x + halfWidth * 0.4f, y - 26.0f * unit });
            cloud.quadTo({ x + halfWidth * 0.75f, y - 36.0f * unit }, { x + halfWidth, y });
            cloud.quadTo({ x, y + 12.0f * unit }, { x - halfWidth, y });
            cloud.close();
            fillShape(frame, cloud, LinearGradient::Simple({ 0.0f, y - 50.0f * unit },
                { 0.0f, y + 8.0f * unit }, GhostColors::CloudTop, GhostColors::CloudBottom));
        }

        // three bats across the moon
        for (int i = 0; i != 3; ++i)
        {
            const float start = static_cast<float>(i) / 3.0f + 0.1f;
            const float fly = fractionalPart(cycleTime * 2.0f + start);
            const float dipTurn = 6.0f * cycleTime + static_cast<float>(i) * 0.3f;
            const float dip = 18.0f * unit * std::sin(k_2Pi * dipTurn);
            const FloatPoint bat = {
                fly * (width + 240.0f * unit) - 120.0f * unit,
                height * (0.12f + 0.07f * static_cast<float>(i)) + dip
            };
            const float flapTurn = 64.0f * cycleTime + static_cast<float>(i) * 0.37f;
            const float flap = std::sin(k_2Pi * flapTurn);
            drawBat(frame, bat, (0.9f + 0.3f * static_cast<float>(i)) * unit, flap);
        }
    }

    void GhostScene::drawBat(const SceneFrame& frame, const FloatPoint center, const float size,
        const float flap)
    {
        const SceneFrame bat = frame.within(Matrix3x2::Translation(center)
            * Matrix3x2::Scale(size));
        const SolidColor black{ GhostColors::Silhouette };
        for (const float side : { -1.0f, 1.0f })
        {
            PixelPath wing;
            wing.moveTo(side * 2.0f, -1.0f);
            wing.lineTo(side * 13.0f, -4.0f - 9.0f * flap);
            wing.lineTo(side * 22.0f, -2.0f - 7.0f * flap);
            wing.quadTo({ side * 18.0f, 3.0f }, { side * 14.0f, 2.0f });
            wing.quadTo({ side * 11.0f, 6.0f }, { side * 8.0f, 3.0f });
            wing.quadTo({ side * 5.0f, 6.0f }, { side * 2.0f, 3.0f });
            wing.close();
            fillShape(bat, wing, black);
        }
        fillOval(bat, { 0.0f, 1.0f }, 3.6f, 5.2f, black);
        PixelPath ears;
        ears.moveTo(-3.0f, -3.0f);
        ears.lineTo(-2.2f, -7.0f);
        ears.lineTo(-0.8f, -3.6f);
        ears.close();
        ears.moveTo(3.0f, -3.0f);
        ears.lineTo(2.2f, -7.0f);
        ears.lineTo(0.8f, -3.6f);
        ears.close();
        fillShape(bat, ears, black);
    }

    // Branches split twice at each fork, six forks deep; each depth is one stroke of one width.
    void GhostScene::drawBareTree(const SceneFrame& frame, const FloatPoint root)
    {
        constexpr int k_depths = 7;
        std::array<PixelPath, k_depths> depths{};
        int seed = 0;
        using Branch = std::function<void(FloatPoint from, float length, float angle, int depth)>;
        const Branch branch = [&](FloatPoint from, float length, float angle, int depth) {
            const FloatPoint to = from + FloatPoint{ std::cos(angle), std::sin(angle) } * length;
            const FloatPoint bow = { std::sin(angle) * length * 0.12f, 0.0f };
            PixelPath& path = depths[static_cast<std::size_t>(depth)];
            path.moveTo(from);
            path.quadTo(pointBetween(from, to, 0.5f) + bow, to);
            if (depth == k_depths - 1)
                return;
            ++seed;
            const float jitter = pseudoRandom(seed, 4.2f) - 0.5f;
            branch(to, length * 0.72f, angle - 0.46f + jitter * 0.3f, depth + 1);
            branch(to, length * (0.62f + jitter * 0.2f), angle + 0.38f + jitter * 0.25f, depth + 1);
        };
        branch(root, 70.0f * frame.unit, -std::numbers::pi_v<float> * 0.5f + 0.08f, 0);
        float width = 13.0f * frame.unit;
        for (const PixelPath& path : depths)
        {
            strokeShape(frame, path, SolidColor{ GhostColors::Tree }, width);
            width *= 0.62f;
        }
    }

    void GhostScene::drawFence(const SceneFrame& frame)
    {
        const float unit = frame.unit;
        const float from = frame.width * 0.44f;
        const float to = frame.width * 0.98f;
        const float spacing = 15.0f * unit;
        PixelPath bars;
        PixelPath rails;
        for (const float railHeight : { 12.0f, 32.0f })
        {
            rails.moveTo(from, groundY(frame, from) - (4.0f + railHeight) * unit);
            for (float x = from + spacing; x <= to; x += spacing)
                rails.lineTo(x, groundY(frame, x) - (4.0f + railHeight) * unit);
        }
        for (float x = from; x < to; x += spacing)
        {
            const float base = groundY(frame, x) - 4.0f * unit;
            bars.moveTo(x - 1.5f * unit, base);
            bars.lineTo(x - 1.5f * unit, base - 40.0f * unit);
            bars.lineTo(x - 4.0f * unit, base - 40.0f * unit);
            bars.lineTo(x, base - 48.0f * unit);
            bars.lineTo(x + 4.0f * unit, base - 40.0f * unit);
            bars.lineTo(x + 1.5f * unit, base - 40.0f * unit);
            bars.lineTo(x + 1.5f * unit, base);
            bars.close();
        }
        fillShape(frame, bars, SolidColor{ GhostColors::Fence });
        strokeShape(frame, rails, SolidColor{ GhostColors::Fence }, 2.5f * unit);
    }

    void GhostScene::drawTombstones(const SceneFrame& frame)
    {
        const float unit = frame.unit;
        for (const Tombstone& stone : k_tombstones)
        {
            const float x = frame.width * stone.share;
            const float sunk = groundY(frame, x) + 8.0f * unit;
            const SceneFrame placed = frame.within(Matrix3x2::Translation(x, sunk)
                * Matrix3x2::Rotation(stone.lean));
            const float halfWidth = stone.width * unit * 0.5f;
            const float height = stone.height * unit;
            PixelPath shape;
            switch (stone.shape)
            {
                case StoneShape::Rounded:
                    shape.moveTo(-halfWidth, 0.0f);
                    shape.lineTo(-halfWidth, -height + halfWidth);
                    shape.arcTo({ halfWidth, halfWidth }, 0.0f, ArcSize::Small, ArcSweep::Clockwise,
                        { halfWidth, -height + halfWidth });
                    shape.lineTo(halfWidth, 0.0f);
                    shape.close();
                    break;
                case StoneShape::Cross:
                {
                    const float post = 5.0f * unit;
                    const float arm = 15.0f * unit;
                    const float armTop = -height * 0.78f;
                    const float armBottom = armTop + 9.0f * unit;
                    shape.moveTo(-post, 0.0f);
                    shape.lineTo(-post, armBottom);
                    shape.lineTo(-arm, armBottom);
                    shape.lineTo(-arm, armTop);
                    shape.lineTo(-post, armTop);
                    shape.lineTo(-post, -height);
                    shape.lineTo(post, -height);
                    shape.lineTo(post, armTop);
                    shape.lineTo(arm, armTop);
                    shape.lineTo(arm, armBottom);
                    shape.lineTo(post, armBottom);
                    shape.lineTo(post, 0.0f);
                    shape.close();
                    break;
                }
                case StoneShape::Slab:
                    shape.drawRoundedRect(FloatRect{ -halfWidth, -height, halfWidth, 0.0f },
                        4.0f * unit, 4.0f * unit);
                    break;
            }
            fillShape(placed, shape,
                LinearGradient::Simple({ -halfWidth, 0.0f }, { halfWidth, 0.0f },
                GhostColors::StoneShade, GhostColors::StoneLight));
            if (stone.shape != StoneShape::Cross)
            {
                PixelPath engraving;
                engraving.moveTo(-halfWidth * 0.5f, -height * 0.55f);
                engraving.lineTo(halfWidth * 0.5f, -height * 0.55f);
                engraving.moveTo(-halfWidth * 0.4f, -height * 0.42f);
                engraving.lineTo(halfWidth * 0.4f, -height * 0.42f);
                strokeShape(placed, engraving, SolidColor{ GhostColors::Engraving }, 2.0f * unit);
            }
        }
    }

    void GhostScene::drawPumpkin(const SceneFrame& frame, const float cycleTime)
    {
        const float unit = frame.unit;
        const float x = frame.width * 0.63f;
        const float y = groundY(frame, x) + 4.0f * unit;
        // Two waves beating against each other and a fast third make the candle flicker.
        const float beat = std::sin(k_2Pi * 37.0f * cycleTime)
            * std::sin(k_2Pi * 53.0f * cycleTime + 1.0f);
        const float flicker = 0.75f + 0.15f * beat + 0.1f * std::sin(k_2Pi * 91.0f * cycleTime);
        drawGlow(frame, { x, y - 12.0f * unit }, 60.0f * unit,
            GhostColors::PumpkinGlow.withOpacity(0.32f * flicker));
        fillOval(frame, { x - 9.0f * unit, y - 12.0f * unit }, 11.0f * unit, 12.0f * unit,
            SolidColor{ GhostColors::PumpkinShade });
        fillOval(frame, { x + 9.0f * unit, y - 12.0f * unit }, 11.0f * unit, 12.0f * unit,
            SolidColor{ GhostColors::PumpkinShade });
        fillOval(frame, { x, y - 13.0f * unit }, 12.0f * unit, 13.0f * unit,
            SolidColor{ GhostColors::Pumpkin });
        drawSegment(frame, { x, y - 25.0f * unit }, { x + 3.0f * unit, y - 31.0f * unit },
            SolidColor{ GhostColors::Stem }, 3.0f * unit);

        // the carved face: two eyes and a jagged mouth
        PixelPath face;
        face.moveTo(x - 9.0f * unit, y - 16.0f * unit);
        face.lineTo(x - 3.0f * unit, y - 16.0f * unit);
        face.lineTo(x - 6.0f * unit, y - 21.0f * unit);
        face.close();
        face.moveTo(x + 3.0f * unit, y - 16.0f * unit);
        face.lineTo(x + 9.0f * unit, y - 16.0f * unit);
        face.lineTo(x + 6.0f * unit, y - 21.0f * unit);
        face.close();
        face.moveTo(x - 9.0f * unit, y - 9.0f * unit);
        for (int i = 0; i <= 6; ++i)
        {
            const float toothX = -9.0f + static_cast<float>(i) * 3.0f;
            const float toothY = i % 2 == 0 ? -9.0f : -6.0f;
            face.lineTo(x + toothX * unit, y + toothY * unit);
        }
        face.lineTo(x + 6.0f * unit, y - 5.0f * unit);
        face.lineTo(x - 6.0f * unit, y - 5.0f * unit);
        face.close();
        const float candle = std::clamp(flicker, 0.0f, 1.0f);
        fillShape(frame, face, SolidColor{ GhostColors::Candle.withOpacity(candle) });
    }

    // The sheet leans into its path and its hem trails, so the ghost reads as moving through air.
    void GhostScene::drawGhost(const SceneFrame& frame, const FloatPoint center, const float size,
        const float cycleTime)
    {
        const float lean = 0.16f * std::cos(k_2Pi * 2.0f * cycleTime);
        const float trail = -6.0f * std::cos(k_2Pi * 2.0f * cycleTime);
        const SceneFrame ghost = frame.within(Matrix3x2::Translation(center)
            * Matrix3x2::Rotation(lean) * Matrix3x2::Scale(size));
        const LinearGradient sheet = LinearGradient::Simple({ 0.0f, -60.0f }, { 0.0f, 72.0f },
            GhostColors::GhostTop, GhostColors::GhostBottom);
        const PathDrawLayer halo = PathDrawLayer::OuterGlow(GhostColors::GhostHalo, 22.0f);
        const PathDrawLayer cloth{ .geometry{}, .brush = sheet };

        for (const float side : { -1.0f, 1.0f })
        {
            const float wave = std::sin(k_2Pi * 12.0f * cycleTime + (side > 0.0f ? 1.4f : 0.0f));
            const Matrix3x2 shoulder = Matrix3x2::Translation(side * 36.0f, 4.0f)
                * Matrix3x2::Rotation(side * (0.45f + 0.35f * wave));
            PixelPath arm;
            arm.moveTo(0.0f, -7.0f);
            arm.quadTo({ side * 26.0f, -11.0f }, { side * 25.0f, 0.0f });
            arm.quadTo({ side * 23.0f, 9.0f }, { 0.0f, 8.0f });
            arm.close();
            const Matrix3x2 toCanvas = ghost.toCanvas * shoulder;
            ghost.canvas.drawPath(arm, { halo, cloth }, &toCanvas);
        }

        PixelPath body;
        body.moveTo(-40.0f, 0.0f);
        body.cubicTo({ -40.0f, -72.0f }, { 40.0f, -72.0f }, { 40.0f, 0.0f });
        body.quadTo({ 42.0f, 28.0f }, { 46.0f + trail, 50.0f });
        constexpr std::array<float, 5> k_hemPoints{ 46.0f, 23.0f, 0.0f, -23.0f, -46.0f };
        for (std::size_t i = 0; i != 4; ++i)
        {
            const float from = k_hemPoints[i] + trail;
            const float to = k_hemPoints[i + 1] + trail;
            const float ripple = std::sin(k_2Pi * 24.0f * cycleTime - static_cast<float>(i) * 1.3f);
            const float depth = 15.0f + 7.0f * ripple;
            body.quadTo({ (from + to) * 0.5f, 50.0f + depth * 2.0f }, { to, 50.0f });
        }
        body.quadTo({ -42.0f, 28.0f }, { -40.0f, 0.0f });
        body.close();
        ghost.canvas.drawPath(body, { halo, cloth }, &ghost.toCanvas);

        const float look = 5.0f * std::cos(k_2Pi * 2.0f * cycleTime);
        const float blink = fractionalPart(cycleTime * 6.0f + 0.3f) > 0.97f ? 0.12f : 1.0f;
        fillOval(ghost, { -24.0f + look, -8.0f }, 6.0f, 3.5f, SolidColor{ GhostColors::Blush });
        fillOval(ghost, { 24.0f + look, -8.0f }, 6.0f, 3.5f, SolidColor{ GhostColors::Blush });
        fillOval(ghost, { -13.0f + look, -24.0f }, 6.0f, 9.0f * blink,
            SolidColor{ GhostColors::Face });
        fillOval(ghost, { 13.0f + look, -24.0f }, 6.0f, 9.0f * blink,
            SolidColor{ GhostColors::Face });
        if (blink > 0.5f)
        {
            fillDisc(ghost, { -15.0f + look, -28.0f }, 2.0f, SolidColor{ GhostColors::White });
            fillDisc(ghost, { 11.0f + look, -28.0f }, 2.0f, SolidColor{ GhostColors::White });
        }
        // The mouth opens and closes eight times a cycle.
        const float open = 0.5f + 0.5f * std::sin(k_2Pi * 8.0f * cycleTime);
        fillOval(ghost, { look * 0.6f, -2.0f }, 5.0f + 2.0f * open, 4.0f + 7.0f * open,
            SolidColor{ GhostColors::Face });
    }
}
