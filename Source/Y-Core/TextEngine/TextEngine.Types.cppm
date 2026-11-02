module;
#include "../System/Switches.h"

export module ClaFi.Core.TextEngine.Types;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
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
        // Whether the caret is drawn on this pass. The engine only reads it: a blink is a clock,
        // and the focus is a question about the form, so both belong to the control that owns the
        // caret. The selection is unaffected - it stays visible whatever this says.
        bool caretVisible{ true };
    };

    // How heavy a face is drawn.
    export enum class FontWeight { Light = 300, Normal = 400, SemiBold = 600, Bold = 700 };
    // Whether a face is upright or italic.
    export enum class FontStyle { Normal, Italic };

    // The families a text style names. Generic, and each platform resolves them to a face of its
    // own - the framework does not know what the machine it runs on has installed. A concrete
    // family a text names - FamilyText{ L"Consolas", ... } - is passed through as written, and
    // stands or falls on the fonts installed.
    export namespace GenericFamily
    {
        constexpr const wchar_t* SansSerif = L"sans-serif";
        constexpr const wchar_t* Monospace = L"monospace";
    }

    export using ColorDef = std::variant<Ink, Color>;

    // Which way off the baseline a script level takes its words.
    export enum class TextScript
    {
        Super,
        Sub
    };

    // A look words are set in, named in a sheet or a text's own. See TextEngine-Types#textstyle
    export struct TextStyle
    {
        std::wstring name{}; // empty on a style cached by its values
        const TextStyle* base{ nullptr }; // the style this one states its fields over
        std::optional<std::wstring> fontFamily{};
        std::optional<float> fontSize{};
        std::optional<FontWeight> fontWeight{};
        std::optional<FontStyle> fontStyle{};
        std::optional<ColorDef> ink{};
        std::vector<TextScript> scripts{}; // the outermost level first, under its base's

        // What the style states for the field, itself or through a style it is based on.
        template<typename Value>
        [[nodiscard]] constexpr const Value* stated(std::optional<Value> TextStyle::*) const;
        // Every script level it states, its bases' first, so the outermost leads.
        [[nodiscard]] constexpr std::vector<TextScript> levels() const;
        // Whether it states nothing: no base, no field, no script level.
        [[nodiscard]] constexpr bool empty() const;
        bool operator==(const TextStyle&) const = default;
    };

    // The names of the framework's styles, which its sheet holds. See TextEngine-Types#textstyles
    export namespace TextStyles
    {
        inline constexpr std::wstring_view Body = L"Body";
        inline constexpr std::wstring_view Title = L"Title";
        inline constexpr std::wstring_view SubTitle = L"SubTitle";
        inline constexpr std::wstring_view Heading = L"Heading";
        inline constexpr std::wstring_view SubHeading = L"SubHeading";
        inline constexpr std::wstring_view Section = L"Section";
        inline constexpr std::wstring_view SubSection = L"SubSection";
        inline constexpr std::wstring_view SubBody = L"SubBody";
        inline constexpr std::wstring_view Code = L"Code";
    }

    // How a paragraph is aligned inside the box it is laid out in.
    export enum class TextAlign { Left, Center, Right, Justified };

    // How a paragraph is laid out, as far as it states. See TextEngine-Types#paragraphlook
    export struct ParagraphLook
    {
        std::optional<TextAlign> align{};
        std::optional<float> indent{};
        std::optional<float> lineSpacing{};

        [[nodiscard]] constexpr bool empty() const;
        bool operator==(const ParagraphLook&) const = default;
    };

    // What a script run comes to, as fractions of the run it stands on. A level multiplies the
    // size and moves the baseline by its share of the size that level was entered at, so a script
    // inside a script compounds both.
    //
    // A line is not made taller for a script: DirectWrite breaks lines from the font metrics of
    // the runs, and the shift is applied where a run is **drawn** rather than where it is measured.
    // superRise plus a script's own cap height stays inside the ascent of the run it stands on,
    // so a raised run keeps to its line. A lowered one has no such room - a sub whose glyphs
    // descend reaches past the descent of the run it stands on, and the line box does not grow
    // for it.
    export namespace ScriptMetrics
    {
        constexpr float SizeFactor = 0.72f;
        constexpr float SuperRise = 0.40f;
        constexpr float SubDrop = 0.16f;
    }

    export template<typename T>
        struct TextSpan {
        TextRange range;
        T value;
        bool operator==(const TextSpan&) const = default;
    };

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

    export struct ParagraphStyle {
        TextRange range;
        TextAlign alignment{ TextAlign::Left };
        float lineSpacing{ 1.0f };
        float indent{ 0.0f };
        bool operator==(const ParagraphStyle&) const = default;
    };

    export using DrawIconFunc = std::function<void(DrawIconEvent&)>;

    export class InTextIcon {
    public:
        InTextIcon(float designWidth, float designHeight, float designBaseline, const DrawIconFunc&, Tag = {});
        InTextIcon(float designWidth, float designHeight, const DrawIconFunc&, Tag = {});
        InTextIcon(float designWidth, const DrawIconFunc&, Tag = {});
    public:
        float designWidth;
        float designHeight;
        float designBaseline;
        DrawIconFunc paintLambda;
        Tag tag;
    public:
        bool operator==(const InTextIcon& other) const {
            return designWidth == other.designWidth && designHeight == other.designHeight && designBaseline == other.designBaseline && tag == other.tag;
        }
    };

    InTextIcon::InTextIcon(float designWidth, float designHeight, float designBaseline, const DrawIconFunc& paintLambda, Tag tag)
        :
        designWidth{ designWidth },
        designHeight{ designHeight },
        designBaseline{ designBaseline },
        paintLambda{ paintLambda },
        tag{ tag }
    {
    }

    InTextIcon::InTextIcon(float designWidth, float designHeight, const DrawIconFunc& paintLambda, Tag tag)
        :
        InTextIcon{ designWidth, designHeight, designHeight * 0.85f, paintLambda, tag }
    {
    }

    InTextIcon::InTextIcon(float designWidth, const DrawIconFunc& paintLambda, Tag tag)
        :
        InTextIcon{ designWidth, designWidth, paintLambda, tag }
    {
    }

    export struct Space { float width; bool operator==(const Space&) const = default; };
    export struct VSpace { float height; bool operator==(const VSpace&) const = default; };
    // A gap taking the room its line has over, never less than its width. See TextEngine-Types
    export struct FlexSpace
    {
        float minWidth{ 0.0f };
        bool operator==(const FlexSpace&) const = default;
    };
    export struct TabTo { float targetX; bool operator==(const TabTo&) const = default; };

    export constexpr wchar_t k_endLine = L'\n'; // the line end - LF alone, which cuts a paragraph

    // How far a paragraph's lines are indented, in design units.
    export struct TextIndent { float indent; bool operator==(const TextIndent&) const = default; };
    // A paragraph's line height as a share of the font's.
    export struct TextLineSpacing { float spacing; bool operator==(const TextLineSpacing&) const = default; };

    // A thing standing in a text as one character of its own. See TextEngine-Types#inlineobject
    export using InlineObject = std::variant<InTextIcon, Space, VSpace, FlexSpace, TabTo>;

    // The character an inline object stands at in the plain text.
    export constexpr wchar_t k_objectChar = L'\uFFFC';

    export struct DrawTextResult
    {
        bool drawn{};
        bool trimmed{};
    };

    // The width a cut line's fade grows to, before the scale.
    export constexpr float k_lineFadeWidth = 30.0f;

    // A line its box cuts at the trailing edge, which it is faded out toward. See TextEngine-Types
    export struct LineFade
    {
        float baseline{ 0.0f };
        float overrun{ 0.0f }; // how far the line runs past the edge
    };

    // How wide the fade of a line running that far past the edge is.
    export [[nodiscard]] constexpr float lineFadeSpan(float overrun, float fadeWidth);

#if CLAFI_TEXT_MOVING
    // What a control's text has to survive, stated as three cases. See TextEngine-Types
    export enum class TextRenderMode
    {
        Static,
        Movable,
        Moving
    };
#endif

    template<typename Value>
    constexpr const Value* TextStyle::stated(std::optional<Value> TextStyle::* field) const
    {
        for (const TextStyle* style = this; style; style = style->base)
        {
            if ((style->*field).has_value())
                return &*(style->*field);
        }
        return nullptr;
    }

    constexpr std::vector<TextScript> TextStyle::levels() const
    {
        std::vector<TextScript> result = base ? base->levels() : std::vector<TextScript>{};
        result.insert(result.end(), scripts.begin(), scripts.end());
        return result;
    }

    constexpr bool TextStyle::empty() const
    {
        return !base && !fontFamily && !fontSize && !fontWeight && !fontStyle && !ink
            && scripts.empty();
    }

    constexpr bool ParagraphLook::empty() const
    {
        return !align && !indent && !lineSpacing;
    }

    constexpr float lineFadeSpan(float overrun, float fadeWidth)
    {
        return std::clamp(overrun, 0.0f, fadeWidth);
    }

}
