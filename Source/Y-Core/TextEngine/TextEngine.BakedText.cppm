export module ClaFi.Core.TextEngine.BakedText;

import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;
import ClaFi.Core.System.InkWell;

namespace ClaFi
{
    // The ink a link is drawn in where no colour is pushed inside it. See TextEngine-Types#links
    constexpr Ink k_linkInk = InkWell::accentInk();

    export struct BakedInlineObject {
        float width{ 0.0f };
        float height{ 0.0f };
        float baseline{ 0.0f };
        PaintIconFunc paintLambda{};
        Tag tag{};
        bool isFlexSpace{ false };
        bool isTabTo{ false };
        // What a TabTo states: the X the text after it starts at, in design units. Apart from
        // `width` because a measuring pass writes the resolved gap into width, and the pass that
        // resolves the stop compares where the character landed against this.
        float tabTargetX{ 0.0f };

        bool operator==(const BakedInlineObject& other) const {
            return width == other.width && height == other.height && baseline == other.baseline && tag == other.tag && isFlexSpace == other.isFlexSpace && isTabTo == other.isTabTo;
        }
    };

    export class BakedText
    {
    public:
        explicit BakedText() = default;
        // The Text is read, never copied, and it is the caller's for as long as this is used:
        // every span here indexes into its characters and plainText hands them straight back.
        // TextLayout is the only caller, and it bakes the Text its own pointer already names.
        void rebuild(const Text& text, bool isEditable = false);
        // Valid from the first rebuild. Nothing reads a BakedText before one - a layout bakes
        // before it shapes.
        const std::wstring& plainText() const { return *m_plainText; }
        const std::vector<ColorSpan>& colors() const { return m_colors; }
        const std::vector<WeightSpan>& weights() const { return m_weights; }
        const std::vector<StyleSpan>& styles() const { return m_styles; }
        const std::vector<SizeSpan>& sizes() const { return m_sizes; }
        const std::vector<FamilySpan>& families() const { return m_families; }
        const std::vector<ScriptSpan>& scripts() const { return m_scripts; }
        const std::vector<ParagraphStyle>& paragraphs() const { return m_paragraphs; }
        const std::vector<std::pair<std::size_t, BakedInlineObject>>& inlineObjects() const { return m_inlineObjects; }
        // The links, in text order and not overlapping. Two that meet and name one target are one
        // span, so a link the edits split into runs is still one link to point at.
        [[nodiscard]] const std::vector<LinkSpan>& links() const { return m_links; }

    private:
        const std::wstring* m_plainText{ nullptr };
        std::vector<ColorSpan> m_colors;
        std::vector<WeightSpan> m_weights;
        std::vector<StyleSpan> m_styles;
        std::vector<SizeSpan> m_sizes;
        std::vector<FamilySpan> m_families;
        std::vector<ScriptSpan> m_scripts;
        std::vector<ParagraphStyle> m_paragraphs;
        std::vector<std::pair<std::size_t, BakedInlineObject>> m_inlineObjects;
        std::vector<LinkSpan> m_links;
    };


    //-------------------------------------------------------------------------


