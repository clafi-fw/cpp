module ClaFi.PathArt.BouquetSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace BouquetColors
    {
        constexpr Color Paper{ 0xFFF3E6CC };
        constexpr Color PaperShade{ 0xFFD8C39A };
        constexpr Color Leaf{ 0xFF4F8A3C };
        constexpr Color LeafShade{ 0xFF3B6D2D };
        constexpr Color Ribbon{ 0xFFC2412D };
    }

    namespace
    {
        // A rose's petals, and the darker heart they curl round.
        struct RoseColors
        {
            Color petals;
            Color heart;
        };

        [[nodiscard]] RoseColors roseColorsOf(BouquetRoses);
    }

    // Leaves fan out behind six roses heaped at the top; the paper cone narrows below them to
    // where the ribbon ties it.
    void drawBouquet(const SceneFrame& frame, const BouquetPose& pose)
    {
        const SceneFrame bouquet = frame.within(Matrix3x2::Translation(pose.middle)
            * Matrix3x2::Rotation(pose.turn) * Matrix3x2::Scale(pose.size));
        struct Leaf
        {
            FloatPoint middle;
            float turn;
        };
        constexpr std::array<Leaf, 4> k_leaves{
            Leaf{ { -13.0f, -10.0f }, -0.9f },
            Leaf{ { 13.0f, -10.0f }, 0.9f },
            Leaf{ { -6.0f, -22.0f }, -0.3f },
            Leaf{ { 7.0f, -22.0f }, 0.35f }
        };
        for (const Leaf& leaf : k_leaves)
        {
            fillOval(bouquet, leaf.middle, 4.0f, 9.0f,
                SolidColor{ leaf.turn < 0.0f ? BouquetColors::Leaf : BouquetColors::LeafShade },
                leaf.turn);
        }

        // The roses overlap, so each is filled on its own.
        constexpr std::array<FloatPoint, 6> k_roses{ FloatPoint{ -8.0f, -12.0f },
            FloatPoint{ 8.0f, -12.0f }, FloatPoint{ 0.0f, -18.0f }, FloatPoint{ -4.0f, -6.0f },
            FloatPoint{ 5.0f, -6.0f }, FloatPoint{ 0.0f, -11.0f } };
        const RoseColors rose = roseColorsOf(pose.roses);
        for (const FloatPoint at : k_roses)
        {
            fillDisc(bouquet, at, 6.2f, SolidColor{ rose.petals });
            fillDisc(bouquet, at + FloatPoint{ 0.8f, -0.6f }, 2.6f, SolidColor{ rose.heart });
        }

        PixelPath cone;
        cone.moveTo(-16.0f, -5.0f);
        cone.lineTo(16.0f, -5.0f);
        cone.lineTo(2.5f, 27.0f);
        cone.lineTo(-2.5f, 27.0f);
        cone.close();
        fillShape(bouquet, cone, LinearGradient::Simple({ -16.0f, 0.0f }, { 16.0f, 0.0f },
            BouquetColors::Paper, BouquetColors::PaperShade));
        PixelPath bow;
        for (const float side : { -1.0f, 1.0f })
        {
            bow.moveTo(0.0f, 12.0f);
            bow.lineTo(side * 8.0f, 8.0f);
            bow.lineTo(side * 8.0f, 16.0f);
            bow.close();
        }
        fillShape(bouquet, bow, SolidColor{ BouquetColors::Ribbon });
        fillDisc(bouquet, { 0.0f, 12.0f }, 2.2f, SolidColor{ BouquetColors::Ribbon });
    }

    namespace
    {
        RoseColors roseColorsOf(const BouquetRoses roses)
        {
            switch (roses)
            {
                case BouquetRoses::Red:
                    return { Color{ 0xFFD83A4A }, Color{ 0xFFA8222F } };
                case BouquetRoses::Pink:
                    return { Color{ 0xFFF27AA0 }, Color{ 0xFFC94F78 } };
                case BouquetRoses::Yellow:
                    return { Color{ 0xFFF5C84B }, Color{ 0xFFD39A1F } };
                case BouquetRoses::White:
                    return { Color{ 0xFFF6F0EA }, Color{ 0xFFD9CFC4 } };
            }
            return { Color{ 0xFFD83A4A }, Color{ 0xFFA8222F } };
        }
    }
}
