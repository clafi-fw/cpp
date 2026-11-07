module ClaFi.PathArt.SceneLayer;

import ClaFi.PathArt.Scene;
import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Cpu.Canvas;
import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // The CPU rasterizer's clip as it stands, put back when this goes.
        class HeldRasterClip
        {
        public:
            HeldRasterClip() = default;
            HeldRasterClip(const HeldRasterClip&) = delete;
            HeldRasterClip& operator=(const HeldRasterClip&) = delete;
            ~HeldRasterClip();
        private:
            bool m_active{ Cpu::g_rasterBuffers.hasActiveClip };
            const float* m_mask{ Cpu::g_rasterBuffers.activeClipMaskPtr };
            FloatRect m_box{ Cpu::g_rasterBuffers.clipBox };
            int m_stride{ Cpu::g_rasterBuffers.m_clipStride };
        };

        // The smallest rect holding every pixel of the bitmap that is not clear.
        [[nodiscard]] IntRect drawnArea(const Bitmap&);
        // The pixels of `area` copied out of the bitmap.
        [[nodiscard]] Bitmap copied(const Bitmap&, const IntRect& area);
    }

    // SceneLayer

    SceneLayer::SceneLayer(const Drawing drawing)
        :
        m_drawing{ drawing }
    {
    }

    // The CPU rasterizer's clip is one state for every canvas, and this may run in the middle of
    // a paint, so the clip that paint stated is held across the drawing.
    void SceneLayer::prepare(const SceneParams& params)
    {
        if (params.backend != BackendType::Cpu)
        {
            m_kept = false;
            m_pixels = {};
            return;
        }

        const Matrix3x2 sceneToPixels = params.transform
            * Matrix3x2::Translation(params.bounds.topLeft());
        const FloatPoint frameSize = { params.bounds.width(), params.bounds.height() };
        if (m_kept && sceneToPixels == m_sceneToPixels && frameSize == m_frameSize)
            return;

        const IntRect scene = params.transform.mapRect(params.bounds).roundedOut();
        if (scene.empty())
        {
            m_kept = false;
            m_pixels = {};
            return;
        }

        const HeldRasterClip heldClip{};
        auto backend = std::make_unique<Cpu::CpuBackend>(IntSize{ scene.width(), scene.height() });
        backend->setTransparentBase(true);
        Canvas canvas{ std::move(backend) };
        const IntRect whole = { 0, 0, scene.width(), scene.height() };
        canvas.beginPaint(nullptr, whole);
        const FloatPoint sceneCorner = scene.topLeft().toFloat();
        canvas.setTransform(Matrix3x2::Translation(-sceneCorner.x, -sceneCorner.y)
            * params.transform);
        m_drawing(SceneFrame::FromBounds(canvas, params.bounds));
        Bitmap* drawn = nullptr;
        canvas.endPaint(nullptr, whole, drawn);

        const IntRect area = drawnArea(*drawn);
        if (area.empty())
            m_pixels = {};
        else if (area == whole)
            m_pixels = std::move(*drawn);
        else
            m_pixels = copied(*drawn, area);
        m_pixelsAt = {
            sceneCorner.x + static_cast<float>(area.left),
            sceneCorner.y + static_cast<float>(area.top)
        };
        m_sceneToPixels = sceneToPixels;
        m_frameSize = frameSize;
        m_kept = true;
    }

    void SceneLayer::draw(const SceneFrame& frame) const
    {
        const bool fits = m_kept
            && frame.canvas.backend()->type() == BackendType::Cpu
            && frame.canvas.transform() * frame.toCanvas == m_sceneToPixels
            && FloatPoint{ frame.width, frame.height } == m_frameSize;
        if (!fits)
        {
            m_drawing(frame);
            return;
        }
        if (!m_pixels.empty())
            frame.canvas.drawBitmap(m_pixels, m_pixelsAt);
    }

    namespace
    {
        // HeldRasterClip

        HeldRasterClip::~HeldRasterClip()
        {
            Cpu::g_rasterBuffers.hasActiveClip = m_active;
            Cpu::g_rasterBuffers.activeClipMaskPtr = m_mask;
            Cpu::g_rasterBuffers.clipBox = m_box;
            Cpu::g_rasterBuffers.m_clipStride = m_stride;
        }

        // A row is searched from each end for its first pixel that is not clear, so a covered row
        // costs two pixels and only a clear one is read through.
        IntRect drawnArea(const Bitmap& bitmap)
        {
            const auto isDrawn = [](const Color& pixel) {
                return pixel.alpha != 0;
            };
            IntRect area = { bitmap.width(), bitmap.height(), 0, 0 };
            for (int y = 0; y != bitmap.height(); ++y)
            {
                const Color* rowStart = bitmap.data()
                    + static_cast<std::ptrdiff_t>(y) * bitmap.stride();
                const Color* rowEnd = rowStart + bitmap.width();
                const Color* first = std::find_if(rowStart, rowEnd, isDrawn);
                if (first == rowEnd)
                    continue;
                const Color* last = std::find_if(std::make_reverse_iterator(rowEnd),
                    std::make_reverse_iterator(first), isDrawn).base();
                area.left = std::min(area.left, static_cast<int>(first - rowStart));
                area.right = std::max(area.right, static_cast<int>(last - rowStart));
                area.top = std::min(area.top, y);
                area.bottom = y + 1;
            }
            return area.left < area.right ? area : IntRect{};
        }

        Bitmap copied(const Bitmap& bitmap, const IntRect& area)
        {
            Bitmap result{ IntSize{ area.width(), area.height() } };
            const std::size_t rowBytes = static_cast<std::size_t>(area.width()) * sizeof(Color);
            for (int y = area.top; y != area.bottom; ++y)
            {
                const Color* from = bitmap.data()
                    + static_cast<std::ptrdiff_t>(y) * bitmap.stride() + area.left;
                Color* to = result.data()
                    + static_cast<std::ptrdiff_t>(y - area.top) * result.stride();
                std::memcpy(to, from, rowBytes);
            }
            return result;
        }
    }
}
