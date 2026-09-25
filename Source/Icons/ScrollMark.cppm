export module ClaFi.Icons.ScrollMark;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Icons::ScrollMark
{
    using namespace ::ClaFi::Graphics;

    // A scroll arrow, drawn as one filled polygon.
    //
    // Both of its boundaries are piecewise linear - a chevron on one side, a flat foot with sloped
    // ends on the other - so the mark is exactly eight points. It was filled scanline by scanline
    // through FuncPainterBase before, which is what a polygon costs when its boundaries are only
    // reachable as functions of x.
    //
    // Axis and direction cannot come off the event, so they are arguments. Everything else does:
    // the mark is sized and placed by the icon rect, exactly as XMark and PlusMark are.
    export void paint(PaintIconEvent&, ScrollAxis, ScrollDirection);


    //-------------------------------------------------------------------------


    void paint(PaintIconEvent& event, ScrollAxis axis, ScrollDirection direction)
    {
        constexpr float k_chevronSlope = 1.66f;
        constexpr float k_footSlope = 1.5f;
        constexpr std::size_t k_boundaryPoints = 4;

        const FloatPoint center = event.iconCenter();
        const float halfSize = event.iconWidth() / 2.0f;
        const float chevronFlat = halfSize / 5.5f;
        const float chevronCrest = halfSize / 2.0f;
        const float chevronDrop = halfSize * 1.2f;
        const float footHeight = halfSize * 0.75f;
        const float reach = halfSize * 0.83f;
        const bool toBegin = direction == ScrollDirection::ToBegin;

        auto chevron = [&](float x){
            float distance = std::abs(x);
            float height = distance <= chevronFlat ? chevronCrest : distance * k_chevronSlope + chevronFlat;
            return height - chevronDrop;
        };

        auto foot = [&](float x){
            float overhang = std::max(0.0f, std::abs(x) - footHeight);
            return footHeight - overhang * k_footSlope;
        };

        // ToBegin is the same mark upside down - the foot becomes the lower boundary and the
        // chevron the upper one, both negated. Each boundary contributes only its own breakpoints.
        const float chevronXs[k_boundaryPoints] = { -reach, -chevronFlat, chevronFlat, reach };
        const float footXs[k_boundaryPoints] = { -reach, -footHeight, footHeight, reach };
        const float* lowerXs = toBegin ? footXs : chevronXs;
        const float* upperXs = toBegin ? chevronXs : footXs;

        auto lowerY = [&](float x){
            if (toBegin)
                return -foot(x);
            return chevron(x);
        };

        auto upperY = [&](float x){
            if (toBegin)
                return -chevron(x);
            return foot(x);
        };

        // Built in the mark's own space, y running up, and mapped on the way out. Only the axis
        // rotation happens here: the canvas transform is composed by the backend, so the mark
        // follows its button's animation without being mapped for it.
        PixelPath path{};
        bool started = false;

        auto append = [&](float x, float y){
            FloatPoint point = axis == ScrollAxis::Vertical
                ? FloatPoint{ center.x + x, center.y - y }
                : FloatPoint{ center.x - y, center.y + x };
            if (started)
            {
                path.lineTo(point);
                return;
            }
            path.moveTo(point);
            started = true;
        };

        for (std::size_t i = 0; i != k_boundaryPoints; ++i)
            append(lowerXs[i], lowerY(lowerXs[i]));
        for (std::size_t i = k_boundaryPoints; i-- != 0; )
            append(upperXs[i], upperY(upperXs[i]));

        path.close();
        event.canvas().fillPath(path, event.textRgb(InkGrade::Muted));
    }

}
