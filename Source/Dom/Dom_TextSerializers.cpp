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
        // The keys of a text's section, of one of its own styles or a sheet's, the names and keys
        // of its pieces, and the keys of a text format's section.
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

        // What a look's name starts with, which no style's does.
        constexpr wchar_t k_lookMark = L'#';

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

        constexpr std::array k_formatKeys{
            Keys::Style,
            Keys::Ink,
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

        // An entry written for a run style that states more than its base: the name the file
        // gives it, and the style, named none.
        struct LookEntry
        {
            std::wstring name{};
            TextStyle style{};
        };

        using LookEntries = std::vector<LookEntry>;

        // Whether a style states its base and nothing more, which a piece names without an entry.
        [[nodiscard]] bool namesBaseOnly(const TextStyle& style)
        {
            return TextStyle{ .base = style.base } == style;
        }

        // Whether a name is a look's, # and a number, which names no style.
        [[nodiscard]] bool isLookName(std::wstring_view name)
        {
            return name.size() > 1 && name.front() == k_lookMark
                && std::ranges::all_of(name.substr(1), [](wchar_t character) {
                    return character >= L'0' && character <= L'9';
                });
        }

        // An entry per run style stating more than its base, each named # and a number.
        [[nodiscard]] LookEntries lookEntriesOf(const TextPieces& pieces)
        {
            LookEntries result;
            for (const TextPiece& piece : pieces)
            {
                if (piece.paragraph || namesBaseOnly(piece.style))
                    continue;
                if (std::ranges::find(result, piece.style, &LookEntry::style) != result.end())
                    continue;
                result.push_back({
                    .name = k_lookMark + std::to_wstring(result.size() + 1),
                    .style = piece.style,
                });
            }
            return result;
        }

        // A style's entry: its name, its base where that is not the style of the same name, which
        // goes without saying, and the fields and script levels it states itself.
        void writeStyle(Section& entry, std::wstring_view name, const TextStyle& style)
        {
            put(entry, Keys::Name, std::wstring{ name });
            if (style.base && style.base->name != name)
                put(entry, Keys::Base, style.base->name);
            if (style.fontFamily)
                put(entry, Keys::FontFamily, *style.fontFamily);
            if (style.fontSize)
                put(entry, Keys::FontSize, floatToStr(*style.fontSize));
            if (style.fontWeight)
                put(entry, Keys::FontWeight, std::wstring{ nameOf(k_weights, *style.fontWeight) });
            if (style.fontStyle)
                put(entry, Keys::FontStyle, std::wstring{ nameOf(k_fontStyles, *style.fontStyle) });
            if (style.ink)
                put(entry, Keys::Ink, inkSpelling(*style.ink));
            std::wstring scripts;
            for (const TextScript script : style.scripts)
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
        void writeRun(Section& entry, const TextPiece& piece, const LookEntries& looks)
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

            if (!namesBaseOnly(piece.style))
            {
                const LookEntries::const_iterator look = std::ranges::find(looks, piece.style,
                    &LookEntry::style);
                put(entry, Keys::Style, look->name);
            }
            else if (piece.style.base)
            {
                put(entry, Keys::Style, piece.style.base->name);
            }
            put(entry, Keys::Link, piece.link);
            put(entry, Keys::Anchor, piece.anchor);
        }

        // **A piece in no style, linking nowhere, is one line**: its words unnamed, or an object
        // under its name. Anything more is a Run, and a paragraph's look a P.
        void writePiece(MultiTypeSequence& pieces, const TextPiece& piece,
            const LookEntries& looks)
        {
            if (piece.paragraph)
            {
                writeParagraph(static_cast<Section&>(*pieces.add(Keys::Paragraph)), *piece.paragraph);
                return;
            }

            if (!piece.style.empty() || !piece.link.empty() || !piece.anchor.empty())
            {
                writeRun(static_cast<Section&>(*pieces.add(Keys::Run)), piece, looks);
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

        // The fields an entry states, the script levels included, written into the style.
        void readFields(const Section& entry, TextStyle& style)
        {
            if (const std::wstring family = valueAt(entry, Keys::FontFamily); !family.empty())
                style.fontFamily = family;
            float size = 0.0f;
            if (tryStrToFloat(valueAt(entry, Keys::FontSize), size))
                style.fontSize = size;
            const std::optional<FontWeight> weight = valueOf(k_weights,
                valueAt(entry, Keys::FontWeight));
            if (weight)
                style.fontWeight = weight;
            const std::optional<FontStyle> fontStyle = valueOf(k_fontStyles,
                valueAt(entry, Keys::FontStyle));
            if (fontStyle)
                style.fontStyle = fontStyle;
            if (const std::optional<ColorDef> ink = inkOf(valueAt(entry, Keys::Ink)))
                style.ink = ink;

            const std::wstring scripts = valueAt(entry, Keys::Scripts);
            std::wstring_view rest = scripts;
            while (!rest.empty())
            {
                if (const std::optional<TextScript> script = valueOf(k_scripts, takeWord(rest)))
                    style.scripts.push_back(*script);
            }
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

        // A P stating nothing ends the look in effect, and the paragraphs from there state none.
        [[nodiscard]] ParagraphLook paragraphOf(const Section& entry)
        {
            ParagraphLook look;
            look.align = valueOf(k_aligns, valueAt(entry, Keys::Align));
            float value = 0.0f;
            if (tryStrToFloat(valueAt(entry, Keys::Indent), value))
                look.indent = value;
            if (tryStrToFloat(valueAt(entry, Keys::LineSpacing), value))
                look.lineSpacing = value;
            return look;
        }

        // **A name a file states that no style answers to is reported**, as one written in code is,
        // and what names it stands on no style.
        [[nodiscard]] const TextStyle* findOrReport(const StyleSheet& sheet, std::wstring_view name)
        {
            const TextStyle* found = sheet.find(name);
            if (!found)
                StyleSheet::events().emit<UnknownStyleEvent>(name);
            return found;
        }

        // The style a piece names: a look of the text's file, else a style the text reaches, else
        // none, and the words read back in no style.
        [[nodiscard]] TextStyle styleNamed(std::wstring_view name, const StyleSheet& styles,
            const LookEntries& looks)
        {
            if (name.empty())
                return TextStyle{};
            const LookEntries::const_iterator look = std::ranges::find(looks, name,
                &LookEntry::name);
            if (look != looks.end())
                return look->style;
            return TextStyle{ .base = findOrReport(styles, name) };
        }

        [[nodiscard]] TextPiece readRun(const Section& entry, const StyleSheet& styles,
            const LookEntries& looks)
        {
            TextPiece piece{
                .words = valueAt(entry, Keys::Words),
                .style = styleNamed(valueAt(entry, Keys::Style), styles, looks),
                .link = valueAt(entry, Keys::Link),
                .anchor = valueAt(entry, Keys::Anchor),
                .object = objectOf(entry),
            };
            if (piece.object)
                piece.words = std::wstring{ k_objectChar };
            return piece;
        }

        [[nodiscard]] TextPiece readPiece(const MultiTypeSequence::Item& item,
            const StyleSheet& styles, const LookEntries& looks)
        {
            if (item.name == Keys::Run)
                return readRun(static_cast<const Section&>(*item.node), styles, looks);
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

        // A style as a sheet's entry states it: its name, its base by name, and its fields.
        struct StatedStyle
        {
            std::wstring base{};
            TextStyle style{};
        };

        using StatedStyles = std::vector<StatedStyle>;

        // How far a stated style has come into the sheet being read.
        enum class Progress
        {
            Waiting,
            Adding,
            Added
        };

        // **A style goes into the sheet after its base**, so every one is added and none changed,
        // and carried, which is no edit. A base the file states none of is the parents' of that
        // name, and one leading back to the style is dropped.
        const TextStyle* addStated(StyleSheet& sheet, const StatedStyles& stated,
            std::vector<Progress>& progress, std::size_t index)
        {
            const StatedStyle& entry = stated[index];
            const std::wstring& name = entry.style.name;
            if (progress[index] == Progress::Added)
                return sheet.findOwn(name);
            if (progress[index] == Progress::Adding)
                return nullptr;
            progress[index] = Progress::Adding;

            TextStyle style = entry.style;
            if (entry.base.empty() || entry.base == name)
            {
                style.base = sheet.find(name);
            }
            else
            {
                const StatedStyles::const_iterator found = std::ranges::find_if(stated,
                    [&entry](const StatedStyle& other) { return other.style.name == entry.base; });
                style.base = found == stated.end()
                    ? findOrReport(sheet, entry.base)
                    : addStated(sheet, stated, progress,
                        static_cast<std::size_t>(found - stated.begin()));
            }
            sheet.carry(style);
            progress[index] = Progress::Added;
            return sheet.findOwn(name);
        }

        // **Every named entry goes into the sheet, each after its base, listed in the file's
        // order.** An entry with no name, or a name an entry above it took, is skipped. Where looks
        // are asked for, an entry named as a look is one of them, based on a style the sheet then
        // reaches.
        void readStyles(const SequenceBase& entries, StyleSheet& sheet, LookEntries* looks)
        {
            StatedStyles stated;
            std::vector<StatedStyle> statedLooks;
            for (std::size_t i = 0; i != entries.size(); ++i)
            {
                const Section& entry = static_cast<const Section&>(*entries.child(i));
                StatedStyle style{
                    .base = valueAt(entry, Keys::Base),
                    .style{ .name = valueAt(entry, Keys::Name) },
                };
                const std::wstring& name = style.style.name;
                if (name.empty())
                    continue;
                const auto namedSo = [&name](const StatedStyle& other) {
                    return other.style.name == name;
                };
                if (std::ranges::any_of(stated, namedSo)
                    || std::ranges::any_of(statedLooks, namedSo))
                {
                    continue;
                }
                readFields(entry, style.style);
                if (looks && isLookName(name))
                    statedLooks.push_back(std::move(style));
                else
                    stated.push_back(std::move(style));
            }

            std::vector<Progress> progress(stated.size(), Progress::Waiting);
            for (std::size_t i = 0; i != stated.size(); ++i)
                addStated(sheet, stated, progress, i);
            for (std::size_t i = 0; i != stated.size(); ++i)
                sheet.moveTo(stated[i].style.name, i);

            for (const StatedStyle& look : statedLooks)
            {
                LookEntry entry{ .name = look.style.name, .style = look.style };
                entry.style.name.clear();
                entry.style.base = look.base.empty() ? nullptr : findOrReport(sheet, look.base);
                looks->push_back(std::move(entry));
            }
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

    // **A text is read over the reading sheet**, or the application's where no scope names one,
    // its own styles and looks first and its pieces based on them or on that sheet.
    void CompositeSerializer<Text>::readTo(const Section& section, Text& out)
    {
        const StyleSheet* reading = StyleSheet::reading();
        StyleSheet styles{ reading ? reading : &StyleSheet::application() };
        LookEntries looks;
        readStyles(listAt(section, Keys::Styles), styles, &looks);

        TextPieces pieces;
        const MultiTypeSequence& stated = static_cast<const MultiTypeSequence&>(
            section / Keys::Pieces);
        for (const MultiTypeSequence::Item& item : stated.items())
            pieces.push_back(readPiece(item, styles, looks));
        out = Text{ std::move(pieces), std::move(styles) };
    }

    // The text's own named styles come first, each after its base, then the looks, so a reader
    // finds every base above the entry naming it.
    void CompositeSerializer<Text>::write(Section& node, const Text& value)
    {
        const TextPieces pieces = value.pieces();
        const LookEntries looks = lookEntriesOf(pieces);

        SequenceBase& styles = listAt(node, Keys::Styles);
        styles.clear();
        for (const TextStyle* style : value.styles().styles())
            writeStyle(static_cast<Section&>(styles.addNode()), style->name, *style);
        for (const LookEntry& look : looks)
            writeStyle(static_cast<Section&>(styles.addNode()), look.name, look.style);

        MultiTypeSequence& written = static_cast<MultiTypeSequence&>(node / Keys::Pieces);
        written.clear();
        for (const TextPiece& piece : pieces)
            writePiece(written, piece, looks);
    }

    // --- CompositeSerializer<StyleSheet> ---

    void CompositeSerializer<StyleSheet>::defineSchema(Section& section,
        const StyleSheet& defaultValue)
    {
        section.addSequence(Keys::Styles, schemaOf(k_styleKeys));
        write(section, defaultValue);
    }

    StyleSheet CompositeSerializer<StyleSheet>::read(const Section& section)
    {
        StyleSheet result;
        readTo(section, result);
        return result;
    }

    // **Read into a sheet of its own over the same parent, then taken**, so a sheet read again
    // over one a text is based on keeps every style the two name alike where it stands.
    void CompositeSerializer<StyleSheet>::readTo(const Section& section, StyleSheet& out)
    {
        StyleSheet result{ out.parent() };
        readStyles(listAt(section, Keys::Styles), result, nullptr);
        out.takeStyles(result);
    }

    void CompositeSerializer<StyleSheet>::write(Section& node, const StyleSheet& value)
    {
        SequenceBase& styles = listAt(node, Keys::Styles);
        styles.clear();
        for (const TextStyle* style : value.styles())
            writeStyle(static_cast<Section&>(styles.addNode()), style->name, *style);
    }

    // --- CompositeSerializer<TextFormat> ---

    void CompositeSerializer<TextFormat>::defineSchema(Section& section,
        const TextFormat& defaultValue)
    {
        for (const std::wstring_view key : k_formatKeys)
            section.addValue(key, std::wstring{});
        write(section, defaultValue);
    }

    TextFormat CompositeSerializer<TextFormat>::read(const Section& section)
    {
        TextFormat result;
        readTo(section, result);
        return result;
    }

    // **The style goes by its name**, found where the text is laid out - see TextFormat.
    void CompositeSerializer<TextFormat>::readTo(const Section& section, TextFormat& out)
    {
        TextFormat result;
        if (const std::wstring style = valueAt(section, Keys::Style); !style.empty())
            result << std::wstring_view{ style };
        if (const std::optional<ColorDef> ink = inkOf(valueAt(section, Keys::Ink)))
        {
            std::visit([&result](const auto& held) {
                result << held;
            }, *ink);
        }
        const ParagraphLook paragraph = paragraphOf(section);
        if (paragraph.align)
            result << *paragraph.align;
        if (paragraph.indent)
            result << TextIndent{ *paragraph.indent };
        if (paragraph.lineSpacing)
            result << TextLineSpacing{ *paragraph.lineSpacing };
        out = std::move(result);
    }

    // Every key is emptied first, so a node written again keeps nothing of the format before.
    void CompositeSerializer<TextFormat>::write(Section& node, const TextFormat& value)
    {
        for (const std::wstring_view key : k_formatKeys)
        {
            if (DomNodeBase* child = node.child(key))
                child->set(std::wstring{});
        }
        put(node, Keys::Style, value.style());
        if (value.fields().ink)
            put(node, Keys::Ink, inkSpelling(*value.fields().ink));
        writeParagraph(node, value.paragraph());
    }
}
