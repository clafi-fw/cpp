export module ClaFi.Core.Dom_TextSerializers;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.DomEngine;
import ClaFi.Core.System.Serialization;
import ClaFi.StdLib;

// A TEXT IN A DOCUMENT IS A SECTION OF TWO CHILDREN: the plain text, and the markers standing in
// it. Import this module wherever a blueprint states a Text, and Dt::Value{ L"Caption", Text{} }
// reads and writes as
//
//     Caption=[
//         Text="Hello world"
//         Markers=[
//             =[
//                 At=6
//                 Tag=b
//             ]
//             =[
//                 At=11
//                 Tag=/b
//             ]
//         ]
//     ]
//
// A marker's Tag is the spelling Fmt reads between its brackets, and At is the index in the plain
// text it applies from. The plain text is stated as it is: a marker that puts a character in - a
// space, an icon, a line end - has that character in the text at its index, so an index counts
// what the text holds and nothing has to be recomputed on the way in or out. The two children
// come in whatever order the section keeps them; each is found by name.
//
// An icon's painter is a callable and cannot be written. It reads back at its stated extent and
// draws nothing.

namespace ClaFi
{
    // Found by argument-dependent lookup: the marker types the variant holds make ClaFi an
    // associated namespace of the pair.
    export constexpr auto serializedFields(const Text::Marker&)
    {
        return std::make_tuple(
            SerializedField{ L"At", &Text::Marker::first },
            SerializedField{ L"Tag", &Text::Marker::second }
        );
    }
}

namespace ClaFi::Dom
{
    // A marker is a scalar in a document, spelled as its tag. A tag no table lists leaves the
    // marker as it was, the way an unreadable number leaves an int.
    template <>
    struct ScalarSerializer<FormatItem>
    {
        [[nodiscard]] static std::wstring toWString(const FormatItem& item);
        static void fromWString(std::wstring_view str, FormatItem& item);
    };

    template <>
    struct CompositeSerializer<Text>
    {
        static void defineSchema(Section& section, const Text& defaultValue);
        [[nodiscard]] static Text read(const Section& section);
        static void readTo(const Section& section, Text& out);
        static void write(Section& node, const Text& value);
    };
}