    void BakedText::rebuild(const Text& text, bool isEditable)
    {
        {
            m_colors.clear();
            m_weights.clear();
            m_styles.clear();
            m_sizes.clear();
            m_families.clear();
            m_scripts.clear();
            m_paragraphs.clear();
            m_inlineObjects.clear();
            m_links.clear();
        }
        m_plainText = &text.plainText();
        const std::wstring& plain = *m_plainText;

        TextAlign currentAlign = TextAlign::Left;
        float currentIndent = 0.0f;
        float currentLineSpacing = 1.0f;
        std::size_t currentParagraphStart = 0;

        std::vector<TextStyleId> styleStack;
        std::vector<ColorDef> colorStack;
        std::size_t boldCount = 0; std::size_t italicCount = 0;
        std::vector<float> sizeStack;
        std::vector<std::wstring_view> familyStack;

        // What one open script comes to: the factor the size of every run inside it is taken at,
        // and how far off the line's baseline those runs are drawn. Both are stated against the
        // baseline rather than against the level beneath, so closing a script restores the level
        // beneath it exactly.
        struct OpenScript
        {
            float scale;
            float shift;
        };
        std::vector<OpenScript> scriptStack;

        // One open link: what it names, and how many colours stood open when it opened. A colour
        // pushed inside the link is the link's own and is drawn; one pushed before it gives way to
        // the link ink.
        struct OpenLink
        {
            std::wstring_view target;
            std::size_t colorDepth;
        };
        std::vector<OpenLink> linkStack;

        auto getEffectiveTextStyle = [&]() { return styleStack.empty() ? TextStyleId::Body : styleStack.back(); };
        auto getEffectiveColor = [&]() -> ColorDef {
            if (!linkStack.empty() && colorStack.size() <= linkStack.back().colorDepth)
                return ColorDef{ k_linkInk };
            return colorStack.empty() ? ColorDef{ InkWell::textInk() } : colorStack.back();
        };
        auto getEffectiveWeight = [&]() { return boldCount > 0 ? FontWeight::Bold : k_textStyles[(std::size_t)getEffectiveTextStyle()].weight; };
        auto getEffectiveStyle = [&]() { return italicCount > 0 ? FontStyle::Italic : k_textStyles[(std::size_t)getEffectiveTextStyle()].style; };
        auto getEffectiveScale = [&]() { return scriptStack.empty() ? 1.0f : scriptStack.back().scale; };
        auto getEffectiveShift = [&]() { return scriptStack.empty() ? 0.0f : scriptStack.back().shift; };
        auto getEffectiveSize = [&]() { return (sizeStack.empty() ? k_textStyles[(std::size_t)getEffectiveTextStyle()].size : sizeStack.back()) * getEffectiveScale(); };
        auto getEffectiveFamily = [&]() -> std::wstring_view { return familyStack.empty() ? k_textStyles[(std::size_t)getEffectiveTextStyle()].family : familyStack.back(); };

        std::size_t colorStart = 0, weightStart = 0, styleStart = 0, sizeStart = 0, familyStart = 0;
        ColorDef currentColor = getEffectiveColor(); FontWeight currentWeight = getEffectiveWeight();
        FontStyle currentStyle = getEffectiveStyle(); float currentSize = getEffectiveSize(); std::wstring_view currentFamily = getEffectiveFamily();
        std::size_t scriptStart = 0;
        float currentShift = getEffectiveShift();

        auto pushColor = [&](std::size_t end) { if (end > colorStart) m_colors.push_back({ {colorStart, end - colorStart}, currentColor }); colorStart = end; };
        auto pushWeight = [&](std::size_t end) { if (end > weightStart) m_weights.push_back({ {weightStart, end - weightStart}, currentWeight }); weightStart = end; };
        auto pushStyle = [&](std::size_t end) { if (end > styleStart) m_styles.push_back({ {styleStart, end - styleStart}, currentStyle }); styleStart = end; };
        auto pushSize = [&](std::size_t end) { if (end > sizeStart) m_sizes.push_back({ {sizeStart, end - sizeStart}, currentSize }); sizeStart = end; };
        auto pushFamily = [&](std::size_t end) { if (end > familyStart) m_families.push_back({ {familyStart, end - familyStart}, std::wstring(currentFamily) }); familyStart = end; };

        auto updateColor = [&](std::size_t index) { auto nc = getEffectiveColor(); if (nc != currentColor) { pushColor(index); currentColor = nc; } };
        auto updateWeight = [&](std::size_t index) { auto nw = getEffectiveWeight(); if (nw != currentWeight) { pushWeight(index); currentWeight = nw; } };
        auto updateStyle = [&](std::size_t index) { auto ns = getEffectiveStyle(); if (ns != currentStyle) { pushStyle(index); currentStyle = ns; } };
        auto updateSize = [&](std::size_t index) { auto nz = getEffectiveSize(); if (nz != currentSize) { pushSize(index); currentSize = nz; } };
        auto updateFamily = [&](std::size_t index) { auto nf = getEffectiveFamily(); if (nf != currentFamily) { pushFamily(index); currentFamily = nf; } };

        // A run on the baseline states nothing, so the spans cover the scripts alone and the
        // reader of one is free to hold none at all.
        auto pushScript = [&](std::size_t end){
            if (end > scriptStart && currentShift != 0.0f)
                m_scripts.push_back({ {scriptStart, end - scriptStart}, currentShift });
            scriptStart = end;
        };
        auto updateScript = [&](std::size_t index){
            float newShift = getEffectiveShift();
            if (newShift == currentShift)
                return;
            pushScript(index);
            currentShift = newShift;
        };

        // Closes the run of the innermost open link at end, joined to the span before it where
        // the two meet and name one target.
        std::size_t linkStart = 0;
        auto pushLink = [&](std::size_t end){
            if (!linkStack.empty() && end > linkStart)
            {
                const std::wstring_view target = linkStack.back().target;
                if (!m_links.empty() && m_links.back().range.end() == linkStart
                    && m_links.back().value == target)
                {
                    m_links.back().range.length += end - linkStart;
                }
                else
                {
                    m_links.push_back({ { linkStart, end - linkStart }, target });
                }
            }
            linkStart = end;
        };

        std::size_t currentPos = 0;
        std::size_t markerIdx = 0;
        const auto& markers = text.markers();
        std::size_t nextNewline = plain.find(L'\n', currentPos);

        while (markerIdx < markers.size() || nextNewline != std::wstring::npos) {
            std::size_t nextMarkerPos = (markerIdx < markers.size()) ? markers[markerIdx].first : std::wstring::npos;

            if (nextMarkerPos <= nextNewline) {
                const auto& item = markers[markerIdx].second;
                std::visit([&](const auto& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, PushThemeColor>) { colorStack.push_back(arg.ink); updateColor(nextMarkerPos); }
                    else if constexpr (std::is_same_v<T, PushCustomColor>) { colorStack.push_back(arg.color); updateColor(nextMarkerPos); }
                    else if constexpr (std::is_same_v<T, PopColor>) { if (!colorStack.empty()) colorStack.pop_back(); updateColor(nextMarkerPos); }
                    else if constexpr (std::is_same_v<T, PushTextStyle>) { styleStack.push_back(arg.style); updateWeight(nextMarkerPos); updateStyle(nextMarkerPos); updateSize(nextMarkerPos); updateFamily(nextMarkerPos); }
                    else if constexpr (std::is_same_v<T, PopTextStyle>) { if (!styleStack.empty()) styleStack.pop_back(); updateWeight(nextMarkerPos); updateStyle(nextMarkerPos); updateSize(nextMarkerPos); updateFamily(nextMarkerPos); }
                    else if constexpr (std::is_same_v<T, PushFontSize>) { sizeStack.push_back(arg.size); updateSize(nextMarkerPos); }
                    else if constexpr (std::is_same_v<T, PopFontSize>) { if (!sizeStack.empty()) sizeStack.pop_back(); updateSize(nextMarkerPos); }
                    else if constexpr (std::is_same_v<T, PushFontFamily>) { familyStack.push_back(arg.family); updateFamily(nextMarkerPos); }
                    else if constexpr (std::is_same_v<T, PopFontFamily>) { if (!familyStack.empty()) familyStack.pop_back(); updateFamily(nextMarkerPos); }
                    else if constexpr (std::is_same_v<T, TextAlign>) { currentAlign = arg; }
                    else if constexpr (std::is_same_v<T, SetIndent>) { currentIndent = arg.indent; }
                    else if constexpr (std::is_same_v<T, SetLineSpacing>) { currentLineSpacing = arg.spacing; }
                    else if constexpr (std::is_same_v<T, InTextIcon>) { m_inlineObjects.push_back({ nextMarkerPos, {arg.designWidth, arg.designHeight, arg.designBaseline, arg.paintLambda, arg.tag, false, false} }); }
                    else if constexpr (std::is_same_v<T, Space>) { m_inlineObjects.push_back({ nextMarkerPos, {arg.width, 0.0f, 0.0f, nullptr, {}, false, false} }); }
                    else if constexpr (std::is_same_v<T, VSpace>) { m_inlineObjects.push_back({ nextMarkerPos, {0.0f, arg.height, arg.height, nullptr, {}, false, false} }); }
                    else if constexpr (std::is_same_v<T, FlexSpace>) { m_inlineObjects.push_back({ nextMarkerPos, {arg.minWidth, 0.0f, 0.0f, nullptr, {}, true, false} }); }
                    else if constexpr (std::is_same_v<T, TabTo>) { m_inlineObjects.push_back({ nextMarkerPos, {arg.targetX, 0.0f, 0.0f, nullptr, {}, false, true, arg.targetX} }); }
                    else if constexpr (std::is_same_v<T, PushLink>)
                    {
                        pushLink(nextMarkerPos);
                        linkStack.push_back({ arg.target, colorStack.size() });
                        updateColor(nextMarkerPos);
                    }
                    else if constexpr (std::is_same_v<T, PopLink>)
                    {
                        pushLink(nextMarkerPos);
                        if (!linkStack.empty())
                            linkStack.pop_back();
                        updateColor(nextMarkerPos);
                    }
                    else if constexpr (std::is_same_v<T, TextOp>) {
                        if (arg == TextOp::EndLine) {
                            // Handled by physical '\n' breaks
                        }
                        else if (arg == TextOp::PushBold) { boldCount++; updateWeight(nextMarkerPos); }
                        else if (arg == TextOp::PopBold) { if (boldCount > 0) boldCount--; updateWeight(nextMarkerPos); }
                        else if (arg == TextOp::PushItalic) { italicCount++; updateStyle(nextMarkerPos); }
                        else if (arg == TextOp::PopItalic) { if (italicCount > 0) italicCount--; updateStyle(nextMarkerPos); }
                        else if (arg == TextOp::PushSuperscript || arg == TextOp::PushSubscript)
                        {
                            // Off the size the level is ENTERED at, so the step a script takes is
                            // the same fraction of what the reader sees beside it at every depth.
                            const float rise = (arg == TextOp::PushSuperscript
                                ? ScriptMetrics::superRise
                                : -ScriptMetrics::subDrop) * getEffectiveSize();
                            scriptStack.push_back({ getEffectiveScale() * ScriptMetrics::sizeFactor, getEffectiveShift() + rise });
                            updateSize(nextMarkerPos);
                            updateScript(nextMarkerPos);
                        }
                        else if (arg == TextOp::PopScript)
                        {
                            if (!scriptStack.empty())
                                scriptStack.pop_back();
                            updateSize(nextMarkerPos);
                            updateScript(nextMarkerPos);
                        }
                    }
                    }, item);
                markerIdx++;
            }
            else {
                m_paragraphs.push_back({ {currentParagraphStart, nextNewline - currentParagraphStart}, currentAlign, currentLineSpacing, currentIndent });
                currentParagraphStart = nextNewline + 1;
                nextNewline = plain.find(L'\n', currentParagraphStart);
            }
        }

        std::size_t sz = plain.size();
        pushColor(sz);
        pushWeight(sz);
        pushStyle(sz);
        pushSize(sz);
        pushFamily(sz);
        pushScript(sz);
        pushLink(sz);

        // Emit tail paragraph:
        // - if text exists post the last newline.
        // - OR if we're in Editable mode AND we're exactly at the end (meaning the whole string ended with \n).
        // - OR if the entire string is empty (sz == 0) to guarantee a layout foundation exists.
        if (currentParagraphStart < sz || (isEditable && currentParagraphStart == sz) || sz == 0) {
            m_paragraphs.push_back({ {currentParagraphStart, sz - currentParagraphStart}, currentAlign, currentLineSpacing, currentIndent });
        }
    }

}
