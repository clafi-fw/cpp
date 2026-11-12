module ClaFi.PathArt.TopHatSprite;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace TopHatColors
    {
        constexpr Color Brim{ 0xFF24222B };
        constexpr Color BrimLip{ 0xFF2E2C37 };
        constexpr Color CrownLight{ 0xFF2C2A35 };
        constexpr Color CrownShade{ 0xFF121118 };
        constexpr Color CrownTop{ 0xFF2A2833 };
        constexpr Color Band{ 0xFFB8233A };
        constexpr Color Inside{ 0xFF07060A };
    }

    namespace
    {
        constexpr float k_brimRadius{ 38.0f };
        constexpr float k_brimDepth{ 7.0f };
        constexpr float k_openingRadius{ 24.0f };
        constexpr float k_openingDepth{ 4.5f };
        constexpr float k_crownHeight{ 54.0f };
    }

    SceneFrame topHatFrame(const SceneFrame& frame, const TopHatPose& pose)
    {
        return frame.within(Matrix3x2::Translation(pose.brim) * Matrix3x2::Rotation(pose.turn)
            * Matrix3x2::Scale(pose.size));
    }

    // The crown flares a little toward its top; upside down, it hangs below the brim.
    void drawTopHat(const SceneFrame& frame, const TopHatPose& pose)
    {
        const SceneFrame hat = topHatFrame(frame, pose);
        const float up = pose.upsideDown ? -1.0f : 1.0f;
        fillOval(hat, { 0.0f, 0.0f }, k_brimRadius, k_brimDepth, SolidColor{ TopHatColors::Brim });
        PixelPath crown;
        crown.moveTo(-23.0f, 0.0f);
        crown.lineTo(23.0f, 0.0f);
        crown.lineTo(25.0f, -k_crownHeight * up);
        crown.lineTo(-25.0f, -k_crownHeight * up);
        crown.close();
        fillShape(hat, crown, LinearGradient::Simple({ -25.0f, 0.0f }, { 25.0f, 0.0f },
            TopHatColors::CrownLight, TopHatColors::CrownShade));
        PixelPath band;
        band.moveTo(-23.2f, -4.0f * up);
        band.lineTo(23.2f, -4.0f * up);
        band.lineTo(23.6f, -13.0f * up);
        band.lineTo(-23.6f, -13.0f * up);
        band.close();
        fillShape(hat, band, SolidColor{ TopHatColors::Band });
        fillOval(hat, { 0.0f, -k_crownHeight * up }, 25.0f, 4.5f,
            SolidColor{ TopHatColors::CrownTop });
        if (pose.upsideDown)
            fillOval(hat, { 0.0f, -0.5f }, k_openingRadius, k_openingDepth,
                SolidColor{ TopHatColors::Inside });
    }

    void drawTopHatBrimLip(const SceneFrame& frame, const TopHatPose& pose)
    {
        PixelPath lip;
        lip.moveTo(-k_brimRadius, 0.0f);
        lip.arcTo({ k_brimRadius, k_brimDepth }, 0.0f, ArcSize::Small, ArcSweep::CounterClockwise,
            { k_brimRadius, 0.0f });
        lip.lineTo(k_openingRadius, 0.0f);
        lip.arcTo({ k_openingRadius, k_openingDepth }, 0.0f, ArcSize::Small, ArcSweep::Clockwise,
            { -k_openingRadius, 0.0f });
        lip.close();
        fillShape(topHatFrame(frame, pose), lip, SolidColor{ TopHatColors::BrimLip });
    }
}
