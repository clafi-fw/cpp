module;
#include "Linux.TextHeaders.h"
export module ClaFi.Platform.Linux.Fonts;

import ClaFi.Core.Graphics.Cpu_GlyphCompositor;
import ClaFi.Core.TextEngine.Types;
import ClaFi.StdLib;

namespace ClaFi::Platform::Linux
{
    // The vertical metrics of a face at one em size, in pixels. See Platform
    export struct FaceMetrics
    {
        float ascent;
        float descent;
        float lineGap;

        [[nodiscard]] float lineHeight() const { return ascent + descent + lineGap; }
    };

    // One font file at one face index, as fontconfig matched it. See Platform
    export class FontFace
    {
    public:
        FontFace(FT_Library, const std::string& path, int faceIndex);
        ~FontFace();
        FontFace(const FontFace&) = delete;
        FontFace& operator=(const FontFace&) = delete;
    public:
        [[nodiscard]] hb_font_t* hbFont() const { return m_hbFont; }
        [[nodiscard]] float unitsPerEm() const { return m_unitsPerEm; }
        [[nodiscard]] FaceMetrics metrics(float emSize) const;
        // Whether the face states every glyph one advance wide.
        [[nodiscard]] bool isFixedWidth() const;
        // Whether the face has a glyph of its own for the code point.
        [[nodiscard]] bool hasGlyph(char32_t codePoint) const;
        // The face's glyph for the code point, or zero - .notdef - where it has none.
        [[nodiscard]] std::uint32_t glyphIndex(char32_t codePoint) const;
        // The advance of a glyph at an em size in pixels.
        [[nodiscard]] float glyphAdvance(std::uint32_t glyph, float emSize) const;
        // The advance of the code point's glyph at an em size in pixels - .notdef's where the face
        // has none.
        [[nodiscard]] float advance(char32_t codePoint, float emSize) const;
        // Coverage for a glyph at an em size in pixels, rasterized on first use and kept. The
        // reference stays valid for the life of the face: the slots for a size are allocated once,
        // at the face's glyph count, and are never grown - which is what the compositor holding a
        // pointer to it across a run relies on.
        [[nodiscard]] const Graphics::Cpu::GlyphCoverage& coverage(std::uint32_t glyphIndex, float emSize);
    private:
        struct GlyphSlot
        {
            Graphics::Cpu::GlyphCoverage glyph{};
            bool rasterized{ false };
        };
        using GlyphSlots = std::vector<GlyphSlot>;
        // Keyed on the em size in 1/64 pixel, which is the granularity FreeType is set to.
        using SlotsBySize = std::unordered_map<long, GlyphSlots>;

        void rasterize(GlyphSlot&, std::uint32_t glyphIndex, long emSize26);
    private:
        FT_Face m_ftFace{ nullptr };
        hb_blob_t* m_hbBlob{ nullptr };
        hb_face_t* m_hbFace{ nullptr };
        hb_font_t* m_hbFont{ nullptr };
        float m_unitsPerEm{ 0.0f };
        long m_currentSize26{ -1 };
        SlotsBySize m_slotsBySize{};
    };

    export class FontSet;

    // The faces fontconfig answers for one family at one weight and slant. See Platform
    export class FaceChain
    {
    public:
        FaceChain(FontSet&, FcFontSet* fonts);
        ~FaceChain();
        FaceChain(const FaceChain&) = delete;
        FaceChain& operator=(const FaceChain&) = delete;
    public:
        [[nodiscard]] FontFace& primary() const { return *m_primary; }
        // The first face of the chain with a glyph for the code point - the primary where it has
        // one - or null where none has, which leaves the primary's .notdef to stand for it.
        [[nodiscard]] FontFace* faceFor(char32_t codePoint);
    private:
        [[nodiscard]] FontFace& faceAt(int index);
    private:
        FontSet& m_set;
        FcFontSet* m_fonts;
        FontFace* m_primary{ nullptr };
        // One slot per font of the chain, null until that face is opened.
        std::vector<FontFace*> m_opened{};
    };

    // The faces of this machine, as fontconfig names them. See Platform
    export class FontSet
    {
    public:
        FontSet();
        ~FontSet();
        FontSet(const FontSet&) = delete;
        FontSet& operator=(const FontSet&) = delete;
    public:
        [[nodiscard]] FaceChain& chain(std::wstring_view family, FontWeight, FontStyle);
        // The face for a file fontconfig named, opened once and shared by every chain naming it.
        [[nodiscard]] FontFace& openFace(const std::string& path, int faceIndex);
    private:
        struct FaceRequest
        {
            std::wstring family;
            FontWeight weight;
            FontStyle style;

            auto operator<=>(const FaceRequest&) const = default;
        };
        struct FaceFile
        {
            std::string path;
            int index;

            auto operator<=>(const FaceFile&) const = default;
        };
        using FacesByFile = std::map<FaceFile, std::unique_ptr<FontFace>>;
        using ChainsByRequest = std::map<FaceRequest, std::unique_ptr<FaceChain>>;

        // fontconfig's faces for the request, best first. Never empty.
        [[nodiscard]] FcFontSet* sortFaces(const FaceRequest&) const;
    private:
        FT_Library m_library{ nullptr };
        FcConfig* m_config{ nullptr };
        FacesByFile m_facesByFile{};
        ChainsByRequest m_chainsByRequest{};
    };

    // The one set for the application, made on first use.
    export [[nodiscard]] FontSet& fontSet();
}
