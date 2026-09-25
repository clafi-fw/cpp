module;
#include "Windows.Headers.h"
#include <d3d11.h>
module ClaFi.Platform.Windows.D2D;

import ClaFi.Platform.Windows.Diagnostic;
import ClaFi.Platform.Windows.Composition;
import ClaFi.Diagnostic.Options;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Windows
{
    using namespace Graphics;
    using Microsoft::WRL::ComPtr;

    static ComPtr<ID2D1Factory1> getD2DFactory()
    {
        static ComPtr<ID2D1Factory1> s_factory;
        if (!s_factory)
        {
            D2D1_FACTORY_OPTIONS options{};

            if constexpr (Diagnostic::Options::apiErrors != Diagnostic::Options::ApiErrors::Ignore)
                options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

            HRESULT hr = D2D1CreateFactory(
                D2D1_FACTORY_TYPE_SINGLE_THREADED,
                __uuidof(ID2D1Factory1),
                &options,
                reinterpret_cast<void**>(s_factory.GetAddressOf())
            );

            if (FAILED(hr))
            {
                options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
                checkHr(D2D1CreateFactory(
                    D2D1_FACTORY_TYPE_SINGLE_THREADED,
                    __uuidof(ID2D1Factory1),
                    &options,
                    reinterpret_cast<void**>(s_factory.GetAddressOf())
                ));
            }
        }
        return s_factory;
    }

    // Nests a caller's matrix inside whatever the render target is already set to: the local one
    // places the path, then the ambient one carries it to the device.
    //
    // The operand order is not interchangeable and the two matrix types will not warn about it.
    // ClaFi::Matrix3x2 and D2D1_MATRIX_3X2_F are binary compatible - deliberately so - but their
    // operator* compose in opposite orders: ClaFi's A * B applies B first, D2D's applies A
    // first. This is the D2D one, so the local matrix goes on the left.
    static D2D1_MATRIX_3X2_F nestTransform(const Matrix3x2& local, const D2D1_MATRIX_3X2_F& ambient)
    {
        return reinterpret_cast<const D2D1_MATRIX_3X2_F&>(local) * ambient;
    }

    // The fields renderingParams() actually reads. Antialiasing is a separate setting on the
    // target, and origin snapping never reaches the target at all - lumping the three together
    // would rebuild rendering params for a run that only wanted a different antialiasing mode.
    static bool sameRenderingParams(const TextRasterizationParams& first, const TextRasterizationParams& second)
    {
        return first.gridFit == second.gridFit
            && first.enhancedContrast == second.enhancedContrast
            && first.rasterizationMode == second.rasterizationMode;
    }

    std::size_t BrushHash::operator()(const Brush& brush) const
    {
        std::size_t hashValue = std::hash<std::size_t>{}(brush.index());

        auto combineFloat = [&hashValue](float value){
            std::uint32_t bits;
            std::memcpy(&bits, &value, sizeof(float));
            hashValue = hashValue * 31 + bits;
        };

        auto combinePoint = [&combineFloat](FloatPoint point){
            combineFloat(point.x);
            combineFloat(point.y);
        };

        auto combineColor = [&hashValue](Color color){
            hashValue = hashValue * 31 + std::hash<ColorAsUint>{}(color.asUint());
        };

        auto combineStops = [&combineFloat, &combineColor](const std::vector<GradientStop>& stops){
            for (const auto& stop : stops)
            {
                combineFloat(stop.position);
                combineColor(stop.color);
            }
        };

        std::visit([&](const auto& arg){
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, SolidColor>)
            {
                combineColor(arg.color);
            }
            else if constexpr (std::is_same_v<T, LinearGradient>)
            {
                combinePoint(arg.startPoint);
                combinePoint(arg.endPoint);
                combineStops(arg.stops);
            }
            else if constexpr (std::is_same_v<T, RadialGradient>)
            {
                combinePoint(arg.center);
                combinePoint(arg.offset);
                combineFloat(arg.radiusX);
                combineFloat(arg.radiusY);
                combineStops(arg.stops);
            }
            else if constexpr (std::is_same_v<T, PointGlow>)
            {
                combinePoint(arg.lightPos);
                combineColor(arg.lightColor);
                combineFloat(arg.lightSpread);
                combineFloat(arg.xRatio);
                hashValue = hashValue * 31 + static_cast<std::size_t>(arg.shape);
                combineFloat(arg.opacity);
            }
        }, brush);

        return hashValue;
    }

    // --- Direct2DBackend Implementations ---

    Direct2DBackend::Direct2DBackend(IPlatformWindow& window, IntSize size)
        :
        m_window{ &window },
        m_size{ size }
    {
    }

    void Direct2DBackend::resize(IntSize size)
    {
        if (size.x <= 0 || size.y <= 0)
        {
            return;
        }

        // Recorded before the target is consulted, because the size is the size whether or not
        // there is a device to hold it: this is what the target built next is built at.
        m_size = size;
        m_brushCache.clear();

        // The frame texture at the new size arrives with the next paint - see ensureTarget - and
        // the target bitmap, the images and the staging bitmap follow it there.
        m_images.clear();
    }

    void Direct2DBackend::pushClip(const FloatRect& exactClipRect)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        m_renderTarget->PushAxisAlignedClip(reinterpret_cast<const D2D1_RECT_F&>(exactClipRect), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        m_clipTypeStack.push_back({ ClipType::AxisAligned, nullptr });
    }

    void Direct2DBackend::pushClip(const PixelPath& path, const Matrix3x2* transform)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        ComPtr<ID2D1PathGeometry> d2dPath = buildPathGeometry(path, true);

        D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
        params.geometricMask = d2dPath.Get();
        if (transform)
        {
            // Zero-cost pointer alias mapping
            params.maskTransform = reinterpret_cast<const D2D1_MATRIX_3X2_F&>(*transform);
        }

        // Passing nullptr leaves D2D to create and manage the internal Layer resource.
        m_renderTarget->PushLayer(&params, nullptr);

        // Keep the PathGeometry mask alive in the scope until it is popped
        m_clipTypeStack.push_back({ ClipType::Layer, d2dPath });
    }

    void Direct2DBackend::popClip()
    {
        if (!m_isPainting || !m_renderTarget || m_clipTypeStack.empty())
        {
            return;
        }

        ClipState state = m_clipTypeStack.back();
        m_clipTypeStack.pop_back();

        if (state.type == ClipType::AxisAligned)
        {
            m_renderTarget->PopAxisAlignedClip();
        }
        else if (state.type == ClipType::Layer)
        {
            m_renderTarget->PopLayer();
        }
    }

    // Direct2D composites a layer's whole content at the layer's opacity when it is popped, which
    // is what this contract asks for and what the render target cannot do primitive by primitive.
    //
    // Text inside a layer is antialiased in grayscale rather than ClearType: the layer starts
    // empty, so a glyph drawn into it has no opaque background to blend its subpixel coverage
    // against. That is for the length of whatever holds the layer open.
    void Direct2DBackend::pushOpacity(float opacity)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters();
        params.opacity = opacity;

        // Passing nullptr leaves D2D to create and manage the internal Layer resource.
        m_renderTarget->PushLayer(&params, nullptr);
        ++m_opacityLayers;
    }

    void Direct2DBackend::popOpacity()
    {
        if (!m_renderTarget || !m_opacityLayers)
        {
            return;
        }

        --m_opacityLayers;
        m_renderTarget->PopLayer();
    }

    // A kept image is a bitmap render target sharing the window target's device, so drawing into
    // it is a BeginDraw of its own inside the frame's - which is what a compatible target is for.
    // Redirecting m_renderTarget is the whole of it: every primitive, and the glyph renderer
    // through getNativeRenderTarget, draws through that one pointer.
    void Direct2DBackend::pushImage(int index)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        // The transform and the text raster state belong to the TARGET, and an image is a second
        // target. Both are carried across so that what a caller has asked the surface for is what
        // the image is drawn under, and both are put back by the pop - a target keeps what it was
        // last told between one BeginDraw and the next, so an image reused on a later frame would
        // otherwise still be carrying the transform the last frame left on it.
        ImagePush push{ .previousTarget = m_renderTarget, .installedRaster = m_installedRaster };
        D2D1_MATRIX_3X2_F transform;
        m_renderTarget->GetTransform(&transform);

        if (ID2D1BitmapRenderTarget* target = image(index))
        {
            push.begun = target;
            m_renderTarget = target;
            m_renderTarget->BeginDraw();
            m_renderTarget->SetTransform(transform);
            // Put into the state m_createdRaster stands for, both halves of it, and said outright
            // rather than assumed: an image is REUSED between frames and keeps whatever the last
            // run inside it installed, so a target claimed to be at the created state has to be
            // put there. Without the second call a run asking for the created state finds
            // m_installedRaster already naming it, installs nothing, and draws through the params
            // the previous frame left on the image.
            m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            m_renderTarget->SetTextRenderingParams(nullptr);
            m_installedRaster = m_createdRaster;
        }

        m_imageStack.push_back(std::move(push));
    }

    void Direct2DBackend::popImage()
    {
        if (m_imageStack.empty())
        {
            return;
        }

        ImagePush push = m_imageStack.back();
        m_imageStack.pop_back();

        // What this EndDraw would report is that the device went while the image was being drawn,
        // and the frame's own EndDraw reports the same loss a moment later - where the whole
        // recovery already lives. Acting on it here would tear the target down inside a paint that
        // is still running.
        if (push.begun)
        {
            push.begun->EndDraw();
        }

        m_renderTarget = push.previousTarget;
        m_installedRaster = push.installedRaster;
    }

    void Direct2DBackend::drawImage(int index, float opacity)
    {
        if (!m_isPainting || !m_renderTarget || !hasImage(index))
        {
            return;
        }

        ComPtr<ID2D1Bitmap> bitmap;
        if (FAILED(m_images[static_cast<std::size_t>(index)]->GetBitmap(bitmap.GetAddressOf())))
        {
            return;
        }

        // An image is a frame rather than content placed in one, so it lands on the surface's own
        // pixels whatever transform the caller is carrying, and at one to one - there is nothing
        // for an interpolation to do.
        D2D1_MATRIX_3X2_F oldTransform;
        m_renderTarget->GetTransform(&oldTransform);
        m_renderTarget->SetTransform(D2D1::IdentityMatrix());

        D2D1_SIZE_F size = bitmap->GetSize();
        m_renderTarget->DrawBitmap(
            bitmap.Get(),
            D2D1::RectF(0.0f, 0.0f, size.width, size.height),
            opacity,
            D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
        );

        m_renderTarget->SetTransform(oldTransform);
    }

    bool Direct2DBackend::hasImage(int index) const
    {
        if (!m_deviceContext || !m_targetBitmap || index < 0 || static_cast<std::size_t>(index) >= m_images.size())
        {
            return false;
        }

        const ComPtr<ID2D1BitmapRenderTarget>& stored = m_images[static_cast<std::size_t>(index)];
        if (!stored)
        {
            return false;
        }

        D2D1_SIZE_F held = stored->GetSize();
        D2D1_SIZE_F surface = m_deviceContext->GetSize();
        return held.width == surface.width && held.height == surface.height;
    }

    // THE FRAME'S DIRTY RECT STANDS AS A CLIP for the whole frame, as it does on the CPU backend:
    // what this pass paints is that rect, and a control reaching past it must not land twice on
    // the pixels beside it. A frame that carries alpha starts that rect from nothing.
    void Direct2DBackend::beginPaint(void* nativeContext, const IntRect* dirtyRect)
    {
        CompositionPresenter* presenter = static_cast<CompositionPresenter*>(nativeContext);
        if (!presenter || !ensureTarget(*presenter))
        {
            return;
        }

        m_isPainting = true;
        // A frame begins on the window's own target whatever the last one left open. Drained
        // rather than dropped: an image abandoned inside its own BeginDraw refuses the next one.
        while (!m_imageStack.empty())
        {
            popImage();
        }
        m_renderTarget = m_deviceContext;
        m_renderTarget->BeginDraw();
        m_renderTarget->SetTransform(D2D1::IdentityMatrix());

        if (dirtyRect)
        {
            const D2D1_RECT_F clip = D2D1::RectF(
                static_cast<float>(dirtyRect->left), static_cast<float>(dirtyRect->top),
                static_cast<float>(dirtyRect->right), static_cast<float>(dirtyRect->bottom));
            m_deviceContext->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_ALIASED);
            m_baseClip = true;
        }
        if (m_transparentBase)
        {
            m_deviceContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
        }
    }

    void Direct2DBackend::endPaint(void* /* nativeContext */, const IntRect* /* dirtyRect */, Bitmap*&)
    {
        if (!m_deviceContext || !m_isPainting)
        {
            return;
        }

        // The frame's EndDraw is the window target's, so an image left open by an unbalanced push
        // is closed here rather than being ended in its place.
        while (!m_imageStack.empty())
        {
            popImage();
        }

        // Text raster state outlives the run that asked for it, so the frame puts it back once
        // rather than every run putting it back for itself.
        if (m_installedRaster.antialias != m_createdRaster.antialias)
        {
            m_renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        }
        if (!sameRenderingParams(m_installedRaster, m_createdRaster))
        {
            m_renderTarget->SetTextRenderingParams(nullptr);
        }
        m_installedRaster = m_createdRaster;

        if (m_baseClip)
        {
            m_deviceContext->PopAxisAlignedClip();
            m_baseClip = false;
        }
        HRESULT hr = m_renderTarget->EndDraw();

        m_isPainting = false;
        m_clipTypeStack.clear();
        m_opacityLayers = 0;

        // --- FRAME-LOCAL BRUSH CACHING ---
        // Resolves unbounded D2D cache growth across frames, which happens when elements shift
        // visually - animated scrolling, for instance - and every frame asks for brushes at new
        // fractional coordinates.
        m_brushCache.clear();

        // The device under this target is gone. The frame just drawn was discarded whole by
        // EndDraw, so there is nothing here to salvage - drop what the device owned and let the
        // next frame build a target again. Not an error to report: it is the one HRESULT this
        // backend is expected to answer rather than fail on.
        if (hr == D2DERR_RECREATE_TARGET)
        {
            discardTarget();
            return;
        }

        checkHr(hr); // Execute the check after cleanup state to prevent deadlock bugs
    }

    void* Direct2DBackend::getNativeBrush(const Brush& brush)
    {
        ID2D1Brush* rawBrush = getNativeBrushInternal(brush);
        if (!rawBrush)
        {
            return nullptr;
        }

        rawBrush->AddRef();
        return rawBrush;
    }

    TextAntialiasToken Direct2DBackend::beginTextRaster(const TextRasterizationParams& params)
    {
        // Read back per glyph run by the renderer, so it is recorded whether or not anything else
        // changes, and even without a render target.
        m_snapTextOrigins = params.snapOrigins;

        if (!m_renderTarget)
        {
            return {};
        }

        if (params.antialias != m_installedRaster.antialias)
        {
            m_renderTarget->SetTextAntialiasMode(params.antialias == TextAntialiasingMode::Grayscale
                ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE
                : D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            m_installedRaster.antialias = params.antialias;
        }

        if (sameRenderingParams(params, m_installedRaster))
        {
            return {};
        }

        // Built before anything is touched, so a run whose params cannot be made leaves the target
        // as it stands rather than half way to what it asked for. Null hands DirectWrite's own back,
        // which is what the created state means.
        ComPtr<IDWriteRenderingParams> rendering;
        if (!sameRenderingParams(params, m_createdRaster))
        {
            rendering = renderingParams(params);
            if (!rendering)
            {
                return {};
            }
        }

        m_renderTarget->SetTextRenderingParams(rendering.Get());
        m_installedRaster.gridFit = params.gridFit;
        m_installedRaster.enhancedContrast = params.enhancedContrast;
        m_installedRaster.rasterizationMode = params.rasterizationMode;

        return {};
    }

    void Direct2DBackend::endTextRaster(TextAntialiasToken)
    {
        // The raster state stays installed. The next run either wants it and pays nothing, or says
        // so and pays for one change - where restoring here would charge every run twice for a
        // setting the following one usually asks straight back. endPaint returns the target to the
        // state it was created in, so nothing leaks past the frame.
        m_snapTextOrigins = true;
    }

    void Direct2DBackend::setTransform(const Matrix3x2& matrix)
    {
        if (!m_renderTarget)
        {
            return;
        }

        m_renderTarget->SetTransform(reinterpret_cast<const D2D1_MATRIX_3X2_F&>(matrix));
    }

    void Direct2DBackend::resetTransform()
    {
        if (!m_renderTarget)
        {
            return;
        }

        m_renderTarget->SetTransform(D2D1::IdentityMatrix());
    }

    void Direct2DBackend::commitStagingView(const PixelView& view)
    {
        if (!m_renderTarget)
        {
            return;
        }

        D2D1_MATRIX_3X2_F oldTransform;
        m_renderTarget->GetTransform(&oldTransform);
        m_renderTarget->SetTransform(D2D1::IdentityMatrix());

        drawPixelView(view);

        m_renderTarget->SetTransform(oldTransform);
    }

    void Direct2DBackend::drawPixelView(const PixelView& view)
    {
        if (!m_isPainting || !m_renderTarget || !m_d2dBitmap)
        {
            return;
        }

        FloatRect drawRectF = view.clippedBounds();
        if (drawRectF.empty())
        {
            return;
        }

        IntRect drawRect = drawRectF.roundedOut();

        D2D1_SIZE_F targetSize = m_renderTarget->GetSize();
        drawRect.intersectWith({ 0, 0, static_cast<int>(targetSize.width), static_cast<int>(targetSize.height) });
        if (drawRect.empty())
        {
            return;
        }

        const FloatRect& bounds = view.bounds();

        int minX = static_cast<int>(std::floor(bounds.left));
        int maxX = minX + static_cast<int>(view.stride());
        int minY = static_cast<int>(std::floor(bounds.top));
        // Bounded by the allocation, the way the x side is bounded by the stride. Deriving this
        // from the bounds instead - as ceil(bottom) - floor(top) - is right only for a buffer
        // allocated from the rounded out rect. A bitmap of whole pixels drawn at a fractional y
        // spans one row more than it owns, and CopyFromMemory then reads a full stride past the
        // end of the buffer.
        int maxY = minY + view.rows();

        drawRect.left = std::max(drawRect.left, minX);
        drawRect.top = std::max(drawRect.top, minY);
        drawRect.right = std::min(drawRect.right, maxX);
        drawRect.bottom = std::min(drawRect.bottom, maxY);

        if (drawRect.left >= drawRect.right || drawRect.top >= drawRect.bottom)
        {
            return;
        }

        D2D1_RECT_U destU = D2D1::RectU(drawRect.left, drawRect.top, drawRect.right, drawRect.bottom);
        UINT32 pitch = static_cast<UINT32>(view.stride() * sizeof(Color));

        int offsetX = drawRect.left - minX;
        int offsetY = drawRect.top - minY;
        const Color* uploadStart = view.data() + (offsetY * view.stride()) + offsetX;

        HRESULT hr = m_d2dBitmap->CopyFromMemory(&destU, uploadStart, pitch);
        if (FAILED(hr))
        {
            return;
        }

        // Both rectangles are the integer region just uploaded. These pixels were rasterized at
        // device pixel positions and copied to exactly those texels, so this is a texel for texel
        // blit with nothing to interpolate.
        //
        // Passing the fractional bounds for both - which is what this did - keeps the 1:1 mapping
        // but puts the sampling window half a texel off the grid. m_d2dBitmap is shared and holds
        // whatever earlier elements uploaded into it, so the filter then reaches past the upload
        // and blends in a neighbour belonging to something else: a stray fringe that appears and
        // disappears with whatever happened to be staged nearby.
        //
        // Scaling a caller asks for still happens, through the render target transform.
        D2D1_RECT_F blitRect = D2D1::RectF(
            static_cast<float>(drawRect.left),
            static_cast<float>(drawRect.top),
            static_cast<float>(drawRect.right),
            static_cast<float>(drawRect.bottom)
        );
        m_renderTarget->DrawBitmap(
            m_d2dBitmap.Get(), &blitRect, 1.0f,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, &blitRect
        );
    }

    void Direct2DBackend::drawLine(FloatPoint pt1, FloatPoint pt2, const Brush& brush, float strokeWidth)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        ID2D1Brush* d2dBrush = getNativeBrushInternal(brush);
        if (!d2dBrush)
        {
            return;
        }

        m_renderTarget->DrawLine(reinterpret_cast<D2D_POINT_2F&>(pt1), reinterpret_cast<D2D_POINT_2F&>(pt2), d2dBrush, strokeWidth);
    }

    void Direct2DBackend::drawRectangle(const FloatRect& rect, const Brush& brush, float strokeWidth)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        ID2D1Brush* d2dBrush = getNativeBrushInternal(brush);
        if (!d2dBrush)
        {
            return;
        }

        // Direct2D strokes centred on the geometry, so the rect handed to it is the centreline
        // of a stroke whose outer edge has to land on the bounds we were given.
        FloatRect centreline = rect;
        centreline.inflate(-strokeWidth * 0.5f);
        m_renderTarget->DrawRectangle(reinterpret_cast<const D2D1_RECT_F&>(centreline), d2dBrush, strokeWidth);
    }

    void Direct2DBackend::fillRectangle(const FloatRect& rect, const Brush& brush)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        ID2D1Brush* d2dBrush = getNativeBrushInternal(brush);
        if (!d2dBrush)
        {
            return;
        }

        m_renderTarget->FillRectangle(reinterpret_cast<const D2D1_RECT_F&>(rect), d2dBrush);
    }

    void Direct2DBackend::drawRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush, float strokeWidth)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        ID2D1Brush* d2dBrush = getNativeBrushInternal(brush);
        if (!d2dBrush)
        {
            return;
        }

        // As drawRectangle: the centreline sits half a stroke inside the bounds, and its radii
        // shrink by the same amount so the outer corner keeps the radius that was asked for.
        FloatRect centreline = rect;
        centreline.inflate(-strokeWidth * 0.5f);
        float centreRadiusX = (std::max)(0.0f, radiusX - strokeWidth * 0.5f);
        float centreRadiusY = (std::max)(0.0f, radiusY - strokeWidth * 0.5f);
        m_renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(reinterpret_cast<const D2D1_RECT_F&>(centreline), centreRadiusX, centreRadiusY), d2dBrush, strokeWidth);
    }

    void Direct2DBackend::fillRoundedRectangle(const FloatRect& rect, float radiusX, float radiusY, const Brush& brush)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        ID2D1Brush* d2dBrush = getNativeBrushInternal(brush);
        if (!d2dBrush)
        {
            return;
        }

        m_renderTarget->FillRoundedRectangle(D2D1::RoundedRect(reinterpret_cast<const D2D1_RECT_F&>(rect), radiusX, radiusY), d2dBrush);
    }

    bool Direct2DBackend::fillPartialRoundedRectangle(const RoundedRectangleParts&, const Brush&)
    {
        return false;
    }

    bool Direct2DBackend::drawPartialRoundedRectangle(const RoundedRectangleParts&, const Brush&, float)
    {
        return false;
    }

    void Direct2DBackend::drawEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush, float strokeWidth)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        ID2D1Brush* d2dBrush = getNativeBrushInternal(brush);
        if (!d2dBrush)
        {
            return;
        }

        // As drawRectangle: the stroke lies inside the ellipse it was given.
        float centreRadiusX = (std::max)(0.0f, radiusX - strokeWidth * 0.5f);
        float centreRadiusY = (std::max)(0.0f, radiusY - strokeWidth * 0.5f);
        m_renderTarget->DrawEllipse(D2D1::Ellipse(reinterpret_cast<D2D_POINT_2F&>(center), centreRadiusX, centreRadiusY), d2dBrush, strokeWidth);
    }

    void Direct2DBackend::fillEllipse(FloatPoint center, float radiusX, float radiusY, const Brush& brush)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return;
        }

        ID2D1Brush* d2dBrush = getNativeBrushInternal(brush);
        if (!d2dBrush)
        {
            return;
        }

        m_renderTarget->FillEllipse(D2D1::Ellipse(reinterpret_cast<D2D_POINT_2F&>(center), radiusX, radiusY), d2dBrush);
    }

    // A SHADOW IS THE DISTANCE FIELD OF ITS SILHOUETTE - full inside, and outside it the falloff
    // of the distance to the outline, out to the reach. A rounded rectangle's distance field is
    // made of pieces a gradient states exactly. Beside a straight edge the distance grows
    // straight out, so the band of the reach's width along the edge is a linear gradient. Round
    // a corner it grows out from the arc's centre, so the square from that centre to the reach
    // past the arc is a radial gradient: full to the arc, the falloff beyond it, and the arc
    // itself drawn by the brush with no edge at all. Between them the silhouette is solid, cut
    // into rectangles the way the CPU painter cuts a rounded rectangle.
    //
    // Every piece is an axis-aligned rectangle, and every pixel falls in exactly one of them:
    // they are drawn ALIASED, so no edge pixel is shared between two pieces and painted twice at
    // the shadow's opacity, and what meets across a seam is the same value on both sides - the
    // one value the distance field has there. The pieces are the silhouette as
    // Canvas::castShadowDef states it: carried by the offset, grown by the spread, a round
    // corner growing with it, no corner past the middle of the shorter side. The brushes are
    // made for the call and let go with it, so the brush cache holds nothing of a shadow
    // whatever it does from one frame to the next.
    bool Direct2DBackend::castShadow(const RoundedRectangleParts& parts, const ShadowParams& shadow)
    {
        if (!m_isPainting || !m_renderTarget)
        {
            return false;
        }

        ID2D1Brush* solid = getNativeBrushInternal(SolidColor{ shadow.color });
        if (!solid)
        {
            return false;
        }

        FloatRect bounds = parts.bounds;
        bounds.offset(shadow.offset);
        bounds.inflate(shadow.spread);
        const float cap = (std::min)(bounds.width(), bounds.height()) / 2.0f;
        CornerRadii radii = parts.radii;
        for (float& radius : radii)
        {
            if (radius > 0.0f)
            {
                radius = std::clamp(radius + shadow.spread, 0.0f, cap);
            }
        }

        const float reach = shadow.blur;
        const float left = bounds.left;
        const float top = bounds.top;
        const float right = bounds.right;
        const float bottom = bounds.bottom;
        const float topLeft = radii[cornerIndex(Corner::TopLeft)];
        const float topRight = radii[cornerIndex(Corner::TopRight)];
        const float bottomRight = radii[cornerIndex(Corner::BottomRight)];
        const float bottomLeft = radii[cornerIndex(Corner::BottomLeft)];

        ComPtr<ID2D1GradientStopCollection> edgeRamp = shadowRamp(shadow, 0.0f, reach);
        if (!edgeRamp)
        {
            return false;
        }

        auto linearBrush = [&](FloatPoint start, FloatPoint end) -> ComPtr<ID2D1Brush> {
            ComPtr<ID2D1LinearGradientBrush> brush;
            m_renderTarget->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(
                    reinterpret_cast<const D2D1_POINT_2F&>(start),
                    reinterpret_cast<const D2D1_POINT_2F&>(end)),
                edgeRamp.Get(),
                brush.GetAddressOf());
            return brush;
        };
        auto radialBrush = [&](FloatPoint centre, float radius) -> ComPtr<ID2D1Brush> {
            ComPtr<ID2D1GradientStopCollection> ramp = shadowRamp(shadow, radius, radius + reach);
            ComPtr<ID2D1RadialGradientBrush> brush;
            if (ramp)
            {
                m_renderTarget->CreateRadialGradientBrush(
                    D2D1::RadialGradientBrushProperties(
                        reinterpret_cast<const D2D1_POINT_2F&>(centre),
                        D2D1::Point2F(0.0f, 0.0f),
                        radius + reach,
                        radius + reach),
                    ramp.Get(),
                    brush.GetAddressOf());
            }
            return brush;
        };
        auto fill = [&](const FloatRect& rect, ID2D1Brush* brush) {
            if (!rect.empty() && brush)
            {
                m_renderTarget->FillRectangle(reinterpret_cast<const D2D1_RECT_F&>(rect), brush);
            }
        };

        const D2D1_ANTIALIAS_MODE restoreMode = m_renderTarget->GetAntialiasMode();
        m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

        // The solid: a band across the middle, a strip between the two corners of the top edge
        // and one of the bottom edge, and under the shorter corner of an edge the strip its
        // neighbour stands taller by.
        const float topBand = top + (std::max)(topLeft, topRight);
        const float bottomBand = bottom - (std::max)(bottomRight, bottomLeft);
        fill({ left, topBand, right, bottomBand }, solid);
        fill({ left + topLeft, top, right - topRight, topBand }, solid);
        fill({ left + bottomLeft, bottomBand, right - bottomRight, bottom }, solid);
        fill({ left, top + topLeft, left + topLeft, topBand }, solid);
        fill({ right - topRight, top + topRight, right, topBand }, solid);
        fill({ left, bottomBand, left + bottomLeft, bottom - bottomLeft }, solid);
        fill({ right - bottomRight, bottomBand, right, bottom - bottomRight }, solid);

        // The bands, each between the arcs of the two corners on its edge.
        fill({ left + topLeft, top - reach, right - topRight, top },
            linearBrush({ left, top }, { left, top - reach }).Get());
        fill({ left + bottomLeft, bottom, right - bottomRight, bottom + reach },
            linearBrush({ left, bottom }, { left, bottom + reach }).Get());
        fill({ left - reach, top + topLeft, left, bottom - bottomLeft },
            linearBrush({ left, top }, { left - reach, top }).Get());
        fill({ right, top + topRight, right + reach, bottom - bottomRight },
            linearBrush({ right, top }, { right + reach, top }).Get());

        // The corners, each the square from its arc's centre out to the reach past the arc.
        fill({ left - reach, top - reach, left + topLeft, top + topLeft },
            radialBrush({ left + topLeft, top + topLeft }, topLeft).Get());
        fill({ right - topRight, top - reach, right + reach, top + topRight },
            radialBrush({ right - topRight, top + topRight }, topRight).Get());
        fill({ right - bottomRight, bottom - bottomRight, right + reach, bottom + reach },
            radialBrush({ right - bottomRight, bottom - bottomRight }, bottomRight).Get());
        fill({ left - reach, bottom - bottomLeft, left + bottomLeft, bottom + reach },
            radialBrush({ left + bottomLeft, bottom - bottomLeft }, bottomLeft).Get());

        m_renderTarget->SetAntialiasMode(restoreMode);
        return true;
    }

    void Direct2DBackend::fillPath(const PixelPath& path, const Brush& brush, const Matrix3x2* transform)
    {
        if (!m_isPainting || !m_renderTarget || path.isEmpty())
        {
            return;
        }

        ComPtr<ID2D1PathGeometry> d2dPath = buildPathGeometry(path, true);
        ID2D1Brush* d2dBrush = getNativeBrushInternal(brush);
        if (!d2dBrush)
        {
            return;
        }

        D2D1_MATRIX_3X2_F oldTransform;
        m_renderTarget->GetTransform(&oldTransform);

        // Apply local transform to the Render Target instead of the geometry
        // to correctly scale and align brush coordinates (like linear gradients) [CP]
        if (transform)
        {
            m_renderTarget->SetTransform(nestTransform(*transform, oldTransform));
        }

        m_renderTarget->FillGeometry(d2dPath.Get(), d2dBrush);

        // Restore target transform state
        if (transform)
        {
            m_renderTarget->SetTransform(oldTransform);
        }
    }

    void Direct2DBackend::drawPath(const PixelPath& path, std::span<const PathDrawLayer> layers, const Matrix3x2* transform)
    {
        // Simple multi-pass delegation on D2D (automatically GPU-parallelized) [CP]
        for (const auto& layer : layers)
        {
            if (layer.geometry.mode == PathRenderMode::Fill)
            {
                fillPath(path, layer.brush, transform);
            }
            else if (layer.geometry.mode == PathRenderMode::Stroke)
            {
                drawPathWithBrush(path, layer.brush, layer.geometry.strokeWidth, transform);
            }
            else if (layer.geometry.mode == PathRenderMode::OuterGlow)
            {
                if (!m_isPainting || !m_renderTarget || path.isEmpty())
                {
                    continue;
                }

                ComPtr<ID2D1PathGeometry> d2dPath = buildPathGeometry(path, false);
                if (!d2dPath)
                {
                    continue;
                }

                // The transform goes on the target rather than on the geometry, for the reason
                // the plain stroke states above: it is what scales the stroke widths the ramp is
                // built out of.
                D2D1_MATRIX_3X2_F oldTransform;
                m_renderTarget->GetTransform(&oldTransform);
                if (transform)
                {
                    m_renderTarget->SetTransform(nestTransform(*transform, oldTransform));
                }

                strokeGlow(*d2dPath.Get(), layer, layer.brush);

                if (transform)
                {
                    m_renderTarget->SetTransform(oldTransform);
                }
            }
            else if (layer.geometry.mode == PathRenderMode::Shadow)
            {
                if (!m_isPainting || !m_renderTarget || path.isEmpty())
                {
                    continue;
                }

                // Filled figures: the same geometry is stroked for the ramp and filled for the
                // silhouette.
                ComPtr<ID2D1PathGeometry> d2dPath = buildPathGeometry(path, true);
                if (!d2dPath)
                {
                    continue;
                }

                D2D1_MATRIX_3X2_F oldTransform;
                m_renderTarget->GetTransform(&oldTransform);
                if (transform)
                {
                    m_renderTarget->SetTransform(nestTransform(*transform, oldTransform));
                }

                fillShadow(*d2dPath.Get(), layer);

                if (transform)
                {
                    m_renderTarget->SetTransform(oldTransform);
                }
            }
        }
    }

    void Direct2DBackend::drawPathWithBrush(const PixelPath& path, const Brush& brush, float strokeWidth,
        const Matrix3x2* transform)
    {
        if (!m_isPainting || !m_renderTarget || path.isEmpty())
        {
            return;
        }

        const ComPtr<ID2D1PathGeometry> d2DPath = buildPathGeometry(path, false);
        ID2D1Brush* d2DBrush = getNativeBrushInternal(brush);
        if (!d2DBrush)
        {
            return;
        }

        D2D1_MATRIX_3X2_F oldTransform;
        m_renderTarget->GetTransform(&oldTransform);

        // Apply local transform to the Render Target instead of the geometry
        // to correctly scale the strokeWidth and the brush coordinates [CP]
        if (transform)
        {
            m_renderTarget->SetTransform(nestTransform(*transform, oldTransform));
        }

        m_renderTarget->DrawGeometry(d2DPath.Get(), d2DBrush, strokeWidth);

        // Restore target transform state
        if (transform)
        {
            m_renderTarget->SetTransform(oldTransform);
        }
    }

    // Ring i covers the band between reach*(i-1)/n and reach*i/n. A stroke straddles the geometry
    // it follows, so a ring reaching d outwards is 2d wide and every ring lies inside the one
    // before it. They are drawn outside in, each over the last, so what a ring is given is what
    // it has to ADD to leave its share of the ramp standing - which is what ringOpacity answers.
    //
    // The opacity is set on the brush and put back afterwards rather than a brush being built per
    // ring: the brush cache is keyed on the whole brush and never evicts, so a glow whose colour
    // rides an animation would leave a new entry in it every frame.
    void Direct2DBackend::strokeGlow(ID2D1Geometry& geometry, const PathDrawLayer& layer, const Brush& brush)
    {
        const float reach = layer.geometry.strokeWidth;
        if (reach <= 0.0f)
        {
            return;
        }

        ID2D1Brush* d2dBrush = getNativeBrushInternal(brush);
        if (!d2dBrush)
        {
            return;
        }

        // One ring per pixel of reach. Coarser and the steps show; finer and consecutive rings
        // land on the same pixel, paying for a stroke that changes nothing.
        const int ringCount = std::max(1, static_cast<int>(std::ceil(reach)));
        const float bandWidth = reach / static_cast<float>(ringCount);
        const FLOAT restoreOpacity = d2dBrush->GetOpacity();

        // A solid colour carries its alpha in the brush's colour, so a ring at full opacity covers
        // that much of a pixel and no more; the shares are sized for it.
        float coverage = restoreOpacity;
        if (const SolidColor* solid = std::get_if<SolidColor>(&brush))
        {
            coverage *= static_cast<float>(solid->color.alpha) / 255.0f;
        }

        float laid = 0.0f;
        for (int i = ringCount; i > 0; --i)
        {
            const float inner = static_cast<float>(i - 1) / static_cast<float>(ringCount);
            const float target = falloffAt(layer.geometry.falloff, inner);
            const float share = ringOpacity(target, laid, coverage);
            laid = target;
            if (share <= 0.0f)
            {
                continue;
            }
            d2dBrush->SetOpacity(restoreOpacity * share);
            m_renderTarget->DrawGeometry(&geometry, d2dBrush, bandWidth * static_cast<float>(i) * 2.0f);
        }

        d2dBrush->SetOpacity(restoreOpacity);
    }

    // THE RAMP IS KEPT OUTSIDE THE SILHOUETTE by a layer whose mask is everything around it but
    // it, and the silhouette is then filled solid. Both are rasterized aliased: the mask's edge
    // and the fill's are then the same pixels, and what meets across them is the ramp's first
    // step against the full colour, which is no step the eye can find. Antialiased, each side
    // would leave the other a share of every edge pixel, and the seam would show lighter.
    void Direct2DBackend::fillShadow(ID2D1Geometry& silhouette, const PathDrawLayer& layer)
    {
        ID2D1Brush* d2dBrush = getNativeBrushInternal(layer.brush);
        if (!d2dBrush)
        {
            return;
        }

        D2D1_RECT_F bounds{};
        if (FAILED(silhouette.GetBounds(nullptr, &bounds)))
        {
            return;
        }
        // A stroke reaching the ramp's length out from the outline, and a pixel to spare.
        const float around = layer.geometry.strokeWidth + 1.0f;

        ComPtr<ID2D1Factory> factory = getD2DFactory();
        ComPtr<ID2D1RectangleGeometry> surround;
        ComPtr<ID2D1PathGeometry> outside;
        ComPtr<ID2D1GeometrySink> sink;
        HRESULT hr = factory->CreateRectangleGeometry(
            D2D1::RectF(bounds.left - around, bounds.top - around, bounds.right + around, bounds.bottom + around),
            surround.GetAddressOf());
        if (SUCCEEDED(hr))
        {
            hr = factory->CreatePathGeometry(outside.GetAddressOf());
        }
        if (SUCCEEDED(hr))
        {
            hr = outside->Open(sink.GetAddressOf());
        }
        if (SUCCEEDED(hr))
        {
            hr = surround->CombineWithGeometry(&silhouette, D2D1_COMBINE_MODE_EXCLUDE, nullptr, sink.Get());
            const HRESULT closed = sink->Close();
            if (SUCCEEDED(hr))
            {
                hr = closed;
            }
        }
        if (FAILED(hr))
        {
            return;
        }

        D2D1_LAYER_PARAMETERS params = D2D1::LayerParameters(D2D1::InfiniteRect(), outside.Get(),
            D2D1_ANTIALIAS_MODE_ALIASED);
        m_renderTarget->PushLayer(&params, nullptr);
        strokeGlow(silhouette, layer, layer.brush);
        m_renderTarget->PopLayer();

        const D2D1_ANTIALIAS_MODE restoreMode = m_renderTarget->GetAntialiasMode();
        m_renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
        m_renderTarget->FillGeometry(&silhouette, d2dBrush);
        m_renderTarget->SetAntialiasMode(restoreMode);
    }

    // Sixteen steps of the falloff, which no reach a theme states can tell from the curve; the
    // shadow's own opacity is the top of the ramp.
    ComPtr<ID2D1GradientStopCollection> Direct2DBackend::shadowRamp(const ShadowParams& shadow, float from, float extent)
    {
        constexpr int k_steps = 16;
        if (extent <= 0.0f)
        {
            return nullptr;
        }

        const float opacity = static_cast<float>(shadow.color.alpha) / 255.0f;
        std::vector<GradientStop> stops;
        stops.reserve(k_steps + 2);
        if (from > 0.0f)
        {
            stops.push_back({ 0.0f, shadow.color });
        }
        const float reach = extent - from;
        for (int step = 0; step <= k_steps; ++step)
        {
            const float t = static_cast<float>(step) / k_steps;
            stops.push_back({
                (from + t * reach) / extent,
                shadow.color.withOpacity(opacity * falloffAt(shadow.falloff, t))
            });
        }
        return createStopCollection(stops);
    }

    // A DEVICE CAN GO AWAY under a running process - a driver reset, a driver update, a switch
    // between the GPUs of a hybrid machine, a session moving to or from a remote desktop - and
    // Direct2D reports it once, from EndDraw. The presenter counts its devices; a Direct2D device
    // built on one that has gone is dropped here and built on the next. The frame texture moves
    // on its own as well - every resize is a new one - and the target bitmap follows it.
    //
    // Called at the top of every frame rather than only after a loss, so the one path covers a
    // backend that never got a target at all.
    bool Direct2DBackend::ensureTarget(CompositionPresenter& presenter)
    {
        // The presenter's device is asked for before the generations are compared: a device that
        // went during the last present is built again here, and it is the new one this frame's
        // target has to stand on.
        ID3D11Device* d3dDevice = presenter.d3dDevice();
        if (!d3dDevice)
        {
            return false;
        }
        if (m_deviceContext && m_generation != presenter.generation())
        {
            discardTarget();
        }

        if (!m_deviceContext)
        {
            ComPtr<ID2D1Factory1> factory = getD2DFactory();
            if (!factory)
            {
                return false;
            }

            ComPtr<IDXGIDevice> dxgiDevice;
            HRESULT hr = d3dDevice->QueryInterface(__uuidof(IDXGIDevice),
                reinterpret_cast<void**>(dxgiDevice.GetAddressOf()));
            if (SUCCEEDED(hr))
            {
                hr = factory->CreateDevice(dxgiDevice.Get(), m_d2dDevice.GetAddressOf());
            }
            if (SUCCEEDED(hr))
            {
                hr = m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                    m_deviceContext.GetAddressOf());
            }
            if (FAILED(hr))
            {
                m_deviceContext.Reset();
                m_d2dDevice.Reset();
                return false;
            }
            m_generation = presenter.generation();

            // Left at the system's own rendering params, which is what a target that is asked for
            // nothing in particular should draw with. This is the state m_createdRaster stands
            // for, and the one endPaint returns to.
            m_deviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            m_installedRaster = m_createdRaster;
        }
        // Nothing is open at the top of a frame, so the target being drawn into is the window's.
        m_renderTarget = m_deviceContext;

        IDXGISurface* surface = presenter.renderSurface(m_size);
        if (!surface)
        {
            return false;
        }
        if (surface != m_targetSurface || !m_targetBitmap)
        {
            m_targetBitmap.Reset();
            m_targetSurface = nullptr;
            const D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
                96.0f, 96.0f);
            if (FAILED(m_deviceContext->CreateBitmapFromDxgiSurface(surface, &properties,
                m_targetBitmap.GetAddressOf())))
            {
                m_targetBitmap.Reset();
                return false;
            }
            m_deviceContext->SetTarget(m_targetBitmap.Get());
            m_targetSurface = surface;

            // An image is the size of the surface it was drawn for, so none of them means anything
            // on a new one. A new frame texture holds nothing either, and the paint that meets one
            // is whole already: a size change is, and so is the paint after a device loss.
            m_images.clear();
            createStagingBitmap();
        }
        return true;
    }

    void Direct2DBackend::discardTarget()
    {
        m_brushCache.clear();
        m_clipTypeStack.clear();
        m_opacityLayers = 0;
        m_imageStack.clear();
        // The images belong to the device that is going, so a caller holding one is told it has
        // gone by hasImage and builds it again.
        m_images.clear();
        m_d2dBitmap.Reset();
        m_renderTarget.Reset();
        m_targetBitmap.Reset();
        m_targetSurface = nullptr;
        m_deviceContext.Reset();
        m_d2dDevice.Reset();
        m_isPainting = false;
        m_baseClip = false;

        // The rendering params cache is kept: those are DirectWrite objects describing how to
        // rasterize, and no device holds them.

        // Asks for the frame that will attempt the rebuild. It is the request that starts the
        // recovery, not the one that guarantees a whole repaint - ensureTarget does that, for the
        // frame that actually succeeds.
        m_window->invalidateRect(nullptr);
    }

    void Direct2DBackend::createStagingBitmap()
    {
        if (!m_renderTarget || m_size.x <= 0 || m_size.y <= 0)
        {
            m_d2dBitmap.Reset();
            return;
        }

        // A bitmap already at this size is on the current device - the only thing that releases it
        // is a size change or the loss of the device that owns it - so it is kept. Building it
        // unconditionally would allocate a second full surface every time the canvas states a size
        // it already holds, which is what it does once per form at construction.
        if (m_d2dBitmap)
        {
            D2D1_SIZE_U held = m_d2dBitmap->GetPixelSize();
            if (held.width == static_cast<UINT32>(m_size.x) && held.height == static_cast<UINT32>(m_size.y))
            {
                return;
            }
        }

        m_d2dBitmap.Reset();

        // Reports rather than throws: a device part way through going or coming back fails here,
        // and this runs inside the frame that is rebuilding. Callers guard on a null bitmap, and
        // EndDraw is what states the target has to be built again.
        D2D1_BITMAP_PROPERTIES props = { { DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }, 96.0f, 96.0f };
        HRESULT hr = m_renderTarget->CreateBitmap(D2D1::SizeU(static_cast<UINT32>(m_size.x), static_cast<UINT32>(m_size.y)), nullptr, 0, &props, m_d2dBitmap.GetAddressOf());
        if (FAILED(hr))
        {
            m_d2dBitmap.Reset();
        }
    }

    ID2D1BitmapRenderTarget* Direct2DBackend::image(int index)
    {
        if (index < 0 || !m_deviceContext || !m_targetBitmap)
        {
            return nullptr;
        }

        const std::size_t slot = static_cast<std::size_t>(index);
        while (m_images.size() <= slot)
        {
            m_images.emplace_back();
        }

        if (hasImage(index))
        {
            return m_images[slot].Get();
        }

        // Reports rather than throws: a device part way through going or coming back fails here,
        // and the caller draws to the surface instead of losing what it was about to draw.
        ComPtr<ID2D1BitmapRenderTarget>& stored = m_images[slot];
        stored.Reset();
        if (FAILED(m_deviceContext->CreateCompatibleRenderTarget(m_deviceContext->GetSize(), stored.GetAddressOf())))
        {
            stored.Reset();
            return nullptr;
        }

        return stored.Get();
    }

    ID2D1Brush* Direct2DBackend::getNativeBrushInternal(const Brush& brush)
    {
        if (!m_renderTarget)
        {
            return nullptr;
        }

        // 1. Lookup in Unified Cache
        auto it = m_brushCache.find(brush);
        if (it != m_brushCache.end())
        {
            return it->second.Get();
        }

        // 2. Resource Allocation on cache-miss with safety guards
        ComPtr<ID2D1Brush> nativeBrush;
        std::visit([&](const auto& arg){
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, SolidColor>)
            {
                ComPtr<ID2D1SolidColorBrush> solidBrush;
                HRESULT hr = m_renderTarget->CreateSolidColorBrush(
                    D2D1::ColorF(arg.color.asUint() & 0xFFFFFF, static_cast<FLOAT>(arg.color.alpha) / 255.0f),
                    solidBrush.GetAddressOf()
                );
                if (SUCCEEDED(hr) && solidBrush)
                {
                    solidBrush->QueryInterface(IID_PPV_ARGS(nativeBrush.GetAddressOf()));
                }
            }
            else if constexpr (std::is_same_v<T, LinearGradient>)
            {
                ComPtr<ID2D1GradientStopCollection> stopCollection = createStopCollection(arg.stops);
                if (stopCollection)
                {
                    ComPtr<ID2D1LinearGradientBrush> linearBrush;
                    HRESULT hr = m_renderTarget->CreateLinearGradientBrush(
                        D2D1::LinearGradientBrushProperties(reinterpret_cast<const D2D1_POINT_2F&>(arg.startPoint), reinterpret_cast<const D2D1_POINT_2F&>(arg.endPoint)),
                        stopCollection.Get(),
                        linearBrush.GetAddressOf()
                    );
                    if (SUCCEEDED(hr) && linearBrush)
                    {
                        linearBrush->QueryInterface(IID_PPV_ARGS(nativeBrush.GetAddressOf()));
                    }
                }
            }
            else if constexpr (std::is_same_v<T, RadialGradient>)
            {
                ComPtr<ID2D1GradientStopCollection> stopCollection = createStopCollection(arg.stops);
                if (stopCollection)
                {
                    ComPtr<ID2D1RadialGradientBrush> radialBrush;
                    HRESULT hr = m_renderTarget->CreateRadialGradientBrush(
                        D2D1::RadialGradientBrushProperties(reinterpret_cast<const D2D1_POINT_2F&>(arg.center), reinterpret_cast<const D2D1_POINT_2F&>(arg.offset), arg.radiusX, arg.radiusY),
                        stopCollection.Get(),
                        radialBrush.GetAddressOf()
                    );
                    if (SUCCEEDED(hr) && radialBrush)
                    {
                        radialBrush->QueryInterface(IID_PPV_ARGS(nativeBrush.GetAddressOf()));
                    }
                }
            }
            else if constexpr (std::is_same_v<T, PointGlow>)
            {
                std::vector<GradientStop> stops = { { 0.0f, arg.lightColor }, { 1.0f, arg.lightColor.withOpacity(0.0f) } };
                ComPtr<ID2D1GradientStopCollection> stopCollection = createStopCollection(stops);
                if (stopCollection)
                {
                    // Corrected Math: horizontal radius stretches by dividing by xRatio [CP]
                    float radiusX = arg.lightSpread / (arg.xRatio > 0.0f ? arg.xRatio : 1.0f);
                    float radiusY = arg.lightSpread;

                    ComPtr<ID2D1RadialGradientBrush> radialBrush;
                    HRESULT hr = m_renderTarget->CreateRadialGradientBrush(
                        D2D1::RadialGradientBrushProperties(
                            reinterpret_cast<const D2D1_POINT_2F&>(arg.lightPos),
                            D2D1_POINT_2F{ 0.0f, 0.0f },
                            radiusX,
                            radiusY
                        ),
                        stopCollection.Get(),
                        radialBrush.GetAddressOf()
                    );
                    if (SUCCEEDED(hr) && radialBrush)
                    {
                        radialBrush->QueryInterface(IID_PPV_ARGS(nativeBrush.GetAddressOf()));
                    }
                }
            }
        }, brush);

        if (!nativeBrush)
        {
            return nullptr;
        }

        m_brushCache[brush] = nativeBrush;
        return nativeBrush.Get();
    }

    ComPtr<ID2D1GradientStopCollection> Direct2DBackend::createStopCollection(const std::vector<GradientStop>& stops)
    {
        std::vector<D2D1_GRADIENT_STOP> d2dStops;
        d2dStops.reserve(stops.size());
        for (const auto& stop : stops)
        {
            d2dStops.push_back({ stop.position, D2D1::ColorF(stop.color.asUint() & 0xFFFFFF, static_cast<FLOAT>(stop.color.alpha) / 255.0f) });
        }

        ComPtr<ID2D1GradientStopCollection> collection;
        checkHr(m_renderTarget->CreateGradientStopCollection(d2dStops.data(), static_cast<UINT32>(d2dStops.size()), D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, collection.GetAddressOf()));
        return collection;
    }

    ComPtr<ID2D1PathGeometry> Direct2DBackend::buildPathGeometry(const PixelPath& path, bool isFilled)
    {
        ComPtr<ID2D1Factory> factory = getD2DFactory();

        ComPtr<ID2D1PathGeometry> d2dPath;
        checkHr(factory->CreatePathGeometry(d2dPath.GetAddressOf()));

        ComPtr<ID2D1GeometrySink> sink;
        checkHr(d2dPath->Open(sink.GetAddressOf()));

        bool figureOpen = false;
        FloatPoint currentPoint = { 0.0f, 0.0f };
        FloatPoint subpathStart = { 0.0f, 0.0f };

        auto beginFigure = [&](FloatPoint pt){
            if (figureOpen)
            {
                sink->EndFigure(D2D1_FIGURE_END_OPEN);
            }
            sink->BeginFigure(reinterpret_cast<const D2D1_POINT_2F&>(pt), isFilled ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
            figureOpen = true;
            currentPoint = pt;
            subpathStart = pt;
        };

        for (const auto& cmd : path.commands())
        {
            switch (cmd.type)
            {
                case PathCommandType::MoveTo:
                    beginFigure(cmd.p1);
                    break;

                case PathCommandType::MoveBy:
                {
                    FloatPoint absolute = { currentPoint.x + cmd.p1.x, currentPoint.y + cmd.p1.y };
                    beginFigure(absolute);
                    break;
                }

                case PathCommandType::LineTo:
                    if (figureOpen)
                    {
                        sink->AddLine(reinterpret_cast<const D2D1_POINT_2F&>(cmd.p1));
                        currentPoint = cmd.p1;
                    }
                    break;

                case PathCommandType::LineBy:
                    if (figureOpen)
                    {
                        FloatPoint absolute = { currentPoint.x + cmd.p1.x, currentPoint.y + cmd.p1.y };
                        sink->AddLine(reinterpret_cast<const D2D1_POINT_2F&>(absolute));
                        currentPoint = absolute;
                    }
                    break;

                case PathCommandType::HLineTo:
                    if (figureOpen)
                    {
                        FloatPoint absolute = { cmd.p1.x, currentPoint.y };
                        sink->AddLine(reinterpret_cast<const D2D1_POINT_2F&>(absolute));
                        currentPoint = absolute;
                    }
                    break;

                case PathCommandType::HLineBy:
                    if (figureOpen)
                    {
                        FloatPoint absolute = { currentPoint.x + cmd.p1.x, currentPoint.y };
                        sink->AddLine(reinterpret_cast<const D2D1_POINT_2F&>(absolute));
                        currentPoint = absolute;
                    }
                    break;

                case PathCommandType::VLineTo:
                    if (figureOpen)
                    {
                        FloatPoint absolute = { currentPoint.x, cmd.p1.y };
                        sink->AddLine(reinterpret_cast<const D2D1_POINT_2F&>(absolute));
                        currentPoint = absolute;
                    }
                    break;

                case PathCommandType::VLineBy:
                    if (figureOpen)
                    {
                        FloatPoint absolute = { currentPoint.x, currentPoint.y + cmd.p1.y };
                        sink->AddLine(reinterpret_cast<const D2D1_POINT_2F&>(absolute));
                        currentPoint = absolute;
                    }
                    break;

                case PathCommandType::QuadTo:
                    if (figureOpen)
                    {
                        D2D1_QUADRATIC_BEZIER_SEGMENT quad = {
                            reinterpret_cast<const D2D1_POINT_2F&>(cmd.p1),
                            reinterpret_cast<const D2D1_POINT_2F&>(cmd.p2)
                        };
                        sink->AddQuadraticBezier(&quad);
                        currentPoint = cmd.p2;
                    }
                    break;

                case PathCommandType::QuadBy:
                    if (figureOpen)
                    {
                        FloatPoint control = { currentPoint.x + cmd.p1.x, currentPoint.y + cmd.p1.y };
                        FloatPoint end = { currentPoint.x + cmd.p2.x, currentPoint.y + cmd.p2.y };
                        D2D1_QUADRATIC_BEZIER_SEGMENT quad = {
                            reinterpret_cast<const D2D1_POINT_2F&>(control),
                            reinterpret_cast<const D2D1_POINT_2F&>(end)
                        };
                        sink->AddQuadraticBezier(&quad);
                        currentPoint = end;
                    }
                    break;

                case PathCommandType::CubicTo:
                    if (figureOpen)
                    {
                        D2D1_BEZIER_SEGMENT cubic = {
                            reinterpret_cast<const D2D1_POINT_2F&>(cmd.p1),
                            reinterpret_cast<const D2D1_POINT_2F&>(cmd.p2),
                            reinterpret_cast<const D2D1_POINT_2F&>(cmd.p3)
                        };
                        sink->AddBezier(&cubic);
                        currentPoint = cmd.p3;
                    }
                    break;

                case PathCommandType::CubicBy:
                    if (figureOpen)
                    {
                        FloatPoint firstControl = { currentPoint.x + cmd.p1.x, currentPoint.y + cmd.p1.y };
                        FloatPoint secondControl = { currentPoint.x + cmd.p2.x, currentPoint.y + cmd.p2.y };
                        FloatPoint end = { currentPoint.x + cmd.p3.x, currentPoint.y + cmd.p3.y };
                        D2D1_BEZIER_SEGMENT cubic = {
                            reinterpret_cast<const D2D1_POINT_2F&>(firstControl),
                            reinterpret_cast<const D2D1_POINT_2F&>(secondControl),
                            reinterpret_cast<const D2D1_POINT_2F&>(end)
                        };
                        sink->AddBezier(&cubic);
                        currentPoint = end;
                    }
                    break;

                case PathCommandType::Close:
                    if (figureOpen)
                    {
                        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                        figureOpen = false;
                        currentPoint = subpathStart;
                    }
                    break;
            }
        }

        if (figureOpen)
        {
            sink->EndFigure(D2D1_FIGURE_END_OPEN);
        }

        checkHr(sink->Close());
        return d2dPath;
    }

    // Translates one set of raster params into DirectWrite's own, and keeps the result.
    //
    // Each flag maps to a setting that decides how much of a glyph's raster depends on where it
    // happens to sit. Grid fitting pulls the outlines onto the pixel grid for whatever scale the
    // run is at, so runs animating at their own rates each cross their own threshold at their own
    // moment. Enhanced contrast boosts coverage non linearly, which turns any change in coverage
    // into a larger change in ink. The rasterizer is the deepest of the three: the cached path
    // rebuilds the glyph at every sub pixel position it is asked for, while OUTLINE fills the
    // beziers as geometry with plain antialiasing that varies smoothly with position.
    //
    // OUTLINE is not cheap. It bypasses the glyph cache, so every glyph is rebuilt from beziers
    // every frame - measured at roughly 11ms of a 14ms frame on a list of cards, against 3ms with
    // text skipped entirely, back when every interactive control was given it whether or not it was
    // moving. Which runs are worth that is not decided here.
    //
    // Gamma and pixel geometry are still the user's. If any of this is unavailable the caller is
    // handed nothing and the target is left exactly as it was.
    ComPtr<IDWriteRenderingParams> Direct2DBackend::renderingParams(const TextRasterizationParams& params)
    {
        for (const CachedRenderingParams& cached : m_renderingParamsCache)
        {
            if (cached.key == params)
            {
                return cached.value;
            }
        }

        ComPtr<IDWriteRenderingParams> value = makeRenderingParams(
            params.rasterizationMode == TextRasterizationMode::Outline
                ? DWRITE_RENDERING_MODE1_OUTLINE
                : DWRITE_RENDERING_MODE1_NATURAL_SYMMETRIC,
            params.gridFit ? DWRITE_GRID_FIT_MODE_DEFAULT : DWRITE_GRID_FIT_MODE_DISABLED,
            !params.enhancedContrast);

        m_renderingParamsCache.push_back({ .key = params, .value = value });
        return value;
    }

    ComPtr<IDWriteRenderingParams> Direct2DBackend::makeRenderingParams(
        DWRITE_RENDERING_MODE1 renderingMode, DWRITE_GRID_FIT_MODE gridFit, bool zeroContrast)
    {
        ComPtr<IDWriteFactory3> factory;
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3),
            reinterpret_cast<IUnknown**>(factory.GetAddressOf()))))
        {
            return nullptr;
        }

        ComPtr<IDWriteRenderingParams> defaults;
        if (FAILED(factory->CreateRenderingParams(defaults.GetAddressOf())))
        {
            return nullptr;
        }

        float contrast = defaults->GetEnhancedContrast();
        float grayscaleContrast = contrast;

        ComPtr<IDWriteRenderingParams2> defaults2;
        if (SUCCEEDED(defaults.As(&defaults2)))
        {
            grayscaleContrast = defaults2->GetGrayscaleEnhancedContrast();
        }

        if (zeroContrast)
        {
            contrast = 0.0f;
            grayscaleContrast = 0.0f;
        }

        ComPtr<IDWriteRenderingParams3> params;
        if (FAILED(factory->CreateCustomRenderingParams(
            defaults->GetGamma(),
            contrast,
            grayscaleContrast,
            defaults->GetClearTypeLevel(),
            defaults->GetPixelGeometry(),
            renderingMode,
            gridFit,
            params.GetAddressOf())))
        {
            return nullptr;
        }

        return params;
    }
}
