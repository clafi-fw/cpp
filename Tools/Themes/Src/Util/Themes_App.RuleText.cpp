module Themes_App.RuleText;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace
    {
        constexpr std::array<std::wstring_view, static_cast<std::size_t>(PaintChannel::Count)>
            k_channelLabels{
                L"surface",
                L"stroke",
                L"band",
                L"text",
                L"shadow"
            };

        constexpr std::array<std::wstring_view, static_cast<std::size_t>(RuleInput::Count)>
            k_inputLabels{
                L"hot",
                L"pressed",
                L"focused",
                L"selected",
                L"disabled",
                L"current",
                L"in edit mode",
                L"window focused"
            };

        constexpr std::size_t k_operationCount{ static_cast<std::size_t>(ColorRuleOp::Set) + 1ull };

        constexpr std::array<std::wstring_view, k_operationCount> k_operationNames{
            L"No change",
            L"Add offset",
            L"Scale value",
            L"Set exact value"
        };

        // Shaped with a line's height and no width.
        constexpr wchar_t k_zeroWidthSpace = 0x200B;

        // A word framing a rule rather than naming its parts, gray and small.
        void writeFramingWord(Text& text, const std::wstring_view word)
        {
            text << ColoredText{ InkGrade::Muted, StyledText{ TextStyles::SubBody, word } };
        }

        // A set's inputs joined by a word, in brackets when asked and there is more than one.
        void writeInputs(Text& text, const RuleInputs inputs, const std::wstring_view joiner,
            const bool bracketed)
        {
            Text result{};
            std::size_t count = 0ull;
            for (std::size_t i = 0ull; i != k_inputLabels.size(); ++i)
            {
                if (!inputs.has(static_cast<RuleInput>(i)))
                    continue;
                if (count != 0ull)
                    writeFramingWord(result, joiner);
                result << k_inputLabels[i];
                ++count;
            }
            if (bracketed and count > 1ull)
                text << L'(' << result << L')';
            else
                text << result;
        }

        // When the rule applies: "a or b", "(a or b) and c and d".
        void writeCondition(Text& text, const ColorRule& rule)
        {
            const bool joined = !rule.inputs.empty() and !rule.andInputs.empty();
            if (!rule.inputs.empty())
                writeInputs(text, rule.inputs, L" or ", joined);
            if (joined)
                writeFramingWord(text, L" and ");
            if (!rule.andInputs.empty())
                writeInputs(text, rule.andInputs, L" and ", false);
        }

        // The mark of an operation: bold in the spot ink on an editor, and muted on a row.
        void writeMark(Text& text, const std::wstring_view mark, const RuleFace face)
        {
            if (face == RuleFace::Editor)
                text << ColoredText{ InkWell::spotInk(), BoldText{ mark } };
            else
                text << ColoredText{ InkGrade::Muted, BoldText{ mark } };
        }

        // The character a value operation is marked with, an offset's taking the sign of its value.
        [[nodiscard]] std::wstring_view operationMark(const ColorRuleOp operation,
            const float value)
        {
            switch (operation)
            {
                case ColorRuleOp::Offset:
                    return value < 0.0f ? L"\u2212" : L"+";
                case ColorRuleOp::Scale:
                    return L"\u00D7";
                case ColorRuleOp::Set:
                    return L"=";
                case ColorRuleOp::NoChange:
                    break;
            }
            return {};
        }

        // The operation a mark names: the marks the list shows, and what a keyboard has in their
        // place.
        [[nodiscard]] std::optional<ColorRuleOp> operationOfMark(const wchar_t mark)
        {
            switch (mark)
            {
                case L'+':
                case L'-':
                    return ColorRuleOp::Offset;
                case L'\u00D7':
                case L'*':
                case L'x':
                case L'X':
                    return ColorRuleOp::Scale;
                case L'=':
                    return ColorRuleOp::Set;
            }
            return std::nullopt;
        }

        // A number as typed, with a point or a comma before the fraction.
        [[nodiscard]] std::optional<float> typedNumber(const std::wstring_view text)
        {
            std::string narrow{};
            for (const wchar_t character : text)
            {
                if (character > L'\x7F')
                    return std::nullopt;
                narrow.push_back(character == L',' ? '.' : static_cast<char>(character));
            }
            // from_chars reads a minus sign and no plus.
            if (!narrow.empty() && narrow.front() == '+')
                narrow.erase(0, 1);

            float value = 0.0f;
            const char* last = narrow.data() + narrow.size();
            const std::from_chars_result result = std::from_chars(narrow.data(), last, value);
            if (result.ec != std::errc{} || result.ptr != last)
                return std::nullopt;
            return value;
        }

        // The values an operation reaches: the slider's two ends, read through the rule's own
        // mapping.
        struct OperationRange
        {
            float low{};
            float high{};
        };

        [[nodiscard]] OperationRange operationRange(const ColorRuleOp operation)
        {
            ColorRuleValue probe{};
            probe.setNormalizedValue(0.0f);
            const float low = probe.value(operation);
            probe.setNormalizedValue(1.0f);
            return { low, probe.value(operation) };
        }
    }

    std::wstring_view channelLabel(const PaintChannel channel)
    {
        return k_channelLabels[static_cast<std::size_t>(channel)];
    }

    std::wstring_view inputLabel(const RuleInput input)
    {
        return k_inputLabels[static_cast<std::size_t>(input)];
    }

    std::wstring_view operationName(const ColorRuleOp operation)
    {
        return k_operationNames[static_cast<std::size_t>(operation)];
    }

    std::wstring capitalised(const std::wstring_view label)
    {
        std::wstring result{ label };
        if (!result.empty())
            result.front() = static_cast<wchar_t>(std::towupper(result.front()));
        return result;
    }

    void writeApplyTo(Text& text, const ColorRule& rule, const RuleFace face,
        const bool secondLineAtRest)
    {
        writeApplyToChannel(text, rule, face);
        if (rule.atRest())
        {
            if (secondLineAtRest)
                text << L'\n' << k_zeroWidthSpace;
            return;
        }
        text << L'\n';
        writeApplyToCondition(text, rule, face);
    }

    // The channel in the sub heading style on an editor, and in the muted ink on a row.
    void writeApplyToChannel(Text& text, const ColorRule& rule, const RuleFace face)
    {
        const std::wstring channel = capitalised(channelLabel(rule.output));
        if (face == RuleFace::Editor)
            text << StyledText{ TextStyles::SubHeading, channel };
        else
            text << ColoredText{ InkGrade::Muted, StyledText{ TextStyles::SubHeading, channel } };
        if (!rule.atRest())
            writeFramingWord(text, L", when");
    }

    // One grade down on an editor, and in the muted ink on a row.
    void writeApplyToCondition(Text& text, const ColorRule& rule, const RuleFace face)
    {
        if (rule.atRest())
            return;
        Text condition{};
        writeCondition(condition, rule);
        const InkGrade grade = face == RuleFace::Editor ? InkGrade::Strong : InkGrade::Muted;
        text << ColoredText{ grade, condition };
    }

    void writeNoChange(Text& text, const std::wstring_view word)
    {
        text << ColoredText{ InkGrade::Subtle, StyledText{ TextStyles::SubBody, word } };
    }

    std::wstring_view hueMark(const ColorRuleHueOp operation)
    {
        constexpr std::array<std::wstring_view, 3> digits{ L"1", L"2", L"3" };
        if (operation == ColorRuleHueOp::ExactValue)
            return L"=";
        if (operation >= ColorRuleHueOp::EndPalette)
            return {};
        return digits[static_cast<std::size_t>(operation)];
    }

    // A palette entry reads the theme's palette; an exact hue is the rule's own.
    Color hueBlobColor(const ColorRuleHue& rule, const ThemeColors& colors)
    {
        const ColorRuleHueOp operation = rule.operation();
        const float hue = operation == ColorRuleHueOp::ExactValue
            ? rule.exactValue()
            : colors.paletteHues[static_cast<std::size_t>(operation)];
        const Hsl color = { hue, k_blobSaturation, k_blobLuminosity };
        return color.toColor();
    }

    void writeValueRule(Text& text, const ColorRuleValue& rule, const RuleFace face)
    {
        if (rule.operation() == ColorRuleOp::NoChange)
        {
            writeNoChange(text, operationName(ColorRuleOp::NoChange));
            return;
        }
        const float value = rule.value();
        writeMark(text, operationMark(rule.operation(), value), face);
        text << L' ';
        const std::wstring number = std::format(L"{:.2f}", std::abs(value));
        if (face == RuleFace::Editor)
            text << StyledText{ TextStyles::Code, number };
        else
            text << ColoredText{ InkGrade::Muted, StyledText{ TextStyles::Code, number } };
    }

    // A mark and a value, the mark alone, or a value for the operation the rule stands on.
    std::optional<ColorRuleValue> typedRule(AcceptEditEvent& event, const ColorRuleValue& current)
    {
        std::wstring_view text = event.text.plainText();
        trimLeft(text);
        trimRight(text);
        if (text.empty())
            return std::nullopt;

        const std::optional<ColorRuleOp> marked = operationOfMark(text.front());
        const bool startsNumber = std::iswdigit(text.front())
            || text.front() == L'.'
            || text.front() == L',';
        if (!marked.has_value() && !startsNumber)
            return std::nullopt;

        ColorRuleOp operation = current.operation();
        bool negative = false;
        if (marked.has_value())
        {
            operation = marked.value();
            negative = text.front() == L'-';
            text.remove_prefix(1);
            trimLeft(text);
            // The mark alone is a pick from the list, and the value stays where it is.
            if (text.empty())
            {
                ColorRuleValue result = current;
                result.setOperation(operation);
                return result;
            }
        }
        if (operation == ColorRuleOp::NoChange)
        {
            event.refuse(L"Type a mark before the value, such as +0.02 or =0.66.");
            return std::nullopt;
        }

        const std::optional<float> number = typedNumber(text);
        if (!number.has_value())
        {
            event.refuse(L"Type a number after the mark, such as +0.02 or =0.66.");
            return std::nullopt;
        }
        const float value = negative ? -number.value() : number.value();
        const OperationRange range = operationRange(operation);
        // Written so that a NaN is refused as well.
        if (!(value >= range.low && value <= range.high))
        {
            event.refuse(std::format(L"Type a value from {:.2f} to {:.2f}.",
                range.low, range.high));
            return std::nullopt;
        }
        return ColorRuleValue{ operation, value };
    }
}
