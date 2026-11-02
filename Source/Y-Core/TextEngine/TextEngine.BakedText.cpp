module ClaFi.Core.TextEngine.BakedText;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // The style of a run that states nothing.
        const TextStyle k_noStyle{};

        // A run's look as it is drawn: every field taken from the run's style and its bases, the
        // format's, and Body, the first that states it winning.
        struct DrawnLook
        {
            std::wstring_view family{};
            float size{ 0.0f };
            FontWeight weight{ FontWeight::Normal };
            FontStyle style{ FontStyle::Normal };
            ColorDef ink{};
            float shift{ 0.0f }; // off the baseline, positive upward
        };

        // The first of the run's value, the format's and the text's Body's. The framework's Body
        // states every field, so it ends the chain where the text's own leaves one unstated.
        template<typename Value>
        [[nodiscard]] const Value& drawnField(const TextStyle& style, const TextStyle& format,
            const TextStyle& body, std::optional<Value> TextStyle::* field)
        {
            if (const Value* value = style.stated(field))
                return *value;
            if (const Value* value = format.stated(field))
                return *value;
            if (const Value* value = body.stated(field))
                return *value;
            return *(StyleSheet::framework().body().*field);
        }

        // A link's ink stands between what the words state and what the format does.
        [[nodiscard]] ColorDef drawnInk(const TextStyle& style, const TextStyle& format,
            bool linked)
        {
            if (const ColorDef* ink = style.stated(&TextStyle::ink))
                return *ink;
            if (linked)
                return k_linkInk;
            if (const ColorDef* ink = format.stated(&TextStyle::ink))
                return *ink;
            return InkWell::textInk();
        }

        // Each script level is entered at the size of the level beneath it: it moves the baseline
        // by its share of that size and takes a share of it for its own words.
        [[nodiscard]] DrawnLook drawnLookOf(const TextStyle& style, const TextStyle& format,
            const TextStyle& body, bool linked)
        {
            DrawnLook drawn{
                .family = drawnField(style, format, body, &TextStyle::fontFamily),
                .size = drawnField(style, format, body, &TextStyle::fontSize),
                .weight = drawnField(style, format, body, &TextStyle::fontWeight),
                .style = drawnField(style, format, body, &TextStyle::fontStyle),
                .ink = drawnInk(style, format, linked),
            };
            for (const TextScript script : style.levels())
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

    // A paragraph takes what it states for itself, and what the format states for the rest. The
    // format's style is found where the text's are, and a name found nowhere is reported.
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
        const StyleSheet& styles = text.styles();
        const TextStyle& body = styles.body();
        TextStyle formatStyle = format.fields();
        if (!format.style().empty())
        {
            formatStyle.base = styles.find(format.style());
            if (!formatStyle.base)
                StyleSheet::events().emit<UnknownStyleEvent>(std::wstring_view{ format.style() });
        }

        const DrawnLook base = drawnLookOf(k_noStyle, formatStyle, body, false);
        m_baseFont = {
            .family = std::wstring{ base.family },
            .size = base.size,
            .weight = base.weight,
            .style = base.style,
        };

        text.forEachRun([&](const TextRunView& run){
            const DrawnLook drawn = drawnLookOf(run.style, formatStyle, body, !run.link.empty());
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
