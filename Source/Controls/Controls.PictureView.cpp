module ClaFi.Controls.PictureView;

import ClaFi.Core.Foundation;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::Controls
{
    namespace
    {
        // The whole pixels a clip leaves whole: a row or a column it cuts through belongs to
        // whatever stands past it.
        [[nodiscard]] IntRect wholePixelsInside(const FloatRect& clip)
        {
            return {
                static_cast<int>(std::ceil(clip.left)),
                static_cast<int>(std::ceil(clip.top)),
                static_cast<int>(std::floor(clip.right)),
                static_cast<int>(std::floor(clip.bottom)),
            };
        }
    }

    // A new picture opens on its middle pixel, so the readout and the zoom have a pixel to stand
    // on before anything is clicked.
    void PictureView::setPicture(const Graphics::Bitmap* picture)
    {
        m_picture = picture;
        m_scrollWanted.reset();
        m_cache = Graphics::Bitmap{};
        m_cacheZoom = 0.0f;
        m_panning = false;
        const bool shown = picture && !picture->empty();
        if (shown)
        {
            m_selection = IntPoint{ picture->width() / 2, picture->height() / 2 };
        }
        else
        {
            m_selection.reset();
            m_pictureAt = {};
            m_originFraction = {};
            m_content = {};
            m_window = {};
        }
        // The fit needs the window the view is seen through, which the pass this asks for lays
        // out - see aligned.
        m_fitPending = shown;
        invalidateFormAlign();
        announceSelection();
    }

    void PictureView::setSelection(std::optional<IntPoint> value)
    {
        if (value && m_picture)
        {
            const bool inside = value->x >= 0 && value->x < m_picture->width()
                && value->y >= 0 && value->y < m_picture->height();
            if (!inside)
                value.reset();
        }
        else
        {
            value.reset();
        }

        if (value == m_selection)
            return;

        m_selection = value;
        invalidate();
        announceSelection();
    }

    std::optional<Color> PictureView::selectedColor() const
    {
        if (!m_selection || !m_picture)
            return std::nullopt;

        return pixelClamped(m_selection->x, m_selection->y);
    }

    void PictureView::setZoom(const float value)
    {
        zoomTo(value, anchorAtSelection());
    }

    void PictureView::zoomBySteps(const int steps)
    {
        setZoom(m_zoom * std::pow(k_zoomStep, static_cast<float>(steps)));
    }

    float PictureView::fitZoom() const
    {
        if (!m_picture || m_picture->empty())
            return 1.0f;

        const FloatRect window = windowInForm();
        if (window.empty())
            return 1.0f;

        const float byWidth = window.width() / static_cast<float>(m_picture->width());
        const float byHeight = window.height() / static_cast<float>(m_picture->height());
        const float fit = std::min({ byWidth, byHeight, 1.0f });
        return std::clamp(fit, k_minZoom, k_maxZoom);
    }

    FloatRect PictureView::scrollHotspot() const
    {
        FloatRect visible = visibleRectInForm();
        if (visible.empty())
            return Control::scrollHotspot();

        visible.offset(-boundsInForm().topLeft());
        return visible;
    }

    // The pixel every command in the menu is about, so one raised from the keyboard drops at the
    // crosshair. A crosshair scrolled out of sight names a place the menu must not drop at, and
    // the view stands in for it.
    FloatRect PictureView::contextMenuAnchor() const
    {
        if (!m_selection)
            return Control::contextMenuAnchor();

        FloatRect pixel = FloatRect::fromDimensions(pointInForm(m_selection->toFloat()),
            { m_zoom, m_zoom });
        if (!pixel.intersectWith(visibleRectInForm()))
            return Control::contextMenuAnchor();

        return pixel;
    }

    ScaledDimensions PictureView::calculateContent(AlignEvent&)
    {
        return m_content;
    }

    void PictureView::paintSurface(PaintEvent& event)
    {
        Control::paintSurface(event);

        if (!m_picture || m_picture->empty())
            return;

        const FloatRect bounds = event.controlBounds();
        const FloatRect pictureRect = FloatRect::fromDimensions(bounds.topLeft() + m_pictureAt,
            pictureSize());
        const FloatRect visible = FloatRect::intersection(event.viewport(),
            FloatRect::intersection(pictureRect, bounds));
        if (!visible.empty())
            paintPicture(event, pictureRect, visible);

        paintCrosshair(event, pictureRect);
    }

    // A PRESS STATES THE TARGET: the crosshair and the reading follow the button down rather than
    // waiting for it to come back up. A press that goes on to travel pans, and leaves selected the
    // pixel it was taken hold of at.
    void PictureView::pressDown(PressDownEvent& event)
    {
        Control::pressDown(event);
        if (event.propagationStopped())
            return;

        m_dragPos = event.clickPos();
        m_panning = false;
        if (const std::optional<IntPoint> pixel = pixelAt(event.clickPos()))
            setSelection(pixel);
    }

    // The second press on a pixel asks for it to be acted on - the colour editor, where this
    // viewer hangs one. A double click on the surface beside the picture names no pixel and asks
    // for nothing.
    void PictureView::doubleClick(DoubleClickEvent& event)
    {
        Control::doubleClick(event);
        if (event.propagationStopped())
            return;

        if (pixelAt(event.clickPos()))
            announceActivate();
    }

    // The picture follows the pointer by what it moved since the last move. Stopped here so
    // the box scrolling this view does not read the drag as a selection reaching past its edge.
    void PictureView::drag(DragEvent& event)
    {
        if (!m_picture || m_picture->empty())
            return;

        if (!m_panning && event.unscaledDistance() < k_panThreshold)
            return;

        m_panning = true;
        event.stopPropagation();

        const PointInForm current = event.currentPos();
        const FloatPoint delta = { current.x - m_dragPos.x, current.y - m_dragPos.y };
        m_dragPos = current;
        if (delta.x != 0.0f || delta.y != 0.0f)
            placeAt(originNow() + delta);
    }

    void PictureView::mouseMove(MouseMoveEvent& event)
    {
        Control::mouseMove(event);
        m_pointerPos = event.posOnForm;
    }

    // A NOTCH SELECTS THE PIXEL UNDER THE POINTER AND ZOOMS ABOUT IT, so the crosshair and the
    // readout land where the zoom is going without a click. Over the surface beside the
    // picture there is no pixel to take, and the selection stands while the zoom still keeps
    // the point under the pointer still.
    void PictureView::mouseWheel(MouseWheelEvent& event)
    {
        if (!m_picture || m_picture->empty())
            return;

        event.handled = true;
        if (const std::optional<IntPoint> pixel = pixelAt(m_pointerPos))
            setSelection(pixel);

        zoomTo(m_zoom * std::pow(k_zoomStep, event.delta), anchorAt(m_pointerPos));
    }

    // THE MENU IS ABOUT THE PIXEL IT WAS RAISED ON: the press selects that pixel first, as a left
    // one does, so the crosshair and the commands name the same one. A menu raised from the
    // keyboard moves nothing - the pixel already selected is what it is about. The commands
    // themselves are the application's: the view holds none of its own.
    void PictureView::contextPopup(ContextPopupEvent& event)
    {
        if (Input::device() == InputDevice::Mouse)
        {
            if (const std::optional<IntPoint> pixel = pixelAt(form().mouseDownPos()))
                setSelection(pixel);
        }

        Control::contextPopup(event);
    }

    FloatSize PictureView::pictureSize() const
    {
        if (!m_picture)
            return {};

        const FloatSize pixels = {
            static_cast<float>(m_picture->width()),
            static_cast<float>(m_picture->height()),
        };
        return pixels * m_zoom;
    }

    // THE PICTURE STANDS WHERE IT IS PUT, small or large, and the view is measured around it:
    // the picture, and any strip of the window it leaves bare while hanging out the other
    // side. The bars range over that, so nothing pulls the picture in to cover the window,
    // and a zoom keeps its point under the pointer across the size of the window as it does
    // everywhere else. A margin of the picture always stays in the window.
    void PictureView::placeAt(const FloatPoint origin)
    {
        const FloatRect window = windowInForm();
        const FloatSize size = pictureSize();
        const float margin = scaler().scale(k_keepInView);
        const AxisPlacement x = placeAlong(origin.x, size.x, window.width(), margin);
        const AxisPlacement y = placeAlong(origin.y, size.y, window.height(), margin);
        m_pictureAt = { x.pictureAt, y.pictureAt };
        m_originFraction = { x.origin - std::floor(x.origin), y.origin - std::floor(y.origin) };
        m_window = { window.width(), window.height() };
        const FloatSize content = { x.extent, y.extent };
        const FloatPoint scroll = { x.scroll, y.scroll };
        if (content == m_content)
        {
            // The bars range over this already: the view is carried now, with no pass between.
            invalidate();
            scrollTo(scroll);
            return;
        }

        m_content = content;
        m_scrollWanted = scroll;
        invalidateFormAlign();
    }

    PictureView::AxisPlacement PictureView::placeAlong(const float origin, const float size,
        const float window, const float margin)
    {
        const float keep = std::min({ margin, size, window });
        const float wanted = std::clamp(origin, keep - size, window - keep);
        const float held = std::floor(wanted);
        if (held < 0.0f)
        {
            // Hanging out the near side: measured from its edge to the far edge of the picture
            // or of the window, whichever is out further, so a strip left bare at the far side
            // is ranged over by the bar too.
            return { wanted, 0.0f, std::max(window, held + size) - held, -held };
        }

        // Inside the window, or hanging out the far side: measured as the picture, and out to
        // the far edge where it hangs out, so a picture inside the window asks for no bar and
        // a move within it asks for no pass.
        return { wanted, held, held + size > window ? held + size : size, 0.0f };
    }

    FloatPoint PictureView::scrolledBy() const
    {
        const FloatRect window = windowInForm();
        const FloatRect bounds = boundsInForm();
        return { window.left - bounds.left, window.top - bounds.top };
    }

    FloatPoint PictureView::originNow() const
    {
        return m_pictureAt + m_originFraction - scrolledBy();
    }

    void PictureView::scrollTo(const FloatPoint scroll)
    {
        const FloatPoint delta = scroll - scrolledBy();
        if (delta.x != 0.0f || delta.y != 0.0f)
            scrollViewBy(delta);
    }

    FloatPoint PictureView::pointInControl(const PointInForm point) const
    {
        const FloatRect bounds = boundsInForm();
        return { point.x - bounds.left, point.y - bounds.top };
    }

    FloatPoint PictureView::picturePointAt(const FloatPoint pointInControl) const
    {
        return {
            (pointInControl.x - m_pictureAt.x) / m_zoom,
            (pointInControl.y - m_pictureAt.y) / m_zoom,
        };
    }

    PointInForm PictureView::pointInForm(const FloatPoint picturePoint) const
    {
        return boundsInForm().topLeft() + m_pictureAt + picturePoint * m_zoom;
    }

    std::optional<IntPoint> PictureView::pixelAt(const PointInForm point) const
    {
        if (!m_picture || m_picture->empty())
            return std::nullopt;

        const FloatPoint picturePoint = picturePointAt(pointInControl(point));
        const IntPoint pixel = {
            static_cast<int>(std::floor(picturePoint.x)),
            static_cast<int>(std::floor(picturePoint.y)),
        };
        const bool inside = pixel.x >= 0 && pixel.x < m_picture->width()
            && pixel.y >= 0 && pixel.y < m_picture->height();
        if (!inside)
            return std::nullopt;

        return pixel;
    }

    // AN ANCHOR IS TAKEN FROM THE ORIGIN THE PLACEMENT FLOORED, so that a zoom measured from
    // what is on screen does not lose that fraction again, and a slider dragged across its track
    // does not carry the picture off a fraction of a pixel at a time.
    PictureView::Anchor PictureView::anchorAt(const PointInForm point) const
    {
        return { picturePointAt(pointInControl(point) - m_originFraction), point };
    }

    PictureView::Anchor PictureView::anchorAtSelection() const
    {
        if (m_selection)
        {
            const FloatPoint centre = m_selection->toFloat() + FloatPoint{ 0.5f, 0.5f };
            return { centre, pointInForm(centre) + m_originFraction };
        }

        FloatRect visible = visibleRectInForm();
        if (visible.empty())
            visible = boundsInForm();

        return anchorAt(visible.center());
    }

    void PictureView::zoomTo(const float zoom, const Anchor anchor)
    {
        const float clamped = std::clamp(zoom, k_minZoom, k_maxZoom);
        if (clamped == m_zoom)
            return;

        m_zoom = clamped;
        // the top left that keeps the anchor's point of the picture under its point in the form
        const FloatPoint origin = anchor.formPoint - windowInForm().topLeft()
            - anchor.picturePoint * m_zoom;
        placeAt(origin);
        announceZoom();
    }

    void PictureView::aligned()
    {
        if (!m_picture || m_picture->empty())
            return;

        const FloatRect window = windowInForm();
        if (window.empty())
            return;

        if (m_fitPending)
        {
            m_fitPending = false;
            const float fit = fitZoom();
            const bool changed = fit != m_zoom;
            m_zoom = fit;
            // in the middle of the window
            const FloatSize size = pictureSize();
            placeAt({ (window.width() - size.x) / 2.0f, (window.height() - size.y) / 2.0f });
            if (changed)
                announceZoom();
            return;
        }

        const FloatSize windowSize = { window.width(), window.height() };
        if (windowSize != m_window)
        {
            // A bar came or went, or the form was resized: measured against the window anew,
            // from where the picture stands or was about to stand.
            const FloatPoint scroll = m_scrollWanted ? *m_scrollWanted : scrolledBy();
            m_scrollWanted.reset();
            placeAt(m_pictureAt + m_originFraction - scroll);
            return;
        }

        if (m_scrollWanted)
        {
            // The bars range over the new content now, and are set where the placement needs
            // them. Set, not left to glide: a bar whose range shrank under it is on its way
            // home, and setting it ends that where the placement says.
            const FloatPoint scroll = *m_scrollWanted;
            m_scrollWanted.reset();
            scrollTo(scroll);
        }
    }

    void PictureView::announceSelection()
    {
        PixelSelectEvent event{ *this };
        emitEvent(event);
    }

    void PictureView::announceActivate()
    {
        PixelActivateEvent event{ *this };
        emitEvent(event);
    }

    void PictureView::announceZoom()
    {
        ZoomChangeEvent event{ *this };
        emitEvent(event);
    }

    // SAMPLED ONCE, COPIED AFTER. The cache holds the part of the picture on screen at the
    // zoom, so a paint copies its rows into the staging view: what a paint costs is set by
    // the size of the view, and what a zoom costs is one sampling of it.
    void PictureView::paintPicture(PaintEvent& event, const FloatRect& pictureRect,
        const FloatRect& visible)
    {
        const IntPoint at = {
            static_cast<int>(std::lround(pictureRect.left)),
            static_cast<int>(std::lround(pictureRect.top)),
        };
        // The whole of the picture on screen, not the part this paint is asked for: a paint of
        // a corner must not shrink the cache to that corner.
        FloatRect onScreen = FloatRect::intersection(visibleRectInForm(), pictureRect);
        if (onScreen.empty())
            onScreen = visible;
        else
            onScreen.unionWith(visible);
        IntRect region = onScreen.roundedOut();
        region.offset(-at.x, -at.y);
        const FloatSize size = pictureSize();
        const IntRect extent = {
            0,
            0,
            static_cast<int>(std::ceil(size.x)),
            static_cast<int>(std::ceil(size.y)),
        };
        if (!region.intersectWith(extent))
            return;

        const bool held = m_cacheZoom == m_zoom
            && m_cacheRect.left <= region.left && m_cacheRect.top <= region.top
            && m_cacheRect.right >= region.right && m_cacheRect.bottom >= region.bottom;
        if (!held)
            renderCache(region);

        const IntRect area = visible.roundedOut();
        IntRect cached = m_cacheRect;
        cached.offset(at);

        using Graphics::PixelView;
        using Graphics::Matrix3x2;
        event.canvas().stagingDraw(area.toFloat(), [&](PixelView view, const Matrix3x2&) {
            IntRect span = area;
            if (!span.intersectWith(view.ownedPixels())
                || !span.intersectWith(wholePixelsInside(view.clipBox()))
                || !span.intersectWith(cached))
            {
                return;
            }

            for (int y = span.top; y < span.bottom; ++y)
            {
                Color* line = view.scanLineAbs(static_cast<float>(span.left),
                    static_cast<float>(y));
                const Color* source = cacheRow(y - at.y) + (span.left - at.x - m_cacheRect.left);
                std::copy_n(source, span.width(), line);
            }
        });
    }

    // ONE HAIRLINE EACH WAY THROUGH THE PIXEL, EVERY PIXEL OF IT THE CONTRAST OF WHAT IT CROSSES:
    // a picture pixel read from the cache, the surface past the picture's edge. See Controls
    void PictureView::paintCrosshair(PaintEvent& event, const FloatRect& pictureRect)
    {
        if (!m_selection)
            return;

        // the device pixel the arms cross in: the middle of the selected pixel's block
        const FloatPoint centre = pictureRect.topLeft()
            + (m_selection->toFloat() + FloatPoint{ 0.5f, 0.5f }) * m_zoom;
        const IntPoint cross = {
            static_cast<int>(std::floor(centre.x)),
            static_cast<int>(std::floor(centre.y)),
        };
        const int arm = static_cast<int>(std::lround(event.scaleF(k_crosshairArm)));
        IntRect reach = { cross.x - arm, cross.y - arm, cross.x + arm + 1, cross.y + arm + 1 };
        if (!reach.intersectWith(event.controlBounds().roundedOut())
            || !reach.intersectWith(event.viewport().roundedOut()))
        {
            return;
        }

        const IntPoint at = {
            static_cast<int>(std::lround(pictureRect.left)),
            static_cast<int>(std::lround(pictureRect.top)),
        };
        const Color surfaceMark = contrasted(event.surfaceRgb().fullyOpaque());

        using Graphics::PixelView;
        using Graphics::Matrix3x2;
        event.canvas().stagingDraw(reach.toFloat(), [&](PixelView view, const Matrix3x2&) {
            IntRect span = reach;
            if (!span.intersectWith(view.ownedPixels())
                || !span.intersectWith(wholePixelsInside(view.clipBox())))
            {
                return;
            }

            if (cross.y >= span.top && cross.y < span.bottom)
            {
                Color* line = view.scanLineAbs(static_cast<float>(span.left),
                    static_cast<float>(cross.y));
                for (int x = span.left; x < span.right; ++x)
                    line[x - span.left] = crosshairPixelAt({ x, cross.y }, at, surfaceMark);
            }
            if (cross.x >= span.left && cross.x < span.right)
            {
                for (int y = span.top; y < span.bottom; ++y)
                {
                    Color* pixel = view.scanLineAbs(static_cast<float>(cross.x),
                        static_cast<float>(y));
                    *pixel = crosshairPixelAt({ cross.x, y }, at, surfaceMark);
                }
            }
        });
    }

    // Only what the cache does not hold yet is sampled: a pan keeps the part of the old cache
    // still on screen and samples the strip it uncovered.
    void PictureView::renderCache(const IntRect& region)
    {
        IntRect kept = region;
        if (m_cacheZoom != m_zoom || m_cache.empty() || !kept.intersectWith(m_cacheRect))
            kept = {};

        const int width = region.width();
        std::vector<Tap> columns(static_cast<std::size_t>(width));
        for (int i = 0; i < width; ++i)
        {
            columns[static_cast<std::size_t>(i)] = tapAt(region.left + i, m_picture->width());
        }

        Graphics::Bitmap fresh{ IntSize{ width, region.height() } };
        for (int y = region.top; y < region.bottom; ++y)
        {
            const std::ptrdiff_t row = static_cast<std::ptrdiff_t>(y - region.top);
            Color* line = fresh.data() + row * fresh.stride();
            const IntPoint at = { region.left, y };
            if (kept.empty() || y < kept.top || y >= kept.bottom)
            {
                renderRun(line, columns.data(), 0, width, at);
                continue;
            }

            const Color* old = cacheRow(y) + (kept.left - m_cacheRect.left);
            std::copy_n(old, kept.width(), line + (kept.left - region.left));
            renderRun(line, columns.data(), 0, kept.left - region.left, at);
            renderRun(line, columns.data(), kept.right - region.left, width, at);
        }

        m_cache = std::move(fresh);
        m_cacheRect = region;
        m_cacheZoom = m_zoom;
    }

    void PictureView::renderRun(Color* line, const Tap* columns, const int from, const int to,
        const IntPoint at) const
    {
        const Tap row = tapAt(at.y, m_picture->height());
        const std::ptrdiff_t stride = m_picture->stride();
        const Color* first = m_picture->data() + static_cast<std::ptrdiff_t>(row.first) * stride;
        const Color* second = m_picture->data() + static_cast<std::ptrdiff_t>(row.second) * stride;
        const bool nearest = m_zoom >= k_nearestFromZoom;
        const bool darkRow = ((at.y / k_checkerSize) & 1) != 0;
        for (int i = from; i < to; ++i)
        {
            const Tap& column = columns[i];
            Color color = nearest
                ? first[column.first]
                : blendOf(first[column.first], first[column.second], second[column.first],
                    second[column.second], column.blend, row.blend);
            if (color.alpha != 255)
            {
                // A TRANSLUCENT PIXEL IS LAID OVER SQUARES, and what leaves here is opaque: the
                // staging view the cache is copied into is premultiplied, and a straight colour
                // at partial alpha would be added to the surface rather than blended into it.
                const bool darkColumn = (((at.x + i) / k_checkerSize) & 1) != 0;
                Color square = darkRow != darkColumn ? k_checkerDark : k_checkerLight;
                square.paint_over_opaque(color);
                color = square;
            }
            line[i] = color;
        }
    }

    PictureView::Tap PictureView::tapAt(const int cachePos, const int limit) const
    {
        const float centre = (static_cast<float>(cachePos) + 0.5f) / m_zoom;
        if (m_zoom >= k_nearestFromZoom)
        {
            const int at = std::clamp(static_cast<int>(std::floor(centre)), 0, limit - 1);
            return { at, at, 0 };
        }

        // between the centres of the two pixels around the sample
        const float offset = centre - 0.5f;
        const int first = static_cast<int>(std::floor(offset));
        const int blend = static_cast<int>((offset - static_cast<float>(first)) * 256.0f);
        return {
            std::clamp(first, 0, limit - 1),
            std::clamp(first + 1, 0, limit - 1),
            std::clamp(blend, 0, 255),
        };
    }

    const Color* PictureView::cacheRow(const int y) const
    {
        return m_cache.data() + static_cast<std::ptrdiff_t>(y - m_cacheRect.top) * m_cache.stride();
    }

    Color PictureView::blendOf(const Color a, const Color b, const Color c, const Color d,
        const int tx, const int ty)
    {
        const auto mix = [](const int p, const int q, const int t) {
            return (p * (256 - t) + q * t + 128) >> 8;
        };
        const auto channel = [&](const int p, const int q, const int r, const int s) {
            return static_cast<ColorByte>(mix(mix(p, q, tx), mix(r, s, tx), ty));
        };
        return {
            channel(a.red, b.red, c.red, d.red),
            channel(a.green, b.green, c.green, d.green),
            channel(a.blue, b.blue, c.blue, d.blue),
            channel(a.alpha, b.alpha, c.alpha, d.alpha),
        };
    }

    Color PictureView::pixelClamped(const int x, const int y) const
    {
        const int column = std::clamp(x, 0, m_picture->width() - 1);
        const int row = std::clamp(y, 0, m_picture->height() - 1);
        const std::size_t stride = static_cast<std::size_t>(m_picture->stride());
        const std::size_t at = static_cast<std::size_t>(row) * stride
            + static_cast<std::size_t>(column);
        return m_picture->data()[at];
    }

    Color PictureView::crosshairPixelAt(const IntPoint devicePoint, const IntPoint pictureAt,
        const Color surfaceMark) const
    {
        const IntPoint inCache = { devicePoint.x - pictureAt.x, devicePoint.y - pictureAt.y };
        if (m_cacheZoom != m_zoom || !m_cacheRect.contains(inCache))
            return surfaceMark;

        return contrasted(cacheRow(inCache.y)[inCache.x - m_cacheRect.left]);
    }

    Color PictureView::contrasted(const Color pixel)
    {
        const auto channel = [](const ColorByte value) {
            return value < k_contrastThreshold ? k_contrastLight : k_contrastDark;
        };
        return { channel(pixel.red), channel(pixel.green), channel(pixel.blue) };
    }
}
