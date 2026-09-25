module;
#include "Windows.Headers.h"
export module ClaFi.Platform.Windows.DWrite;

import ClaFi.Platform.Windows.DWrite.Renderer;
import ClaFi.Platform.Windows.Diagnostic;

import ClaFi.Core.Context.ControlContext;
import ClaFi.Core.AppTheme_Colors;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.BakedText;
import ClaFi.Core.TextEngine.Layout;
import ClaFi.Core.TextEngine.Mono;

import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Cpu_GlyphCompositor;
import ClaFi.Core.Graphics.Types;

import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;
import ClaFi.Core.Context.PaintIconEvent;

namespace ClaFi::PlatformImplementation::Windows
{
    using Microsoft::WRL::ComPtr;

    // Concrete COM Implementation of our run effect [CP]
    class DWriteRunEffect : public IdWriteRunEffect
    {
    public:
        DWriteRunEffect(const Graphics::Brush& brush, float baselineShift) : m_refCount(1), m_brush(brush), m_baselineShift(baselineShift) {}
        virtual ~DWriteRunEffect() = default;

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
            if (riid == __uuidof(IUnknown) || riid == IID_DWriteRunEffect) {
                *ppv = static_cast<IdWriteRunEffect*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
        ULONG STDMETHODCALLTYPE Release() override {
            ULONG count = InterlockedDecrement(&m_refCount);
            if (count == 0) delete this;
            return count;
        }

        Graphics::Brush getBrush() const override { return m_brush; }
        float baselineShift() const override { return m_baselineShift; }
    private:
        ULONG m_refCount;
        Graphics::Brush m_brush;
        float m_baselineShift;
    };

    export class CustomInlineObject : public IDWriteInlineObject
    {
    public:
        CustomInlineObject(const BakedInlineObject& icon, float scale, float overridedWidth);
        virtual ~CustomInlineObject() = default;

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        HRESULT STDMETHODCALLTYPE Draw(
            void* clientDrawingContext,
            IDWriteTextRenderer* renderer,
            FLOAT originX,
            FLOAT originY,
            BOOL isSideways,
            BOOL isRightToLeft,
            IUnknown* clientDrawingEffect) override;

        HRESULT STDMETHODCALLTYPE GetMetrics(DWRITE_INLINE_OBJECT_METRICS* metrics) override;
        HRESULT STDMETHODCALLTYPE GetOverhangMetrics(DWRITE_OVERHANG_METRICS* overhangs) override;

        HRESULT STDMETHODCALLTYPE GetBreakConditions(
            DWRITE_BREAK_CONDITION* breakBefore,
            DWRITE_BREAK_CONDITION* breakAfter) override;

    private:
        ULONG m_refCount;
        BakedInlineObject m_icon;
        float m_scale;
    };

    template <typename TContainer, typename TApply>
    void applySpansToLayout(const TContainer& spans, std::size_t pStart, std::size_t pEnd, TApply&& applyFn)
    {
        for (const auto& span : spans)
        {
            std::size_t sStart = span.range.start;
            std::size_t sEnd = sStart + span.range.length;

            if (sStart < pEnd && sEnd > pStart)
            {
                std::size_t intersectStart = std::max(pStart, sStart);
                std::size_t intersectEnd = std::min(pEnd, sEnd);

                DWRITE_TEXT_RANGE dRange;
                dRange.startPosition = static_cast<UINT32>(intersectStart - pStart);
                dRange.length = static_cast<UINT32>(intersectEnd - intersectStart);

                applyFn(span, dRange);
            }
        }
    }

    // DirectWrite answers into a buffer the caller sizes, so every question about a range or a
    // line needs one. Held across calls, and no-init because DirectWrite writes every element it
    // is given room for - what a longer earlier answer left past the end is never read.
    //
    // Two buffers rather than one: getLineMetrics walks the lines and hit tests inside that walk,
    // so both answers are alive at the same moment.
    using LineMetricsBuffer = std::vector<DWRITE_LINE_METRICS, NoInitAllocator<DWRITE_LINE_METRICS>>;
    using HitTestBuffer = std::vector<DWRITE_HIT_TEST_METRICS, NoInitAllocator<DWRITE_HIT_TEST_METRICS>>;

    static LineMetricsBuffer g_lineMetrics;
    static HitTestBuffer g_hitTestMetrics;

    // One font at the size a layout works in - what a tab stop interval is measured from.
    struct FontRequest
    {
        std::wstring family;
        float size;
        FontWeight weight;
        FontStyle style;

        auto operator<=>(const FontRequest&) const = default;
    };

    // The advance of a space in each font asked about so far. A document builds a layout per
    // paragraph and every paragraph of it starts in the same few fonts, so a space is measured
    // once per font rather than once per paragraph.
    using SpaceAdvances = std::map<FontRequest, float>;
    static SpaceAdvances g_spaceAdvances;

    // One space laid out in the font, over the base format the layouts are built from - the
    // layout every measurement of a font is read off, so that it is measured the way the layouts
    // measure it.
    [[nodiscard]] static ComPtr<IDWriteTextLayout> probeLayout(const FontRequest&);
    // The advance of a space in the font, in the layout's units.
    [[nodiscard]] static float spaceAdvance(const FontRequest&);
    // The value in force at a paragraph's first character - or, for an empty paragraph, the last
    // one stated, which is the style in force where it stands.
    template <typename Span, typename Value>
    [[nodiscard]] static Value leadingSpanValue(const std::vector<Span>& spans, bool isEmpty,
        const Value& fallback);
    // The font a paragraph starts in, at the size its layout works in.
    [[nodiscard]] static FontRequest leadingFont(const INativeTextLayout::BuildParams&);

    class DWriteLayout : public INativeTextLayout
    {
    public:
        DWriteLayout() = default;
        ~DWriteLayout() override = default;

