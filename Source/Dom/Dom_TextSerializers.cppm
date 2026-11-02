export module ClaFi.Core.Dom_TextSerializers;

import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.DomEngine;
import ClaFi.StdLib;

// **A text in a document is a section of its own styles and its pieces**, a style sheet a section
// of its styles, and a text format a section of its values. Import this module wherever a
// blueprint states one, and Dt::Value{ L"Caption", Text{} } reads and writes it.
// See Dom#compositeserializer-text

namespace ClaFi::Dom
{
    template <>
    struct CompositeSerializer<Text>
    {
        static void defineSchema(Section& section, const Text& defaultValue);
        [[nodiscard]] static Text read(const Section& section);
        static void readTo(const Section& section, Text& out);
        static void write(Section& node, const Text& value);
    };

    template <>
    struct CompositeSerializer<StyleSheet>
    {
        static void defineSchema(Section& section, const StyleSheet& defaultValue);
        [[nodiscard]] static StyleSheet read(const Section& section);
        static void readTo(const Section& section, StyleSheet& out);
        static void write(Section& node, const StyleSheet& value);
    };

    template <>
    struct CompositeSerializer<TextFormat>
    {
        static void defineSchema(Section& section, const TextFormat& defaultValue);
        [[nodiscard]] static TextFormat read(const Section& section);
        static void readTo(const Section& section, TextFormat& out);
        static void write(Section& node, const TextFormat& value);
    };
}
