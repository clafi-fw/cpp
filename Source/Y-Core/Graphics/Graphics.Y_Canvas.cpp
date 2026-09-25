module ClaFi.Core.Graphics.Canvas;

import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Graphics
{

    // No corner reaches past the middle of the shorter side, so two corners on one edge never
    // cross.
    static CornerRadii clampedRadii(const RoundedRectangleParts& parts)
    {
        const float cap = std::min(parts.bounds.width(), parts.bounds.height()) / 2.0f;
        CornerRadii result = parts.radii;
        for (float& radius : result)
            radius = std::clamp(radius, 0.0f, cap);
        return result;
    }

    // The distance an open end travels past the point the outline reaches. A stroke straddles the
    // outline it follows, so an end that stops on the outline stops half a stroke short of the
    // bounds, and a shape sharing that edge with another starts its ink half a stroke past it -
    // the notch between the two is one stroke wide. Half a stroke carries the end to the bounds.
    static PixelPath buildDrawPartialPath(const RoundedRectangleParts& parts, float openEndExtent)
    {
        PixelPath path;
        float l = parts.bounds.left;
        float t = parts.bounds.top;
        float r = parts.bounds.right;
        float b = parts.bounds.bottom;
        const CornerRadii radii = clampedRadii(parts);

        FloatPoint pTL_H = { l + radii[0], t };
        FloatPoint pTR_H = { r - radii[1], t };
        FloatPoint pTR_V = { r, t + radii[1] };
        FloatPoint pBR_V = { r, b - radii[2] };
        FloatPoint pBR_H = { r - radii[2], b };
        FloatPoint pBL_H = { l + radii[3], b };
        FloatPoint pBL_V = { l, b - radii[3] };
        FloatPoint pTL_V = { l, t + radii[0] };

        struct Seg
        {
            FloatPoint start;
            FloatPoint end;
            bool isArc;
            bool isDrawn;
            FloatPoint ctrl;
        };

        Seg segs[8] = {
            { pTL_H, pTR_H, false, parts.sides[0], { 0.0f, 0.0f } },
            { pTR_H, pTR_V, radii[1] > 0.0f, (parts.sides[0] || parts.sides[1]), { r, t } },
            { pTR_V, pBR_V, false, parts.sides[1], { 0.0f, 0.0f } },
            { pBR_V, pBR_H, radii[2] > 0.0f, (parts.sides[1] || parts.sides[2]), { r, b } },
            { pBR_H, pBL_H, false, parts.sides[2], { 0.0f, 0.0f } },
            { pBL_H, pBL_V, radii[3] > 0.0f, (parts.sides[2] || parts.sides[3]), { l, b } },
            { pBL_V, pTL_V, false, parts.sides[3], { 0.0f, 0.0f } },
            { pTL_V, pTL_H, radii[0] > 0.0f, (parts.sides[3] || parts.sides[0]), { l, t } }
        };

        // A square corner is a segment of zero length and states no direction of its own, so a
        // walk that lands on one steps past it to the side it joins. It stops at the first
        // segment the figure does not draw: past that there is no ink for the end to meet.
        auto unitVector = [](FloatPoint from, FloatPoint to) -> FloatPoint {
            FloatPoint delta = to - from;
            float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (length <= 0.0f)
                return { 0.0f, 0.0f };
            return delta * (1.0f / length);
        };
        auto departure = [&](int index) -> FloatPoint {
            for (int i = 0; i < 8; ++i)
            {
                const Seg& seg = segs[(index + i) % 8];
                if (!seg.isDrawn)
                    break;
                FloatPoint direction = seg.isArc
                    ? unitVector(seg.start, seg.ctrl)
                    : unitVector(seg.start, seg.end);
                if (direction.x || direction.y)
                    return direction;
            }
            return { 0.0f, 0.0f };
        };
        auto arrival = [&](int index) -> FloatPoint {
            for (int i = 0; i < 8; ++i)
            {
                const Seg& seg = segs[(index + 8 - i) % 8];
                if (!seg.isDrawn)
                    break;
                FloatPoint direction = seg.isArc
                    ? unitVector(seg.ctrl, seg.end)
                    : unitVector(seg.start, seg.end);
                if (direction.x || direction.y)
                    return direction;
            }
            return { 0.0f, 0.0f };
        };

        if (parts.sides == k_allRectSidesTrue)
        {
            path.moveTo(segs[0].start);
            for (int i = 0; i < 8; ++i)
            {
                if (segs[i].isArc)
                {
                    path.quadTo(segs[i].ctrl, segs[i].end);
                }
                else if (segs[i].start.x != segs[i].end.x || segs[i].start.y != segs[i].end.y)
                {
                    path.lineTo(segs[i].end);
                }
            }
            path.close();
        }
        else
        {
            int startIndex = 0;
            for (int i = 0; i < 8; ++i)
            {
                if (segs[i].isDrawn && !segs[(i + 7) % 8].isDrawn)
                {
                    startIndex = i;
                    break;
                }
            }

            bool inFigure = false;
            for (int i = 0; i < 8; ++i)
            {
                int index = (startIndex + i) % 8;
                const Seg& seg = segs[index];
                if (seg.isDrawn)
                {
                    if (!inFigure)
                    {
                        path.moveTo(seg.start - departure(index) * openEndExtent);
                        inFigure = true;
                    }
                    if (seg.isArc)
                    {
                        path.quadTo(seg.ctrl, seg.end);
                    }
                    else if (seg.start.x != seg.end.x || seg.start.y != seg.end.y)
                    {
                        path.lineTo(seg.end);
                    }
                    if (!segs[(index + 1) % 8].isDrawn)
                    {
                        // The figure's last point carries the extension of the end it closes on.
                        // A square corner draws nothing of its own, so this is the one line it
                        // contributes, and it lands on the side the corner joins.
                        if (openEndExtent > 0.0f)
                            path.lineTo(seg.end + arrival(index) * openEndExtent);
                        inFigure = false;
                    }
                }
            }
        }
        return path;
    }

    static PixelPath buildFillPartialPath(const RoundedRectangleParts& parts)
    {
        PixelPath path;
        float l = parts.bounds.left;
        float t = parts.bounds.top;
        float r_bound = parts.bounds.right;
        float b = parts.bounds.bottom;
        const CornerRadii radii = clampedRadii(parts);

        FloatPoint pTL_H = { l + radii[0], t };
        FloatPoint pTR_H = { r_bound - radii[1], t };
        FloatPoint pTR_V = { r_bound, t + radii[1] };
        FloatPoint pBR_V = { r_bound, b - radii[2] };
        FloatPoint pBR_H = { r_bound - radii[2], b };
        FloatPoint pBL_H = { l + radii[3], b };
        FloatPoint pBL_V = { l, b - radii[3] };
        FloatPoint pTL_V = { l, t + radii[0] };

        struct Seg
        {
            FloatPoint end;
            bool isArc;
            FloatPoint ctrl;
        };

        Seg segs[8] = {
            { pTR_H, false, { 0.0f, 0.0f } },
            { pTR_V, radii[1] > 0.0f, { r_bound, t } },
            { pBR_V, false, { 0.0f, 0.0f } },
            { pBR_H, radii[2] > 0.0f, { r_bound, b } },
            { pBL_H, false, { 0.0f, 0.0f } },
            { pBL_V, radii[3] > 0.0f, { l, b } },
            { pTL_V, false, { 0.0f, 0.0f } },
            { pTL_H, radii[0] > 0.0f, { l, t } }
        };

        path.moveTo(pTL_H);
        for (int i = 0; i < 8; ++i)
        {
            if (segs[i].isArc)
            {
                path.quadTo(segs[i].ctrl, segs[i].end);
            }
            else
            {
                path.lineTo(segs[i].end);
            }
        }
        path.close();
        return path;
    }

    Canvas::Canvas(std::unique_ptr<IBackend> backend)
        :
        m_backend{ std::move(backend) }
    {
        resize(m_backend->size());
    }

    void Canvas::setBackend(std::unique_ptr<IBackend> backend)
    {
        m_backend = std::move(backend);
        // The size the new backend was built at is the one the canvas takes, the same as at
        // construction: the clip stack belongs to the canvas and starts again with it.
        resize(m_backend->size());
    }

    void Canvas::resize(IntSize newSize)
    {
        if (newSize.x <= 0 || newSize.y <= 0)
            return;

        m_width = newSize.x;
        m_height = newSize.y;
        m_clipStack.clear();
        m_clipStack.push_back({ 0, 0, static_cast<float>(newSize.x), static_cast<float>(newSize.y) });
        if (m_backend)
            m_backend->resize(newSize);
    }

    void Canvas::pushClip(const FloatRect& exactClipRect)
    {
        m_clipStack.push_back(exactClipRect);
        if (m_backend)
            m_backend->pushClip(exactClipRect);
    }

    void Canvas::pushClip(const PixelPath& path, const Matrix3x2* transform)
    {
        float minX = 1e10f;
        float maxX = -1e10f;
        float minY = 1e10f;
        float maxY = -1e10f;
        FloatPoint cp = { 0.0f, 0.0f };
        FloatPoint subpathStart = { 0.0f, 0.0f };

        for (const auto& cmd : path.commands())
        {
            switch (cmd.type)
            {
                case PathCommandType::MoveTo:
                    cp = cmd.p1;
                    subpathStart = cmd.p1;
                    break;
                case PathCommandType::MoveBy:
                    cp = { cp.x + cmd.p1.x, cp.y + cmd.p1.y };
                    subpathStart = cp;
                    break;
                case PathCommandType::LineTo:
                    cp = cmd.p1;
                    break;
                case PathCommandType::LineBy:
                    cp = { cp.x + cmd.p1.x, cp.y + cmd.p1.y };
                    break;
                case PathCommandType::HLineTo:
                    cp.x = cmd.p1.x;
                    break;
                case PathCommandType::HLineBy:
                    cp.x += cmd.p1.x;
                    break;
                case PathCommandType::VLineTo:
                    cp.y = cmd.p1.y;
                    break;
                case PathCommandType::VLineBy:
                    cp.y += cmd.p1.y;
                    break;
                case PathCommandType::QuadTo:
                {
                    FloatPoint p = transform ? transform->transform(cmd.p1) : cmd.p1;
                    minX = std::min(minX, p.x);
                    maxX = std::max(maxX, p.x);
                    minY = std::min(minY, p.y);
                    maxY = std::max(maxY, p.y);
                }
                cp = cmd.p2;
                break;
                case PathCommandType::QuadBy:
                {
                    FloatPoint ctrl = { cp.x + cmd.p1.x, cp.y + cmd.p1.y };
                    FloatPoint end = { cp.x + cmd.p2.x, cp.y + cmd.p2.y };
                    FloatPoint p1 = transform ? transform->transform(ctrl) : ctrl;
                    FloatPoint p2 = transform ? transform->transform(end) : end;
                    minX = std::min({ minX, p1.x, p2.x });
                    maxX = std::max({ maxX, p1.x, p2.x });
                    minY = std::min({ minY, p1.y, p2.y });
                    maxY = std::max({ maxY, p1.y, p2.y });
                    cp = end;
                }
                break;
                case PathCommandType::CubicTo:
                {
                    FloatPoint p1 = transform ? transform->transform(cmd.p1) : cmd.p1;
                    FloatPoint p2 = transform ? transform->transform(cmd.p2) : cmd.p2;
                    FloatPoint p3 = transform ? transform->transform(cmd.p3) : cmd.p3;
                    minX = std::min({ minX, p1.x, p2.x, p3.x });
                    maxX = std::max({ maxX, p1.x, p2.x, p3.x });
                    minY = std::min({ minY, p1.y, p2.y, p3.y });
                    maxY = std::max({ maxY, p1.y, p2.y, p3.y });
                    cp = cmd.p3;
                }
                break;
                case PathCommandType::CubicBy:
                {
                    FloatPoint ctrl1 = { cp.x + cmd.p1.x, cp.y + cmd.p1.y };
                    FloatPoint ctrl2 = { cp.x + cmd.p2.x, cp.y + cmd.p2.y };
                    FloatPoint end = { cp.x + cmd.p3.x, cp.y + cmd.p3.y };
                    FloatPoint p1 = transform ? transform->transform(ctrl1) : ctrl1;
                    FloatPoint p2 = transform ? transform->transform(ctrl2) : ctrl2;
                    FloatPoint p3 = transform ? transform->transform(end) : end;
                    minX = std::min({ minX, p1.x, p2.x, p3.x });
                    maxX = std::max({ maxX, p1.x, p2.x, p3.x });
                    minY = std::min({ minY, p1.y, p2.y, p3.y });
                    maxY = std::max({ maxY, p1.y, p2.y, p3.y });
                    cp = end;
                }
                break;
                case PathCommandType::Close:
                    cp = subpathStart;
                    break;
            }

            if (cmd.type != PathCommandType::Close)
            {
                FloatPoint p = transform ? transform->transform(cp) : cp;
                minX = std::min(minX, p.x);
                maxX = std::max(maxX, p.x);
                minY = std::min(minY, p.y);
                maxY = std::max(maxY, p.y);
            }
        }

        FloatRect bounds = { minX, minY, maxX, maxY };
        if (bounds.empty())
        {
            bounds = m_clipStack.back();
        }
        else
        {
            bounds.intersectWith(m_clipStack.back());
        }
        m_clipStack.push_back(bounds);

        if (m_backend)
            m_backend->pushClip(path, transform);
    }

    void Canvas::popClip()
    {
        if (m_clipStack.size() > 1)
        {
            m_clipStack.pop_back();
            if (m_backend)
                m_backend->popClip();
        }
    }

    void Canvas::pushOpacity(float opacity)
    {
        if (m_backend)
            m_backend->pushOpacity(opacity);
    }

    void Canvas::popOpacity()
    {
        if (m_backend)
            m_backend->popOpacity();
    }

    void Canvas::pushImage(int index)
    {
        if (m_backend)
            m_backend->pushImage(index);
    }

    void Canvas::popImage()
    {
        if (m_backend)
            m_backend->popImage();
    }

    void Canvas::drawImage(int index, float opacity)
    {
        if (m_backend)
            m_backend->drawImage(index, opacity);
    }

    bool Canvas::hasImage(int index) const
    {
        return m_backend && m_backend->hasImage(index);
    }

    FloatRect Canvas::clipBox() const
    {
        if (m_clipStack.empty())
            return { 0, 0, static_cast<float>(m_width), static_cast<float>(m_height) };

        return m_clipStack.back();
    }

    int Canvas::width() const
    {
        return m_width;
    }

    int Canvas::height() const
    {
        return m_height;
    }

    void Canvas::beginPaint(void* nativeContext, const IntRect& dirtyRect)
    {
        if (m_backend)
            m_backend->beginPaint(nativeContext, &dirtyRect);
    }

    void Canvas::endPaint(void* nativeContext, const IntRect& dirtyRect, Bitmap*& outData)
    {
        while (m_clipStack.size() > 1)
            popClip();

        if (m_backend)
            m_backend->endPaint(nativeContext, &dirtyRect, outData);
    }

    void Canvas::setTransform(const Matrix3x2& matrix)
    {
        m_transform = matrix;
        if (m_backend)
            m_backend->setTransform(matrix);
    }

    void Canvas::resetTransform()
    {
        m_transform = Matrix3x2::identity();
        if (m_backend)
            m_backend->resetTransform();
    }

    void Canvas::drawLine(FloatPoint pt1, FloatPoint pt2, const Brush& brush, float strokeWidth)
    {
        if (m_backend)
            m_backend->drawLine(pt1, pt2, brush, strokeWidth);
    }

    void Canvas::drawLine(FloatPoint pt1, FloatPoint pt2, Color color, float strokeWidth)
    {
        drawLine(pt1, pt2, SolidColor{ color }, strokeWidth);
    }

    void Canvas::drawRectangle(const FloatRect& rect, const Brush& brush, float strokeWidth)
    {
        if (m_backend)
            m_backend->drawRectangle(rect, brush, strokeWidth);
    }

    void Canvas::drawRectangle(const FloatRect& rect, Color color, float strokeWidth)
    {
        drawRectangle(rect, SolidColor{ color }, strokeWidth);
    }

    void Canvas::fillRectangle(const FloatRect& rect, const Brush& brush)
    {
        if (m_backend)
            m_backend->fillRectangle(rect, brush);
    }

    void Canvas::fillRectangle(const FloatRect& rect, Color color)
    {
        fillRectangle(rect, SolidColor{ color });
    }

    void Canvas::drawRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush, float strokeWidth)
    {
        if (m_backend)
            m_backend->drawRoundedRectangle(rect, radiusX, radiusY, brush, strokeWidth);
    }

    void Canvas::drawRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, Color color, float strokeWidth)
    {
        drawRoundedRectangle(rect, radiusX, radiusY, SolidColor{ color }, strokeWidth);
    }

    void Canvas::fillRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush)
    {
        if (m_backend)
            m_backend->fillRoundedRectangle(rect, radiusX, radiusY, brush);
    }

    void Canvas::fillRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, Color color)
    {
        fillRoundedRectangle(rect, radiusX, radiusY, SolidColor{ color });
    }

    void Canvas::drawEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush, float strokeWidth)
    {
        if (m_backend)
            m_backend->drawEllipse(center, radiusX, radiusY, brush, strokeWidth);
    }

    void Canvas::drawEllipse(FloatPoint center, float radiusX, float radiusY, Color color, float strokeWidth)
    {
        drawEllipse(center, radiusX, radiusY, SolidColor{ color }, strokeWidth);
    }

    void Canvas::fillEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush)
    {
        if (m_backend)
            m_backend->fillEllipse(center, radiusX, radiusY, brush);
    }

    void Canvas::fillEllipse(FloatPoint center, float radiusX, float radiusY, Color color)
    {
        fillEllipse(center, radiusX, radiusY, SolidColor{ color });
    }

    void Canvas::drawCircle(FloatPoint center, float radius, const Brush& brush, float strokeWidth)
    {
        if (m_backend)
            m_backend->drawEllipse(center, radius, radius, brush, strokeWidth);
    }

    void Canvas::drawCircle(FloatPoint center, float radius, Color color, float strokeWidth)
    {
        drawCircle(center, radius, SolidColor{ color }, strokeWidth);
    }

    void Canvas::fillCircle(FloatPoint center, float radius, const Brush& brush)
    {
        if (m_backend)
            m_backend->fillEllipse(center, radius, radius, brush);
    }

    void Canvas::fillCircle(FloatPoint center, float radius, Color color)
    {
        fillCircle(center, radius, SolidColor{ color });
    }

    void Canvas::drawCorner(const CornerNook& nook, const Brush& brush, float strokeWidth)
    {
        if (nook.radius <= 0.0f)
            return;

        PixelPath path;
        path.moveTo(nook.startPt);
        path.quadTo(nook.pivot, nook.endPt); // Perfect quadratic curve representation of the corner

        drawPath(path, brush, strokeWidth);
    }

    void Canvas::drawCorner(const CornerNook& nook, Color color, float strokeWidth)
    {
        drawCorner(nook, SolidColor{ color }, strokeWidth);
    }

    void Canvas::fillCorner(const CornerNook& nook, const Brush& brush)
    {
        if (nook.radius <= 0.0f)
            return;

        PixelPath path;
        path.moveTo(nook.pivot);
        path.lineTo(nook.startPt);
        path.quadTo(nook.pivot, nook.endPt);
        path.close();

        fillPath(path, brush);
    }

    void Canvas::fillCorner(const CornerNook& nook, Color color)
    {
        fillCorner(nook, SolidColor{ color });
    }

    void Canvas::fillDustyCorner(const CornerNook& nook, const Brush& brush)
    {
        if (nook.radius <= 0.0f)
            return;

        PixelPath path;
        path.moveTo(nook.sharpPt);
        path.lineTo(nook.startPt);
        path.quadTo(nook.pivot, nook.endPt);
        path.close();

        fillPath(path, brush);
    }

    void Canvas::fillDustyCorner(const CornerNook& nook, Color color)
    {
        fillDustyCorner(nook, SolidColor{ color });
    }

    void Canvas::drawPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush, float strokeWidth)
    {
        if (!m_backend->drawPartialRoundedRectangle(parts, brush, strokeWidth))
            drawPartialRoundedRectangleDef(parts, brush, strokeWidth);
    }

    void Canvas::drawPartialRoundedRectangle(const RoundedRectangleParts& parts, Color color, float strokeWidth)
    {
        drawPartialRoundedRectangle(parts, SolidColor{ color }, strokeWidth);
    }

    void Canvas::fillPartialRoundedRectangle(const RoundedRectangleParts& parts, const Brush& brush)
    {
        if (!m_backend->fillPartialRoundedRectangle(parts, brush))
            fillPartialRoundedRectangleDef(parts, brush);
    }

    void Canvas::fillPartialRoundedRectangle(const RoundedRectangleParts& parts, Color color)
    {
        fillPartialRoundedRectangle(parts, SolidColor{ color });
    }

    void Canvas::drawPartialRoundedRectangleDef(const RoundedRectangleParts& parts, const Brush& brush, float strokeWidth)
    {
        if (parts.bounds.empty())
            return;

        if (parts.sides == k_allRectSidesTrue)
        {
            if (isSquare(parts.radii))
            {
                drawRectangle(parts.bounds, brush, strokeWidth);
                return;
            }
            if (isUniform(parts.radii))
            {
                drawRoundedRectangle(parts.bounds, parts.radii[0], parts.radii[0], brush, strokeWidth);
                return;
            }
        }
        if (parts.sides == k_allRectSidesFalse)
            return;

        // The two branches above delegate to primitives that place the stroke inside their
        // bounds themselves. A stroked path has no inside, so this one carries the inset.
        const float halfStroke = strokeWidth * 0.5f;
        RoundedRectangleParts centreline = parts;
        centreline.bounds.inflate(-halfStroke);
        if (centreline.bounds.empty())
            return;
        for (float& radius : centreline.radii)
            radius = (std::max)(0.0f, radius - halfStroke);
        PixelPath path = buildDrawPartialPath(centreline, halfStroke);
        drawPath(path, brush, strokeWidth);
    }

    void Canvas::fillPartialRoundedRectangleDef(const RoundedRectangleParts& parts, const Brush& brush)
    {
        if (parts.bounds.empty())
            return;

        if (isSquare(parts.radii))
        {
            fillRectangle(parts.bounds, brush);
            return;
        }
        if (isUniform(parts.radii))
        {
            fillRoundedRectangle(parts.bounds, parts.radii[0], parts.radii[0], brush);
            return;
        }

        PixelPath path = buildFillPartialPath(parts);
        fillPath(path, brush);
    }

    void Canvas::castShadow(const RoundedRectangleParts& parts, const ShadowParams& shadow)
    {
        if (parts.bounds.empty() || shadow.blur <= 0.0f || !shadow.color.alpha)
            return;
        if (!m_backend->castShadow(parts, shadow))
            castShadowDef(parts, shadow);
    }

    // A shadow is the silhouette, solid, with a ramp running out from it, so it is asked for as
    // one layer. What that costs and how it comes out is then the backend's own affair: the CPU
    // painters measure the distance to the shape once per pixel, and Direct2D lays the ramp down
    // as rings. Neither of those belongs here, and stating the shadow as rings at this level would
    // force the ring cost on the backend that has something better.
    //
    // Solid rather than a glow, because an offset carries the silhouette out from under the
    // shape: the strip between the shape's edge and the silhouette's is shadow all the way, and
    // the ramp begins only past it.
    //
    // The silhouette is the fill path rather than the outline: a shadow follows what the shape
    // covers. `sides` says which edges the shape draws, which is a question about the shape's own
    // ink and not about what it stands in front of.
    void Canvas::castShadowDef(const RoundedRectangleParts& parts, const ShadowParams& shadow)
    {
        // Spread grows the silhouette, which moves the whole ramp outwards; blur is the ramp's
        // own length. Keeping them apart is what lets a shadow sit wider without softening.
        RoundedRectangleParts silhouette = parts;
        silhouette.bounds.offset(shadow.offset);
        silhouette.bounds.inflate(shadow.spread);
        // A round corner grows with the spread; a square one stays square.
        for (float& radius : silhouette.radii)
        {
            if (radius > 0.0f)
                radius = std::max(0.0f, radius + shadow.spread);
        }

        PixelPath path = buildFillPartialPath(silhouette);
        drawPath(path, {
            PathDrawLayer::shadow(shadow.color, shadow.blur, shadow.falloff)
        });
    }

    void Canvas::drawPath(const PixelPath& path, const Brush& brush, float strokeWidth, const Matrix3x2* transform) const
    {
        drawPath(
            path,
            { PathDrawLayer{
                .geometry = {
                    .mode = PathRenderMode::Stroke,
                    .strokeWidth = strokeWidth,
                    .strokeCap = StrokeCap::Round,
                    //.falloff = GlowFalloff::Smooth
                },
                .brush{ brush }
            } },
            transform);
        //m_backend->drawPath(path, brush, strokeWidth, transform);
    }

    void Canvas::drawPath(const PixelPath& path, Color color, float strokeWidth, const Matrix3x2* transform)
    {
        drawPath(path, SolidColor{ color }, strokeWidth, transform);
    }

    void Canvas::fillPath(const PixelPath& path, const Brush& brush, const Matrix3x2* transform) const
    {
        m_backend->fillPath(path, brush, transform);
    }

    void Canvas::fillPath(const PixelPath& path, Color color, const Matrix3x2* transform) const
    {
        fillPath(path, SolidColor{ color }, transform);
    }

    void Canvas::drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2* transform) const
    {
        if (m_backend)
            m_backend->drawPath(path, layers, transform);
    }

    void Canvas::drawPath(const PixelPath& path, std::initializer_list<PathDrawLayer> layers, const Matrix3x2* transform) const
    {
        drawPath(path, std::span<const PathDrawLayer>{ layers.begin(), layers.end() }, transform);
    }

    void Canvas::fillLinearGradientRectangle(const FloatRect& rect, FloatPoint startPoint, FloatPoint endPoint, const std::vector<GradientStop>& stops)
    {
        fillRectangle(rect,
            LinearGradient{
                .startPoint{ startPoint },
                .endPoint{ endPoint },
                .stops{ stops }
            }
        );
    }

    void Canvas::fillRadialGradientRectangle(const FloatRect& rect, FloatPoint center, FloatPoint offset, float radiusX, float radiusY, const std::vector<GradientStop>& stops)
    {
        fillRectangle(rect,
            RadialGradient{
                .center{ center },
                .offset{ offset },
                .radiusX = radiusX,
                .radiusY = radiusY,
                .stops{ stops }
            }
        );
    }

    void Canvas::fillTopBottomGradientRectangle(const FloatRect& rect, Color topColor, Color bottomColor)
    {
        std::vector<GradientStop> stops = { { 0.0f, topColor }, { 1.0f, bottomColor } };
        fillLinearGradientRectangle(rect, rect.topLeft(), rect.bottomLeft(), stops);
    }

    void Canvas::fillLeftRightGradientRectangle(const FloatRect& rect, Color leftColor, Color rightColor)
    {
        std::vector<GradientStop> stops = { { 0.0f, leftColor }, { 1.0f, rightColor } };
        fillLinearGradientRectangle(rect, rect.topLeft(), rect.topRight(), stops);
    }

    void Canvas::fadeEdge(const FloatRect& rect, const CornerRadii& radii, RectSide side, float fadeSize, Color opaqueColor)
    {
        const Color transparentColor = opaqueColor.withOpacity(0.0f);
        RoundedRectangleParts band{ .bounds = rect, .radii = k_squareCorners };
        FloatPoint opaqueEnd = {};
        FloatPoint transparentEnd = {};
        switch (side)
        {
            case RectSide::Top:
                band.bounds.bottom = rect.top + fadeSize;
                band.radii[cornerIndex(Corner::TopLeft)] = radii[cornerIndex(Corner::TopLeft)];
                band.radii[cornerIndex(Corner::TopRight)] = radii[cornerIndex(Corner::TopRight)];
                opaqueEnd = band.bounds.topLeft();
                transparentEnd = band.bounds.bottomLeft();
                break;
            case RectSide::Bottom:
                band.bounds.top = rect.bottom - fadeSize;
                band.radii[cornerIndex(Corner::BottomRight)] = radii[cornerIndex(Corner::BottomRight)];
                band.radii[cornerIndex(Corner::BottomLeft)] = radii[cornerIndex(Corner::BottomLeft)];
                opaqueEnd = band.bounds.bottomLeft();
                transparentEnd = band.bounds.topLeft();
                break;
            case RectSide::Left:
                band.bounds.right = rect.left + fadeSize;
                band.radii[cornerIndex(Corner::TopLeft)] = radii[cornerIndex(Corner::TopLeft)];
                band.radii[cornerIndex(Corner::BottomLeft)] = radii[cornerIndex(Corner::BottomLeft)];
                opaqueEnd = band.bounds.topLeft();
                transparentEnd = band.bounds.topRight();
                break;
            case RectSide::Right:
                band.bounds.left = rect.right - fadeSize;
                band.radii[cornerIndex(Corner::TopRight)] = radii[cornerIndex(Corner::TopRight)];
                band.radii[cornerIndex(Corner::BottomRight)] = radii[cornerIndex(Corner::BottomRight)];
                opaqueEnd = band.bounds.topRight();
                transparentEnd = band.bounds.topLeft();
                break;
            case RectSide::Count:
                break;
        }
        fillPartialRoundedRectangle(band, LinearGradient::simple(opaqueEnd, transparentEnd, opaqueColor, transparentColor));
    }

    void Canvas::stagingDraw(const FloatRect& requestedRect, const StagingDrawFunc& drawFunc)
    {
        // The buffer has to cover both the rect that was asked for and the rect the transform maps
        // it to. Covering only the latter would clip a caller that has not been taught to map its
        // own coordinates yet, and the union costs nothing while the animation only ever shrinks -
        // the mapped rect is then inside the requested one and the union is the requested one.
        FloatRect stagedRect = requestedRect;
        stagedRect.unionWith(m_transform.mapRect(requestedRect));

        // stagingView is overrided for Cpu Canvas
        PixelView pw = m_backend->stagingView(stagedRect);
        if (!pw.data())
            // FallBack for Gpu Canvas
            pw = stagingView(stagedRect);

        drawFunc(pw, m_transform);
        m_backend->commitStagingView(pw);
    }

    void Canvas::drawPixelView(const PixelView& view) const
    {
        if (m_backend)
            m_backend->drawPixelView(view);
    }

    void Canvas::drawBitmap(const Bitmap& bitmap, const FloatPoint& position) const
    {
        const FloatRect canvasClip = clipBox();
        const PixelView view = bitmap.pixelView(position, canvasClip);
        drawPixelView(view);
    }

    IBackend* Canvas::backend() const
    {
        return m_backend.get();
    }

    // private, later I'll move it to Cpu Canvas implementation
    PixelView Canvas::stagingView(const FloatRect& requestedRect)
    {
        FloatRect clipF = clipBox();
        FloatRect activeArea = FloatRect::intersection(requestedRect, clipF);
        if (activeArea.empty())
            return PixelView{ requestedRect, clipF, {} };

        IntRect outerRect = requestedRect.roundedOut();
        int w = outerRect.width();
        int h = outerRect.height();
        std::size_t requiredPixels = static_cast<std::size_t>(w) * h;
        m_stagingBuffer.growTo(requiredPixels);
        m_stagingBuffer.fill(Color{}, requiredPixels);
        // Allocated from the rounded out rect, so this buffer does cover the partly
        // covered first and last rows that requestedRect touches.
        return PixelView{
            requestedRect,
            clipF,
            {
                .data = m_stagingBuffer.data(),
                .stride = w,
                .columns = w,
                .rows = h,
            }
        };
    }

    // ScopedCanvasTransform

    ScopedCanvasTransform::ScopedCanvasTransform(Canvas& canvas, const Matrix3x2& local)
    {
        if (local == Matrix3x2::identity())
            return;

        m_canvas = &canvas;
        m_restoreTo = canvas.transform();
        // Local first, then whatever was already in effect, so a scaled control nested inside
        // another scaled control lands where both transforms put it.
        canvas.setTransform(m_restoreTo * local);
    }

    ScopedCanvasTransform::~ScopedCanvasTransform()
    {
        if (m_canvas)
            m_canvas->setTransform(m_restoreTo);
    }

    ScopedCanvasOpacity::ScopedCanvasOpacity(Canvas& canvas, float opacity)
    {
        if (opacity >= 1.0f)
            return;

        m_canvas = &canvas;
        canvas.pushOpacity(opacity);
    }

    ScopedCanvasOpacity::~ScopedCanvasOpacity()
    {
        if (m_canvas)
            m_canvas->popOpacity();
    }

    ScopedCanvasImage::ScopedCanvasImage(Canvas& canvas, int index)
        :
        m_canvas{ canvas }
    {
        m_canvas.pushImage(index);
    }

    ScopedCanvasImage::~ScopedCanvasImage()
    {
        m_canvas.popImage();
    }

}
