module Themes_App.FishIcon;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace Themes_App::FishIcon
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // In shares of the icon's size, measured from its centre.
        constexpr float k_noseX = 0.44f;
        constexpr float k_tailJointX = -0.2f;
        constexpr float k_tailEndX = -0.46f;
        constexpr float k_tailHalfHeight = 0.2f;
        // A quadratic reaches half way to its control point, so the body is half this tall.
        constexpr float k_bodyControlY = 0.42f;
        constexpr float k_bodyControlX = 0.08f;
        constexpr float k_eyeX = 0.22f;
        constexpr float k_eyeY = -0.05f;
        constexpr float k_eyeRadius = 0.045f;
    }

    // The body in the strong ink and the tail in the accent, as an icon's object and its mark.
    void draw(DrawIconEvent& event)
    {
        const FloatRect& iconRect = event.iconRect();
        const FloatPoint center = iconRect.center();
        const float size = std::min(iconRect.width(), iconRect.height());
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const Color bodyColor = event.textRgb(InkGrade::Strong);
        const Color accentColor = event.accentRgb(InkGrade::Strongest);
        Canvas& canvas = event.canvas();

        const FloatPoint nose = { center.x + size * k_noseX, center.y };
        const FloatPoint tailJoint = { center.x + size * k_tailJointX, center.y };
        const float bodyControlX = center.x + size * k_bodyControlX;
        const float bodyControlRise = size * k_bodyControlY;
        PixelPath path;
        path.moveTo(nose);
        path.quadTo({ bodyControlX, center.y - bodyControlRise }, tailJoint);
        path.quadTo({ bodyControlX, center.y + bodyControlRise }, nose);
        path.close();
        canvas.drawPath(path, bodyColor, strokeWidth);

        path.clear();
        path.moveTo(tailJoint);
        path.lineTo({ center.x + size * k_tailEndX, center.y - size * k_tailHalfHeight });
        path.lineTo({ center.x + size * k_tailEndX, center.y + size * k_tailHalfHeight });
        path.close();
        canvas.drawPath(path, accentColor, strokeWidth);

        const FloatPoint eye = { center.x + size * k_eyeX, center.y + size * k_eyeY };
        canvas.fillCircle(eye, size * k_eyeRadius, bodyColor);
    }
}
