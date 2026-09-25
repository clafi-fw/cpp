module ClaFi.Core.TextEngine.Tags;

import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // A tag that takes no argument, and the marker it stands for. The value is read as the
        // enum the kind names, and is unused for a kind that carries none.
        enum class PlainTagKind
        {
            Op,
            Align,
            PopStyle,
            PopColor,
            PopFontSize,
            PopFontFamily,
            PopLink,
            PopAnchor,
            Flex
        };

        struct PlainTag
        {
            std::wstring_view name{};
            PlainTagKind kind{ PlainTagKind::Op };
            int value{ 0 };
        };

        // The first entry for a marker is the spelling written; the rest are read only.
        constexpr std::array k_plainTags{
            PlainTag{ L"n", PlainTagKind::Op, static_cast<int>(TextOp::EndLine) },
            PlainTag{ L"end", PlainTagKind::Op, static_cast<int>(TextOp::EndLine) },
            PlainTag{ L"b", PlainTagKind::Op, static_cast<int>(TextOp::PushBold) },
            PlainTag{ L"/b", PlainTagKind::Op, static_cast<int>(TextOp::PopBold) },
            PlainTag{ L"i", PlainTagKind::Op, static_cast<int>(TextOp::PushItalic) },
            PlainTag{ L"/i", PlainTagKind::Op, static_cast<int>(TextOp::PopItalic) },
            PlainTag{ L"sup", PlainTagKind::Op, static_cast<int>(TextOp::PushSuperscript) },
            PlainTag{ L"sub", PlainTagKind::Op, static_cast<int>(TextOp::PushSubscript) },
            PlainTag{ L"/script", PlainTagKind::Op, static_cast<int>(TextOp::PopScript) },
            PlainTag{ L"/sup", PlainTagKind::Op, static_cast<int>(TextOp::PopScript) },
            PlainTag{ L"/sub", PlainTagKind::Op, static_cast<int>(TextOp::PopScript) },

            PlainTag{ L"/color", PlainTagKind::PopColor, 0 },

            PlainTag{ L"/style", PlainTagKind::PopStyle, 0 },

            PlainTag{ L"left", PlainTagKind::Align, static_cast<int>(TextAlign::Left) },
            PlainTag{ L"center", PlainTagKind::Align, static_cast<int>(TextAlign::Center) },
            PlainTag{ L"right", PlainTagKind::Align, static_cast<int>(TextAlign::Right) },
            PlainTag{ L"justified", PlainTagKind::Align, static_cast<int>(TextAlign::Justified) },

            PlainTag{ L"/size", PlainTagKind::PopFontSize, 0 },
            PlainTag{ L"/font", PlainTagKind::PopFontFamily, 0 },
            PlainTag{ L"/link", PlainTagKind::PopLink, 0 },
            PlainTag{ L"/anchor", PlainTagKind::PopAnchor, 0 },

            PlainTag{ L"flex", PlainTagKind::Flex, 0 },
            PlainTag{ L"fill", PlainTagKind::Flex, 0 },
        };

        // An ink colour as "color" spells it.
        struct ColorName
        {
            std::wstring_view name{};
            InkColor color{ InkColor::Text };
        };

        constexpr std::array k_colorNames{
            ColorName{ L"text", InkColor::Text },
            ColorName{ L"accent", InkColor::Accent },
            ColorName{ L"spot", InkColor::Spot },
            ColorName{ L"yellow", InkColor::Yellow },
            ColorName{ L"green", InkColor::Green },
            ColorName{ L"blue", InkColor::Blue },
            ColorName{ L"red", InkColor::Red },
            ColorName{ L"black", InkColor::Black },
            ColorName{ L"white", InkColor::White },
        };

        // A grade step as "color" spells it.
        struct GradeName
        {
            std::wstring_view name{};
            InkGrade grade{ InkGrade::Strongest };
        };

        constexpr std::array k_gradeNames{
            GradeName{ L"faint", InkGrade::Faint },
            GradeName{ L"subtle", InkGrade::Subtle },
            GradeName{ L"muted", InkGrade::Muted },
            GradeName{ L"strong", InkGrade::Strong },
            GradeName{ L"strongest", InkGrade::Strongest },
        };

        // A text style as "style" spells it, and as Fmt's shorthand names it alone.
        struct StyleName
        {
            std::wstring_view name{};
            TextStyleId style{ TextStyleId::Body };
        };

        constexpr std::array k_styleNames{
            StyleName{ L"section", TextStyleId::Section },
            StyleName{ L"subsection", TextStyleId::SubSection },
            StyleName{ L"title", TextStyleId::Title },
            StyleName{ L"subtitle", TextStyleId::SubTitle },
            StyleName{ L"heading", TextStyleId::Heading },
            StyleName{ L"subheading", TextStyleId::SubHeading },
            StyleName{ L"body", TextStyleId::Body },
            StyleName{ L"subbody", TextStyleId::SubBody },
            StyleName{ L"code", TextStyleId::Code },
        };

        constexpr std::wstring_view k_styleCommand = L"style";
        constexpr std::wstring_view k_colorCommand = L"color";
        constexpr std::wstring_view k_iconCommand = L"icon";
        constexpr std::wstring_view k_linkCommand = L"link";
        constexpr std::wstring_view k_anchorCommand = L"anchor";

        // The word at the front of text, taken off it together with the blanks that follow.
        [[nodiscard]] std::wstring_view takeWord(std::wstring_view& text)
        {
            const std::size_t end = text.find(L' ');
            const std::wstring_view word = text.substr(0, end);
            text = end == std::wstring_view::npos ? std::wstring_view{} : text.substr(end + 1);
            trimLeft(text);
            return word;
        }

        [[nodiscard]] float takeFloat(std::wstring_view& text, float fallback)
        {
            return strToFloatDef(takeWord(text), fallback);
        }

        [[nodiscard]] std::wstring_view plainTagName(PlainTagKind kind, int value)
        {
            for (const PlainTag& tag : k_plainTags)
            {
                if (tag.kind == kind && tag.value == value)
                    return tag.name;
            }
            unreachable("a marker no plain tag spells");
        }

        [[nodiscard]] std::wstring_view colorName(InkColor color)
        {
            for (const ColorName& named : k_colorNames)
            {
                if (named.color == color)
                    return named.name;
            }
            unreachable("an ink colour with no name");
        }

        // A grade as the step it stands at, or as the share itself where no step stands.
        [[nodiscard]] std::wstring gradeSpelling(float grade)
        {
            for (const GradeName& named : k_gradeNames)
            {
                if (gradeOf(named.grade) == grade)
                    return std::wstring{ named.name };
            }
            return floatToStr(grade);
        }

        [[nodiscard]] std::optional<InkColor> inkColorOf(std::wstring_view word)
        {
            for (const ColorName& named : k_colorNames)
            {
                if (named.name == word)
                    return named.color;
            }
            return std::nullopt;
        }

        // A grade read as a step's name or as a share.
        [[nodiscard]] std::optional<float> inkGradeOf(std::wstring_view word)
        {
            for (const GradeName& named : k_gradeNames)
            {
                if (named.name == word)
                    return gradeOf(named.grade);
            }
            float share = 0.0f;
            if (tryStrToFloat(word, share))
                return share;
            return std::nullopt;
        }

        [[nodiscard]] std::wstring_view styleName(TextStyleId style)
        {
            for (const StyleName& named : k_styleNames)
            {
                if (named.style == style)
                    return named.name;
            }
            unreachable("a text style with no name");
        }

        [[nodiscard]] FormatItem markerOf(const PlainTag& tag)
        {
            switch (tag.kind)
            {
                case PlainTagKind::Op:
                    return static_cast<TextOp>(tag.value);
                case PlainTagKind::Align:
                    return static_cast<TextAlign>(tag.value);
                case PlainTagKind::PopStyle:
                    return PopTextStyle{};
                case PlainTagKind::PopColor:
                    return PopColor{};
                case PlainTagKind::PopFontSize:
                    return PopFontSize{};
                case PlainTagKind::PopFontFamily:
                    return PopFontFamily{};
                case PlainTagKind::PopLink:
                    return PopLink{};
                case PlainTagKind::PopAnchor:
                    return PopAnchor{};
                case PlainTagKind::Flex:
                    return FlexSpace{};
            }
            unreachable("a plain tag of no kind");
        }

        // What follows "color": a colour stated outright, or an ink colour and then a grade, either
        // of which may be left out.
        [[nodiscard]] std::optional<FormatItem> colorOf(std::wstring_view arguments)
        {
            if (arguments.starts_with(L'#'))
                return PushCustomColor{ Color{ arguments } };

            Ink ink;
            std::wstring_view rest = arguments;
            std::wstring_view word = takeWord(rest);
            if (const std::optional<InkColor> color = inkColorOf(word))
            {
                ink.color = *color;
                word = takeWord(rest);
            }
            if (!word.empty())
            {
                const std::optional<float> grade = inkGradeOf(word);
                if (!grade || !rest.empty())
                    return std::nullopt;
                ink.grade = *grade;
            }
            return PushThemeColor{ ink };
        }

        [[nodiscard]] std::optional<TextStyleId> styleOf(std::wstring_view name)
        {
            for (const StyleName& named : k_styleNames)
            {
                if (named.name == name)
                    return named.style;
            }
            return std::nullopt;
        }

        // --- One spelling per marker ---

        [[nodiscard]] std::wstring spellWith(std::wstring_view command, float argument)
        {
            std::wstring result{ command };
            result += L' ';
            result += floatToStr(argument);
            return result;
        }

        [[nodiscard]] std::wstring spell(const PushThemeColor& push)
        {
            const Ink& ink = push.ink;
            std::wstring result{ k_colorCommand };
            if (ink.color != InkColor::Text)
            {
                result += L' ';
                result += colorName(ink.color);
            }
            // The text ink at strongest still states its grade, so the tag names an ink.
            if (ink.grade != gradeOf(InkGrade::Strongest) || ink.color == InkColor::Text)
            {
                result += L' ';
                result += gradeSpelling(ink.grade);
            }
            return result;
        }

        [[nodiscard]] std::wstring spell(const PushCustomColor& push)
        {
            std::wstring result{ k_colorCommand };
            result += L' ';
            result += push.color.toStr();
            return result;
        }

        [[nodiscard]] std::wstring spell(const PopColor&)
        {
            return std::wstring{ plainTagName(PlainTagKind::PopColor, 0) };
        }

        [[nodiscard]] std::wstring spell(const PushTextStyle& push)
        {
            std::wstring result{ k_styleCommand };
            result += L' ';
            result += styleName(push.style);
            return result;
        }

        [[nodiscard]] std::wstring spell(const PopTextStyle&)
        {
            return std::wstring{ plainTagName(PlainTagKind::PopStyle, 0) };
        }

        [[nodiscard]] std::wstring spell(TextAlign align)
        {
            return std::wstring{ plainTagName(PlainTagKind::Align, static_cast<int>(align)) };
        }

        [[nodiscard]] std::wstring spell(const SetIndent& indent)
        {
            return spellWith(L"indent", indent.indent);
        }

        [[nodiscard]] std::wstring spell(const SetLineSpacing& spacing)
        {
            return spellWith(L"linespacing", spacing.spacing);
        }

        [[nodiscard]] std::wstring spell(const InTextIcon& icon)
        {
            std::wstring result{ k_iconCommand };
            result += L' ';
            result += floatToStr(icon.designWidth);
            result += L',';
            result += floatToStr(icon.designHeight);
            result += L',';
            result += floatToStr(icon.designBaseline);
            return result;
        }

        [[nodiscard]] std::wstring spell(TextOp op)
        {
            return std::wstring{ plainTagName(PlainTagKind::Op, static_cast<int>(op)) };
        }

        [[nodiscard]] std::wstring spell(const PushFontSize& size)
        {
            return spellWith(L"size", size.size);
        }

        [[nodiscard]] std::wstring spell(const PopFontSize&)
        {
            return std::wstring{ plainTagName(PlainTagKind::PopFontSize, 0) };
        }

        [[nodiscard]] std::wstring spell(const PushFontFamily& family)
        {
            std::wstring result{ L"font " };
            result += family.family;
            return result;
        }

        [[nodiscard]] std::wstring spell(const PopFontFamily&)
        {
            return std::wstring{ plainTagName(PlainTagKind::PopFontFamily, 0) };
        }

        [[nodiscard]] std::wstring spell(const PushLink& link)
        {
            std::wstring result{ k_linkCommand };
            result += L' ';
            result += link.target;
            return result;
        }

        [[nodiscard]] std::wstring spell(const PopLink&)
        {
            return std::wstring{ plainTagName(PlainTagKind::PopLink, 0) };
        }

        [[nodiscard]] std::wstring spell(const PushAnchor& anchor)
        {
            std::wstring result{ k_anchorCommand };
            result += L' ';
            result += anchor.name;
            return result;
        }

        [[nodiscard]] std::wstring spell(const PopAnchor&)
        {
            return std::wstring{ plainTagName(PlainTagKind::PopAnchor, 0) };
        }

        [[nodiscard]] std::wstring spell(const Space& space)
        {
            return spellWith(L"space", space.width);
        }

        [[nodiscard]] std::wstring spell(const VSpace& space)
        {
            return spellWith(L"vspace", space.height);
        }

        [[nodiscard]] std::wstring spell(const FlexSpace&)
        {
            return std::wstring{ plainTagName(PlainTagKind::Flex, 0) };
        }

        [[nodiscard]] std::wstring spell(const TabTo& tab)
        {
            return spellWith(L"tabto", tab.targetX);
        }
    }

    std::optional<FormatItem> formatItemOf(std::wstring_view tag)
    {
        trim(tag);
        std::wstring_view arguments = tag;
        const std::wstring_view command = takeWord(arguments);

        if (command == k_styleCommand)
        {
            if (const std::optional<TextStyleId> style = styleOf(arguments))
                return PushTextStyle{ *style };
            return std::nullopt;
        }
        if (command == k_colorCommand)
            return colorOf(arguments);
        if (command == k_iconCommand)
            return iconOf(arguments);
        if (command == L"space")
        {
            if (arguments == L"flex")
                return FlexSpace{};
            return Space{ takeFloat(arguments, 0.0f) };
        }
        if (command == L"vspace")
            return VSpace{ takeFloat(arguments, 0.0f) };
        if (command == L"indent")
            return SetIndent{ takeFloat(arguments, 0.0f) };
        if (command == L"linespacing")
            return SetLineSpacing{ takeFloat(arguments, 1.0f) };
        if (command == L"tabto")
            return TabTo{ takeFloat(arguments, 0.0f) };
        if (command == L"size")
            return PushFontSize{ takeFloat(arguments, k_textStyles[static_cast<std::size_t>(TextStyleId::Body)].size) };
        if (command == L"font")
            return PushFontFamily{ std::wstring{ arguments } };
        if (command == k_linkCommand)
            return PushLink{ std::wstring{ arguments } };
        if (command == k_anchorCommand)
            return PushAnchor{ std::wstring{ arguments } };

        for (const PlainTag& plain : k_plainTags)
        {
            if (plain.name == tag)
                return markerOf(plain);
        }

        // Fmt's shorthand: a style opened by its name alone and closed by that name.
        if (const std::optional<TextStyleId> style = styleOf(tag))
            return PushTextStyle{ *style };
        if (tag.starts_with(L'/') && styleOf(tag.substr(1)))
            return PopTextStyle{};
        return std::nullopt;
    }

    std::wstring tagOf(const FormatItem& item)
    {
        return std::visit([](const auto& value) {
            return spell(value);
        }, item);
    }

    InTextIcon iconOf(std::wstring_view extent)
    {
        float width = 16.0f;
        float height = 16.0f;
        float baseline = -1.0f;
        const std::size_t firstComma = extent.find(L',');
        if (firstComma == std::wstring_view::npos)
        {
            width = strToFloatDef(extent, width);
            height = width;
        }
        else
        {
            width = strToFloatDef(extent.substr(0, firstComma), width);
            const std::size_t secondComma = extent.find(L',', firstComma + 1);
            if (secondComma == std::wstring_view::npos)
            {
                height = strToFloatDef(extent.substr(firstComma + 1), height);
            }
            else
            {
                height = strToFloatDef(extent.substr(firstComma + 1, secondComma - firstComma - 1), height);
                baseline = strToFloatDef(extent.substr(secondComma + 1), baseline);
            }
        }
        if (baseline < 0.0f)
            baseline = height * 0.85f;
        return InTextIcon{ width, height, baseline, {} };
    }
}
