export module ClaFi.Icons.SideBar;

import ClaFi.Icons.Magnifier;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;
import ClaFi.Core.System.InkWell;

namespace ClaFi::Icons::SideBar
{
    namespace
    {
        // Both of these are previews, and without the badge they read as what their outlines
        // actually are - a window and a window with a side panel, which is a layout toggle. The
        // magnifier goes bottom left, the one corner neither icon is using: the panel icon is
        // empty all over, and the right bar icon keeps its divider on the other side.
        //
        // It is an overlay, so it protrudes rather than fitting inside: the picture gives up a
        // strip on the left and at the bottom, the badge keeps the full icon rect, and the two are
        // held apart by the gap of theme surface the glass carries.
        constexpr float k_badgeRatio = 0.62f;
        constexpr float k_protrusionRatio = 0.25f;

        [[nodiscard]] float badgeSide(const FloatRect& iconRect)
        {
            return std::min(iconRect.width(), iconRect.height()) * k_badgeRatio;
        }

        // What is left of the icon rect once the badge has been let out of it.
        [[nodiscard]] FloatRect pictureRect(const PaintIconEvent& event)
        {
            const FloatRect& iconRect = event.iconRect();
            const float protrusion = badgeSide(iconRect) * k_protrusionRatio;
            return { iconRect.left + protrusion, iconRect.top, iconRect.right, iconRect.bottom - protrusion };
        }

        void paintPreviewBadge(PaintIconEvent& event)
        {
            const FloatRect& iconRect = event.iconRect();
            Magnifier::paintIn(
                event,
                iconRect.bottomLeftSquare(badgeSide(iconRect)),
                Magnifier::Lens::Empty,
                Magnifier::Tail::Yes,
                Magnifier::Gap::Yes
            );
        }
    }


    export void paintRightBarIcon(PaintIconEvent& event)
    {
        const FloatRect iconRect = pictureRect(event);
        RoundedRectangleParts parts;
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const float midPoint = iconRect.relativeX(0.66);
        const float radius = event.scaleF(4);
        parts.bounds = iconRect;
        event.canvas().fillRoundedRectangle(
            iconRect,
            radius,
            radius,
            event.bakedColors().formSurface().toColor()
        );

        parts.bounds.right = midPoint;
        parts.radii = { radius, 0.0f, 0.0f, radius };
        parts.sides = { true, false, true, true };
        event.canvas().drawPartialRoundedRectangle(parts, event.textRgb(InkGrade::Muted), strokeWidth);

        parts.bounds.left = midPoint;
        parts.bounds.right = iconRect.right;
        parts.radii = { 0.0f, radius, radius, 0.0f };
        parts.sides = { true, true, true, true };
        event.canvas().drawPartialRoundedRectangle(parts, event.accentRgb(InkGrade::Strongest), strokeWidth);

        paintPreviewBadge(event);
    }

    export void paintPanelIcon(PaintIconEvent& event)
    {
        const FloatRect iconRect = pictureRect(event);
        const float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        const float radius = event.scaleF(4);
        event.canvas().fillRoundedRectangle(
            iconRect,
            radius,
            radius,
            event.bakedColors().formSurface().toColor()
        );

        event.canvas().drawRoundedRectangle(iconRect, radius, radius, event.accentRgb(InkGrade::Strongest), strokeWidth);
        paintPreviewBadge(event);
    }
}
