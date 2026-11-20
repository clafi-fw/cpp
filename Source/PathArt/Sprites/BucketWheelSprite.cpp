module ClaFi.PathArt.BucketWheelSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    void drawBucketWheel(const SceneFrame& frame, const BucketWheelPose& pose,
        const BucketWheelColors& colors)
    {
        constexpr float k_rimRadius{ 46.0f };
        constexpr float k_innerRimRadius{ 36.0f };
        constexpr int k_buckets{ 18 };
        constexpr int k_spokes{ 8 };
        const Color black{ 0xFF000000 };
        const SceneFrame wheel = frame.within(Matrix3x2::Translation(pose.center)
            * Matrix3x2::Scale(pose.size));

        PixelPath farRim;
        farRim.drawCircle({ -5.0f, 2.0f }, k_rimRadius);
        strokeShape(wheel, farRim, SolidColor{ Color::between(colors.wood, black, 0.45f) }, 4.0f);

        const SceneFrame turning = wheel.within(Matrix3x2::Rotation(-pose.turns * k_2Pi));
        PixelPath bucket;
        bucket.moveTo(-2.0f, -k_rimRadius - 4.0f);
        bucket.lineTo(5.0f, -k_rimRadius - 3.0f);
        bucket.lineTo(3.0f, -k_rimRadius + 9.0f);
        bucket.lineTo(-2.0f, -k_rimRadius + 9.0f);
        bucket.close();
        const SolidColor bucketWood{ Color::between(colors.wood, black, 0.3f) };
        for (int i = 0; i != k_buckets; ++i)
        {
            const float turns = static_cast<float>(i) / static_cast<float>(k_buckets);
            fillShape(turning, bucket, bucketWood, Matrix3x2::Rotation(turns * k_2Pi));
        }

        PixelPath rim;
        rim.drawCircle({}, k_rimRadius);
        strokeShape(turning, rim, SolidColor{ colors.wood }, 4.5f);
        rim.drawCircle({}, k_innerRimRadius);
        strokeShape(turning, rim, SolidColor{ colors.wood }, 3.0f);

        PixelPath spokes;
        for (int i = 0; i != k_spokes; ++i)
        {
            const float angle = static_cast<float>(i) / static_cast<float>(k_spokes) * k_2Pi;
            spokes.moveTo(0.0f, 0.0f);
            spokes.lineTo(std::cos(angle) * k_innerRimRadius, std::sin(angle) * k_innerRimRadius);
        }
        strokeShape(turning, spokes, SolidColor{ colors.spokes }, 3.2f);

        fillDisc(wheel, {}, 7.0f, SolidColor{ colors.hub });
        fillDisc(wheel, {}, 3.5f, SolidColor{ colors.axle });
    }
}
