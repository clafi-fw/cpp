module Themes_App.RuleText;

import ClaFi.Controls.InPlaceEdit;

import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.System.Utils;
import ClaFi.Core.TextEngine.Text;

import ClaFi.StdLib;

namespace Themes_App
{
    namespace
    {
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
