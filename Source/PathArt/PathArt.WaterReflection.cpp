module ClaFi.PathArt.WaterReflection;

import ClaFi.PathArt.SceneHelpers;
import ClaFi.Core.Graphics.Cpu.Canvas;
import ClaFi.Core.Graphics.Cpu_Rasterizer;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi::PathArt
{
    using namespace ::ClaFi::Graphics;

    namespace
    {
        // Each row's shift is two waves, the second this share of the first.
        constexpr float k_secondWaveShare{ 0.4f };
        // How often each wave and the rows' shading run, in turns a cycle.
        constexpr float k_firstWaveTurns{ 13.0f };
        constexpr float k_secondWaveTurns{ 21.0f };
        constexpr float k_shadingTurns{ 9.0f };
        // The share of the reflection's opacity the shading takes off its darkest rows.
        constexpr float k_shadingDepth{ 0.18f };
        // The surface's kept images off the CPU: the still part, and each frame's whole mirror.
        constexpr int k_stillImage{ 6 };
        constexpr int k_mirrorImage{ 7 };

        // How the ripples move and shade one row.
        struct RowRipple
        {
            float shift; // sideways, in the frame's pixels
            float opacity;
        };

        // A run of the surface's rows laid into the water with one shift and one opacity.
        struct Strip
        {
            int top;
            int bottom;
            int shift;
            int opacity; // in 256ths
        };

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

        // The frame's rows from `top` down, on a canvas whose top row is that row.
        [[nodiscard]] SceneFrame bandFrame(Canvas&, const SceneFrame&, float top);
        // The row at `y` of a band of water from `top` to `bottom`, all in the frame's pixels.
        [[nodiscard]] RowRipple rippleAt(const Ripples&, float y, float top, float bottom,
            float unit);
        // The parts drawn as they are, clipped to the water and faded, with no ripples.
        void drawStillMirror(const SceneFrame&, const PixelPath& water,
            const WaterReflection::Drawing& still, const WaterReflection::Drawing& moving,
            float opacity);
    }

    float rippleShiftAt(const Ripples& ripples, const SceneFrame& frame, const float y)
    {
        return rippleAt(ripples, y, ripples.top, frame.height, frame.unit).shift;
    }

    // WaterReflection

    WaterReflection::WaterReflection() = default;

    WaterReflection::~WaterReflection() = default;

    void WaterReflection::forget()
    {
        m_kept = false;
        m_stillImageKept = false;
    }

    void WaterReflection::draw(const SceneFrame& frame, const PixelPath& water,
        const Drawing& still, const Drawing& moving, const Ripples& ripples)
    {
        const IBackend* backend = frame.canvas.backend();
        if (backend == nullptr)
            return;
        if (backend->type() == BackendType::Cpu)
            drawRows(frame, water, still, moving, ripples);
        else
            drawStrips(frame, water, still, moving, ripples);
    }

    void WaterReflection::drawRows(const SceneFrame& frame, const PixelPath& water,
        const Drawing& still, const Drawing& moving, const Ripples& ripples)
    {
        Canvas& canvas = frame.canvas;
        const float top = std::floor(ripples.top);
        const IntSize size = {
            static_cast<int>(std::ceil(frame.width)),
            static_cast<int>(std::ceil(frame.height - top))
        };
        if (size.x <= 0 || size.y <= 0)
            return;
        if (!m_kept || size != m_keptSize || top != m_keptTop)
            keep(frame, water, still, top);

        Bitmap* mirrored = nullptr;
        {
            const HeldRasterClip heldClip{};
            const IntRect whole = { 0, 0, size.x, size.y };
            m_mirrorCanvas->beginPaint(nullptr, whole);
            if (still)
                m_mirrorCanvas->drawBitmap(m_still, {});
            moving(bandFrame(*m_mirrorCanvas, frame, top));
            m_mirrorCanvas->endPaint(nullptr, whole, mirrored);
        }

        const float bottom = top + static_cast<float>(size.y);
        const Color* source = mirrored->data();
        Color* target = m_rippled.data();
        // **The rows are the surface's own, and each writes only itself**, so a band wide enough to
        // be worth the waking is worked in stripes across the cores. A band over little water is
        // worked through on this thread, where the helpers' waking would cost more than the rows.
        std::size_t coveredPixels = 0;
        for (const ColumnSpan& span : m_waterSpans)
            coveredPixels += static_cast<std::size_t>(std::max(0, span.last - span.first + 1));
        constexpr std::size_t k_spreadFromPixels{ 1000000 };
        constexpr int k_stripeRows{ 64 };
        // One stripe over the whole band runs on this thread alone.
        const int stripeRows = coveredPixels >= k_spreadFromPixels ? k_stripeRows : size.y;
        const int stripeCount = (size.y + stripeRows - 1) / stripeRows;
        spreadOverCores(stripeCount, [&](const int stripe) {
            const int firstRow = stripe * stripeRows;
            const int endRow = std::min(firstRow + stripeRows, size.y);
            std::array<std::uint32_t, 256> scaleOfCoverage{}; // in 256ths, once a row
            for (int row = firstRow; row != endRow; ++row)
            {
                const ColumnSpan span = m_waterSpans[static_cast<std::size_t>(row)];
                if (span.first > span.last)
                    continue;
                const RowRipple ripple = rippleAt(ripples, top + static_cast<float>(row), top,
                    bottom, frame.unit);
                const int shift = static_cast<int>(std::lround(ripple.shift));
                const std::uint32_t rowOpacity = static_cast<std::uint32_t>(
                    std::clamp(ripple.opacity, 0.0f, 1.0f) * 256.0f);
                for (std::uint32_t coverage = 0; coverage != scaleOfCoverage.size(); ++coverage)
                    scaleOfCoverage[coverage] = rowOpacity * coverage / 255;

                const std::size_t rowStart = static_cast<std::size_t>(row)
                    * static_cast<std::size_t>(size.x);
                const Color* sourceRow = source + rowStart;
                Color* targetRow = target + rowStart;
                const std::uint8_t* coverageRow = m_waterCoverage.data() + rowStart;
                // Premultiplied, so all four channels scale alike: blue with red, green with
                // alpha.
                const auto scaled = [&](const Color color, const std::uint8_t coverage) {
                    const std::uint32_t pixel = std::bit_cast<std::uint32_t>(color);
                    const std::uint32_t scale = scaleOfCoverage[coverage];
                    const std::uint32_t blueRed = (((pixel & 0x00FF00FFu) * scale) >> 8)
                        & 0x00FF00FFu;
                    const std::uint32_t greenAlpha = (((pixel >> 8) & 0x00FF00FFu) * scale)
                        & 0xFF00FF00u;
                    return std::bit_cast<Color>(blueRed | greenAlpha);
                };
                // A column whose source lies past the band's side repeats that side's pixel.
                const int firstInside = std::max(span.first, shift);
                const int lastInside = std::min(span.last, size.x - 1 + shift);
                for (int x = span.first; x <= span.last && x < firstInside; ++x)
                    targetRow[x] = scaled(sourceRow[0], coverageRow[x]);
                for (int x = firstInside; x <= lastInside; ++x)
                    targetRow[x] = scaled(sourceRow[x - shift], coverageRow[x]);
                for (int x = std::max(span.first, lastInside + 1); x <= span.last; ++x)
                    targetRow[x] = scaled(sourceRow[size.x - 1], coverageRow[x]);
            }
        });
        canvas.drawBitmap(m_rippled, frame.toCanvas.transform({ 0.0f, top }));
    }


    // **The still part is drawn once into its image**, and each frame's mirror is that image with
    // the moving parts over it. A backend that cannot build the images draws the still mirror.
    // With no still part the still image is left alone, for a reflection that has one.
    void WaterReflection::drawStrips(const SceneFrame& frame, const PixelPath& water,
        const Drawing& still, const Drawing& moving, const Ripples& ripples)
    {
        Canvas& canvas = frame.canvas;
        const bool hasStill = static_cast<bool>(still);
        if (hasStill && (!m_stillImageKept || !canvas.hasImage(k_stillImage)))
        {
            canvas.clearImage(k_stillImage);
            m_stillImageKept = canvas.hasImage(k_stillImage);
            if (m_stillImageKept)
            {
                const ScopedCanvasImage into{ canvas, k_stillImage };
                still(frame);
            }
        }
        canvas.clearImage(k_mirrorImage);
        if ((hasStill && !m_stillImageKept) || !canvas.hasImage(k_mirrorImage))
        {
            drawStillMirror(frame, water, still, moving, ripples.opacity);
            return;
        }
        {
            const ScopedCanvasImage into{ canvas, k_mirrorImage };
            if (hasStill)
                canvas.drawImage(k_stillImage, 1.0f);
            moving(frame);
        }

        // Rows are the surface's, so the ripples are as fine as its pixels at any scale.
        const Matrix3x2 toSurface = canvas.transform() * frame.toCanvas;
        const float surfacePerFrame = toSurface.getScaleFactor();
        const FloatPoint bandTopLeft = toSurface.transform({ 0.0f, ripples.top });
        const FloatPoint bandBottomRight = toSurface.transform({ frame.width, frame.height });
        const int left = static_cast<int>(std::floor(bandTopLeft.x));
        const int right = static_cast<int>(std::ceil(bandBottomRight.x));
        const int firstRow = static_cast<int>(std::floor(bandTopLeft.y));
        const int endRow = static_cast<int>(std::ceil(bandBottomRight.y));
        const auto lay = [&](const Strip& strip) {
            if (strip.bottom <= strip.top || strip.opacity <= 0)
                return;
            canvas.drawImagePart(k_mirrorImage, { left, strip.top, right, strip.bottom },
                { strip.shift, 0 }, static_cast<float>(strip.opacity) / 256.0f);
        };

        canvas.pushClip(water, &frame.toCanvas);
        Strip pending = { firstRow, firstRow, 0, 0 };
        for (int row = firstRow; row < endRow; ++row)
        {
            const float y = ripples.top
                + (static_cast<float>(row) - bandTopLeft.y) / surfacePerFrame;
            const RowRipple ripple = rippleAt(ripples, y, ripples.top, frame.height, frame.unit);
            const int shift = static_cast<int>(std::lround(ripple.shift * surfacePerFrame));
            const int opacity = static_cast<int>(std::clamp(ripple.opacity, 0.0f, 1.0f) * 256.0f);
            if (shift == pending.shift && opacity == pending.opacity)
            {
                pending.bottom = row + 1;
                continue;
            }
            lay(pending);
            pending = { row, row + 1, shift, opacity };
        }
        lay(pending);
        canvas.popClip();
    }

    // The water's coverage is drawn as white on clear, and its alpha read per pixel.
    void WaterReflection::keep(const SceneFrame& frame, const PixelPath& water,
        const Drawing& still, const float top)
    {
        const HeldRasterClip heldClip{};
        const IntSize size = {
            static_cast<int>(std::ceil(frame.width)),
            static_cast<int>(std::ceil(frame.height - top))
        };
        const IntRect whole = { 0, 0, size.x, size.y };
        auto backend = std::make_unique<Cpu::CpuBackend>(size);
        backend->setTransparentBase(true);
        m_mirrorCanvas = std::make_unique<Canvas>(std::move(backend));

        Bitmap* drawn = nullptr;
        m_mirrorCanvas->beginPaint(nullptr, whole);
        const SceneFrame band = bandFrame(*m_mirrorCanvas, frame, top);
        fillShape(band, water, SolidColor{ Color{ 0xFFFFFFFF } });
        m_mirrorCanvas->endPaint(nullptr, whole, drawn);
        const std::size_t pixelCount = static_cast<std::size_t>(size.x)
            * static_cast<std::size_t>(size.y);
        m_waterCoverage.assign(pixelCount, 0);
        m_waterSpans.assign(static_cast<std::size_t>(size.y), ColumnSpan{ 0, -1 });
        for (int row = 0; row != size.y; ++row)
        {
            ColumnSpan& span = m_waterSpans[static_cast<std::size_t>(row)];
            const std::size_t rowStart = static_cast<std::size_t>(row)
                * static_cast<std::size_t>(size.x);
            for (int x = 0; x != size.x; ++x)
            {
                const std::size_t index = rowStart + static_cast<std::size_t>(x);
                const std::uint8_t coverage = drawn->data()[index].alpha;
                m_waterCoverage[index] = coverage;
                if (coverage == 0)
                    continue;
                if (span.first > span.last)
                    span.first = x;
                span.last = x;
            }
        }

        m_mirrorCanvas->beginPaint(nullptr, whole);
        if (still)
            still(band);
        m_mirrorCanvas->endPaint(nullptr, whole, drawn);
        m_still = *drawn;
        m_rippled = Bitmap{ size };
        std::fill_n(m_rippled.data(), pixelCount, Color{ 0x00000000 });

        m_keptSize = size;
        m_keptTop = top;
        m_kept = true;
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

        SceneFrame bandFrame(Canvas& canvas, const SceneFrame& frame, const float top)
        {
            return {
                .canvas = canvas,
                .width = frame.width,
                .height = frame.height,
                .unit = frame.unit,
                .toCanvas = Matrix3x2::Translation(0.0f, -top)
            };
        }

        RowRipple rippleAt(const Ripples& ripples, const float y, const float top,
            const float bottom, const float unit)
        {
            const float phase = ripples.cycleTime * k_2Pi;
            const float nearness = (y - top) / (bottom - top);
            const float rowInUnits = y / unit;
            const float reachInUnits = ripples.farReach
                + (ripples.nearReach - ripples.farReach) * nearness;
            const float wave = std::sin(rowInUnits * 0.81f / (0.4f + nearness)
                + phase * k_firstWaveTurns)
                + k_secondWaveShare * std::sin(rowInUnits * 2.34f - phase * k_secondWaveTurns);
            const float shading = 1.0f - k_shadingDepth * 0.5f * (1.0f + std::sin(
                rowInUnits * 1.62f / (0.3f + nearness) + phase * k_shadingTurns));
            return {
                .shift = reachInUnits * unit * wave,
                .opacity = ripples.opacity * shading
            };
        }

        void drawStillMirror(const SceneFrame& frame, const PixelPath& water,
            const WaterReflection::Drawing& still, const WaterReflection::Drawing& moving,
            const float opacity)
        {
            Canvas& canvas = frame.canvas;
            canvas.pushClip(water, &frame.toCanvas);
            canvas.pushOpacity(opacity);
            if (still)
                still(frame);
            moving(frame);
            canvas.popOpacity();
            canvas.popClip();
        }
    }
}
