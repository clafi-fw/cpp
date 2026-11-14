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
    }

    // WaterReflection

    WaterReflection::WaterReflection() = default;

    WaterReflection::~WaterReflection() = default;

    // **On the CPU backend the rows ripple.** Elsewhere the parts are drawn live, clipped to the
    // water and faded, and the water lies still under them.
    void WaterReflection::draw(const SceneFrame& frame, const PixelPath& water,
        const Drawing& still, const Drawing& moving, const Ripples& ripples)
    {
        Canvas& canvas = frame.canvas;
        if (canvas.backend() == nullptr || canvas.backend()->type() != BackendType::Cpu)
        {
            canvas.pushClip(water, &frame.toCanvas);
            canvas.pushOpacity(ripples.opacity);
            still(frame);
            moving(frame);
            canvas.popOpacity();
            canvas.popClip();
            return;
        }

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
            m_mirrorCanvas->drawBitmap(m_still, {});
            moving(bandFrame(*m_mirrorCanvas, frame, top));
            m_mirrorCanvas->endPaint(nullptr, whole, mirrored);
        }

        const float unit = frame.unit;
        const float phase = ripples.cycleTime * k_2Pi;
        const Color* source = mirrored->data();
        Color* target = m_rippled.data();
        for (int row = 0; row != size.y; ++row)
        {
            const ColumnSpan span = m_waterSpans[static_cast<std::size_t>(row)];
            if (span.first > span.last)
                continue;
            const float nearness = static_cast<float>(row) / static_cast<float>(size.y);
            const float rowInUnits = (top + static_cast<float>(row)) / unit;
            const float reachInUnits = ripples.farReach
                + (ripples.nearReach - ripples.farReach) * nearness;
            const float reach = reachInUnits * unit;
            const float wave = std::sin(rowInUnits * 0.81f / (0.4f + nearness)
                + phase * k_firstWaveTurns)
                + k_secondWaveShare * std::sin(rowInUnits * 2.34f - phase * k_secondWaveTurns);
            const int shift = static_cast<int>(std::lround(reach * wave));
            const float shading = 1.0f - k_shadingDepth * 0.5f * (1.0f + std::sin(
                rowInUnits * 1.62f / (0.3f + nearness) + phase * k_shadingTurns));
            const std::uint32_t rowOpacity = static_cast<std::uint32_t>(
                std::clamp(ripples.opacity * shading, 0.0f, 1.0f) * 256.0f);

            const std::size_t rowStart = static_cast<std::size_t>(row)
                * static_cast<std::size_t>(size.x);
            // Premultiplied, so a faded pixel has all four channels scaled alike.
            for (int x = span.first; x <= span.last; ++x)
            {
                const std::size_t index = rowStart + static_cast<std::size_t>(x);
                const int sourceX = x - shift;
                Color& faded = target[index];
                if (sourceX < 0 || sourceX >= size.x)
                {
                    faded = Color{ 0x00000000 };
                    continue;
                }
                faded = source[rowStart + static_cast<std::size_t>(sourceX)];
                const std::uint32_t scale = rowOpacity * m_waterCoverage[index] / 255;
                faded.blue = static_cast<ColorByte>((faded.blue * scale) >> 8);
                faded.green = static_cast<ColorByte>((faded.green * scale) >> 8);
                faded.red = static_cast<ColorByte>((faded.red * scale) >> 8);
                faded.alpha = static_cast<ColorByte>((faded.alpha * scale) >> 8);
            }
        }
        canvas.drawBitmap(m_rippled, frame.toCanvas.transform({ 0.0f, top }));
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
    }
}
