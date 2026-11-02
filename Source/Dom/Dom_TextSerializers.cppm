export module ClaFi.Core.Dom_TextSerializers;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.DomEngine;
import ClaFi.StdLib;

// **A text in a document is a section of its own styles and its pieces.** Import this module
// wherever a blueprint states a Text, and Dt::Value{ L"Caption", Text{} } reads and writes it.
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
}
