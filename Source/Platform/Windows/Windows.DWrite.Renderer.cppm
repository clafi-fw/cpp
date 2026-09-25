module;
#include "Windows.Headers.h"
export module ClaFi.Platform.Windows.DWrite.Renderer;

import ClaFi.Platform.Windows.Diagnostic;
import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.Graphics.Cpu_GlyphCompositor;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;
import ClaFi.Core.AppTheme_Colors;

namespace ClaFi::PlatformImplementation::Windows
{
    using Microsoft::WRL::ComPtr;

    // {C0A20E41-52F0-492B-BFA8-29C72EE0A468}
    export const GUID IID_DWriteRunEffect = { 0xc0a20e41, 0x52f0, 0x492b, { 0xbf, 0xa8, 0x29, 0xc7, 0x2e, 0xe0, 0xa4, 0x68 } };

    // What one run is drawn with, over and above the format the layout was built from. See Platform
    export class IdWriteRunEffect : public IUnknown {
    public:
        virtual ~IdWriteRunEffect() = default;
        virtual Graphics::Brush getBrush() const = 0;
        // How far off the baseline the layout placed it the run is drawn, in device units,
        // positive upward.
        virtual float baselineShift() const = 0;
    };

    export inline ComPtr<IDWriteFactory> g_dwriteFactory;
    static ComPtr<IDWriteTextFormat> g_baseFormat;

    // The face DirectWrite is handed for a generic family - see GenericFamily. Any other name is
    // the caller's own and is handed over as written.
    export [[nodiscard]] const wchar_t* nativeFamilyName(const wchar_t* family);

    export inline ComPtr<IDWriteTextFormat> getOrCreateBaseFormat2()
    {
        if (!g_dwriteFactory)
        {
            checkHr(DWriteCreateFactory(
                DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown**>(g_dwriteFactory.GetAddressOf())
            ));
        }

        if (g_baseFormat) return g_baseFormat;

        const auto& bodyProps = ClaFi::k_textStyles[static_cast<std::size_t>(TextStyleId::Body)];

        checkHr(g_dwriteFactory->CreateTextFormat(
            nativeFamilyName(bodyProps.family),
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            bodyProps.size,
            L"en-US",
            g_baseFormat.GetAddressOf()
        ));

        g_baseFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WHOLE_WORD);

        DWRITE_TRIMMING trimming;
        trimming.granularity = DWRITE_TRIMMING_GRANULARITY_NONE;
        trimming.delimiter = 0;
        trimming.delimiterCount = 0;

