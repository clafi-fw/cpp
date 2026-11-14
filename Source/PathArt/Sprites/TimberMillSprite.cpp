module ClaFi.PathArt.TimberMillSprite;

import ClaFi.PathArt.BucketWheelSprite;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        struct Segment
        {
            FloatPoint from;
            FloatPoint to;
        };

        constexpr Color k_black{ 0xFF000000 };
        constexpr FloatPoint k_wheelCenter{ -84.0f, -24.0f };
        constexpr float k_wheelRadius{ 46.0f };
        constexpr float k_hubHeight{ 126.0f };
        constexpr std::array<FloatPoint, 2> k_windowMiddles{ {
            { -29.0f, -75.0f },
            { 80.0f, -74.0f }
        } };
        // How many times a cycle the smoke's and the water's motions come round.
        constexpr float k_smokeRises{ 4.0f };
        constexpr float k_smokeSways{ 6.0f };
        constexpr float k_dropFalls{ 64.0f };
        constexpr float k_sprayLeaps{ 36.0f };
        constexpr float k_foamBobs{ 57.0f };
        constexpr float k_foamSwells{ 45.0f };

        // The frame in the mill's units, its origin at the middle of the front wall's foot.
        [[nodiscard]] SceneFrame millFrame(const SceneFrame&, const TimberMillPose&);
        [[nodiscard]] PixelPath polygon(std::initializer_list<FloatPoint> corners);
        [[nodiscard]] PixelPath segments(std::initializer_list<Segment> lines);
        [[nodiscard]] Color darker(Color, float share);
        // The walls and the plinth under them, the timber frame and the door.
        void drawHouseFront(const SceneFrame& house, const TimberMillColors&);
        // The glass is the lamp's colour as far as its opacity goes, and the lamp glows around.
        void drawHouseWindows(const SceneFrame& house, const TimberMillColors&);
        // The eaves' shadow, the roof's right plane, the chimney and the gable's barge boards.
        void drawRoof(const SceneFrame& house, const TimberMillColors&);
    }

    void drawTimberMillSmoke(const SceneFrame& frame, const TimberMillPose& pose,
        const TimberMillColors& colors)
    {
        constexpr int k_puffs{ 16 };
        const SceneFrame mill = millFrame(frame, pose);
        for (int i = 0; i != k_puffs; ++i)
        {
            const float index = static_cast<float>(i);
            const float rise = fractionalPart(pose.cycleTime * k_smokeRises
                + index / static_cast<float>(k_puffs));
            const float sway = std::sin(pose.cycleTime * k_smokeSways * k_2Pi + index * 2.0f);
            const FloatPoint center = {
                72.0f + rise * 80.0f + sway * 5.0f * rise,
                -184.0f - rise * 90.0f - std::sin(rise * 3.0f) * 6.0f
            };
            const float opacity = 0.32f * (1.0f - rise) * std::min(1.0f, rise * 10.0f);
            fillDisc(mill, center, 4.0f + rise * 20.0f, SolidColor{ faded(colors.smoke, opacity) });
        }
    }

    void drawTimberMillHouse(const SceneFrame& frame, const TimberMillPose& pose,
        const TimberMillColors& colors)
    {
        const SceneFrame house = millFrame(frame, pose);
        drawHouseFront(house, colors);
        drawHouseWindows(house, colors);
        drawRoof(house, colors);
    }

    void drawTimberMillWheel(const SceneFrame& frame, const TimberMillPose& pose,
        const TimberMillColors& colors)
    {
        drawBucketWheel(millFrame(frame, pose), BucketWheelPose{
            .center = k_wheelCenter,
            .size = 1.0f,
            .turns = pose.wheelTurns
        }, BucketWheelColors{
            .wood = colors.wheel,
            .spokes = colorBetween(colors.wheel, colors.sunlight, 0.1f),
            .hub = colors.barkShade,
            .axle = colors.timber
        });
    }

    void drawTimberMillWater(const SceneFrame& frame, const TimberMillPose& pose,
        const TimberMillColors& colors)
    {
        constexpr int k_drops{ 9 };
        constexpr int k_spray{ 14 };
        constexpr int k_foamHeaps{ 7 };
        const SceneFrame mill = millFrame(frame, pose);
        const float turn = pose.cycleTime * k_2Pi;

        fillShape(mill, polygon({ { -40.0f, -82.0f }, { -86.0f, -80.0f }, { -86.0f, -73.0f },
            { -40.0f, -75.0f } }), SolidColor{ colors.timber });
        fillShape(mill, polygon({ { -40.0f, -75.0f }, { -86.0f, -73.0f }, { -86.0f, -70.0f },
            { -40.0f, -72.0f } }), SolidColor{ darker(colors.timber, 0.35f) });
        fillBox(mill, { -66.0f, -72.0f, -62.0f, -42.0f }, SolidColor{ colors.timber });

        PixelPath pour;
        pour.moveTo(-86.0f, -78.0f);
        pour.quadTo({ -95.0f, -77.0f }, { -99.0f, -67.0f });
        pour.lineTo(-95.0f, -66.0f);
        pour.quadTo({ -92.0f, -73.0f }, { -86.0f, -73.0f });
        pour.close();
        fillShape(mill, pour, SolidColor{ faded(colors.water, 0.8f) });

        // Drops running down the wheel's left side, where the water pushes it round.
        for (int i = 0; i != k_drops; ++i)
        {
            const float index = static_cast<float>(i);
            const float fall = fractionalPart(pose.cycleTime * k_dropFalls
                + index / static_cast<float>(k_drops));
            const float angle = -2.0f - 1.6f * fall;
            const float reach = k_wheelRadius + 4.0f + 3.0f * std::sin(index * 3.1f);
            const FloatPoint top = {
                k_wheelCenter.x + std::cos(angle) * reach,
                k_wheelCenter.y + std::sin(angle) * reach
            };
            drawSegment(mill, top, { top.x - 1.0f, top.y + 7.0f },
                SolidColor{ faded(colors.foam, 0.81f * (1.0f - fall * 0.6f)) }, 1.6f);
        }
        for (int i = 0; i != k_spray; ++i)
        {
            const float leap = fractionalPart(pose.cycleTime * k_sprayLeaps
                + pseudoRandom(i, 61.0f));
            const float height = 6.0f + pseudoRandom(i, 63.0f) * 10.0f;
            const FloatPoint center = {
                k_wheelCenter.x - 40.0f + pseudoRandom(i, 62.0f) * 60.0f + leap * 6.0f,
                -2.0f - std::sin(leap * std::numbers::pi_v<float>) * height
            };
            fillDisc(mill, center, 2.0f + pseudoRandom(i, 64.0f) * 3.0f,
                SolidColor{ faded(colors.foam, 0.9f * (1.0f - leap)) });
        }

        // The river over the wheel's foot, which shows through it.
        frame.canvas.pushClip(mill.toCanvas.mapRect({ k_wheelCenter.x - 70.0f, 0.0f,
            k_wheelCenter.x + 70.0f, 40.0f }));
        fillDisc(mill, k_wheelCenter, k_wheelRadius + 8.0f,
            SolidColor{ faded(colors.river, 0.62f) });
        frame.canvas.popClip();
        PixelPath foam;
        for (int i = 0; i != k_foamHeaps; ++i)
        {
            const float index = static_cast<float>(i);
            const float x = k_wheelCenter.x - 48.0f + index * 15.0f
                + std::sin(turn * k_foamBobs + index) * 3.0f;
            const float crest = -6.0f - 2.0f * std::sin(turn * k_foamSwells + index * 2.0f);
            foam.moveTo(x - 9.0f, 1.0f);
            foam.quadTo({ x, crest }, { x + 9.0f, 1.0f });
            foam.close();
        }
        fillShape(mill, foam, SolidColor{ faded(colors.foam, 0.9f) });
    }

    void drawTimberMillWings(const SceneFrame& frame, const TimberMillPose& pose,
        const TimberMillColors& colors)
    {
        constexpr int k_wings{ 4 };
        constexpr float k_stockLength{ 124.0f };
        const SceneFrame hub = millFrame(frame, pose).within(
            Matrix3x2::Translation(0.0f, -k_hubHeight));
        const Color timberShade = darker(colors.timber, 0.35f);
        const Color fabricShade = colorBetween(colors.wingFabric, colors.barkShade, 0.12f);

        PixelPath cloth = polygon({ { 5.0f, -40.0f }, { 31.0f, -44.0f }, { 31.0f, -121.0f },
            { 5.0f, -117.0f } });
        PixelPath bars;
        for (float reach = 49.0f; reach <= 112.0f; reach += 9.0f)
        {
            bars.moveTo(5.0f, -reach);
            bars.lineTo(31.0f, -reach - 4.0f * (reach - 40.0f) / 76.0f);
        }
        const PixelPath bareBars = segments({
            { { 4.0f, -28.0f }, { 31.0f, -28.5f } },
            { { 4.0f, -34.0f }, { 31.0f, -34.5f } }
        });
        const PixelPath stock = polygon({ { -4.0f, 0.0f }, { 4.0f, 0.0f }, { 3.0f, -k_stockLength },
            { -3.0f, -k_stockLength } });

        for (int i = 0; i != k_wings; ++i)
        {
            const float turns = pose.wingTurns
                + static_cast<float>(i) / static_cast<float>(k_wings);
            const SceneFrame wing = hub.within(Matrix3x2::Rotation(turns * k_2Pi));
            fillShape(wing, cloth, LinearGradient::Simple({ 5.0f, 0.0f }, { 31.0f, 0.0f },
                faded(colors.wingFabric, 0.92f), faded(fabricShade, 0.92f)));
            strokeShape(wing, bars, SolidColor{ faded(timberShade, 0.28f) }, 1.1f,
                StrokeCap::Butt);
            strokeShape(wing, bareBars, SolidColor{ faded(colors.timber, 0.9f) }, 1.6f,
                StrokeCap::Butt);
            drawSegment(wing, { 31.0f, -28.0f }, { 31.0f, -122.0f }, SolidColor{ colors.timber },
                2.6f, StrokeCap::Butt);
            fillShape(wing, stock, SolidColor{ colors.timber });
            fillBox(wing, { -4.0f, -k_stockLength, -1.5f, 0.0f }, SolidColor{ timberShade });
        }
        fillDisc(hub, {}, 10.0f, SolidColor{ colors.timber });
        fillDisc(hub, {}, 4.5f, SolidColor{ timberShade });
    }

    namespace
    {
        SceneFrame millFrame(const SceneFrame& frame, const TimberMillPose& pose)
        {
            return frame.within(Matrix3x2::Translation(pose.base) * Matrix3x2::Scale(pose.size));
        }

        PixelPath polygon(const std::initializer_list<FloatPoint> corners)
        {
            PixelPath path;
            for (const FloatPoint corner : corners)
            {
                if (path.isEmpty())
                    path.moveTo(corner);
                else
                    path.lineTo(corner);
            }
            path.close();
            return path;
        }

        PixelPath segments(const std::initializer_list<Segment> lines)
        {
            PixelPath path;
            for (const Segment& line : lines)
            {
                path.moveTo(line.from);
                path.lineTo(line.to);
            }
            return path;
        }

        Color darker(const Color color, const float share)
        {
            return colorBetween(color.fullyOpaque(), k_black, share);
        }

        void drawHouseFront(const SceneFrame& house, const TimberMillColors& colors)
        {
            fillShape(house, polygon({ { 56.0f, -18.0f }, { 108.0f, -32.0f }, { 108.0f, -108.0f },
                { 56.0f, -100.0f } }), LinearGradient::Simple({ 56.0f, -100.0f },
                { 108.0f, -30.0f }, colorBetween(colors.bodyTop, colors.sunlight, 0.12f),
                colors.bodyTop));

            fillShape(house, polygon({ { 60.0f, 0.0f }, { 112.0f, -14.0f }, { 112.0f, -32.0f },
                { 60.0f, -18.0f } }), SolidColor{ colors.stoneLight });
            fillBox(house, { -62.0f, -18.0f, 62.0f, 0.0f },
                SolidColor{ colorBetween(colors.stoneLight, colors.stoneDark, 0.55f) });
            PixelPath joints = segments({
                { { -62.0f, -9.0f }, { 62.0f, -9.0f } },
                { { 60.0f, 0.0f }, { 112.0f, -14.0f } },
                { { 60.0f, -9.0f }, { 112.0f, -23.0f } }
            });
            for (int i = 0; i != 8; ++i)
            {
                const float x = -54.0f + static_cast<float>(i) * 16.0f;
                joints.moveTo(x, -18.0f);
                joints.lineTo(x, -9.0f);
                joints.moveTo(x + 8.0f, -9.0f);
                joints.lineTo(x + 8.0f, 0.0f);
            }
            strokeShape(house, joints, SolidColor{ faded(colors.stoneCrack, 0.7f) }, 0.9f,
                StrokeCap::Butt);

            fillBox(house, { -56.0f, -100.0f, 56.0f, -18.0f }, LinearGradient::Simple(
                { 40.0f, -100.0f }, { -40.0f, -20.0f }, colors.bodyTop, colors.bodyBottom));
            fillShape(house, polygon({ { -56.0f, -100.0f }, { 0.0f, -148.0f },
                { 56.0f, -100.0f } }), LinearGradient::Simple({ 30.0f, -148.0f },
                { -30.0f, -100.0f }, colors.bodyTop,
                colorBetween(colors.bodyTop, colors.bodyBottom, 0.5f)));

            const SolidColor timber{ colors.timber };
            strokeShape(house, segments({
                { { -53.0f, -18.0f }, { -53.0f, -100.0f } },
                { { 53.0f, -18.0f }, { 53.0f, -100.0f } },
                { { -56.0f, -100.0f }, { 56.0f, -100.0f } },
                { { -56.0f, -60.0f }, { 56.0f, -60.0f } }
            }), timber, 6.0f, StrokeCap::Butt);
            strokeShape(house, segments({
                { { -53.0f, -62.0f }, { -30.0f, -98.0f } },
                { { 53.0f, -62.0f }, { 30.0f, -98.0f } },
                { { 82.0f, -25.0f }, { 82.0f, -104.0f } },
                { { 56.0f, -100.0f }, { 108.0f, -108.0f } },
                { { 56.0f, -60.0f }, { 108.0f, -69.0f } }
            }), timber, 4.0f, StrokeCap::Butt);

            PixelPath door;
            door.moveTo(6.0f, -18.0f);
            door.lineTo(6.0f, -46.0f);
            door.quadTo({ 18.0f, -58.0f }, { 30.0f, -46.0f });
            door.lineTo(30.0f, -18.0f);
            door.close();
            fillShape(house, door, SolidColor{ colors.bark });
            strokeShape(house, segments({
                { { 12.0f, -18.0f }, { 12.0f, -50.0f } },
                { { 18.0f, -18.0f }, { 18.0f, -50.0f } },
                { { 24.0f, -18.0f }, { 24.0f, -50.0f } }
            }), SolidColor{ colors.barkShade }, 1.2f, StrokeCap::Butt);
            fillDisc(house, { 25.0f, -32.0f }, 1.6f, SolidColor{ colors.stoneLight });
        }

        void drawHouseWindows(const SceneFrame& house, const TimberMillColors& colors)
        {
            const float lampShare = static_cast<float>(colors.lamp.alpha) / 255.0f;
            const SolidColor glass{ colorBetween(colors.glass, colors.lamp.fullyOpaque(),
                lampShare) };
            const SolidColor timber{ colors.timber };
            const PixelPath front = polygon({ { -40.0f, -84.0f }, { -18.0f, -84.0f },
                { -18.0f, -66.0f }, { -40.0f, -66.0f } });
            const PixelPath side = polygon({ { 68.0f, -82.0f }, { 92.0f, -88.0f },
                { 92.0f, -66.0f }, { 68.0f, -61.0f } });
            for (const PixelPath* window : { &front, &side })
            {
                fillShape(house, *window, glass);
                strokeShape(house, *window, timber, 2.4f);
            }
            strokeShape(house, segments({
                { { -29.0f, -84.0f }, { -29.0f, -66.0f } },
                { { -40.0f, -75.0f }, { -18.0f, -75.0f } },
                { { 80.0f, -85.0f }, { 80.0f, -63.5f } }
            }), timber, 1.6f);
            for (const FloatPoint middle : k_windowMiddles)
                drawGlow(house, middle, 40.0f, faded(colors.lamp, 0.35f));
        }

        void drawRoof(const SceneFrame& house, const TimberMillColors& colors)
        {
            constexpr int k_planks{ 8 };
            constexpr FloatPoint k_frontRidge{ 0.0f, -158.0f };
            constexpr FloatPoint k_backRidge{ 52.0f, -167.0f };
            constexpr FloatPoint k_backEave{ 120.0f, -104.0f };
            constexpr FloatPoint k_frontEave{ 68.0f, -94.0f };
            const float eaveShadowShare = std::min(1.4f, 255.0f / colors.shadow.alpha);

            fillShape(house, polygon({ { 56.0f, -100.0f }, { 108.0f, -108.0f },
                { 108.0f, -100.0f }, { 56.0f, -93.0f } }),
                SolidColor{ faded(colors.shadow, eaveShadowShare) });
            fillShape(house, polygon({ k_frontRidge, k_backRidge, k_backEave, k_frontEave }),
                LinearGradient::Simple({ 20.0f, -165.0f }, { 90.0f, -96.0f }, colors.roofTop,
                colors.roofBottom));
            PixelPath seams;
            for (int i = 1; i != k_planks; ++i)
            {
                const float share = static_cast<float>(i) / static_cast<float>(k_planks);
                seams.moveTo(pointBetween(k_frontRidge, k_frontEave, share));
                seams.lineTo(pointBetween(k_backRidge, k_backEave, share));
            }
            strokeShape(house, seams, SolidColor{ faded(darker(colors.roofBottom, 0.25f), 0.55f) },
                1.1f, StrokeCap::Butt);

            fillShape(house, polygon({ { 66.0f, -150.0f }, { 66.0f, -176.0f }, { 78.0f, -178.0f },
                { 78.0f, -144.0f } }),
                SolidColor{ colorBetween(colors.stoneLight, colors.stoneDark, 0.3f) });
            fillShape(house, polygon({ { 64.0f, -176.0f }, { 80.0f, -179.0f }, { 80.0f, -183.0f },
                { 64.0f, -180.0f } }), SolidColor{ colors.stoneDark });

            fillShape(house, polygon({ { -72.0f, -91.0f }, { 0.0f, -162.0f }, { 72.0f, -91.0f },
                { 66.0f, -87.0f }, { 0.0f, -151.0f }, { -66.0f, -87.0f } }),
                SolidColor{ darker(colors.roofBottom, 0.15f) });
            drawSegment(house, k_frontRidge, k_backRidge,
                SolidColor{ darker(colors.roofBottom, 0.3f) }, 3.0f);
            fillShape(house, polygon({ { -60.0f, -94.0f }, { 0.0f, -149.0f }, { 60.0f, -94.0f },
                { 52.0f, -94.0f }, { 0.0f, -141.0f }, { -52.0f, -94.0f } }),
                SolidColor{ faded(colors.shadow, 0.9f) });
        }
    }
}
