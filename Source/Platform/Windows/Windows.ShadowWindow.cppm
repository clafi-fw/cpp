module;
#include "Windows.Headers.h"
export module ClaFi.Platform.Windows.ShadowWindow;

import ClaFi.Core.Graphics.ShadowPainter;
import ClaFi.Core.Graphics.Cpu.Canvas;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Windows
{
    // THE SHADOW OF A COMPOSED WINDOW, as a window of its own. See Platform
    export class ShadowWindow
    {
    public:
        ShadowWindow(HINSTANCE, const wchar_t* className, HWND owner);
        ~ShadowWindow();
        [[nodiscard]] HWND handle() const { return m_handle; }
        // Moves the window; the surface stays what it was.
        void moveTo(IntPoint screenPosition);
        // Paints the shadow around geometry into a surface of this size, the hole left clear, and
        // shows it at screenPosition. geometry and hole are in the surface's coordinates.
        void fill(const Graphics::ShadowPainter&, IntSize surface, const FloatRect& geometry,
            const IntRect& hole, IntPoint screenPosition, ColorByte alpha);
        void setAlpha(ColorByte);
        // Answers whether the window's visibility changed.
        bool show(bool);
    private:
        using Strips = std::array<IntRect, 4>;
        // Grows the DIB and the canvas to hold the surface; neither ever shrinks. See Platform
        void ensureSurface(IntSize);
        void releaseSurface();
        // The margins around the hole, as four strips of the surface.
        [[nodiscard]] static Strips stripsAround(const IntRect& hole, IntSize surface);
        void clearStrip(const IntRect&);
        void copyStrip(const Graphics::Bitmap& from, const IntRect&);
        void present();
    private:
        // The DIB and the canvas grow by whole steps of this, so a drag outward grows them now
        // and then rather than per pixel.
        static constexpr int k_capacityStep = 256;
        HWND m_handle{ nullptr };
        HDC m_dc{ nullptr };
        HBITMAP m_bitmap{ nullptr };
        Color* m_pixels{ nullptr };
        IntSize m_capacity{};   // what the DIB and the canvas hold
        IntSize m_size{};       // what the window shows of it
        // Over a CPU backend; the strips are copied out of its bitmap into the DIB.
        std::unique_ptr<Graphics::Canvas> m_canvas;
        Strips m_strips{};      // what the last fill painted, taken back by the next
        IntPoint m_position{};
        ColorByte m_alpha{ 255 };
        bool m_filled{ false };
        bool m_shown{ false };
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi::PlatformImplementation::Windows
{
    ShadowWindow::ShadowWindow(HINSTANCE instance, const wchar_t* className, HWND owner)
    {
        m_handle = ::CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
            className, nullptr, WS_POPUP,
            0, 0, 0, 0,
            owner, nullptr, instance, nullptr);
    }

    ShadowWindow::~ShadowWindow()
    {
        if (m_handle)
            ::DestroyWindow(m_handle);
        releaseSurface();
    }

    void ShadowWindow::moveTo(IntPoint screenPosition)
    {
        if (!m_handle || m_position == screenPosition)
            return;
        m_position = screenPosition;
        ::SetWindowPos(m_handle, nullptr, m_position.x, m_position.y, 0, 0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    }

    void ShadowWindow::fill(const Graphics::ShadowPainter& painter, IntSize surface,
        const FloatRect& geometry, const IntRect& hole, IntPoint screenPosition, ColorByte alpha)
    {
        if (!m_handle || surface.x <= 0 || surface.y <= 0)
            return;
        ensureSurface(surface);
        m_size = surface;

        // Only the strips are ever written, so taking the last ones back leaves the whole DIB
        // clear - a wider window's strip cannot stand inside a narrower one's hole.
        for (const IntRect& strip : m_strips)
            clearStrip(strip);

        m_strips = stripsAround(hole, surface);
        for (const IntRect& strip : m_strips)
        {
            if (strip.empty())
                continue;
            m_canvas->beginPaint(nullptr, strip);
            m_canvas->pushClip(strip.toFloat());
            painter.paint(*m_canvas, geometry);
            m_canvas->popClip();
            Graphics::Bitmap* painted = nullptr;
            m_canvas->endPaint(nullptr, strip, painted);
            copyStrip(*painted, strip);
        }

        m_position = screenPosition;
        m_alpha = alpha;
        m_filled = true;
        present();
    }

    void ShadowWindow::setAlpha(ColorByte value)
    {
        if (m_alpha == value)
            return;
        m_alpha = value;
        if (m_filled)
            present();
    }

    bool ShadowWindow::show(bool value)
    {
        if (!m_handle || m_shown == value)
            return false;
        m_shown = value;
        ::ShowWindow(m_handle, value ? SW_SHOWNOACTIVATE : SW_HIDE);
        return true;
    }

    void ShadowWindow::ensureSurface(IntSize size)
    {
        if (m_bitmap && size.x <= m_capacity.x && size.y <= m_capacity.y)
            return;
        const IntSize capacity = {
            std::max(m_capacity.x, (size.x + k_capacityStep - 1) / k_capacityStep * k_capacityStep),
            std::max(m_capacity.y, (size.y + k_capacityStep - 1) / k_capacityStep * k_capacityStep)
        };
        releaseSurface();

        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof BITMAPINFOHEADER;
        info.bmiHeader.biWidth = capacity.x;
        info.bmiHeader.biHeight = -capacity.y;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        void* bits = nullptr;
        m_bitmap = ::CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
        m_pixels = static_cast<Color*>(bits);
        std::memset(m_pixels, 0, static_cast<std::size_t>(capacity.x) * capacity.y * sizeof(Color));
        m_dc = ::CreateCompatibleDC(nullptr);
        ::SelectObject(m_dc, m_bitmap);
        m_capacity = capacity;
        // A new DIB holds nothing to take back.
        m_strips = {};

        if (m_canvas)
        {
            m_canvas->resize(capacity);
            return;
        }
        auto backend = std::make_unique<Graphics::Cpu::CpuBackend>(capacity);
        backend->setTransparentBase(true);
        m_canvas = std::make_unique<Graphics::Canvas>(std::move(backend));
    }

    void ShadowWindow::releaseSurface()
    {
        if (m_dc)
            ::DeleteDC(m_dc);
        if (m_bitmap)
            ::DeleteObject(m_bitmap);
        m_dc = nullptr;
        m_bitmap = nullptr;
        m_pixels = nullptr;
        m_capacity = {};
        m_size = {};
        m_filled = false;
    }

    ShadowWindow::Strips ShadowWindow::stripsAround(const IntRect& hole, IntSize surface)
    {
        IntRect gap = hole;
        if (!gap.intersectWith({ 0, 0, surface.x, surface.y }))
            gap = {};
        return { {
            { 0, 0, surface.x, gap.top },
            { 0, gap.bottom, surface.x, surface.y },
            { 0, gap.top, gap.left, gap.bottom },
            { gap.right, gap.top, surface.x, gap.bottom }
        } };
    }

    void ShadowWindow::clearStrip(const IntRect& strip)
    {
        if (strip.empty())
            return;
        const std::size_t rowBytes = static_cast<std::size_t>(strip.width()) * sizeof(Color);
        for (int y = strip.top; y < strip.bottom; ++y)
        {
            Color* target = m_pixels + static_cast<std::size_t>(y) * m_capacity.x + strip.left;
            std::memset(target, 0, rowBytes);
        }
    }

    void ShadowWindow::copyStrip(const Graphics::Bitmap& from, const IntRect& strip)
    {
        const std::size_t rowBytes = static_cast<std::size_t>(strip.width()) * sizeof(Color);
        for (int y = strip.top; y < strip.bottom; ++y)
        {
            const std::size_t row = static_cast<std::size_t>(y);
            const Color* source = from.data() + row * from.stride() + strip.left;
            Color* target = m_pixels + row * m_capacity.x + strip.left;
            std::memcpy(target, source, rowBytes);
        }
    }

    // The bitmap is premultiplied, which is what AC_SRC_ALPHA means; the window-wide alpha rides
    // on the blend as its constant factor. The call places and sizes the window as well.
    void ShadowWindow::present()
    {
        if (!m_handle || !m_dc)
            return;
        POINT destination{ m_position.x, m_position.y };
        SIZE size{ m_size.x, m_size.y };
        POINT origin{ 0, 0 };
        BLENDFUNCTION blend{ AC_SRC_OVER, 0, m_alpha, AC_SRC_ALPHA };
        UPDATELAYEREDWINDOWINFO info{};
        info.cbSize = sizeof UPDATELAYEREDWINDOWINFO;
        info.pptDst = &destination;
        info.psize = &size;
        info.hdcSrc = m_dc;
        info.pptSrc = &origin;
        info.pblend = &blend;
        info.dwFlags = ULW_ALPHA;
        ::UpdateLayeredWindowIndirect(m_handle, &info);
    }
}