        g_baseFormat->SetTrimming(&trimming, nullptr);
        return g_baseFormat;
    }

    // Global GlyphRunCache

    // One face at one size in one measuring mode: what a cached glyph is keyed on. See Platform
    export struct FontKey {
        IDWriteFontFace* fontFace;
        float fontSize;
        DWRITE_MEASURING_MODE measuringMode;

        bool operator==(const FontKey&) const = default;
    };

    // The hash of a FontKey.
    export struct FontKeyHash {
        std::size_t operator()(const FontKey& key) const {
            std::size_t h = 17;
            h = h * 31 + reinterpret_cast<std::size_t>(key.fontFace);
            h = h * 31 + std::hash<float>{}(key.fontSize);
            h = h * 31 + static_cast<std::size_t>(key.measuringMode);
            return h;
        }
    };

    // One glyph's coverage, rasterized on first use.
    export struct GlyphSlot
    {
        Graphics::Cpu::GlyphCoverage glyph{};
        bool rasterized{ false };
    };

    // The glyphs of one face at one size, indexed by glyph index and never grown. See Platform
    export struct FontGlyphs
    {
        ComPtr<IDWriteFontFace> face{};
        std::vector<GlyphSlot> slots{};
    };

    // Global in-memory glyph cache [CP]
    export inline std::unordered_map<FontKey, FontGlyphs, FontKeyHash> g_GlyphCache;

    // One glyph of a face at an em size, rasterized at the origin: the coverage every occurrence
    // of that glyph is placed from. Shared by every run drawn on the CPU path, whichever layout
    // it came out of.
    export [[nodiscard]] Graphics::Cpu::GlyphCoverage rasterizeGlyph(IDWriteFontFace*,
        float emSize, UINT16 glyphIndex, DWRITE_MEASURING_MODE);
    // The cached glyphs of one face at one size, made on first use.
    export [[nodiscard]] FontGlyphs& glyphsOf(IDWriteFontFace*, float emSize,
        DWRITE_MEASURING_MODE);
    // One glyph out of those, rasterized on first use. The reference stands for as long as the
    // cache does, since the slots are never grown - see FontGlyphs.
    export [[nodiscard]] const Graphics::Cpu::GlyphCoverage& coverageOf(FontGlyphs&,
        UINT16 glyphIndex, float emSize, DWRITE_MEASURING_MODE);

    export class FadeTextRenderer : public IDWriteTextRenderer
    {
    public:
        FadeTextRenderer(
            ID2D1RenderTarget* rt,
            const FloatRect& bounds,
            float fadeWidth,
            std::span<const float> localBaselinesToFade,
            float globalCollapseBaseline,
            float collapseXOffset,
            Graphics::Canvas* canvas
        );
        virtual ~FadeTextRenderer() = default;
        void notifyNewParagraph();

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override final;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void* ctx, BOOL* disabled) override final;
        HRESULT STDMETHODCALLTYPE GetCurrentTransform(void* ctx, DWRITE_MATRIX* transform) override final;
        HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void* ctx, FLOAT* pixelsPerDip) override final;

        HRESULT STDMETHODCALLTYPE DrawGlyphRun(
            void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_MEASURING_MODE measuringMode,
            DWRITE_GLYPH_RUN const* glyphRun,
            DWRITE_GLYPH_RUN_DESCRIPTION const*,
            IUnknown* clientDrawingEffect) override final;

        HRESULT STDMETHODCALLTYPE DrawInlineObject(
            void* clientDrawingContext,
            FLOAT originX,
            FLOAT originY,
            IDWriteInlineObject* inlineObject,
            BOOL isSideways,
            BOOL isRightToLeft,
            IUnknown* clientDrawingEffect) override final;

        HRESULT STDMETHODCALLTYPE DrawUnderline(
            void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_UNDERLINE const* underline,
            IUnknown* clientDrawingEffect) override final;

        HRESULT STDMETHODCALLTYPE DrawStrikethrough(
            void* clientDrawingContext,
            FLOAT baselineOriginX,
            FLOAT baselineOriginY,
            DWRITE_STRIKETHROUGH const* strikethrough,
            IUnknown* clientDrawingEffect) override final;

    private:
        ID2D1RenderTarget* m_rt;
        FloatRect m_bounds;
        float m_fadeWidth;
        // Stated relative to m_bounds.top, which is where the layout's own lines are measured
        // from. Held as a view onto the caller's buffer: the renderer lives only for the Draw call
        // it was made for, which is inside the call that owns that buffer.
        std::span<const float> m_localBaselinesToFade;
        float m_globalCollapseBaseline;
        float m_collapseXOffset;
        bool m_collapsing{ false };
        float m_collapseX{ 0.0f };
        bool m_needsGap{ false };
        bool m_collapseOffsetClamped{ false };
        float m_currentLayoutBaseline{ -9999.0f };
        float m_currentLineEndX{ 0.0f };

        Graphics::Canvas* m_canvas;
        Graphics::Cpu::GlyphCompositor m_compositor;
    };

    const wchar_t* nativeFamilyName(const wchar_t* family)
    {
        const std::wstring_view name = family;
        if (name == GenericFamily::sansSerif)
            return L"Segoe UI";
        if (name == GenericFamily::monospace)
            return L"Consolas";
        return family;
    }

    FadeTextRenderer::FadeTextRenderer(
        ID2D1RenderTarget* rt,
        const FloatRect& bounds,
        float fadeWidth,
        std::span<const float> localBaselinesToFade,
        float globalCollapseBaseline,
        float collapseXOffset,
        Graphics::Canvas* canvas
    )
        : m_rt(rt)
        , m_bounds(bounds)
        , m_fadeWidth(fadeWidth)
        , m_localBaselinesToFade(localBaselinesToFade)
        , m_globalCollapseBaseline(globalCollapseBaseline)
        , m_collapseXOffset(collapseXOffset)
        , m_canvas(canvas)
    {
    }

    void FadeTextRenderer::notifyNewParagraph()
    {
        m_needsGap = true;
    }

    HRESULT STDMETHODCALLTYPE FadeTextRenderer::QueryInterface(REFIID riid, void** ppv)
    {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWritePixelSnapping) || riid == __uuidof(IDWriteTextRenderer))
        {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE FadeTextRenderer::AddRef() { return 1; }
    ULONG STDMETHODCALLTYPE FadeTextRenderer::Release() { return 1; }

    HRESULT STDMETHODCALLTYPE FadeTextRenderer::IsPixelSnappingDisabled(void* clientDrawingContext, BOOL* disabled)
    {
        // Snapping puts each run's baseline origin on a whole device pixel, which is what keeps
        // still text crisp. Under a scale it quantizes: DirectWrite rounds every run's transformed
        // origin independently, so as the scale animates a run holds still and then jumps a whole
        // pixel, at a different moment from its neighbours.
        //
        // Whether that trade is worth making is not decided here. The params in force say, and this
        // reports them, because DirectWrite asks per glyph run and the caller is too far away to be
        // passed a flag.
        const auto* context = static_cast<ControlPaintContext*>(clientDrawingContext);
        const bool snap = !context || !context->canvas().backend() || context->canvas().backend()->snapTextOrigins();
        *disabled = snap ? FALSE : TRUE;
        return S_OK;
    }

    Graphics::Cpu::GlyphCoverage rasterizeGlyph(IDWriteFontFace* fontFace, float emSize,
        UINT16 glyphIndex, DWRITE_MEASURING_MODE measuringMode)
    {
        // One glyph, at the origin, carrying no advance or offset of its own. The pen position is
        // applied when the coverage is placed, which is what lets a single texture serve every
        // occurrence of that glyph in every string. bidiLevel is left at zero for the same reason -
        // direction decides where the glyph goes, not what it looks like.
        FLOAT advance = 0.0f;
        DWRITE_GLYPH_OFFSET offset{ 0.0f, 0.0f };

        DWRITE_GLYPH_RUN single{};
        single.fontFace = fontFace;
        single.fontEmSize = emSize;
        single.glyphCount = 1;
        single.glyphIndices = &glyphIndex;
        single.glyphAdvances = &advance;
        single.glyphOffsets = &offset;
        single.isSideways = FALSE;
        single.bidiLevel = 0;

        Graphics::Cpu::GlyphCoverage result{};
        ComPtr<IDWriteGlyphRunAnalysis> analysis;
        HRESULT hr = g_dwriteFactory->CreateGlyphRunAnalysis(
            &single, 1.0f, nullptr, DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
            measuringMode, 0.0f, 0.0f, analysis.GetAddressOf()
        );
        RECT textureBounds{};
        if (FAILED(hr) || FAILED(analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_CLEARTYPE_3x1, &textureBounds)))
            return result;

        // The texture bounds are relative to the origin the run was analysed at, so they are the
        // bearings.
        result.left = textureBounds.left;
        result.top = textureBounds.top;

        const int width = textureBounds.right - textureBounds.left;
        const int height = textureBounds.bottom - textureBounds.top;
        if (width <= 0 || height <= 0)
            return result;

        std::vector<BYTE> texture(static_cast<std::size_t>(width) * height * 3);
        if (FAILED(analysis->CreateAlphaTexture(DWRITE_TEXTURE_CLEARTYPE_3x1, &textureBounds,
            texture.data(), static_cast<UINT32>(texture.size()))))
        {
            return result;
        }

        // DirectWrite's ClearType triples averaged to one coverage. Averaging the three channels is
        // not reversible and this path draws grey, so it is done once per glyph here rather than
        // once per pixel per draw.
        result.allocate(width, height);
        constexpr float k_toCoverage = 1.0f / (3.0f * 255.0f);
        for (int y = 0; y != height; ++y)
        {
            const BYTE* src = &texture[static_cast<std::size_t>(y) * width * 3];
            float* dst = result.row(y);
            for (int x = 0; x != width; ++x)
                dst[x] = (static_cast<int>(src[x * 3]) + src[x * 3 + 1] + src[x * 3 + 2]) * k_toCoverage;
        }
        return result;
    }

    FontGlyphs& glyphsOf(IDWriteFontFace* fontFace, float emSize,
        DWRITE_MEASURING_MODE measuringMode)
    {
        FontGlyphs& fontGlyphs = g_GlyphCache[FontKey{ fontFace, emSize, measuringMode }];
        if (!fontGlyphs.face)
        {
            fontGlyphs.face = fontFace;
            fontGlyphs.slots.resize(fontFace->GetGlyphCount());
        }
        return fontGlyphs;
    }

    const Graphics::Cpu::GlyphCoverage& coverageOf(FontGlyphs& fontGlyphs, UINT16 glyphIndex,
        float emSize, DWRITE_MEASURING_MODE measuringMode)
    {
        GlyphSlot& slot = fontGlyphs.slots[glyphIndex];
        if (!slot.rasterized)
        {
            slot.glyph = rasterizeGlyph(fontGlyphs.face.Get(), emSize, glyphIndex, measuringMode);
            slot.rasterized = true;
        }
        return slot.glyph;
    }

    HRESULT STDMETHODCALLTYPE FadeTextRenderer::GetCurrentTransform(void*, DWRITE_MATRIX* transform)
    {
        if (m_rt) {
            m_rt->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform));
        }
        else {
            // The CPU path maps each pen position itself when it places the glyph, so DirectWrite is
            // told about no transform at all.
            *reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform) = D2D1::IdentityMatrix();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE FadeTextRenderer::GetPixelsPerDip(void*, FLOAT* pixelsPerDip)
    {
        *pixelsPerDip = 1.0f;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE FadeTextRenderer::DrawGlyphRun(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        DWRITE_GLYPH_RUN const* glyphRun,
        DWRITE_GLYPH_RUN_DESCRIPTION const*,
        IUnknown* clientDrawingEffect)
    {
        // 1. Establish the unified active drawing brush [CP]
        Graphics::Brush effectBrush;
        auto* controlContext = static_cast<ControlPaintContext*>(clientDrawingContext);
        bool brushAssigned = false;
        float baselineShift = 0.0f;

        // A drawing effect is whatever the caller of SetDrawingEffect left there, so the interface
        // is asked for rather than assumed. An effect that does not answer to it is left to the
        // theme fallback instead of being read through the wrong vtable.
        if (clientDrawingEffect)
        {
            IdWriteRunEffect* runEffect = nullptr;
            if (SUCCEEDED(clientDrawingEffect->QueryInterface(IID_DWriteRunEffect, reinterpret_cast<void**>(&runEffect))))
            {
                effectBrush = runEffect->getBrush();
                baselineShift = runEffect->baselineShift();
                runEffect->Release();
                brushAssigned = true;
            }
        }

        if (!brushAssigned)
        {
            Color fallbackColor = controlContext->textRgb(InkGrade::Strongest);
            effectBrush = Graphics::SolidColor{ fallbackColor };
        }

        float fontSize = glyphRun->fontEmSize;
        float runWidth = 0.0f;
        for (UINT32 i = 0; i < glyphRun->glyphCount; ++i)
            runWidth += glyphRun->glyphAdvances[i];

        float effectiveBaselineX = baselineOriginX;
        if (std::abs(baselineOriginY - m_globalCollapseBaseline) < 0.5f)
        {
            if (!m_collapseOffsetClamped)
            {
                if (baselineOriginX + m_collapseXOffset < m_bounds.left) m_collapseXOffset = m_bounds.left - baselineOriginX;
                m_collapseOffsetClamped = true;
            }
            effectiveBaselineX += m_collapseXOffset;
        }

        float drawX = effectiveBaselineX;
        float drawY = baselineOriginY;

        if (!m_collapsing && m_globalCollapseBaseline > -9000.0f && baselineOriginY > m_globalCollapseBaseline + 0.5f)
            m_collapsing = true;

        if (m_collapsing) {
            drawY = m_globalCollapseBaseline;
            drawX = m_collapseX;
            if (m_needsGap)
            {
                drawX += fontSize * 0.25f;
                m_needsGap = false;
            }
            m_collapseX = drawX + runWidth;
        }
        else
        {
            m_needsGap = false;
            if (std::abs(baselineOriginY - m_currentLayoutBaseline) > 0.5f)
            {
                m_currentLayoutBaseline = baselineOriginY;
                m_currentLineEndX = effectiveBaselineX + runWidth;
            }
            else
                {
                m_currentLineEndX = std::max(m_currentLineEndX, effectiveBaselineX + runWidth);
            }
            if (std::abs(baselineOriginY - m_globalCollapseBaseline) < 0.5f)
                m_collapseX = m_currentLineEndX;
        }

        if (drawX >= m_bounds.right)
            return S_OK;

        bool shouldFade = false;
        // Both sides of the comparison are put in the layout's own coordinates, so the baselines
        // arrive as the layout states them and no absolute copy of them is made.
        float checkBaseline = (m_collapsing ? m_globalCollapseBaseline : baselineOriginY) - m_bounds.top;
        for (float bf : m_localBaselinesToFade)
        {
            if (std::abs(checkBaseline - bf) < 0.5f) {
                shouldFade = true;
                break;
            }
        }

        // Apply linear gradient fade brush if requested
        if (shouldFade)
        {
            Color baseColor = std::holds_alternative<Graphics::SolidColor>(effectBrush)
                ? std::get<Graphics::SolidColor>(effectBrush).color
                : controlContext->textRgb(InkGrade::Strongest);

            effectBrush = Graphics::LinearGradient{
              .startPoint = { m_bounds.right - m_fadeWidth, 0.0f },
              .endPoint = { m_bounds.right, 0.0f },
              .stops = { { 0.0f, baseColor }, { 1.0f, baseColor.withOpacity(0.0f) } }
            };
        }

        // A script stands off the baseline the layout placed its run on, and only where the run
        // is drawn: every question asked above - which line this is, whether it fades, whether it
        // is the collapsed one - is about the line, and a line is where the layout put it.
        drawY -= baselineShift;

        if (m_rt)
        {
            // GPU PATH: Render using standard hardware accelerated Direct2D
            //
            // The brush is resolved here, against the backend that owns m_rt. A native brush belongs
            // to the ID2D1Device that created it and D2D refuses one from any other, so a brush
            // reaching this point from anywhere but m_canvas - carried in on the layout, or held over
            // from an earlier form - is the wrong resource domain. Every form has its own device.
            ComPtr<ID2D1Brush> finalBrush;
            finalBrush.Attach(static_cast<ID2D1Brush*>(m_canvas->backend()->getNativeBrush(effectBrush)));
            if (!finalBrush)
            {
                return S_OK;
            }
            m_rt->DrawGlyphRun(D2D1::Point2F(drawX, drawY), glyphRun, finalBrush.Get(), measuringMode);
        }

        else
        {
            // CPU PATH: rasterize each glyph once, place them all into one mask, blend once. The
            // compositor states why one mask.
            //
            // Placing per glyph is what lets the run follow the canvas transform. The cached
            // coverage is a fixed size, so a glyph cannot shrink - but its pen position can be
            // mapped, which leaves only a glyph body's worth of unscaled width instead of the whole
            // run's.
            const bool rightToLeft = (glyphRun->bidiLevel & 1) != 0;

            // Sideways runs would need the whole pen walk rotated. Nothing here produces them, and
            // silently laying them out horizontally would be worse than not drawing them.
            if (glyphRun->isSideways || glyphRun->glyphCount == 0)
                return S_OK;

            const Graphics::Matrix3x2& canvasTransform = m_canvas->transform();
            const bool transformed = !(canvasTransform == Graphics::Matrix3x2::identity());

            FontGlyphs& fontGlyphs = glyphsOf(glyphRun->fontFace, glyphRun->fontEmSize,
                measuringMode);

            m_compositor.begin();
            float penX = drawX;

            for (UINT32 i = 0; i != glyphRun->glyphCount; ++i)
            {
                const float advance = glyphRun->glyphAdvances ? glyphRun->glyphAdvances[i] : 0.0f;
                // The pen steps before the glyph in a right to left run, so the origin of the glyph
                // about to be drawn is the far side of its own advance.
                if (rightToLeft)
                    penX -= advance;

                const Graphics::Cpu::GlyphCoverage& coverage = coverageOf(fontGlyphs,
                    glyphRun->glyphIndices[i], glyphRun->fontEmSize, measuringMode);

                FloatPoint origin = { penX, drawY };
                if (glyphRun->glyphOffsets)
                {
                    origin.x += glyphRun->glyphOffsets[i].advanceOffset;
                    // Positive ascenderOffset lifts the glyph, and y grows downwards.
                    origin.y -= glyphRun->glyphOffsets[i].ascenderOffset;
                }
                if (transformed)
                    origin = canvasTransform.transform(origin);

                m_compositor.place(coverage, origin);

                if (!rightToLeft)
                    penX += advance;
            }

            m_compositor.composite(*m_canvas->backend(), effectBrush);
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE FadeTextRenderer::DrawInlineObject(
        void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        IUnknown* clientDrawingEffect)
    {
        DWRITE_INLINE_OBJECT_METRICS metrics;
        checkHr(inlineObject->GetMetrics(&metrics));

        float baselineOriginY = originY + metrics.baseline;
        float effectiveOriginX = originX;

        if (std::abs(baselineOriginY - m_globalCollapseBaseline) < 0.5f)
        {
            if (!m_collapseOffsetClamped)
            {
                if (originX + m_collapseXOffset < m_bounds.left)
                {
                    m_collapseXOffset = m_bounds.left - originX;
                }
                m_collapseOffsetClamped = true;
            }
            effectiveOriginX += m_collapseXOffset;
        }

        float drawX = effectiveOriginX;
        float drawY = originY;

        if (!m_collapsing && m_globalCollapseBaseline > -9000.0f && baselineOriginY > m_globalCollapseBaseline + 0.5f)
        {
            m_collapsing = true;
        }

        if (m_collapsing)
        {
            drawY = m_globalCollapseBaseline - metrics.baseline;
            drawX = m_collapseX;
            if (m_needsGap)
            {
                drawX += metrics.height * 0.25f;
                m_needsGap = false;
            }
            m_collapseX = drawX + metrics.width;
        }
        else
        {
            m_needsGap = false;
            if (std::abs(baselineOriginY - m_currentLayoutBaseline) > 0.5f)
            {
                m_currentLayoutBaseline = baselineOriginY;
                m_currentLineEndX = effectiveOriginX + metrics.width;
            }
            else
            {
                m_currentLineEndX = std::max(m_currentLineEndX, effectiveOriginX + metrics.width);
            }

            if (std::abs(baselineOriginY - m_globalCollapseBaseline) < 0.5f)
            {
                m_collapseX = m_currentLineEndX;
            }
        }

        if (drawX >= m_bounds.right)
        {
            return S_OK;
        }

        checkHr(inlineObject->Draw(clientDrawingContext, this, drawX, drawY, isSideways, isRightToLeft, clientDrawingEffect));
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE FadeTextRenderer::DrawUnderline(void*, FLOAT, FLOAT, DWRITE_UNDERLINE const*, IUnknown*) { return S_OK; }
    HRESULT STDMETHODCALLTYPE FadeTextRenderer::DrawStrikethrough(void*, FLOAT, FLOAT, DWRITE_STRIKETHROUGH const*, IUnknown*) { return S_OK; }
}
