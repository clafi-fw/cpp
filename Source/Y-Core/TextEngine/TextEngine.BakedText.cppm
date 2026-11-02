export module ClaFi.Core.TextEngine.BakedText;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi
{
    export struct BakedInlineObject {
        float width{ 0.0f };
        float height{ 0.0f };
        float baseline{ 0.0f };
        DrawIconFunc paintLambda{};
        Tag tag{};
        bool isFlexSpace{ false };
        bool isTabTo{ false };
        // What a TabTo states: the X the text after it starts at, in design units. Apart from
        // `width` because a measuring pass writes the resolved gap into width, and the pass that
        // resolves the stop compares where the character landed against this.
        float tabTargetX{ 0.0f };

        bool operator==(const BakedInlineObject& other) const {
            return width == other.width && height == other.height && baseline == other.baseline && tag == other.tag && isFlexSpace == other.isFlexSpace && isTabTo == other.isTabTo;
        }
    };

    // The font where no span states one. See TextEngine-Types#basefont
    export struct BaseFont
    {
        std::wstring family{};
        float size{ 0.0f };
        FontWeight weight{ FontWeight::Normal };
        FontStyle style{ FontStyle::Normal };
    };

    export class BakedText
    {
    public:
        explicit BakedText() = default;
        // The Text is read, never copied, and it is the caller's for as long as this is used:
        // every span here indexes into its characters and plainText hands them straight back.
        // TextLayout is the only caller, and it bakes the Text its own pointer already names.
        void rebuild(const Text& text, const TextFormat& format, bool isEditable = false);
        // Valid from the first rebuild. Nothing reads a BakedText before one - a layout bakes
        // before it shapes.
        const std::wstring& plainText() const { return *m_plainText; }
        const std::vector<ColorSpan>& colors() const { return m_colors; }
        const std::vector<WeightSpan>& weights() const { return m_weights; }
        const std::vector<StyleSpan>& styles() const { return m_styles; }
        const std::vector<SizeSpan>& sizes() const { return m_sizes; }
        const std::vector<FamilySpan>& families() const { return m_families; }
        const std::vector<ScriptSpan>& scripts() const { return m_scripts; }
        const std::vector<ParagraphStyle>& paragraphs() const { return m_paragraphs; }
        const std::vector<std::pair<std::size_t, BakedInlineObject>>& inlineObjects() const { return m_inlineObjects; }
        // The links, in text order and not overlapping. Two that meet and name one target are one
        // span, so a link the edits split into runs is still one link to point at.
        [[nodiscard]] const std::vector<LinkSpan>& links() const { return m_links; }
        [[nodiscard]] const BaseFont& baseFont() const { return m_baseFont; }

    private:
        [[nodiscard]] static BakedInlineObject bakedObjectOf(const InlineObject&);

    private:
        const std::wstring* m_plainText{ nullptr };
        std::vector<ColorSpan> m_colors;
        std::vector<WeightSpan> m_weights;
        std::vector<StyleSpan> m_styles;
        std::vector<SizeSpan> m_sizes;
        std::vector<FamilySpan> m_families;
        std::vector<ScriptSpan> m_scripts;
        std::vector<ParagraphStyle> m_paragraphs;
        std::vector<std::pair<std::size_t, BakedInlineObject>> m_inlineObjects;
        std::vector<LinkSpan> m_links;
        BaseFont m_baseFont{};
    };
}