        void build(const BuildParams& params) override;
        void setAlignment(TextAlign) override;
        void setMaxWidth(float availableWidth) override;
        NativeParagraphMetrics getMetrics() const override;
        void appendLineMetrics(std::vector<NativeLineMetrics>&) const override;
        float getCharX(std::size_t localPos, bool trailing) const override;
        std::size_t hitTestPoint(FloatPoint localPt, bool* isTrailing) const override;
        FloatRect getCharRect(std::size_t localPos, bool trailing) const override;
        void appendInkAcross(TextRange localRange, float bandTop, float bandBottom,
            std::vector<Graphics::InkExtent>&) const override;

        void draw(ControlPaintContext&, const FloatRect& bounds,
            std::span<const ColorSpan> colors,
            std::span<const ScriptSpan> scripts,
            std::span<const TextRange> hitRanges,
            const TextRange& selectionRange,
            std::span<const float> localBaselinesToFade,
            float localCollapseBaseline,
            float collapseXOffset) override;

    private:
        // Aligns m_layout as the box allows, and answers whether that moved anything.
        bool alignLines();
    private:
        ComPtr<IDWriteTextLayout> m_layout;
        NativeParagraphMetrics m_metrics{ 0.0f, 0.0f };
        // What the run effects last applied to m_layout came to. Setting a drawing effect throws
        // away the layout analysis, which a retained layout has already paid for - so a run whose
        // colours and scripts have not moved since the last draw must not touch it.
        std::size_t m_appliedRuns{ 0 };
        bool m_runsWereApplied{ false };
        // The factor the layout was built at. A script shift arrives in design units, the way a
        // font size does, and is put into the units the layout works in here.
        float m_scaleFactor{ 1.0f };
        // What the layout was built over. DirectWrite does not report it back, and clearing the
        // drawing effects needs a range covering the whole of it.
        UINT32 m_textLength{ 0 };
        // The width the lines were broken at. DirectWrite reports the box it was given rather than
        // this, and alignLines has to know what a line is wider than.
        float m_availableWidth{ 0.0f };
        // The widest line without its trailing whitespace, as the build broke it.
        float m_inkWidth{ 0.0f };
        // The alignment the paragraph asked for, which a line too wide for the box overrules.
        DWRITE_TEXT_ALIGNMENT m_requestedAlignment{ DWRITE_TEXT_ALIGNMENT_LEADING };
        // What m_layout is actually aligned to. Every setter on an IDWriteTextLayout throws away
        // the analysis the build has already paid for, and the GetMetrics after it pays for that
        // again - so an alignment that has not moved must not be stated a second time. The same
        // reason m_appliedColors exists, and the same cost: a paragraph of Left text is LEADING
        // already, so stating it re-breaks every line to arrive at the lines it had.
        DWRITE_TEXT_ALIGNMENT m_appliedAlignment{ DWRITE_TEXT_ALIGNMENT_LEADING };
    };

    // A monospace face DirectWrite resolved, drawn through the layouts' run primitive. See Platform
    class DWriteMonoFont : public INativeMonoFont
    {
    public:
        DWriteMonoFont(ComPtr<IDWriteFontFace> face, float emSize, float cellWidth,
            const DWRITE_LINE_METRICS& line, UINT32 cellDesignAdvance);
        ~DWriteMonoFont() override = default;
    public:
        [[nodiscard]] float cellWidth() const override { return m_cellWidth; }
        [[nodiscard]] float lineHeight() const override { return m_lineHeight; }
        [[nodiscard]] float baseline() const override { return m_baseline; }
        [[nodiscard]] MonoGlyph glyphOf(char32_t) const override;
        void drawRun(ControlPaintContext&, FloatPoint baselineOrigin, std::span<const MonoGlyph>,
            const Graphics::Brush&) override;
        [[nodiscard]] std::optional<Graphics::InkExtent> inkAcross(MonoGlyph, float bandTop,
            float bandBottom) override;
    private:
        ComPtr<IDWriteFontFace> m_face;
        float m_emSize;
        float m_cellWidth;
        float m_lineHeight;
        float m_baseline;
        // The advance of the space in design units, which every glyph on the cells has to
        // match. Compared as the integers the face states, so no scaled float is compared.
        UINT32 m_cellDesignAdvance;
        // One cell per glyph of a run, as DirectWrite asks for advances. Grown to the longest
        // run seen and filled with the cell.
        std::vector<float> m_advances{};
        Graphics::Cpu::GlyphCompositor m_compositor{};
    };

    // A renderer that draws nothing and records where the glyphs of a text range put ink between
    // two heights. A layout is walked through it at its own origin, so what it records is in the
    // layout's coordinates. See TextEngine-Types#links
    class InkCollector : public IDWriteTextRenderer
    {
    public:
        InkCollector(TextRange range, float bandTop, float bandBottom,
            std::vector<Graphics::InkExtent>& result);
        virtual ~InkCollector() = default;

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
        ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
        ULONG STDMETHODCALLTYPE Release() override { return 1; }

        HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void*, BOOL* disabled) override;
        HRESULT STDMETHODCALLTYPE GetCurrentTransform(void*, DWRITE_MATRIX* transform) override;
        HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void*, FLOAT* pixelsPerDip) override;

