export module ClaFi.Core.TextEngine.Types;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    export struct TextRange {
    public:
        std::size_t start{ k_maxSize };
        std::size_t length{ 0 };
    public:
        void reset() {
            start = k_maxSize;
            length = 0;
        }
        // use last() only if the length is not 0;
        std::size_t last() const { return start + length - 1; }
        constexpr std::size_t end() const { return start + length; }
        bool operator==(const TextRange&) const = default;
    };

    export struct CaretHit
    {
        std::size_t pos{ 0 };
        bool trailing{ false };
    };

    // Where a position stands as a reader counts it, both numbers from one. See TextEngine-Types
    export struct TextLineColumn
    {
        std::size_t line{ 1 };
        std::size_t column{ 1 };
    };

    export class EditProps
    {
    public:
        void reset() {
            selRange.reset();
            caretOnLeft = false;
            affinityTrailing = false;
        }
        std::size_t caretPos() const {
            return caretOnLeft ? selRange.start : selRange.end();
        }
    public:
        TextRange selRange{};
        bool caretOnLeft{};
        bool affinityTrailing{ caretOnLeft };
        std::optional<float> targetX{};
        std::vector<TextRange> hits{};
        // Whether the caret is drawn on this pass. The engine only reads it: a blink is a clock,
        // and the focus is a question about the form, so both belong to the control that owns the
        // caret. The selection is unaffected - it stays visible whatever this says.
        bool caretVisible{ true };
    };

    // The named styles a run of text can carry.
    export enum class TextStyleId : std::size_t {
        Body = 0ull,
        Title = 1ull,
        SubTitle = 2ull,
        Heading = 3ull,
        SubHeading = 4ull,
        Section = 5ull,
        SubSection = 6ull,
        SubBody = 7ull,
        Code = 8ull,
        Count = 9ull
    };

    // How heavy a face is drawn.
    export enum class FontWeight { Light = 300, Normal = 400, SemiBold = 600, Bold = 700 };
    // Whether a face is upright or italic.
    export enum class FontStyle { Normal, Italic };

    export struct TextStyle {
        const wchar_t* family;
        float size;
        FontWeight weight;
        FontStyle style;
    };

    // The families a text style names. Generic, and each platform resolves them to a face of its
    // own - the framework does not know what the machine it runs on has installed. A concrete
    // family written into a text - [font Consolas] - is passed through as written, and stands or
    // falls on the fonts installed.
    export namespace GenericFamily
    {
        constexpr const wchar_t* sansSerif = L"sans-serif";
        constexpr const wchar_t* monospace = L"monospace";
    }

    export constexpr std::array<TextStyle, static_cast<std::size_t>(TextStyleId::Count)> k_textStyles = { {
            { GenericFamily::sansSerif, 14.0f, FontWeight::Normal, FontStyle::Normal },  // Body
            { GenericFamily::sansSerif, 28.0f, FontWeight::Bold,   FontStyle::Normal },  // Title
            { GenericFamily::sansSerif, 20.0f, FontWeight::Normal, FontStyle::Normal },  // Subtitle
            { GenericFamily::sansSerif, 18.0f, FontWeight::SemiBold, FontStyle::Normal },// Heading
            { GenericFamily::sansSerif, 16.0f, FontWeight::Normal, FontStyle::Normal },  // SubHeading
            { GenericFamily::sansSerif, 14.0f, FontWeight::Bold,   FontStyle::Normal },  // Section
            { GenericFamily::sansSerif, 14.0f, FontWeight::Normal, FontStyle::Italic },  // SubSection
            { GenericFamily::sansSerif, 12.0f, FontWeight::Normal, FontStyle::Normal },  // SubBody
            { GenericFamily::monospace, 14.0f, FontWeight::Normal, FontStyle::Normal },  // Code
        } };

    // How far apart the tab stops of a paragraph stand, in spaces of the font it starts in - so in
    // a monospace font a tab reaches the next multiple of this many columns.
    export constexpr float k_tabStopSpaces = 4.0f;

    // What a script run comes to, as fractions of the run it stands on. A level multiplies the
    // size and moves the baseline by its share of the size that level was entered at, so a script
    // inside a script compounds both.
    //
    // A line is not made taller for a script: DirectWrite breaks lines from the font metrics of
    // the runs, and the shift is applied where a run is DRAWN rather than where it is measured.
    // superRise plus a script's own cap height stays inside the ascent of the run it stands on,
    // so a raised run keeps to its line. A lowered one has no such room - a sub whose glyphs
    // descend reaches past the descent of the run it stands on, and the line box does not grow
    // for it.
    export namespace ScriptMetrics
    {
        constexpr float sizeFactor = 0.72f;
        constexpr float superRise = 0.40f;
        constexpr float subDrop = 0.16f;
    }

    export template<typename T>
        struct TextSpan {
        TextRange range;
        T value;
        bool operator==(const TextSpan&) const = default;
    };

    export using ColorDef = std::variant<Ink, Color>;
    export using ColorSpan = TextSpan<ColorDef>;
    export using WeightSpan = TextSpan<FontWeight>;
    export using StyleSpan = TextSpan<FontStyle>;
    export using SizeSpan = TextSpan<float>;
    export using FamilySpan = TextSpan<std::wstring>;
    // How far off the baseline of its line a run is drawn, in design units, positive upward.
    // Only the ranges a script covers are spanned - a run standing on the baseline states
    // nothing.
    export using ScriptSpan = TextSpan<float>;
    // A run of text that is a link, and its target. The view is into the Text the span was baked
    // from, and holds while that Text stands unchanged.
    export using LinkSpan = TextSpan<std::wstring_view>;
    // A link found under a point, and the character of it the point stands on.
    export struct LinkHit
    {
        LinkSpan link{};
        std::size_t pos{ 0 };
    };

    // How a paragraph is aligned inside the box it is laid out in.
    export enum class TextAlign { Left, Center, Right, Justified };

    export struct ParagraphStyle {
        TextRange range;
        TextAlign alignment{ TextAlign::Left };
        float lineSpacing{ 1.0f };
        float indent{ 0.0f };
        bool operator==(const ParagraphStyle&) const = default;
    };

    export using PaintIconFunc = std::function<void(PaintIconEvent&)>;

    export class InTextIcon {
    public:
        InTextIcon(float designWidth, float designHeight, float designBaseline, const PaintIconFunc&, Tag = 0);
        InTextIcon(float designWidth, float designHeight, const PaintIconFunc&, Tag = 0);
        InTextIcon(float designWidth, const PaintIconFunc&, Tag = 0);
    public:
        float designWidth;
        float designHeight;
        float designBaseline;
        PaintIconFunc paintLambda;
        Tag tag;
    public:
        bool operator==(const InTextIcon& other) const {
            return designWidth == other.designWidth && designHeight == other.designHeight && designBaseline == other.designBaseline && tag == other.tag;
        }
    };

    InTextIcon::InTextIcon(float designWidth, float designHeight, float designBaseline, const PaintIconFunc& paintLambda, Tag tag)
        :
        designWidth{ designWidth },
        designHeight{ designHeight },
        designBaseline{ designBaseline },
        paintLambda{ paintLambda },
        tag{ tag }
    {
    }

    InTextIcon::InTextIcon(float designWidth, float designHeight, const PaintIconFunc& paintLambda, Tag tag)
        :
        InTextIcon{ designWidth, designHeight, designHeight * 0.85f, paintLambda, tag }
    {
    }

    InTextIcon::InTextIcon(float designWidth, const PaintIconFunc& paintLambda, Tag tag)
        :
        InTextIcon{ designWidth, designWidth, paintLambda, tag }
    {
    }

    export struct Space { float width; bool operator==(const Space&) const = default; };
    export struct VSpace { float height; bool operator==(const VSpace&) const = default; };
    // A gap that takes the room its line has over, and never less than the width it states.
    // That width is what shows on a line with nothing to spare, so a caller wanting the two
    // sides held apart there states it here rather than writing a space beside the gap.
    export struct FlexSpace
    {
        float minWidth{ 0.0f };
        bool operator==(const FlexSpace&) const = default;
    };
    export struct TabTo { float targetX; bool operator==(const TabTo&) const = default; };

    // One instruction in a run of text: a break, or a style pushed or popped.
    export enum class TextOp {
        EndLine,
        PushBold,
        PopBold,
        PushItalic,
        PopItalic,
        PushSuperscript,
        PushSubscript,
        PopScript // one pop for both, because one stack holds them. See TextEngine-Types
    };

    export struct SetIndent { float indent; bool operator==(const SetIndent&) const = default; };
    export struct SetLineSpacing { float spacing; bool operator==(const SetLineSpacing&) const = default; };
    export struct PushTextStyle { TextStyleId style; bool operator==(const PushTextStyle&) const = default; };
    export struct PopTextStyle { bool operator==(const PopTextStyle&) const = default; };
    export struct PushFontSize { float size; bool operator==(const PushFontSize&) const = default; };
    export struct PopFontSize { bool operator==(const PopFontSize&) const = default; };
    export struct PushFontFamily { std::wstring family; bool operator==(const PushFontFamily&) const = default; };
    export struct PopFontFamily { bool operator==(const PopFontFamily&) const = default; };
    export struct PushThemeColor { Ink ink; bool operator==(const PushThemeColor&) const = default; };
    export struct PushCustomColor { ClaFi::Color color; bool operator==(const PushCustomColor&) const = default; };
    export struct PopColor { bool operator==(const PopColor&) const = default; };
    // Opens a link to the target it names. See TextEngine-Types#links
    export struct PushLink
    {
        std::wstring target;
        bool operator==(const PushLink&) const = default;
    };
    // Closes the innermost open link.
    export struct PopLink { bool operator==(const PopLink&) const = default; };
    // Opens an anchor, which a link reaches with # and the name. See TextEngine-Types#anchors
    export struct PushAnchor
    {
        std::wstring name;
        bool operator==(const PushAnchor&) const = default;
    };
    // Closes the innermost open anchor.
    export struct PopAnchor { bool operator==(const PopAnchor&) const = default; };

    export using FormatItem = std::variant<
        PushThemeColor, PushCustomColor, PopColor, PushTextStyle, PopTextStyle,
        TextAlign, SetIndent, SetLineSpacing, InTextIcon, TextOp,
        PushFontSize, PopFontSize, PushFontFamily, PopFontFamily,
        Space, VSpace, FlexSpace, TabTo, PushLink, PopLink, PushAnchor, PopAnchor
    >;

    export struct DrawTextResult
    {
        bool drawn{};
        bool trimmed{};
    };

    // What a control's text has to survive, stated as three cases. See TextEngine-Types
    export enum class TextRenderMode
    {
        Static,
        Movable,
        Moving
    };

}
