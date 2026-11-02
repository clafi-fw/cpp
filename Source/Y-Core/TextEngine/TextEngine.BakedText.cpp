module ClaFi.Core.TextEngine.BakedText;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // A run's look as it is drawn: every field taken from the run, its style, the format and
        // its style, and Body, the first that states it winning.
        struct DrawnLook
        {
            std::wstring_view family{};
            float size{ 0.0f };
            FontWeight weight{ FontWeight::Normal };
            FontStyle style{ FontStyle::Normal };
            ColorDef ink{};
            float shift{ 0.0f }; // off the baseline, positive upward
        };

        // The first of the run's own value, its style's and then the format's own and its
        // style's. Body states every field, so it ends the chain.
        template<typename LookValue, typename StyleValue>
        [[nodiscard]] StyleValue drawnField(const TextLook& look, const TextLook& format,
            std::optional<LookValue> TextLook::* own, std::optional<StyleValue> TextStyle::* styled)
        {
            if ((look.*own).has_value())
                return StyleValue{ *(look.*own) };
            if (look.style)
            {
                if (const std::optional<StyleValue> value = look.style->stated(styled))
                    return *value;
            }
            if ((format.*own).has_value())
                return StyleValue{ *(format.*own) };
            if (format.style)
            {
                if (const std::optional<StyleValue> value = format.style->stated(styled))
                    return *value;
            }
            return *(TextStyles::Body.*styled);
        }

        // A link's ink stands between what the words state and what the format does.
        [[nodiscard]] ColorDef drawnInk(const TextLook& look, const TextLook& format, bool linked)
        {
            if (look.ink)
                return *look.ink;
            if (look.style)
            {
                if (const std::optional<ColorDef> ink = look.style->stated(&TextStyle::ink))
                    return *ink;
            }
            if (linked)
                return k_linkInk;
            if (format.ink)
                return *format.ink;
            if (format.style)
            {
                if (const std::optional<ColorDef> ink = format.style->stated(&TextStyle::ink))
                    return *ink;
            }
            return InkWell::textInk();
        }

        // Each script level is entered at the size of the level beneath it: it moves the baseline
        // by its share of that size and takes a share of it for its own words.
        [[nodiscard]] DrawnLook drawnLookOf(const TextLook& look, const TextLook& format,
            bool linked)
        {
            DrawnLook drawn{
                .family = drawnField(look, format, &TextLook::fontFamily, &TextStyle::fontFamily),
                .size = drawnField(look, format, &TextLook::fontSize, &TextStyle::fontSize),
                .weight = drawnField(look, format, &TextLook::fontWeight, &TextStyle::fontWeight),
                .style = drawnField(look, format, &TextLook::fontStyle, &TextStyle::fontStyle),
                .ink = drawnInk(look, format, linked),
            };
            for (const TextScript script : look.scripts)
            {
                const float share = script == TextScript::Super
                    ? ScriptMetrics::SuperRise
                    : -ScriptMetrics::SubDrop;
                drawn.shift += share * drawn.size;
                drawn.size *= ScriptMetrics::SizeFactor;
            }
            return drawn;
        }

        // One property's spans as the runs go by: a span grows while the value holds and a new
        // one opens where it changes.
        template<typename Value>
        void extendSpans(std::vector<TextSpan<Value>>& spans, const TextRange& range,
            const Value& value)
        {
            if (!spans.empty() && spans.back().value == value
                && spans.back().range.end() == range.start)
            {
                spans.back().range.length += range.length;
                return;
            }
            spans.push_back({ range, value });
        }
    }

    // A paragraph takes what it states for itself, and what the format states for the rest.
    void BakedText::rebuild(const Text& text, const TextFormat& format, bool isEditable)
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
        m_plainText = &text.plainText();
        const std::wstring& plain = *m_plainText;
        const TextLook& formatLook = format.look();

        text.forEachRun([&](const TextRunView& run){
            const DrawnLook drawn = drawnLookOf(run.look, formatLook, !run.link.empty());
            extendSpans(m_colors, run.range, drawn.ink);
            extendSpans(m_weights, run.range, drawn.weight);
            extendSpans(m_styles, run.range, drawn.style);
            extendSpans(m_sizes, run.range, drawn.size);
            if (m_families.empty() || m_families.back().value != drawn.family)
                m_families.push_back({ run.range, std::wstring{ drawn.family } });
            else
                m_families.back().range.length += run.range.length;
            // A run on the baseline states nothing, so the spans cover the scripts alone and the
            // reader of one is free to hold none at all.
            if (drawn.shift != 0.0f)
                extendSpans(m_scripts, run.range, drawn.shift);
            // Two links in a row naming one target are one span to point at.
            if (!run.link.empty())
                extendSpans(m_links, run.range, run.link);
            if (run.object)
                m_inlineObjects.push_back({ run.range.start, bakedObjectOf(*run.object) });
        });

        const ParagraphLook& formatParagraph = format.paragraph();
        const auto pushParagraph = [this, &text, &formatParagraph](std::size_t start,
            std::size_t end){
            const ParagraphLook stated = text.paragraphLookAt(start);
            m_paragraphs.push_back({
                .range = { start, end - start },
                .alignment = stated.align.value_or(formatParagraph.align.value_or(TextAlign::Left)),
                .lineSpacing = stated.lineSpacing.value_or(formatParagraph.lineSpacing.value_or(1.0f)),
                .indent = stated.indent.value_or(formatParagraph.indent.value_or(0.0f)),
            });
        };
        std::size_t paragraphStart = 0;
        for (std::size_t lineEnd = plain.find(k_endLine); lineEnd != std::wstring::npos;
            lineEnd = plain.find(k_endLine, paragraphStart))
        {
            pushParagraph(paragraphStart, lineEnd);
            paragraphStart = lineEnd + 1;
        }

        // The words after the last line end are a paragraph; so is nothing at all, which a
        // layout has to stand on, and the empty line after a last line end in an editable text,
        // where the caret goes.
        if (paragraphStart < plain.size() || (isEditable && paragraphStart == plain.size())
            || plain.empty())
        {
            pushParagraph(paragraphStart, plain.size());
        }
    }

    BakedInlineObject BakedText::bakedObjectOf(const InlineObject& object)
    {
        return std::visit([](const auto& held) -> BakedInlineObject {
            using Held = std::decay_t<decltype(held)>;
            if constexpr (std::is_same_v<Held, InTextIcon>)
            {
                return {
                    .width = held.designWidth,
                    .height = held.designHeight,
                    .baseline = held.designBaseline,
                    .paintLambda = held.paintLambda,
                    .tag = held.tag,
                };
            }
            else if constexpr (std::is_same_v<Held, Space>)
                return { .width = held.width };
            else if constexpr (std::is_same_v<Held, VSpace>)
                return { .height = held.height, .baseline = held.height };
            else if constexpr (std::is_same_v<Held, FlexSpace>)
                return { .width = held.minWidth, .isFlexSpace = true };
            else
                return { .width = held.targetX, .isTabTo = true, .tabTargetX = held.targetX };
        }, object);
    }
}
