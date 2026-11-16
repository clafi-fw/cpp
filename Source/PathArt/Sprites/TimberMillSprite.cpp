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
        constexpr FloatPoint k_wheelCenter{ -84.0f, -12.0f };
        constexpr float k_wheelRadius{ 42.0f };
        constexpr float k_bucketWheelRadius{ 46.0f }; // the rim drawBucketWheel draws at size 1
        constexpr float k_hubHeight{ 126.0f };
        constexpr float k_wallReach{ 58.0f }; // the tower's radius at its top and its foot
        constexpr float k_wallBulge{ 6.0f }; // how much more its radius is at its middle
        constexpr float k_wallTop{ -100.0f }; // under the roof
        constexpr float k_plinthReach{ 62.0f };
        constexpr float k_plinthTop{ -16.0f }; // at the front, as every height round the tower
        // A ring round the tower drops this share of its radius at its front, seen from above.
        constexpr float k_ringDrop{ 0.1f };
        constexpr int k_ringSteps{ 32 };
        constexpr float k_jointWidth{ 0.9f }; // the mortar's, round the plinth and the chimney
        constexpr std::array<FloatPoint, 2> k_windowMiddles{ {
            { -32.5f, -68.0f },
            { 32.5f, -68.0f }
        } };
        constexpr FloatPoint k_chimneyTop{ 37.0f, -174.0f };
        constexpr FloatPoint k_troughMouth{ -86.0f, -78.0f };
        // How many times a cycle the smoke's and the water's motions come round.
        constexpr float k_smokeRises{ 4.0f };
        constexpr float k_smokeSways{ 6.0f };
        constexpr float k_dropFalls{ 64.0f };
        constexpr float k_steamRises{ 16.0f };
        constexpr float k_foamBobs{ 57.0f };
        constexpr float k_foamSwells{ 45.0f };

        // The frame in the mill's units, its origin at the middle of the front wall's foot.
        [[nodiscard]] SceneFrame millFrame(const SceneFrame&, const TimberMillPose&);
        [[nodiscard]] PixelPath polygon(std::initializer_list<FloatPoint> corners);
        [[nodiscard]] PixelPath segments(std::initializer_list<Segment> lines);
        [[nodiscard]] Color darker(Color, float share);
        // The point at `share` across the front of a ring round the tower, from -1 left to 1 right.
        [[nodiscard]] FloatPoint onRing(float frontY, float radius, float share);
        // A ring's front from one share to another, joined to the path or as a figure of its own.
        void addRing(PixelPath&, float frontY, float radius, float fromShare, float toShare,
            bool joined);
        // The tower's radius at a height: a barrel's, k_wallBulge more at its middle.
        [[nodiscard]] float wallReachAt(float frontY);
        // The tower's side from one height to another, `side` -1 for its left and 1 for its right.
        void addWallSide(PixelPath&, float side, float fromY, float toY);
        // The tower's outline from its foot to under the roof.
        [[nodiscard]] PixelPath wallOutline();
        // Light from the right on a round surface from `left` to `right` across.
        [[nodiscard]] LinearGradient roundShading(float left, float right, Color lit, Color dark);
        // The round wall, the plinth under it, and the door framed as the windows are.
        void drawHouseFront(const SceneFrame& house, const TimberMillColors&);
        // The glass is the lamp's colour as far as its opacity goes, and the lamp glows around.
        void drawHouseWindows(const SceneFrame& house, const TimberMillColors&);
        // The chimney behind the cone, its tile rows, its eave, and the eave's shadow on the wall.
        void drawRoof(const SceneFrame& house, const TimberMillColors&);
    }

    void drawTimberMillSmoke(const SceneFrame& frame, const TimberMillPose& pose,
        const TimberMillColors& colors)
    {
        constexpr int k_puffs{ 14 };
        const SceneFrame mill = millFrame(frame, pose);
        for (int i = 0; i != k_puffs; ++i)
        {
            const float index = static_cast<float>(i);
            const float rise = fractionalPart(pose.cycleTime * k_smokeRises
                + index / static_cast<float>(k_puffs));
            const float sway = std::sin(pose.cycleTime * k_smokeSways * k_2Pi + index * 2.0f);
            const FloatPoint center = {
                k_chimneyTop.x + rise * 70.0f + sway * 5.0f * rise,
                k_chimneyTop.y - rise * 80.0f - std::sin(rise * 3.0f) * 6.0f
            };
            const float opacity = 0.3f * (1.0f - rise) * std::min(1.0f, rise * 10.0f);
            fillDisc(mill, center, 3.5f + rise * 17.0f, SolidColor{ colors.smoke.faded(opacity) });
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

    // The house stands in front of the wheel, and a reflection shows the wheel after the house, so
    // the wheel is kept to what the house leaves in view, and to above the water.
    void drawTimberMillWheel(const SceneFrame& frame, const TimberMillPose& pose,
        const TimberMillColors& colors)
    {
        constexpr float k_far{ 400.0f };
        const SceneFrame mill = millFrame(frame, pose);
        const float plinthSideTop = onRing(k_plinthTop, k_plinthReach, -1.0f).y;
        PixelPath beside;
        beside.moveTo(-k_far, -k_far);
        beside.lineTo(-wallReachAt(k_wallTop), -k_far);
        addWallSide(beside, -1.0f, k_wallTop, plinthSideTop);
        beside.lineTo(-k_plinthReach, plinthSideTop);
        beside.lineTo(-k_plinthReach, 0.0f);
        beside.lineTo(-k_far, 0.0f);
        beside.close();
        const Matrix3x2 millToCanvas = mill.toCanvas;
        frame.canvas.pushClip(beside, &millToCanvas);

        const FloatPoint mouth = k_troughMouth;
        fillShape(mill, polygon({ { -50.0f, mouth.y }, { mouth.x, mouth.y + 2.0f },
            { mouth.x, mouth.y + 9.0f }, { -50.0f, mouth.y + 7.0f } }),
            SolidColor{ colors.timber });
        fillShape(mill, polygon({ { -50.0f, mouth.y + 7.0f }, { mouth.x, mouth.y + 9.0f },
            { mouth.x, mouth.y + 12.0f }, { -50.0f, mouth.y + 10.0f } }),
            SolidColor{ darker(colors.timber, 0.35f) });
        drawBucketWheel(mill, BucketWheelPose{
            .center = k_wheelCenter,
            .size = k_wheelRadius / k_bucketWheelRadius,
            .turns = pose.wheelTurns
        }, BucketWheelColors{
            .wood = colors.wheel,
            .spokes = colorBetween(colors.wheel, colors.sunlight, 0.1f),
            .hub = colors.barkShade,
            .axle = colors.timber
        });
        frame.canvas.popClip();
    }

    void drawTimberMillWater(const SceneFrame& frame, const TimberMillPose& pose,
        const TimberMillColors& colors)
    {
        constexpr int k_drops{ 9 };
        constexpr int k_steamPuffs{ 8 };
        constexpr int k_foamHeaps{ 6 };
        const SceneFrame mill = millFrame(frame, pose);
        const float turn = pose.cycleTime * k_2Pi;
        const FloatPoint mouth = k_troughMouth;
        const FloatPoint landing = { mouth.x - 14.0f, mouth.y + 25.5f }; // in the wheel's buckets

        PixelPath pour;
        pour.moveTo(mouth.x, mouth.y + 3.0f);
        pour.quadTo({ mouth.x - 13.0f, mouth.y + 4.0f }, { landing.x - 2.0f, landing.y - 0.5f });
        pour.lineTo(landing.x + 2.0f, landing.y + 0.5f);
        pour.quadTo({ mouth.x - 9.0f, mouth.y + 9.0f }, { mouth.x, mouth.y + 8.0f });
        pour.close();
        fillShape(mill, pour, SolidColor{ colors.water.faded(0.8f) });

        // Steam rising where the pour lands, most of it leaning away with the wind.
        for (int i = 0; i != k_steamPuffs; ++i)
        {
            const float index = static_cast<float>(i);
            const float rise = fractionalPart(pose.cycleTime * k_steamRises
                + index / static_cast<float>(k_steamPuffs));
            const float lean = std::sin(index * 2.4f);
            const FloatPoint center = {
                landing.x + rise * (6.0f + 8.0f * lean),
                landing.y - rise * 16.0f
            };
            const float opacity = 0.4f * (1.0f - rise) * std::min(1.0f, rise * 8.0f);
            fillDisc(mill, center, 2.0f + rise * 6.0f, SolidColor{ colors.foam.faded(opacity) });
        }

        // Drops running down the wheel's left side, where the water pushes it, to the river.
        frame.canvas.pushClip(mill.toCanvas.mapRect({ k_wheelCenter.x - 2.0f * k_wheelRadius,
            k_troughMouth.y, k_wheelCenter.x, 0.0f }));
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
                SolidColor{ colors.foam.faded(0.81f * (1.0f - fall * 0.6f)) }, 1.6f);
        }
        frame.canvas.popClip();

        const float foamStep = 2.0f * k_wheelRadius / static_cast<float>(k_foamHeaps - 1);
        PixelPath foam;
        for (int i = 0; i != k_foamHeaps; ++i)
        {
            const float index = static_cast<float>(i);
            const float x = k_wheelCenter.x - k_wheelRadius + index * foamStep
                + std::sin(turn * k_foamBobs + index) * 3.0f;
            const float crest = -5.0f - 2.0f * std::sin(turn * k_foamSwells + index * 2.0f);
            foam.moveTo(x - 8.0f, 1.0f);
            foam.quadTo({ x, crest }, { x + 8.0f, 1.0f });
            foam.close();
        }
        fillShape(mill, foam, SolidColor{ colors.foam.faded(0.9f) });
    }

    void drawTimberMillWings(const SceneFrame& frame, const TimberMillPose& pose,
        const TimberMillColors& colors)
    {
        constexpr int k_wings{ 4 };
        constexpr float k_stockLength{ 120.0f };
        const SceneFrame hub = millFrame(frame, pose).within(
            Matrix3x2::Translation(0.0f, -k_hubHeight));
        const Color timberShade = darker(colors.timber, 0.35f);

        const PixelPath cloth = polygon({ { 4.0f, -40.0f }, { 30.0f, -45.0f }, { 30.0f, -115.0f },
            { 4.0f, -110.0f } });
        const PixelPath stock = polygon({ { -3.5f, 0.0f }, { 3.5f, 0.0f },
            { 3.0f, -k_stockLength }, { -3.0f, -k_stockLength } });

        for (int i = 0; i != k_wings; ++i)
        {
            const float turns = pose.wingTurns
                + static_cast<float>(i) / static_cast<float>(k_wings);
            const SceneFrame wing = hub.within(Matrix3x2::Rotation(turns * k_2Pi));
            fillShape(wing, cloth, SolidColor{ colors.wingFabric.faded(0.72f) });
            drawSegment(wing, { 30.0f, -45.0f }, { 30.0f, -115.0f },
                SolidColor{ colors.timber.faded(0.85f) }, 2.0f, StrokeCap::Butt);
            fillShape(wing, stock, SolidColor{ colors.timber });
            fillBox(wing, { -3.5f, -k_stockLength, -1.3f, 0.0f }, SolidColor{ timberShade });
        }
        fillDisc(hub, {}, 9.0f, SolidColor{ colors.timber });
        fillDisc(hub, {}, 4.0f, SolidColor{ timberShade });
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

        FloatPoint onRing(const float frontY, const float radius, const float share)
        {
            const float rise = 1.0f - std::sqrt(std::max(0.0f, 1.0f - share * share));
            return { radius * share, frontY - radius * k_ringDrop * rise };
        }

        void addRing(PixelPath& path, const float frontY, const float radius,
            const float fromShare, const float toShare, const bool joined)
        {
            for (int i = 0; i <= k_ringSteps; ++i)
            {
                const float along = static_cast<float>(i) / static_cast<float>(k_ringSteps);
                const FloatPoint point = onRing(frontY, radius,
                    fromShare + (toShare - fromShare) * along);
                if (i == 0 && !joined)
                    path.moveTo(point);
                else
                    path.lineTo(point);
            }
        }

        float wallReachAt(const float frontY)
        {
            const float middleY = (k_wallTop + k_plinthTop) * 0.5f;
            const float fromMiddle = (frontY - middleY) / (k_plinthTop - middleY);
            return k_wallReach + k_wallBulge * (1.0f - fromMiddle * fromMiddle);
        }

        void addWallSide(PixelPath& path, const float side, const float fromY, const float toY)
        {
            for (int i = 0; i <= k_ringSteps; ++i)
            {
                const float along = static_cast<float>(i) / static_cast<float>(k_ringSteps);
                const float frontY = fromY + (toY - fromY) * along;
                path.lineTo(side * wallReachAt(frontY), frontY);
            }
        }

        PixelPath wallOutline()
        {
            PixelPath wall;
            wall.moveTo(-wallReachAt(k_plinthTop), k_plinthTop);
            addWallSide(wall, -1.0f, k_plinthTop, k_wallTop);
            addWallSide(wall, 1.0f, k_wallTop, k_plinthTop);
            wall.close();
            return wall;
        }

        LinearGradient roundShading(const float left, const float right, const Color lit,
            const Color dark)
        {
            return {
                .startPoint = { left, 0.0f },
                .endPoint = { right, 0.0f },
                .stops = {
                    { .position = 0.0f, .color = dark },
                    { .position = 0.45f, .color = colorBetween(dark, lit, 0.6f) },
                    { .position = 0.75f, .color = lit },
                    { .position = 1.0f, .color = colorBetween(lit, dark, 0.35f) }
                }
            };
        }

        void drawHouseFront(const SceneFrame& house, const TimberMillColors& colors)
        {
            const float middleReach = k_wallReach + k_wallBulge;
            fillShape(house, wallOutline(),
                roundShading(-middleReach, middleReach, colors.bodyTop, colors.bodyBottom));

            PixelPath plinth;
            addRing(plinth, k_plinthTop, k_plinthReach, -1.0f, 1.0f, false);
            addRing(plinth, 0.0f, k_plinthReach, 1.0f, -1.0f, true);
            plinth.close();
            fillShape(house, plinth, roundShading(-k_plinthReach, k_plinthReach,
                colorBetween(colors.stoneLight, colors.stoneDark, 0.35f),
                colorBetween(colors.stoneLight, colors.stoneDark, 0.75f)));

            constexpr float k_course{ -8.0f };
            constexpr int k_bricks{ 11 }; // across the front half of a course
            // Half a joint in from each edge, so the mortar round the plinth lies on its stone.
            constexpr float k_edge{ k_jointWidth * 0.5f };
            const float edgeReach = k_plinthReach - k_edge;
            PixelPath joints;
            for (const float frontY : { k_plinthTop + k_edge, k_course, -k_edge })
                addRing(joints, frontY, edgeReach, -1.0f, 1.0f, false);
            for (const float side : { -1.0f, 1.0f })
            {
                joints.moveTo(onRing(k_plinthTop + k_edge, edgeReach, side));
                joints.lineTo(onRing(-k_edge, edgeReach, side));
            }
            // Joints an equal turn apart round the tower, a course's half a brick off the other's.
            const float brickTurn = std::numbers::pi_v<float> / static_cast<float>(k_bricks);
            for (int i = 1; i != 2 * k_bricks; ++i)
            {
                const float share = std::sin(-std::numbers::pi_v<float> * 0.5f
                    + brickTurn * 0.5f * static_cast<float>(i));
                const bool upper = i % 2 == 0;
                joints.moveTo(onRing(upper ? k_plinthTop : k_course, k_plinthReach, share));
                joints.lineTo(onRing(upper ? k_course : 0.0f, k_plinthReach, share));
            }
            strokeShape(house, joints, SolidColor{ colors.stoneCrack.faded(0.6f) }, k_jointWidth,
                StrokeCap::Butt);

            PixelPath doorway;
            doorway.moveTo(-12.0f, k_plinthTop);
            doorway.lineTo(-12.0f, -38.0f);
            doorway.quadTo({ 0.0f, -50.0f }, { 12.0f, -38.0f });
            doorway.lineTo(12.0f, k_plinthTop);
            PixelPath door = doorway;
            door.close();
            fillShape(house, door, SolidColor{ colors.bark });
            strokeShape(house, segments({
                { { -6.0f, k_plinthTop }, { -6.0f, -42.0f } },
                { { 0.0f, k_plinthTop }, { 0.0f, -42.0f } },
                { { 6.0f, k_plinthTop }, { 6.0f, -42.0f } }
            }), SolidColor{ colors.barkShade }, 1.2f, StrokeCap::Butt);
            fillDisc(house, { 7.0f, -31.0f }, 1.5f, SolidColor{ colors.stoneLight });
            strokeShape(house, doorway, SolidColor{ colors.timber }, 2.6f, StrokeCap::Butt);
        }

        void drawHouseWindows(const SceneFrame& house, const TimberMillColors& colors)
        {
            constexpr float k_halfWidth{ 9.0f }; // foreshortened by the tower's turn
            constexpr float k_halfHeight{ 10.0f };
            const float lampShare = static_cast<float>(colors.lamp.alpha) / 255.0f;
            const SolidColor glass{ colorBetween(colors.glass, colors.lamp.fullyOpaque(),
                lampShare) };
            const SolidColor timber{ colors.timber };
            for (const FloatPoint middle : k_windowMiddles)
            {
                const FloatRect pane = {
                    middle.x - k_halfWidth,
                    middle.y - k_halfHeight,
                    middle.x + k_halfWidth,
                    middle.y + k_halfHeight
                };
                fillBox(house, pane, glass);
                strokeShape(house, polygon({ { pane.left, pane.top }, { pane.right, pane.top },
                    { pane.right, pane.bottom }, { pane.left, pane.bottom } }), timber, 2.6f);
                strokeShape(house, segments({
                    { { middle.x, pane.top }, { middle.x, pane.bottom } },
                    { { pane.left, middle.y }, { pane.right, middle.y } }
                }), timber, 1.5f, StrokeCap::Butt);
            }
            for (const FloatPoint middle : k_windowMiddles)
                drawGlow(house, middle, 36.0f, colors.lamp.faded(0.35f));
        }

        void drawRoof(const SceneFrame& house, const TimberMillColors& colors)
        {
            constexpr FloatPoint k_apex{ 0.0f, -160.0f };
            constexpr float k_eaveFront{ -92.0f };
            constexpr float k_eaveReach{ 74.0f };
            constexpr float k_eaveShadow{ 5.0f };
            const Color roofShade = darker(colors.roofBottom, 0.18f);

            const FloatRect stack = { 30.0f, -168.0f, 43.0f, -118.0f };
            const FloatRect cap = { 28.0f, -172.0f, 45.0f, -167.0f };
            fillBox(house, stack,
                SolidColor{ colorBetween(colors.stoneLight, colors.stoneDark, 0.3f) });
            fillBox(house, cap, SolidColor{ colors.stoneDark });
            // Half a joint in from each edge, as round the plinth.
            const float edge = k_jointWidth * 0.5f;
            PixelPath mortar;
            for (const FloatRect& box : { stack, cap })
            {
                mortar.moveTo(box.left + edge, box.top + edge);
                mortar.lineTo(box.right - edge, box.top + edge);
                mortar.lineTo(box.right - edge, box.bottom - edge);
                mortar.lineTo(box.left + edge, box.bottom - edge);
                mortar.close();
            }
            strokeShape(house, mortar, SolidColor{ colors.stoneCrack.faded(0.6f) }, k_jointWidth);

            PixelPath eave;
            addRing(eave, k_eaveFront, k_eaveReach, -1.0f, 1.0f, false);
            PixelPath cone = eave;
            cone.lineTo(k_apex);
            cone.close();

            // The shadow is the eave's ring moved down, on the wall.
            PixelPath shadow;
            addRing(shadow, k_eaveFront, k_eaveReach, -1.0f, 1.0f, false);
            addRing(shadow, k_eaveFront + k_eaveShadow, k_eaveReach, 1.0f, -1.0f, true);
            shadow.close();
            const Matrix3x2 houseToCanvas = house.toCanvas;
            house.canvas.pushClip(wallOutline(), &houseToCanvas);
            fillShape(house, shadow, SolidColor{ colors.shadow.faded(0.8f) });
            house.canvas.popClip();

            fillShape(house, cone, roundShading(-k_eaveReach, k_eaveReach, colors.roofTop,
                colors.roofBottom));
            PixelPath tiles;
            for (float frontY = -150.0f; frontY < k_eaveFront - 4.0f; frontY += 8.0f)
            {
                const float downShare = (frontY - k_apex.y) / (k_eaveFront - k_apex.y);
                addRing(tiles, frontY, k_eaveReach * downShare, -1.0f, 1.0f, false);
            }
            strokeShape(house, tiles, SolidColor{ roofShade.faded(0.45f) }, 1.0f,
                StrokeCap::Butt);
            strokeShape(house, eave, SolidColor{ roofShade }, 4.0f);
        }
    }
}
