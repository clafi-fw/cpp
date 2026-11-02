module ClaFi.Core.Dom_TextSerializers;

import ClaFi.Core.TextEngine.StyleSheet;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.Dom_UiSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Dom
{
    namespace
    {
        // The keys of a text's section, of one of its own styles or a sheet's, and the names and
        // keys of its pieces.
        namespace Keys
        {
            constexpr std::wstring_view Styles = L"Styles";
            constexpr std::wstring_view Pieces = L"Pieces";

            constexpr std::wstring_view Name = L"Name";
            constexpr std::wstring_view Base = L"Base";
            constexpr std::wstring_view FontFamily = L"FontFamily";
            constexpr std::wstring_view FontSize = L"FontSize";
            constexpr std::wstring_view FontWeight = L"FontWeight";
            constexpr std::wstring_view FontStyle = L"FontStyle";
            constexpr std::wstring_view Ink = L"Ink";
            constexpr std::wstring_view Scripts = L"Scripts";

            constexpr std::wstring_view Words = L""; // the words, unnamed as a piece and in a run
            constexpr std::wstring_view Run = L"Run";
            constexpr std::wstring_view Paragraph = L"P";
            constexpr std::wstring_view Style = L"Style";
            constexpr std::wstring_view Link = L"Link";
            constexpr std::wstring_view Anchor = L"Anchor";
            constexpr std::wstring_view Icon = L"Icon";
            constexpr std::wstring_view Space = L"Space";
            constexpr std::wstring_view VSpace = L"VSpace";
            constexpr std::wstring_view FlexSpace = L"FlexSpace";
            constexpr std::wstring_view TabTo = L"TabTo";
            constexpr std::wstring_view Align = L"Align";
            constexpr std::wstring_view Indent = L"Indent";
            constexpr std::wstring_view LineSpacing = L"LineSpacing";
        }

        constexpr std::array k_styleKeys{
            Keys::Name,
            Keys::Base,
            Keys::FontFamily,
            Keys::FontSize,
            Keys::FontWeight,
            Keys::FontStyle,
            Keys::Ink,
            Keys::Scripts,
        };

        // A sheet's style states no script level, which only a run stands in.
        constexpr std::array k_sheetStyleKeys{
            Keys::Name,
            Keys::Base,
            Keys::FontFamily,
            Keys::FontSize,
            Keys::FontWeight,
            Keys::FontStyle,
            Keys::Ink,
        };

        constexpr std::array k_runKeys{
            Keys::Words,
            Keys::Style,
            Keys::Link,
            Keys::Anchor,
            Keys::Icon,
            Keys::Space,
            Keys::VSpace,
            Keys::FlexSpace,
            Keys::TabTo,
        };

        constexpr std::array k_paragraphKeys{
            Keys::Align,
            Keys::Indent,
            Keys::LineSpacing,
        };

        // The pieces that are an object and nothing more, each one line under the object's name.
        constexpr std::array k_objectNames{
            Keys::Icon,
            Keys::Space,
            Keys::VSpace,
            Keys::FlexSpace,
            Keys::TabTo,
        };

        // A value as a file spells it.
        template<typename Value>
        struct Spelling
        {
            std::wstring_view name{};
            Value value{};
        };

        constexpr std::array k_weights{
            Spelling<FontWeight>{ L"Light", FontWeight::Light },
            Spelling<FontWeight>{ L"Normal", FontWeight::Normal },
            Spelling<FontWeight>{ L"SemiBold", FontWeight::SemiBold },
            Spelling<FontWeight>{ L"Bold", FontWeight::Bold },
        };

        constexpr std::array k_fontStyles{
            Spelling<FontStyle>{ L"Normal", FontStyle::Normal },
            Spelling<FontStyle>{ L"Italic", FontStyle::Italic },
        };

        constexpr std::array k_scripts{
            Spelling<TextScript>{ L"Super", TextScript::Super },
            Spelling<TextScript>{ L"Sub", TextScript::Sub },
        };

        constexpr std::array k_aligns{
            Spelling<TextAlign>{ L"Left", TextAlign::Left },
            Spelling<TextAlign>{ L"Center", TextAlign::Center },
            Spelling<TextAlign>{ L"Right", TextAlign::Right },
            Spelling<TextAlign>{ L"Justified", TextAlign::Justified },
        };

        constexpr std::array k_inkColors{
            Spelling<InkColor>{ L"text", InkColor::Text },
            Spelling<InkColor>{ L"accent", InkColor::Accent },
            Spelling<InkColor>{ L"spot", InkColor::Spot },
            Spelling<InkColor>{ L"yellow", InkColor::Yellow },
            Spelling<InkColor>{ L"green", InkColor::Green },
            Spelling<InkColor>{ L"blue", InkColor::Blue },
            Spelling<InkColor>{ L"red", InkColor::Red },
            Spelling<InkColor>{ L"black", InkColor::Black },
            Spelling<InkColor>{ L"white", InkColor::White },
        };

        constexpr std::array k_inkGrades{
            Spelling<InkGrade>{ L"faint", InkGrade::Faint },
            Spelling<InkGrade>{ L"subtle", InkGrade::Subtle },
            Spelling<InkGrade>{ L"muted", InkGrade::Muted },
            Spelling<InkGrade>{ L"strong", InkGrade::Strong },
            Spelling<InkGrade>{ L"strongest", InkGrade::Strongest },
        };

        template<typename Value, std::size_t Count>
        [[nodiscard]] std::wstring_view nameOf(const std::array<Spelling<Value>, Count>& spellings,
            Value value)
        {
            for (const Spelling<Value>& spelling : spellings)
            {
                if (spelling.value == value)
                    return spelling.name;
            }
            noReach("a value no spelling names");
        }

        template<typename Value, std::size_t Count>
        [[nodiscard]] std::optional<Value> valueOf(
            const std::array<Spelling<Value>, Count>& spellings, std::wstring_view name)
        {
            for (const Spelling<Value>& spelling : spellings)
            {
                if (spelling.name == name)
                    return spelling.value;
            }
            return std::nullopt;
        }

        // The word at the front of text, taken off it together with the blanks that follow.
        [[nodiscard]] std::wstring_view takeWord(std::wstring_view& text)
        {
            const std::size_t end = text.find(L' ');
            const std::wstring_view word = text.substr(0, end);
            text = end == std::wstring_view::npos ? std::wstring_view{} : text.substr(end + 1);
            trimLeft(text);
            return word;
        }

        // --- Inks ---

        // A grade as the step it stands at, or as the share itself where no step stands.
        [[nodiscard]] std::wstring gradeSpelling(float grade)
        {
            for (const Spelling<InkGrade>& spelling : k_inkGrades)
            {
                if (gradeOf(spelling.value) == grade)
                    return std::wstring{ spelling.name };
            }
            return floatToStr(grade);
        }

        // **An ink is an ink colour and a grade**, each left out where it is text or strongest,
        // and a colour is stated outright. See Dom#compositeserializer-text
        [[nodiscard]] std::wstring inkSpelling(const ColorDef& ink)
        {
            if (const Color* color = std::get_if<Color>(&ink))
                return ScalarSerializer<Color>::toWString(*color);

            const ClaFi::Ink& themed = std::get<ClaFi::Ink>(ink);
            std::wstring result;
            if (themed.color != InkColor::Text)
                result = nameOf(k_inkColors, themed.color);
            // The text ink at strongest still states its grade, so the spelling names an ink.
            if (themed.grade != gradeOf(InkGrade::Strongest) || themed.color == InkColor::Text)
            {
                if (!result.empty())
                    result += L' ';
                result += gradeSpelling(themed.grade);
            }
            return result;
        }

        [[nodiscard]] std::optional<ColorDef> inkOf(std::wstring_view spelling)
        {
            if (spelling.empty())
                return std::nullopt;
            if (spelling.starts_with(L'#'))
            {
                Color color{};
                ScalarSerializer<Color>::fromWString(spelling, color);
                return color;
            }

            ClaFi::Ink ink;
            std::wstring_view rest = spelling;
            std::wstring_view word = takeWord(rest);
            if (const std::optional<InkColor> color = valueOf(k_inkColors, word))
            {
                ink.color = *color;
                word = takeWord(rest);
            }
            if (!word.empty())
            {
                if (const std::optional<InkGrade> step = valueOf(k_inkGrades, word))
                    ink.grade = gradeOf(*step);
                else if (!tryStrToFloat(word, ink.grade))
                    return std::nullopt;
                if (!rest.empty())
                    return std::nullopt;
            }
            return ink;
        }

        // --- Objects ---

        // "w", "w,h" or "w,h,baseline" in design units, the height defaulting to the width and
        // the baseline to 0.85 of the height. An icon read back has no painter and draws nothing.
        [[nodiscard]] InTextIcon iconOf(std::wstring_view extent)
        {
            std::array<float, 3> values = { 16.0f, -1.0f, -1.0f };
            for (float& value : values)
            {
                const std::size_t comma = extent.find(L',');
                value = strToFloatDef(extent.substr(0, comma), value);
                if (comma == std::wstring_view::npos)
                    break;
                extent = extent.substr(comma + 1);
            }
            const float width = values[0];
            const float height = values[1] < 0.0f ? width : values[1];
            const float baseline = values[2] < 0.0f ? height * 0.85f : values[2];
            return InTextIcon{ width, height, baseline, {} };
        }

        // The key an object is written under, and its value.
        [[nodiscard]] std::pair<std::wstring_view, std::wstring> objectEntry(
            const InlineObject& object)
        {
            return std::visit([](const auto& held) -> std::pair<std::wstring_view, std::wstring> {
                using Held = std::decay_t<decltype(held)>;
                if constexpr (std::is_same_v<Held, InTextIcon>)
                {
                    return {
                        Keys::Icon,
                        floatToStr(held.designWidth) + L',' + floatToStr(held.designHeight) + L','
                            + floatToStr(held.designBaseline),
                    };
                }
                else if constexpr (std::is_same_v<Held, ClaFi::Space>)
                    return { Keys::Space, floatToStr(held.width) };
                else if constexpr (std::is_same_v<Held, ClaFi::VSpace>)
                    return { Keys::VSpace, floatToStr(held.height) };
                else if constexpr (std::is_same_v<Held, ClaFi::FlexSpace>)
                    return { Keys::FlexSpace, floatToStr(held.minWidth) };
                else
                    return { Keys::TabTo, floatToStr(held.targetX) };
            }, object);
        }

        // --- Nodes ---

        [[nodiscard]] Section schemaOf(std::span<const std::wstring_view> keys)
        {
            Section schema{ nullptr };
            for (const std::wstring_view key : keys)
                schema.addValue(key, std::wstring{});
            return schema;
        }

        // What an entry states under the key, empty where it states nothing.
        [[nodiscard]] std::wstring valueAt(const Section& entry, std::wstring_view key)
        {
            const DomNodeBase* child = entry.child(key);
            return child ? child->get<std::wstring>() : std::wstring{};
        }

        // A value is put where it says something, so an entry holds the default elsewhere.
        void put(Section& entry, std::wstring_view key, std::wstring value)
        {
            if (value.empty())
                return;
            if (DomNodeBase* child = entry.child(key))
                child->set(std::move(value));
        }

        // --- Writing ---

        // One of the text's own styles: a look that says more than a style's name.
        struct OwnStyle
        {
            std::wstring name{};
            TextLook look{};
        };

        using OwnStyles = std::vector<OwnStyle>;

        // A look as a file can name it. A style with no name is no name to write, so the fields
        // it states are written as the look's own.
        [[nodiscard]] TextLook writtenLook(const TextLook& look)
        {
            if (!look.style || !look.style->name.empty())
                return look;

            TextLook result = look;
            const TextStyle& style = *look.style;
            result.style = nullptr;
            if (!result.fontFamily)
            {
                const std::optional<std::wstring_view> family = style.stated(&TextStyle::fontFamily);
                if (family)
                    result.fontFamily = std::wstring{ *family };
            }
            if (!result.fontSize)
                result.fontSize = style.stated(&TextStyle::fontSize);
            if (!result.fontWeight)
                result.fontWeight = style.stated(&TextStyle::fontWeight);
            if (!result.fontStyle)
                result.fontStyle = style.stated(&TextStyle::fontStyle);
            if (!result.ink)
                result.ink = style.stated(&TextStyle::ink);
            return result;
        }

        // Whether a look is a style named and nothing more, which a piece names without an entry.
        [[nodiscard]] bool namesStyleOnly(const TextLook& look)
        {
            return look.empty() || TextLook{ .style = look.style } == look;
        }

        // How many entries are named after one base so far.
        struct BaseCount
        {
            std::wstring_view base{};
            std::size_t count{ 0 };
        };

        // Whether a piece names a style of that name, which an entry of the text's own leaves to
        // it: an own entry is found first, and would take the piece's words.
        [[nodiscard]] bool namedByAPiece(const TextPieces& pieces, std::wstring_view name)
        {
            return std::ranges::any_of(pieces, [name](const TextPiece& piece) {
                return piece.look.style && piece.look.style->name == name;
            });
        }

        // An entry per look carrying more than a style's name, named after the style it is based
        // on and numbered: Code 1, Code 2, Style 1.
        [[nodiscard]] OwnStyles ownStylesOf(const TextPieces& pieces)
        {
            OwnStyles result;
            std::vector<BaseCount> counts;
            for (const TextPiece& piece : pieces)
            {
                if (piece.paragraph || namesStyleOnly(piece.look))
                    continue;
                if (std::ranges::find(result, piece.look, &OwnStyle::look) != result.end())
                    continue;

                const std::wstring_view base = piece.look.style ? piece.look.style->name : L"Style";
                std::vector<BaseCount>::iterator counted = std::ranges::find(counts, base,
                    &BaseCount::base);
                if (counted == counts.end())
                    counted = counts.insert(counts.end(), BaseCount{ base });
                std::wstring name;
                do
                {
                    ++counted->count;
                    name = std::wstring{ base } + L' ' + std::to_wstring(counted->count);
                }
                while (namedByAPiece(pieces, name));
                result.push_back({
                    .name = std::move(name),
                    .look = piece.look,
                });
            }
            return result;
        }

        void writeStyle(Section& entry, const OwnStyle& own)
        {
            const TextLook& look = own.look;
            put(entry, Keys::Name, own.name);
            if (look.style)
                put(entry, Keys::Base, std::wstring{ look.style->name });
            if (look.fontFamily)
                put(entry, Keys::FontFamily, *look.fontFamily);
            if (look.fontSize)
                put(entry, Keys::FontSize, floatToStr(*look.fontSize));
            if (look.fontWeight)
                put(entry, Keys::FontWeight, std::wstring{ nameOf(k_weights, *look.fontWeight) });
            if (look.fontStyle)
                put(entry, Keys::FontStyle, std::wstring{ nameOf(k_fontStyles, *look.fontStyle) });
            if (look.ink)
                put(entry, Keys::Ink, inkSpelling(*look.ink));
            std::wstring scripts;
            for (const TextScript script : look.scripts)
            {
                if (!scripts.empty())
                    scripts += L' ';
                scripts += nameOf(k_scripts, script);
            }
            put(entry, Keys::Scripts, std::move(scripts));
        }

        void writeParagraph(Section& entry, const ParagraphLook& look)
        {
            if (look.align)
                put(entry, Keys::Align, std::wstring{ nameOf(k_aligns, *look.align) });
            if (look.indent)
                put(entry, Keys::Indent, floatToStr(*look.indent));
            if (look.lineSpacing)
                put(entry, Keys::LineSpacing, floatToStr(*look.lineSpacing));
        }

        // An object's run states the object, and its character goes without saying.
        void writeRun(Section& entry, const TextPiece& piece, const OwnStyles& ownStyles)
        {
            if (piece.object)
            {
                std::pair<std::wstring_view, std::wstring> object = objectEntry(*piece.object);
                put(entry, object.first, std::move(object.second));
            }
            else
            {
                put(entry, Keys::Words, piece.words);
            }

            if (!namesStyleOnly(piece.look))
            {
                const OwnStyles::const_iterator own = std::ranges::find(ownStyles, piece.look,
                    &OwnStyle::look);
                put(entry, Keys::Style, own->name);
            }
            else if (piece.look.style)
            {
                put(entry, Keys::Style, std::wstring{ piece.look.style->name });
            }
            put(entry, Keys::Link, piece.link);
            put(entry, Keys::Anchor, piece.anchor);
        }

        // **A piece in no look, linking nowhere, is one line**: its words unnamed, or an object
        // under its name. Anything more is a Run, and a paragraph's look a P.
        void writePiece(MultiTypeSequence& pieces, const TextPiece& piece,
            const OwnStyles& ownStyles)
        {
            if (piece.paragraph)
            {
                writeParagraph(static_cast<Section&>(*pieces.add(Keys::Paragraph)), *piece.paragraph);
                return;
            }

            if (!piece.look.empty() || !piece.link.empty() || !piece.anchor.empty())
            {
                writeRun(static_cast<Section&>(*pieces.add(Keys::Run)), piece, ownStyles);
                return;
            }

            if (piece.object)
            {
                std::pair<std::wstring_view, std::wstring> object = objectEntry(*piece.object);
                pieces.add(object.first)->set(std::move(object.second));
                return;
            }
            pieces.add(Keys::Words)->set(piece.words);
        }

        // --- Reading ---

        // A style the text names: one of its own first, then the reading sheet's, then the
        // framework's. A name none knows names no style, and the words read back in the look of
        // none.
        [[nodiscard]] TextLook lookNamed(std::wstring_view name, const OwnStyles& ownStyles)
        {
            for (const OwnStyle& own : ownStyles)
            {
                if (own.name == name)
                    return own.look;
            }
            if (const TextStyle* style = StyleSheet::findReading(name))
                return TextLook{ .style = style };
            return {};
        }

        // The fields an entry states, the script levels included, written into the look.
        void readFields(const Section& entry, TextLook& look)
        {
            if (const std::wstring family = valueAt(entry, Keys::FontFamily); !family.empty())
                look.fontFamily = family;
            float size = 0.0f;
            if (tryStrToFloat(valueAt(entry, Keys::FontSize), size))
                look.fontSize = size;
            const std::optional<FontWeight> weight = valueOf(k_weights,
                valueAt(entry, Keys::FontWeight));
            if (weight)
                look.fontWeight = weight;
            const std::optional<FontStyle> fontStyle = valueOf(k_fontStyles,
                valueAt(entry, Keys::FontStyle));
            if (fontStyle)
                look.fontStyle = fontStyle;
            if (const std::optional<ColorDef> ink = inkOf(valueAt(entry, Keys::Ink)))
                look.ink = ink;

            const std::wstring scripts = valueAt(entry, Keys::Scripts);
            std::wstring_view rest = scripts;
            while (!rest.empty())
            {
                if (const std::optional<TextScript> script = valueOf(k_scripts, takeWord(rest)))
                    look.scripts.push_back(*script);
            }
        }

        // An entry stating no base is based on the style of its own name, so an entry named
        // Code states what this text's code looks like beyond the reading sheet's or the
        // framework's.
        [[nodiscard]] OwnStyle readStyle(const Section& entry, const OwnStyles& known)
        {
            OwnStyle result{ .name = valueAt(entry, Keys::Name) };
            const std::wstring base = valueAt(entry, Keys::Base);
            result.look = lookNamed(base.empty() ? std::wstring_view{ result.name } : base, known);
            readFields(entry, result.look);
            return result;
        }

        // The object a name and its value state, or nothing for a name no object has.
        [[nodiscard]] std::optional<InlineObject> objectNamed(std::wstring_view name,
            std::wstring_view value)
        {
            if (name == Keys::Icon)
                return iconOf(value);
            if (name == Keys::Space)
                return ClaFi::Space{ strToFloatDef(value, 0.0f) };
            if (name == Keys::VSpace)
                return ClaFi::VSpace{ strToFloatDef(value, 0.0f) };
            if (name == Keys::FlexSpace)
                return ClaFi::FlexSpace{ strToFloatDef(value, 0.0f) };
            if (name == Keys::TabTo)
                return ClaFi::TabTo{ strToFloatDef(value, 0.0f) };
            return std::nullopt;
        }

        // The object a run states under one of the objects' names.
        [[nodiscard]] std::optional<InlineObject> objectOf(const Section& entry)
        {
            for (const std::wstring_view name : k_objectNames)
            {
                const std::wstring value = valueAt(entry, name);
                if (!value.empty())
                    return objectNamed(name, value);
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional<ParagraphLook> paragraphOf(const Section& entry)
        {
            ParagraphLook look;
            look.align = valueOf(k_aligns, valueAt(entry, Keys::Align));
            float value = 0.0f;
            if (tryStrToFloat(valueAt(entry, Keys::Indent), value))
                look.indent = value;
            if (tryStrToFloat(valueAt(entry, Keys::LineSpacing), value))
                look.lineSpacing = value;
            if (look.empty())
                return std::nullopt;
            return look;
        }

        [[nodiscard]] TextPiece readRun(const Section& entry, const OwnStyles& ownStyles)
        {
            TextPiece piece{
                .words = valueAt(entry, Keys::Words),
                .look = lookNamed(valueAt(entry, Keys::Style), ownStyles),
                .link = valueAt(entry, Keys::Link),
                .anchor = valueAt(entry, Keys::Anchor),
                .object = objectOf(entry),
            };
            if (piece.object)
                piece.words = std::wstring{ k_objectChar };
            return piece;
        }

        [[nodiscard]] TextPiece readPiece(const MultiTypeSequence::Item& item,
            const OwnStyles& ownStyles)
        {
            if (item.name == Keys::Run)
                return readRun(static_cast<const Section&>(*item.node), ownStyles);
            if (item.name == Keys::Paragraph)
                return TextPiece{ .paragraph = paragraphOf(static_cast<const Section&>(*item.node)) };
            if (item.name == Keys::Words)
                return TextPiece{ .words = item.node->get<std::wstring>() };
            TextPiece piece{ .object = objectNamed(item.name, item.node->get<std::wstring>()) };
            if (piece.object)
                piece.words = std::wstring{ k_objectChar };
            return piece;
        }

        [[nodiscard]] const SequenceBase& listAt(const Section& section, std::wstring_view key)
        {
            return static_cast<const SequenceBase&>(section / key);
        }

        [[nodiscard]] SequenceBase& listAt(Section& section, std::wstring_view key)
        {
            return static_cast<SequenceBase&>(section / key);
        }

        // --- Style sheets ---

        // A sheet's style as an entry states it, the way a text's own style is stated. A base of
        // the style's own name is the framework's, and goes without saying.
        [[nodiscard]] OwnStyle entryOf(const TextStyle& style)
        {
            const bool basedOnOwnName = style.base && TextStyles::contains(style.base)
                && style.base->name == style.name;
            TextLook look{ .style = basedOnOwnName ? nullptr : style.base };
            if (style.fontFamily)
                look.fontFamily = std::wstring{ *style.fontFamily };
            look.fontSize = style.fontSize;
            look.fontWeight = style.fontWeight;
            look.fontStyle = style.fontStyle;
            look.ink = style.ink;
            return { .name = std::wstring{ style.name }, .look = std::move(look) };
        }

        // A style as a sheet's entry states it: its base by name, and its fields.
        struct StatedStyle
        {
            std::wstring name{};
            std::wstring base{};
            TextLook fields{};
        };

        using StatedStyles = std::vector<StatedStyle>;

        // How far a stated style has come into the sheet being read.
        enum class Progress
        {
            Waiting,
            Adding,
            Added
        };

        [[nodiscard]] TextStyle styleOf(const StatedStyle& stated, const TextStyle* base)
        {
            const TextLook& fields = stated.fields;
            TextStyle result{ .name = stated.name, .base = base };
            if (fields.fontFamily)
                result.fontFamily = *fields.fontFamily;
            result.fontSize = fields.fontSize;
            result.fontWeight = fields.fontWeight;
            result.fontStyle = fields.fontStyle;
            result.ink = fields.ink;
            return result;
        }

        // **A style goes into the sheet after its base**, so every one is added and none changed,
        // and a sheet read is no edit. A base the sheet lacks is the framework's of that name; a
        // base leading back to the style is dropped.
        const TextStyle* addStated(StyleSheet& sheet, const StatedStyles& stated,
            std::vector<Progress>& progress, std::size_t index)
        {
            const StatedStyle& style = stated[index];
            if (progress[index] == Progress::Added)
                return sheet.findOwn(style.name);
            if (progress[index] == Progress::Adding)
                return nullptr;
            progress[index] = Progress::Adding;

            const TextStyle* base = nullptr;
            if (style.base.empty() || style.base == style.name)
            {
                base = TextStyles::named(style.name);
            }
            else
            {
                const StatedStyles::const_iterator found = std::ranges::find(stated, style.base,
                    &StatedStyle::name);
                base = found == stated.end()
                    ? TextStyles::named(style.base)
                    : addStated(sheet, stated, progress,
                        static_cast<std::size_t>(found - stated.begin()));
            }
            sheet.set(styleOf(style, base));
            progress[index] = Progress::Added;
            return sheet.findOwn(style.name);
        }
    }

    // --- CompositeSerializer<Text> ---

    void CompositeSerializer<Text>::defineSchema(Section& section, const Text& defaultValue)
    {
        section.addSequence(Keys::Styles, schemaOf(k_styleKeys));
        MultiTypeSequence& pieces = section.addMultiTypeSequence(Keys::Pieces);
        pieces.addBlueprint(Keys::Words, std::wstring{});
        pieces.addBlueprint(Keys::Run, schemaOf(k_runKeys));
        pieces.addBlueprint(Keys::Paragraph, schemaOf(k_paragraphKeys));
        for (const std::wstring_view name : k_objectNames)
            pieces.addBlueprint(name, std::wstring{});
        write(section, defaultValue);
    }

    Text CompositeSerializer<Text>::read(const Section& section)
    {
        Text result;
        readTo(section, result);
        return result;
    }

    void CompositeSerializer<Text>::readTo(const Section& section, Text& out)
    {
        OwnStyles ownStyles;
        const SequenceBase& styles = listAt(section, Keys::Styles);
        for (std::size_t i = 0; i != styles.size(); ++i)
        {
            const Section& entry = static_cast<const Section&>(*styles.child(i));
            ownStyles.push_back(readStyle(entry, ownStyles));
        }

        TextPieces pieces;
        const MultiTypeSequence& stated = static_cast<const MultiTypeSequence&>(
            section / Keys::Pieces);
        for (const MultiTypeSequence::Item& item : stated.items())
            pieces.push_back(readPiece(item, ownStyles));
        out = Text{ std::move(pieces) };
    }

    void CompositeSerializer<Text>::write(Section& node, const Text& value)
    {
        TextPieces pieces = value.pieces();
        for (TextPiece& piece : pieces)
            piece.look = writtenLook(piece.look);
        const OwnStyles ownStyles = ownStylesOf(pieces);

        SequenceBase& styles = listAt(node, Keys::Styles);
        styles.clear();
        for (const OwnStyle& own : ownStyles)
            writeStyle(static_cast<Section&>(styles.addNode()), own);

        MultiTypeSequence& written = static_cast<MultiTypeSequence&>(node / Keys::Pieces);
        written.clear();
        for (const TextPiece& piece : pieces)
            writePiece(written, piece, ownStyles);
    }

    // --- CompositeSerializer<StyleSheet> ---

    void CompositeSerializer<StyleSheet>::defineSchema(Section& section,
        const StyleSheet& defaultValue)
    {
        section.addSequence(Keys::Styles, schemaOf(k_sheetStyleKeys));
        write(section, defaultValue);
    }

    StyleSheet CompositeSerializer<StyleSheet>::read(const Section& section)
    {
        StyleSheet result;
        readTo(section, result);
        return result;
    }

    // **Read into a sheet of its own, then assigned**, so a sheet read again over the one a text
    // names keeps every style the two name alike where it stands. An entry with no name, or a name
    // an entry above it took, is skipped.
    void CompositeSerializer<StyleSheet>::readTo(const Section& section, StyleSheet& out)
    {
        StatedStyles stated;
        const SequenceBase& styles = listAt(section, Keys::Styles);
        for (std::size_t i = 0; i != styles.size(); ++i)
        {
            const Section& entry = static_cast<const Section&>(*styles.child(i));
            StatedStyle style{
                .name = valueAt(entry, Keys::Name),
                .base = valueAt(entry, Keys::Base),
            };
            if (style.name.empty())
                continue;
            if (std::ranges::find(stated, style.name, &StatedStyle::name) != stated.end())
                continue;
            readFields(entry, style.fields);
            stated.push_back(std::move(style));
        }

        StyleSheet result;
        std::vector<Progress> progress(stated.size(), Progress::Waiting);
        for (std::size_t i = 0; i != stated.size(); ++i)
            addStated(result, stated, progress, i);
        // Added base first, then listed as the file lists them.
        for (std::size_t i = 0; i != stated.size(); ++i)
            result.moveTo(stated[i].name, i);
        out = result;
    }

    void CompositeSerializer<StyleSheet>::write(Section& node, const StyleSheet& value)
    {
        SequenceBase& styles = listAt(node, Keys::Styles);
        styles.clear();
        for (const TextStyle* style : value.styles())
            writeStyle(static_cast<Section&>(styles.addNode()), entryOf(*style));
    }
}