        HRESULT STDMETHODCALLTYPE DrawGlyphRun(void*, FLOAT baselineOriginX,
            FLOAT baselineOriginY, DWRITE_MEASURING_MODE, DWRITE_GLYPH_RUN const*,
            DWRITE_GLYPH_RUN_DESCRIPTION const*, IUnknown* clientDrawingEffect) override;
        HRESULT STDMETHODCALLTYPE DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*,
            BOOL, BOOL, IUnknown*) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE DrawUnderline(void*, FLOAT, FLOAT, DWRITE_UNDERLINE const*,
            IUnknown*) override { return S_OK; }
        HRESULT STDMETHODCALLTYPE DrawStrikethrough(void*, FLOAT, FLOAT,
            DWRITE_STRIKETHROUGH const*, IUnknown*) override { return S_OK; }
    private:
        TextRange m_range;
        float m_bandTop;
        float m_bandBottom;
        std::vector<Graphics::InkExtent>& m_result;
        // Which glyphs of the run being walked belong to the range.
        std::vector<bool> m_wanted{};
    };

    // Everything the effect loop would install, in the order it would install it. The strongest
    // text ink is in here because it decides which pieces are skipped, not only what the rest come
    // out as.
    static std::size_t runSignature(std::span<const ColorSpan> colors, std::span<const ScriptSpan> scripts,
        const ControlPaintContext& theme)
    {
        std::size_t hash = 14695981039346656037ull;
        auto combine = [&hash](std::size_t value)
            {
                hash ^= value;
                hash *= 1099511628211ull;
            };

        combine(theme.textRgb(InkGrade::Strongest).asUint());
        for (const auto& colorSpan : colors)
        {
            combine(colorSpan.range.start);
            combine(colorSpan.range.length);
            combine(colorSpan.value.index() == 0
                ? theme.inkRgb(std::get<0>(colorSpan.value)).asUint()
                : std::get<1>(colorSpan.value).asUint());
        }
        for (const ScriptSpan& script : scripts)
        {
            combine(script.range.start);
            combine(script.range.length);
            std::uint32_t bits = 0;
            std::memcpy(&bits, &script.value, sizeof(float));
            combine(bits);
        }
        return hash;
    }


    //-------------------------------------------------------------------------



    CustomInlineObject::CustomInlineObject(const BakedInlineObject& icon, float scale, float overridedWidth)
        : m_refCount{ 1 }
        , m_icon{ icon }
        , m_scale{ scale }
    {
        m_icon.width = overridedWidth;
    }

    HRESULT STDMETHODCALLTYPE CustomInlineObject::QueryInterface(REFIID riid, void** ppv)
    {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWriteInlineObject)) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG CustomInlineObject::AddRef() { return InterlockedIncrement(&m_refCount); }
    ULONG CustomInlineObject::Release()
    {
        ULONG newCount = InterlockedDecrement(&m_refCount);
        if (newCount == 0) delete this;
        return newCount;
    }

    HRESULT STDMETHODCALLTYPE CustomInlineObject::Draw(
        void* clientDrawingContext,
        IDWriteTextRenderer*,
        FLOAT originX,
        FLOAT originY,
        BOOL, BOOL, IUnknown*)
    {
        if (clientDrawingContext && m_icon.paintLambda)
        {
            auto* controlContext = static_cast<ControlPaintContext*>(clientDrawingContext);

            FloatRect rect;
            rect.left = originX;
            rect.top = originY;
            rect.right = originX + m_icon.width * m_scale;
            rect.bottom = originY + m_icon.height * m_scale;

            // No state factors: a run has no way to reach the control it is inside, so an in text icon
            // sees none - which is what it saw before the merge too. Full opacity for the same reason:
            // there was nothing here to fade it with.
            // A text run has no control behind it to be disabled, and the colours it hands over
            // are the ones the layout resolved, so nothing is faded on the way in.
            PaintIconEvent evt{ *controlContext, rect, m_icon.tag, 0.0f };
            m_icon.paintLambda(evt);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CustomInlineObject::GetMetrics(DWRITE_INLINE_OBJECT_METRICS* metrics)
    {
        metrics->width = m_icon.width * m_scale;
        metrics->height = m_icon.height * m_scale;
        metrics->baseline = m_icon.baseline * m_scale;
        metrics->supportsSideways = FALSE;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CustomInlineObject::GetOverhangMetrics(DWRITE_OVERHANG_METRICS* overhangs)
    {
        overhangs->left = 0; overhangs->right = 0; overhangs->top = 0; overhangs->bottom = 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CustomInlineObject::GetBreakConditions(DWRITE_BREAK_CONDITION* breakBefore, DWRITE_BREAK_CONDITION* breakAfter)
    {
        // An inline object breaks like the text around it. A control that must not be broken at
        // all says so once, with WordWrap::No.
        *breakBefore = DWRITE_BREAK_CONDITION_NEUTRAL;
        *breakAfter = DWRITE_BREAK_CONDITION_NEUTRAL;
        return S_OK;
    }

    ComPtr<IDWriteTextLayout> probeLayout(const FontRequest& font)
    {
        ComPtr<IDWriteTextLayout> probe;
        checkHr(g_dwriteFactory->CreateTextLayout(
            L" ",
            1,
            getOrCreateBaseFormat2().Get(),
            999999.0f,
            999999.0f,
            &probe
        ));
        const DWRITE_TEXT_RANGE range = { 0, 1 };
        checkHr(probe->SetFontFamilyName(nativeFamilyName(font.family.c_str()), range));
        checkHr(probe->SetFontSize(font.size, range));
        checkHr(probe->SetFontWeight(static_cast<DWRITE_FONT_WEIGHT>(font.weight), range));
        checkHr(probe->SetFontStyle(
            font.style == FontStyle::Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
            range));
        return probe;
    }

    float spaceAdvance(const FontRequest& font)
    {
        SpaceAdvances::const_iterator found = g_spaceAdvances.find(font);
        if (found != g_spaceAdvances.end())
        {
            return found->second;
        }

        DWRITE_TEXT_METRICS metrics;
        checkHr(probeLayout(font)->GetMetrics(&metrics));
        const float advance = metrics.widthIncludingTrailingWhitespace;
        g_spaceAdvances.emplace(font, advance);
        return advance;
    }

    template <typename Span, typename Value>
    Value leadingSpanValue(const std::vector<Span>& spans, bool isEmpty, const Value& fallback)
    {
        if (spans.empty())
        {
            return fallback;
        }
        if (isEmpty)
        {
            return spans.back().value;
        }
        return spans.front().range.start == 0 ? spans.front().value : fallback;
    }

    FontRequest leadingFont(const INativeTextLayout::BuildParams& params)
    {
        const TextStyle& body = k_textStyles[static_cast<std::size_t>(TextStyleId::Body)];
        return {
            leadingSpanValue(params.families, params.isEmpty, std::wstring{ body.family }),
            leadingSpanValue(params.sizes, params.isEmpty, body.size) * params.scaleFactor,
            leadingSpanValue(params.weights, params.isEmpty, body.weight),
            leadingSpanValue(params.styles, params.isEmpty, body.style),
        };
    }

    void DWriteLayout::build(const BuildParams& params)
    {
        // A new layout carries no drawing effects, whatever the old one carried, and is aligned
        // LEADING whatever the old one was aligned to.
        m_runsWereApplied = false;
        m_requestedAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
        m_appliedAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
        m_textLength = static_cast<UINT32>(params.text.length());
        m_availableWidth = params.availableWidth;
        m_scaleFactor = params.scaleFactor;

        m_layout.Reset();
        auto baseFormat = getOrCreateBaseFormat2();

        checkHr(g_dwriteFactory->CreateTextLayout(
            params.text.data(),
            static_cast<UINT32>(params.text.length()),
            baseFormat.Get(),
            params.availableWidth,
            999999.0f,
            &m_layout
        ));

        // A run that is not broken to its box is one line however narrow the box gets, which is
        // what lets a horizontally scrolled control come out as wide as its text.
        checkHr(m_layout->SetWordWrapping(params.wrap
            ? DWRITE_WORD_WRAPPING_WHOLE_WORD
            : DWRITE_WORD_WRAPPING_NO_WRAP));

        // The base format's own stop is 4 body ems, no whole number of columns in any font. Stated
        // ahead of the first metric read, where a setter costs nothing.
        checkHr(m_layout->SetIncrementalTabStop(k_tabStopSpaces * spaceAdvance(leadingFont(params))));

        if (params.isEmpty) {
            DWRITE_TEXT_RANGE dRange{ 0, 1 };
            const auto& bodyProps = k_textStyles[static_cast<std::size_t>(TextStyleId::Body)];

            auto family = params.families.empty() ? std::wstring(bodyProps.family) : params.families.back().value;
            checkHr(m_layout->SetFontFamilyName(nativeFamilyName(family.c_str()), dRange));

            auto size = params.sizes.empty() ? bodyProps.size : params.sizes.back().value;
            checkHr(m_layout->SetFontSize(size * params.scaleFactor, dRange));

            auto weight = params.weights.empty() ? bodyProps.weight : params.weights.back().value;
            checkHr(m_layout->SetFontWeight(static_cast<DWRITE_FONT_WEIGHT>(weight), dRange));

            auto style = params.styles.empty() ? bodyProps.style : params.styles.back().value;
            checkHr(m_layout->SetFontStyle(style == FontStyle::Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, dRange));
        }
        else {
            applySpansToLayout(params.families, 0, params.text.length(), [&](const auto& span, DWRITE_TEXT_RANGE dRange){
                checkHr(m_layout->SetFontFamilyName(nativeFamilyName(span.value.c_str()), dRange));
            });
            applySpansToLayout(params.sizes, 0, params.text.length(), [&](const auto& span, DWRITE_TEXT_RANGE dRange){
                checkHr(m_layout->SetFontSize(span.value * params.scaleFactor, dRange));
            });
            applySpansToLayout(params.weights, 0, params.text.length(), [&](const auto& span, DWRITE_TEXT_RANGE dRange){
                checkHr(m_layout->SetFontWeight(static_cast<DWRITE_FONT_WEIGHT>(span.value), dRange));
            });
            applySpansToLayout(params.styles, 0, params.text.length(), [&](const auto& span, DWRITE_TEXT_RANGE dRange){
                checkHr(m_layout->SetFontStyle(span.value == FontStyle::Italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, dRange));
            });
        }

        // Set Inline Objects with finalized unscaled widths passed from buildParams
        for (const auto& [localIdx, bakedIcon] : params.inlineObjects) {
            DWRITE_TEXT_RANGE dRange{ static_cast<UINT32>(localIdx), 1 };
            IDWriteInlineObject* nativeIcon = new CustomInlineObject(
                bakedIcon,
                params.scaleFactor,
                bakedIcon.width // Finalized unscaled width
            );
            checkHr(m_layout->SetInlineObject(nativeIcon, dRange));
            nativeIcon->Release();
        }

        DWRITE_TEXT_METRICS metrics;
        checkHr(m_layout->GetMetrics(&metrics));

        m_metrics = { metrics.widthIncludingTrailingWhitespace, metrics.height };
        m_inkWidth = metrics.width;
    }

    void DWriteLayout::setAlignment(TextAlign alignment)
    {
        DWRITE_TEXT_ALIGNMENT dAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
        switch (alignment)
        {
            case TextAlign::Left:
                dAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
                break;
            case TextAlign::Center:
                dAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
                break;
            case TextAlign::Right:
                dAlignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
                break;
            case TextAlign::Justified:
                dAlignment = DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
                break;
        }
        m_requestedAlignment = dAlignment;

        if (!alignLines())
            return;

        // Justified stretches the lines it fills, so what the layout came to is read again rather
        // than kept from the build that had not been told where the lines would sit. Only on the
        // branch that moved something: the metrics of a layout left alone are the ones it already
        // reported.
        //
        // This is the layout's placed extent, and it is not what a paragraph is MEASURED by -
        // TextLayout::shapeParagraph reads that before it gets here, and the comment there says
        // why it has to.
        DWRITE_TEXT_METRICS aligned;
        checkHr(m_layout->GetMetrics(&aligned));
        m_metrics = { aligned.widthIncludingTrailingWhitespace, aligned.height };
    }

    void DWriteLayout::setMaxWidth(float availableWidth)
    {
        m_availableWidth = availableWidth;
        checkHr(m_layout->SetMaxWidth(availableWidth));
        alignLines();

        // The box the lines are placed in is the box a justified line is stretched to, so what the
        // paragraph comes to is read again.
        DWRITE_TEXT_METRICS metrics;
        checkHr(m_layout->GetMetrics(&metrics));
        m_metrics = { metrics.widthIncludingTrailingWhitespace, metrics.height };
    }

    NativeParagraphMetrics DWriteLayout::getMetrics() const
    {
        return m_metrics;
    }

    void DWriteLayout::appendLineMetrics(std::vector<NativeLineMetrics>& result) const {
        if (!m_layout) return;

        UINT32 lineCount = 0;
        m_layout->GetLineMetrics(nullptr, 0, &lineCount);
        if (lineCount > 0) {
            g_lineMetrics.resize(lineCount);
            m_layout->GetLineMetrics(g_lineMetrics.data(), lineCount, &lineCount);

            std::size_t currentPos = 0;
            for (const auto& dwLine : g_lineMetrics) {
                // Calculate the line width via hit-testing (excluding trailing whitespace)
                float lineWidth = 0.0f;
                UINT32 lineLen = dwLine.length;
                if (lineLen > dwLine.trailingWhitespaceLength) {
                    UINT32 checkLen = lineLen - dwLine.trailingWhitespaceLength;
                    UINT32 count = 0;
                    m_layout->HitTestTextRange(static_cast<UINT32>(currentPos), checkLen, 0.0f, 0.0f, nullptr, 0, &count);
                    if (count > 0) {
                        g_hitTestMetrics.resize(count);
                        if (SUCCEEDED(m_layout->HitTestTextRange(static_cast<UINT32>(currentPos), checkLen, 0.0f, 0.0f, g_hitTestMetrics.data(), count, &count))) {
                            for (UINT32 i = 0; i < count; ++i) {
                                lineWidth += g_hitTestMetrics[i].width;
                            }
                        }
                    }
                }

                result.push_back({
                  .textStart = currentPos,
                  .textLength = dwLine.length,
                  .width = lineWidth, // Correctly resolved!
                  .height = dwLine.height,
                  .baseline = dwLine.baseline,
                  .isTrimmed = dwLine.isTrimmed == TRUE
                    });
                currentPos += dwLine.length;
            }
        }
    }

    float DWriteLayout::getCharX(std::size_t localPos, bool trailing) const
    {
        if (!m_layout) return 0.0f;
        FLOAT hx, hy;
        DWRITE_HIT_TEST_METRICS htm;
        if (SUCCEEDED(m_layout->HitTestTextPosition(static_cast<UINT32>(localPos), trailing ? TRUE : FALSE, &hx, &hy, &htm))) {
            return hx;
        }
        return 0.0f;
    }

    std::size_t DWriteLayout::hitTestPoint(FloatPoint localPt, bool* isTrailing) const
    {
        if (!m_layout) return 0;
        BOOL trailing = FALSE;
        BOOL isInside = FALSE;
        DWRITE_HIT_TEST_METRICS htm;
        m_layout->HitTestPoint(localPt.x, localPt.y, &trailing, &isInside, &htm);
        *isTrailing = (trailing == TRUE);
        return htm.textPosition;
    }

    FloatRect DWriteLayout::getCharRect(std::size_t localPos, bool trailing) const
    {
        if (!m_layout) return {};
        DWRITE_HIT_TEST_METRICS htm;
        FLOAT cx, cy;
        if (SUCCEEDED(m_layout->HitTestTextPosition(static_cast<UINT32>(localPos), trailing ? TRUE : FALSE, &cx, &cy, &htm))) {
            FLOAT finalY = cy;
            FLOAT finalH = htm.height;

            UINT32 lineCount = 0;
            m_layout->GetLineMetrics(nullptr, 0, &lineCount);
            if (lineCount > 0) {
                g_lineMetrics.resize(lineCount);
                m_layout->GetLineMetrics(g_lineMetrics.data(), lineCount, &lineCount);

                FLOAT lineTop = 0.0f;
                UINT32 currentStrPos = 0;
                for (UINT32 i = 0; i < lineCount; ++i) {
                    if (localPos >= currentStrPos && localPos < currentStrPos + g_lineMetrics[i].length) {
                        finalY = lineTop;
                        finalH = g_lineMetrics[i].height;
                        break;
                    }
                    lineTop += g_lineMetrics[i].height;
                    currentStrPos += g_lineMetrics[i].length;
                }
            }

            return { cx, finalY, cx, finalY + finalH };
        }
        return {};
    }

    void DWriteLayout::appendInkAcross(TextRange localRange, float bandTop, float bandBottom,
        std::vector<Graphics::InkExtent>& result) const
    {
        if (!m_layout || !localRange.length)
            return;
        InkCollector collector{ localRange, bandTop, bandBottom, result };
        checkHr(m_layout->Draw(nullptr, &collector, 0.0f, 0.0f));
    }

    void DWriteLayout::draw(
        ControlPaintContext& controlContext,
        const FloatRect& bounds,
        std::span<const ColorSpan> colors,
        std::span<const ScriptSpan> scripts,
        std::span<const TextRange> hitRanges,
        const TextRange& selectionRange,
        std::span<const float> localBaselinesToFade,
        float localCollapseBaseline,
        float collapseXOffset)
    {
        if (!m_layout) return;

        const auto nativeCanvas = controlContext.canvas().backend();
        if (!nativeCanvas) return;

        ID2D1RenderTarget* rt = nativeCanvas->type() == Graphics::BackendType::Direct2D
            ? static_cast<ID2D1RenderTarget*>(nativeCanvas->getNativeRenderTarget())
            : nullptr;

        auto drawLocalRangeBg = [&](const TextRange& range, Color color) {
            if (range.length == 0 || range.start == static_cast<std::size_t>(-1)) return;

            if (rt) {
                ComPtr<ID2D1SolidColorBrush> brush;
                brush.Attach(static_cast<ID2D1SolidColorBrush*>(nativeCanvas->getNativeBrush(Graphics::SolidColor{ color })));
                if (!brush) return;

                UINT32 count = 0;
                checkHr(m_layout->HitTestTextRange(static_cast<UINT32>(range.start), static_cast<UINT32>(range.length), 0, 0, nullptr, 0, &count));
                if (count > 0) {
                    g_hitTestMetrics.resize(count);
                    checkHr(m_layout->HitTestTextRange(static_cast<UINT32>(range.start), static_cast<UINT32>(range.length), 0, 0, g_hitTestMetrics.data(), count, &count));
                    for (UINT32 i = 0; i < count; ++i) {
                        const auto& htm = g_hitTestMetrics[i];
                        D2D1_RECT_F d2dRect = {
                          bounds.left + htm.left,
                          bounds.top + htm.top,
                          bounds.left + htm.left + htm.width,
                          bounds.top + htm.top + htm.height
                        };
                        rt->FillRectangle(&d2dRect, brush.Get());
                    }
                }
            }
            else {
                // CPU Path: Draw selection/highlight background using native CPU Canvas fills [CP]
                UINT32 count = 0;
                checkHr(m_layout->HitTestTextRange(static_cast<UINT32>(range.start), static_cast<UINT32>(range.length), 0, 0, nullptr, 0, &count));
                if (count > 0) {
                    g_hitTestMetrics.resize(count);
                    checkHr(m_layout->HitTestTextRange(static_cast<UINT32>(range.start), static_cast<UINT32>(range.length), 0, 0, g_hitTestMetrics.data(), count, &count));
                    for (UINT32 i = 0; i < count; ++i) {
                        const auto& htm = g_hitTestMetrics[i];
                        nativeCanvas->fillRectangle({
                          bounds.left + htm.left,
                          bounds.top + htm.top,
                          bounds.left + htm.left + htm.width,
                          bounds.top + htm.top + htm.height
                            }, Graphics::SolidColor{ color });
                    }
                }
            }
            };

        for (const auto& hr : hitRanges)
        {
            drawLocalRangeBg(hr, controlContext.hit);
        }
        drawLocalRangeBg(selectionRange, controlContext.selectionRgb());

        const std::size_t signature = runSignature(colors, scripts, controlContext);
        if (!m_runsWereApplied || signature != m_appliedRuns)
        {
            m_appliedRuns = signature;
            m_runsWereApplied = true;

            // Applying an effect replaces what that range carried, but a range the new set does
            // not name at all - the ink under a selection that has moved on - keeps the effect it
            // was given. Cleared first, so the loop below states the whole layout rather than the
            // difference from whatever it held before.
            checkHr(m_layout->SetDrawingEffect(nullptr, DWRITE_TEXT_RANGE{ 0, m_textLength }));

            // The scripts are cut INTO the colour spans rather than laid over them: one run
            // carries one drawing effect, so a range that is both coloured and raised has to be
            // stated as a single effect saying both.
            std::size_t scriptCursor = 0;
            for (const auto& colorSpan : colors)
            {
                const Color color = colorSpan.value.index() == 0
                    ? controlContext.inkRgb(std::get<0>(colorSpan.value))
                    : std::get<1>(colorSpan.value);

                // A piece in the default colour standing on the baseline asks for exactly what the
                // renderer falls back to when a run carries no effect, so applying one changes
                // nothing on screen. It is not free though - setting a drawing effect discards the
                // layout analysis that build() has already paid for, and Draw() then repeats it.
                const bool defaultColor = color == controlContext.textRgb(InkGrade::Strongest);

                const std::size_t colorEnd = colorSpan.range.end();
                while (scriptCursor != scripts.size() && scripts[scriptCursor].range.end() <= colorSpan.range.start)
                    ++scriptCursor;

                // A script can reach past this colour span into the next one, so the walk inside
                // one span keeps a cursor of its own and leaves the shared one where it stands.
                std::size_t cursor = scriptCursor;
                std::size_t pieceStart = colorSpan.range.start;
                while (pieceStart < colorEnd)
                {
                    float shift = 0.0f;
                    std::size_t pieceEnd = colorEnd;
                    if (cursor != scripts.size() && scripts[cursor].range.start < colorEnd)
                    {
                        const TextRange& scriptRange = scripts[cursor].range;
                        if (scriptRange.start <= pieceStart)
                        {
                            shift = scripts[cursor].value * m_scaleFactor;
                            pieceEnd = std::min(colorEnd, scriptRange.end());
                            ++cursor;
                        }
                        else
                            pieceEnd = scriptRange.start;
                    }

                    if (!defaultColor || shift != 0.0f)
                    {
                        // The effect stays on the layout for as long as the signature holds,
                        // which is longer than one frame and longer than one backend: the same
                        // layout is drawn through whichever backend owns the form it appears in,
                        // and a dropdown owns its own render target and so its own ID2D1Device. A
                        // native brush carries that device with it, so what the effect holds is
                        // the colour, resolved against the drawing backend in
                        // FadeTextRenderer::DrawGlyphRun. That keeps one effect type across both
                        // paths.
                        ComPtr<IdWriteRunEffect> drawingEffect;
                        drawingEffect.Attach(new DWriteRunEffect(Graphics::SolidColor{ color }, shift));

                        DWRITE_TEXT_RANGE dRange{ static_cast<UINT32>(pieceStart), static_cast<UINT32>(pieceEnd - pieceStart) };
                        checkHr(m_layout->SetDrawingEffect(drawingEffect.Get(), dRange));
                    }

                    pieceStart = pieceEnd;
                }
            }
        }

        const float absCollapseBaseline = localCollapseBaseline > -9000.0f ? localCollapseBaseline + bounds.top : localCollapseBaseline;

        FadeTextRenderer renderer(
            rt,
            bounds,
            controlContext.scaleF(30.0f),
            localBaselinesToFade,
            absCollapseBaseline,
            collapseXOffset,
            &controlContext.canvas()
        );

        checkHr(m_layout->Draw(&controlContext, &renderer, bounds.left, bounds.top));
    }

    bool DWriteLayout::alignLines()
    {
        // A line wider than the box has no room left to be moved in, and moving it anyway carries
        // its start off the leading edge where the clip takes it. Measured without the trailing
        // whitespace, which is what a line is allowed to overhang with.
        const DWRITE_TEXT_ALIGNMENT resolved = m_inkWidth > m_availableWidth + 0.1f
            ? DWRITE_TEXT_ALIGNMENT_LEADING
            : m_requestedAlignment;

        // Resolved rather than requested, because the overflow test above can answer LEADING for
        // a paragraph that asked for Center - and that answer moves with the box width while the
        // TextAlign stands still, so it is the resolved value that has to be remembered.
        if (resolved == m_appliedAlignment)
            return false;

        checkHr(m_layout->SetTextAlignment(resolved));
        m_appliedAlignment = resolved;
        return true;
    }


    //-------------------------------------------------------------------------


    // InkCollector

    InkCollector::InkCollector(TextRange range, float bandTop, float bandBottom,
        std::vector<Graphics::InkExtent>& result)
        :
        m_range{ range },
        m_bandTop{ bandTop },
        m_bandBottom{ bandBottom },
        m_result{ result }
    {
    }

    HRESULT STDMETHODCALLTYPE InkCollector::QueryInterface(REFIID riid, void** ppv)
    {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWritePixelSnapping)
            || riid == __uuidof(IDWriteTextRenderer))
        {
            *ppv = this;
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    // The glyphs where the layout puts them, and not where a snapped draw rounds them to - the
    // gap the ink cuts is wider than the rounding by the underline's own thickness.
    HRESULT STDMETHODCALLTYPE InkCollector::IsPixelSnappingDisabled(void*, BOOL* disabled)
    {
        *disabled = TRUE;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE InkCollector::GetCurrentTransform(void*, DWRITE_MATRIX* transform)
    {
        *reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform) = D2D1::IdentityMatrix();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE InkCollector::GetPixelsPerDip(void*, FLOAT* pixelsPerDip)
    {
        *pixelsPerDip = 1.0f;
        return S_OK;
    }

    // Read off the coverage the CPU path draws the same glyphs from, which Direct2D does not
    // fill - so on that path the glyphs of a hovered link are rasterized into it here. Their pen
    // positions are the ones FadeTextRenderer walks, a script's shift included.
    HRESULT STDMETHODCALLTYPE InkCollector::DrawGlyphRun(void*, FLOAT baselineOriginX,
        FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode,
        DWRITE_GLYPH_RUN const* glyphRun, DWRITE_GLYPH_RUN_DESCRIPTION const* description,
        IUnknown* clientDrawingEffect)
    {
        if (glyphRun->isSideways || glyphRun->glyphCount == 0 || !description
            || !description->clusterMap)
        {
            return S_OK;
        }

        const std::size_t runStart = description->textPosition;
        const std::size_t runEnd = runStart + description->stringLength;
        if (runEnd <= m_range.start || runStart >= m_range.end())
            return S_OK;

        // A character names the first glyph of its cluster, and the cluster's glyphs run to the
        // first glyph of the next one.
        m_wanted.assign(glyphRun->glyphCount, false);
        for (UINT32 k = 0; k != description->stringLength; ++k)
        {
            const std::size_t pos = runStart + k;
            if (pos < m_range.start || pos >= m_range.end())
                continue;

            const UINT16 first = description->clusterMap[k];
            UINT32 next = k + 1;
            while (next != description->stringLength && description->clusterMap[next] == first)
                ++next;
            const UINT32 last = next == description->stringLength
                ? glyphRun->glyphCount
                : description->clusterMap[next];
            for (UINT32 glyph = first; glyph < last && glyph < glyphRun->glyphCount; ++glyph)
                m_wanted[glyph] = true;
        }

        float shift = 0.0f;
        if (clientDrawingEffect)
        {
            IdWriteRunEffect* runEffect = nullptr;
            if (SUCCEEDED(clientDrawingEffect->QueryInterface(IID_DWriteRunEffect,
                reinterpret_cast<void**>(&runEffect))))
            {
                shift = runEffect->baselineShift();
                runEffect->Release();
            }
        }

        FontGlyphs& fontGlyphs = glyphsOf(glyphRun->fontFace, glyphRun->fontEmSize,
            measuringMode);
        const bool rightToLeft = (glyphRun->bidiLevel & 1) != 0;
        float penX = baselineOriginX;
        for (UINT32 i = 0; i != glyphRun->glyphCount; ++i)
        {
            const float advance = glyphRun->glyphAdvances ? glyphRun->glyphAdvances[i] : 0.0f;
            if (rightToLeft)
                penX -= advance;

            if (m_wanted[i])
            {
                FloatPoint origin = { penX, baselineOriginY - shift };
                if (glyphRun->glyphOffsets)
                {
                    origin.x += glyphRun->glyphOffsets[i].advanceOffset;
                    origin.y -= glyphRun->glyphOffsets[i].ascenderOffset;
                }
                const std::optional<Graphics::InkExtent> ink = coverageOf(fontGlyphs,
                    glyphRun->glyphIndices[i], glyphRun->fontEmSize, measuringMode)
                    .inkAcross(m_bandTop - origin.y, m_bandBottom - origin.y);
                if (ink.has_value())
                    m_result.push_back({ origin.x + ink->left, origin.x + ink->right });
            }

            if (!rightToLeft)
                penX += advance;
        }
        return S_OK;
    }


    //-------------------------------------------------------------------------


    // DWriteMonoFont

    DWriteMonoFont::DWriteMonoFont(ComPtr<IDWriteFontFace> face, float emSize, float cellWidth,
        const DWRITE_LINE_METRICS& line, UINT32 cellDesignAdvance)
        :
        m_face{ std::move(face) },
        m_emSize{ emSize },
        m_cellWidth{ cellWidth },
        m_lineHeight{ line.height },
        m_baseline{ line.baseline },
        m_cellDesignAdvance{ cellDesignAdvance }
    {
    }

    MonoGlyph DWriteMonoFont::glyphOf(char32_t codePoint) const
    {
        const UINT32 code = static_cast<UINT32>(codePoint);
        UINT16 glyph = 0;
        if (FAILED(m_face->GetGlyphIndices(&code, 1, &glyph)) || glyph == 0)
            return k_noMonoGlyph;

        DWRITE_GLYPH_METRICS metrics{};
        if (FAILED(m_face->GetDesignGlyphMetrics(&glyph, 1, &metrics)))
            return k_noMonoGlyph;
        if (metrics.advanceWidth != m_cellDesignAdvance)
            return k_noMonoGlyph;
        return glyph;
    }

    void DWriteMonoFont::drawRun(ControlPaintContext& controlContext, FloatPoint baselineOrigin,
        std::span<const MonoGlyph> glyphs, const Graphics::Brush& brush)
    {
        Graphics::IBackend* nativeCanvas = controlContext.canvas().backend();
        if (!nativeCanvas || glyphs.empty())
            return;

        if (m_advances.size() < glyphs.size())
            m_advances.resize(glyphs.size(), m_cellWidth);

        DWRITE_GLYPH_RUN run{};
        run.fontFace = m_face.Get();
        run.fontEmSize = m_emSize;
        run.glyphCount = static_cast<UINT32>(glyphs.size());
        run.glyphIndices = glyphs.data();
        run.glyphAdvances = m_advances.data();
        run.glyphOffsets = nullptr;
        run.isSideways = FALSE;
        run.bidiLevel = 0;

        if (nativeCanvas->type() == Graphics::BackendType::Direct2D)
        {
            // The brush is resolved against the backend drawing, the way FadeTextRenderer resolves
            // one: a native brush belongs to the device that made it, and every form has its own.
            ID2D1RenderTarget* rt = static_cast<ID2D1RenderTarget*>(
                nativeCanvas->getNativeRenderTarget());
            ComPtr<ID2D1Brush> nativeBrush;
            nativeBrush.Attach(static_cast<ID2D1Brush*>(nativeCanvas->getNativeBrush(brush)));
            if (!nativeBrush)
                return;
            rt->DrawGlyphRun(D2D1::Point2F(baselineOrigin.x, baselineOrigin.y), &run,
                nativeBrush.Get(), DWRITE_MEASURING_MODE_NATURAL);
            return;
        }

        // The CPU path: the same glyph cache and compositor every layout's runs go through, with
        // the pen stepping one cell per glyph. Placing per glyph is what lets the run follow the
        // canvas transform.
        FontGlyphs& fontGlyphs = glyphsOf(m_face.Get(), m_emSize, DWRITE_MEASURING_MODE_NATURAL);

        const Graphics::Matrix3x2& canvasTransform = controlContext.canvas().transform();
        const bool transformed = !(canvasTransform == Graphics::Matrix3x2::identity());

        m_compositor.begin();
        float penX = baselineOrigin.x;
        for (const MonoGlyph glyph : glyphs)
        {
            FloatPoint origin = { penX, baselineOrigin.y };
            if (transformed)
                origin = canvasTransform.transform(origin);
            m_compositor.place(coverageOf(fontGlyphs, glyph, m_emSize,
                DWRITE_MEASURING_MODE_NATURAL), origin);
            penX += m_cellWidth;
        }
        m_compositor.composite(*nativeCanvas, brush);
    }

    std::optional<Graphics::InkExtent> DWriteMonoFont::inkAcross(MonoGlyph glyph, float bandTop,
        float bandBottom)
    {
        FontGlyphs& fontGlyphs = glyphsOf(m_face.Get(), m_emSize, DWRITE_MEASURING_MODE_NATURAL);
        return coverageOf(fontGlyphs, glyph, m_emSize, DWRITE_MEASURING_MODE_NATURAL)
            .inkAcross(bandTop, bandBottom);
    }
}

namespace ClaFi
{
    export std::unique_ptr<INativeTextLayout> createNativeTextLayout();
    export std::unique_ptr<INativeMonoFont> createNativeMonoFont(const MonoFontRequest&);

    std::unique_ptr<INativeTextLayout> createNativeTextLayout()
    {
        return std::make_unique<PlatformImplementation::Windows::DWriteLayout>();
    }

    // The face the layouts would set the request in: the system collection's first match on
    // weight and style, which is what a layout's own matching finds. A family the collection
    // does not carry answers null, and the request stays with the layouts and their fallback.
    std::unique_ptr<INativeMonoFont> createNativeMonoFont(const MonoFontRequest& request)
    {
        using namespace PlatformImplementation::Windows;
        using Microsoft::WRL::ComPtr;

        // Made on first use, and the factory with it.
        getOrCreateBaseFormat2();

        ComPtr<IDWriteFontCollection> collection;
        checkHr(g_dwriteFactory->GetSystemFontCollection(&collection));
        UINT32 familyIndex = 0;
        BOOL exists = FALSE;
        checkHr(collection->FindFamilyName(nativeFamilyName(request.family.c_str()),
            &familyIndex, &exists));
        if (!exists)
            return nullptr;

        ComPtr<IDWriteFontFamily> family;
        checkHr(collection->GetFontFamily(familyIndex, &family));
        ComPtr<IDWriteFont> font;
        checkHr(family->GetFirstMatchingFont(
            static_cast<DWRITE_FONT_WEIGHT>(request.weight),
            DWRITE_FONT_STRETCH_NORMAL,
            request.style == FontStyle::Italic
                ? DWRITE_FONT_STYLE_ITALIC
                : DWRITE_FONT_STYLE_NORMAL,
            &font));
        ComPtr<IDWriteFontFace> face;
        checkHr(font->CreateFontFace(&face));

        // The face's own word on its pitch. A face that is not monospaced answers null once, and
        // no paragraph set in it is asked about again.
        ComPtr<IDWriteFontFace1> face1;
        if (FAILED(face.As(&face1)) || !face1->IsMonospacedFont())
            return nullptr;

        const UINT32 space = static_cast<UINT32>(U' ');
        UINT16 spaceGlyph = 0;
        checkHr(face->GetGlyphIndices(&space, 1, &spaceGlyph));
        if (spaceGlyph == 0)
            return nullptr;
        DWRITE_GLYPH_METRICS spaceMetrics{};
        checkHr(face->GetDesignGlyphMetrics(&spaceGlyph, 1, &spaceMetrics));

        // The cell and the line are read off the probe rather than computed from the design
        // metrics, so they are the numbers a layout of the same font comes to - the cell being
        // the space advance the tab stops are already measured from.
        const FontRequest probeRequest = {
            request.family,
            request.size,
            request.weight,
            request.style,
        };
        ComPtr<IDWriteTextLayout> probe = probeLayout(probeRequest);
        DWRITE_TEXT_METRICS metrics;
        checkHr(probe->GetMetrics(&metrics));
        DWRITE_LINE_METRICS line{};
        UINT32 lineCount = 0;
        checkHr(probe->GetLineMetrics(&line, 1, &lineCount));

        return std::make_unique<DWriteMonoFont>(std::move(face), request.size,
            metrics.widthIncludingTrailingWhitespace, line, spaceMetrics.advanceWidth);
    }
}
