module ClaFi.Core.Dom_TextSerializers;

import ClaFi.Core.TextEngine.Tags;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    namespace
    {
        constexpr std::wstring_view k_textKey = L"Text";
        constexpr std::wstring_view k_markersKey = L"Markers";
    }

    // --- ScalarSerializer<FormatItem> ---

    std::wstring ScalarSerializer<FormatItem>::toWString(const FormatItem& item)
    {
        return tagOf(item);
    }

    void ScalarSerializer<FormatItem>::fromWString(std::wstring_view str, FormatItem& item)
    {
        if (const std::optional<FormatItem> read = formatItemOf(str))
            item = *read;
    }

    // --- CompositeSerializer<Text> ---

    void CompositeSerializer<Text>::defineSchema(Section& section, const Text& defaultValue)
    {
        section.addValue(k_textKey, defaultValue.plainText());
        // A sequence is declared from the shape of one item and holds none until set.
        section.addSequence(k_markersKey, Text::Marker{}).set(defaultValue.markers());
    }

    Text CompositeSerializer<Text>::read(const Section& section)
    {
        Text result;
        readTo(section, result);
        return result;
    }

    void CompositeSerializer<Text>::readTo(const Section& section, Text& out)
    {
        std::wstring plainText = (section / k_textKey).get<std::wstring>();
        Text::Markers markers;
        (section / k_markersKey).getTo(markers);
        out = Text{ std::move(plainText), std::move(markers) };
    }

    void CompositeSerializer<Text>::write(Section& node, const Text& value)
    {
        (node / k_textKey).set(value.plainText());
        (node / k_markersKey).set(value.markers());
    }
}
