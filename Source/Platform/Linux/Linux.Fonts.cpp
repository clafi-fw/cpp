module;
#include "Linux.TextHeaders.h"
module ClaFi.Platform.Linux.Fonts;

import ClaFi.Core.Graphics.Cpu_GlyphCompositor;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Platform::Linux
{
    namespace
    {
        // Grid fitting in y only, the way DirectWrite's natural modes fit: horizontal stems land on
        // the pixel grid and nothing moves sideways, which is what lets the compositor place a
        // glyph at a fractional x by sampling. Embedded bitmaps are refused because a face
        // carrying them would answer some sizes from strikes and others from outlines.
        constexpr FT_Int32 k_loadFlags = FT_LOAD_TARGET_LIGHT | FT_LOAD_NO_BITMAP;

        // FreeType answers linear coverage, and taken as it stands a run carries about 9% more ink
        // than DirectWrite lays down for the same glyphs. The curve is applied once, where the bytes
        // become floats: 1.0 is linear, a value above it thickens and one below thins.
        //
        // 0.6 levels the two to within half a percent. Measured by summing luminance coverage over
        // a line of body text in Noto Sans 2.015 at a 28px em, against the same showcase built for
        // Windows on the CPU backend: the same compositor on both sides, so what is left differing
        // is the glyph rasterizer. Both at 200%, the family named outright so neither resolved a
        // generic to a face of its own. Against that figure 0.6 is +0.5%, 0.7 is +3.4%, 0.8 is
        // +5.9%, 1.0 is +10.2%.
        //
        // The Direct2D backend is the wrong thing to fit against. It draws a run with subpixel
        // antialiasing - the red and blue of an edge pixel differing by as much as 150, where either
        // CPU backend answers grayscale and differs by 12 - and carries 1.4% more ink for it. Close
        // enough that a curve fitted to it lands near this one, and not the same measurement.
        //
        // MEASURED AGAINST ONE FACE. What a generic family resolves to is machine by machine, and a
        // heavier face moves more ink than the whole usable range of this curve does - so a machine
        // whose sans-serif is not Noto Sans is not covered by this number.
        constexpr float k_coverageGamma = 0.6f;

        constexpr float k_coverageScale = 1.0f / 255.0f;

        // AN IDENTITY CURVE IS A MULTIPLY, AND ANY OTHER IS A TABLE. This is read once per pixel of
        // every glyph rasterized, and where the curve does nothing a lookup buys nothing: a byte
        // scaled by 1/255 is an instruction the vectoriser takes, and a gather is not.
        //
        // SCALED AND NOT DIVIDED. The reciprocal is not exact, so a coverage can land one 8-bit
        // level from what dividing by 255 answers - which is under a quarter of a percent of the
        // pixels of a glyph, and invisible. Dividing to close that gap costs the vectorised
        // multiply, because 1/255 being inexact is the same reason the compiler will not fold it.
        constexpr bool k_coverageIsLinear = k_coverageGamma == 1.0f;

        using CoverageTable = std::array<float, 256>;

        CoverageTable makeCoverageTable()
        {
            CoverageTable table{};
            for (std::size_t i = 0; i != table.size(); ++i)
                table[i] = std::pow(static_cast<float>(i) * k_coverageScale, 1.0f / k_coverageGamma);
            return table;
        }

        // Function-local, so a build whose curve is identity never reaches the initializer and
        // never pays for the table - neither the pow over 256 entries nor the kilobyte it holds.
        const CoverageTable& coverageTable()
        {
            static const CoverageTable table = makeCoverageTable();
            return table;
        }

        // fontconfig's weight scale is its own; this is the conversion it publishes for an
        // OpenType weight, which is what FontWeight's values are.
        int fontconfigWeight(FontWeight weight)
        {
            return FcWeightFromOpenType(static_cast<int>(weight));
        }

        int fontconfigSlant(FontStyle style)
        {
            return style == FontStyle::Italic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN;
        }
    }

    //-------------------------------------------------------------------------


    FontSet& fontSet()
    {
        static FontSet s_fontSet{};
        return s_fontSet;
    }


    //-------------------------------------------------------------------------


    // FontFace

    FontFace::FontFace(FT_Library library, const std::string& path, int faceIndex)
    {
        if (FT_New_Face(library, path.c_str(), faceIndex, &m_ftFace) != 0)
            unreachable("FreeType cannot open the face fontconfig matched: " + path);

        m_unitsPerEm = static_cast<float>(m_ftFace->units_per_EM);

        // The low sixteen bits name the face inside a collection; the high bits, which FreeType
        // reads as a named instance of a variable face, are not carried to HarfBuzz, so a variable
        // face shapes at its default coordinates.
        m_hbBlob = hb_blob_create_from_file(path.c_str());
        m_hbFace = hb_face_create(m_hbBlob, static_cast<unsigned int>(faceIndex & 0xffff));
        m_hbFont = hb_font_create(m_hbFace);
        hb_font_set_scale(m_hbFont, m_ftFace->units_per_EM, m_ftFace->units_per_EM);
    }

    FontFace::~FontFace()
    {
        hb_font_destroy(m_hbFont);
        hb_face_destroy(m_hbFace);
        hb_blob_destroy(m_hbBlob);
        FT_Done_Face(m_ftFace);
    }

    FaceMetrics FontFace::metrics(float emSize) const
    {
        const float scale = emSize / m_unitsPerEm;
        const float ascender = static_cast<float>(m_ftFace->ascender);
        const float descender = static_cast<float>(m_ftFace->descender);
        const float height = static_cast<float>(m_ftFace->height);
        return {
            ascender * scale,
            -descender * scale,
            (height - (ascender - descender)) * scale,
        };
    }

    bool FontFace::isFixedWidth() const
    {
        return FT_IS_FIXED_WIDTH(m_ftFace) != 0;
    }

    bool FontFace::hasGlyph(char32_t codePoint) const
    {
        return glyphIndex(codePoint) != 0;
    }

    std::uint32_t FontFace::glyphIndex(char32_t codePoint) const
    {
        return FT_Get_Char_Index(m_ftFace, codePoint);
    }

    float FontFace::glyphAdvance(std::uint32_t glyph, float emSize) const
    {
        // The hb_font is scaled to the face's units per em, so this is in design units - and
        // the advance HarfBuzz shapes a glyph of this face with, before any positioning.
        const hb_position_t advance = hb_font_get_glyph_h_advance(m_hbFont, glyph);
        return static_cast<float>(advance) * emSize / m_unitsPerEm;
    }

    float FontFace::advance(char32_t codePoint, float emSize) const
    {
        return glyphAdvance(glyphIndex(codePoint), emSize);
    }

    const Graphics::Cpu::GlyphCoverage& FontFace::coverage(std::uint32_t glyphIndex, float emSize)
    {
        const long emSize26 = std::lround(emSize * 64.0f);
        GlyphSlots& slots = m_slotsBySize[emSize26];
        if (slots.empty())
            slots.resize(static_cast<std::size_t>(m_ftFace->num_glyphs));

        GlyphSlot& slot = slots[glyphIndex];
        if (!slot.rasterized)
            rasterize(slot, glyphIndex, emSize26);
        return slot.glyph;
    }

    void FontFace::rasterize(GlyphSlot& slot, std::uint32_t glyphIndex, long emSize26)
    {
        // Recorded first, so a glyph FreeType refuses is a coverage with no ink rather than one
        // asked for again on every run.
        slot.rasterized = true;

        if (m_currentSize26 != emSize26)
        {
            // 72 dots per inch makes a point a pixel, so the 26.6 size is the em in pixels.
            if (FT_Set_Char_Size(m_ftFace, 0, emSize26, 72, 72) != 0)
                return;
            m_currentSize26 = emSize26;
        }

        if (FT_Load_Glyph(m_ftFace, glyphIndex, k_loadFlags) != 0)
            return;
        if (FT_Render_Glyph(m_ftFace->glyph, FT_RENDER_MODE_NORMAL) != 0)
            return;

        const FT_GlyphSlot rendered = m_ftFace->glyph;
        const FT_Bitmap& bitmap = rendered->bitmap;
        Graphics::Cpu::GlyphCoverage& glyph = slot.glyph;

        // bitmap_top is the distance from the baseline up to the first row, and y grows downwards.
        glyph.left = rendered->bitmap_left;
        glyph.top = -rendered->bitmap_top;

        if (bitmap.width == 0 || bitmap.rows == 0 || bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
            return;

        glyph.allocate(static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows));
        for (unsigned int y = 0; y != bitmap.rows; ++y)
        {
            // The pitch is signed: it is what steps one row down whichever way the rows are laid
            // out in memory.
            const unsigned char* src = bitmap.buffer + static_cast<std::ptrdiff_t>(y) * bitmap.pitch;
            float* dst = glyph.row(static_cast<int>(y));
            for (unsigned int x = 0; x != bitmap.width; ++x)
            {
                if constexpr (k_coverageIsLinear)
                    dst[x] = static_cast<float>(src[x]) * k_coverageScale;
                else
                    dst[x] = coverageTable()[src[x]];
            }
        }
    }


    //-------------------------------------------------------------------------


    // FaceChain

    FaceChain::FaceChain(FontSet& set, FcFontSet* fonts)
        :
        m_set{ set },
        m_fonts{ fonts }
    {
        m_opened.resize(static_cast<std::size_t>(fonts->nfont), nullptr);
        m_primary = &faceAt(0);
    }

    FaceChain::~FaceChain()
    {
        FcFontSetDestroy(m_fonts);
    }

    FontFace* FaceChain::faceFor(char32_t codePoint)
    {
        if (m_primary->hasGlyph(codePoint))
            return m_primary;

        // The coverage fontconfig recorded says which faces are worth opening; the face itself has
        // the last word, its cmap being what a glyph is looked up in.
        for (int i = 1; i < m_fonts->nfont; ++i)
        {
            FcCharSet* coverage = nullptr;
            if (FcPatternGetCharSet(m_fonts->fonts[i], FC_CHARSET, 0, &coverage) != FcResultMatch)
                continue;
            if (!FcCharSetHasChar(coverage, codePoint))
                continue;
            FontFace& face = faceAt(i);
            if (face.hasGlyph(codePoint))
                return &face;
        }
        return nullptr;
    }

    FontFace& FaceChain::faceAt(int index)
    {
        FontFace*& slot = m_opened[static_cast<std::size_t>(index)];
        if (!slot)
        {
            FcPattern* font = m_fonts->fonts[index];
            FcChar8* path = nullptr;
            int faceIndex = 0;
            if (FcPatternGetString(font, FC_FILE, 0, &path) != FcResultMatch)
                unreachable("fontconfig named a face with no file");
            FcPatternGetInteger(font, FC_INDEX, 0, &faceIndex);
            slot = &m_set.openFace(reinterpret_cast<const char*>(path), faceIndex);
        }
        return *slot;
    }


    //-------------------------------------------------------------------------


    // FontSet

    FontSet::FontSet()
    {
        if (FT_Init_FreeType(&m_library) != 0)
            unreachable("FreeType failed to initialize");
        m_config = FcInitLoadConfigAndFonts();
        if (!m_config)
            unreachable("fontconfig failed to load its configuration");
    }

    FontSet::~FontSet()
    {
        m_chainsByRequest.clear();
        m_facesByFile.clear();
        FcConfigDestroy(m_config);
        FT_Done_FreeType(m_library);
    }

    FaceChain& FontSet::chain(std::wstring_view family, FontWeight weight, FontStyle style)
    {
        FaceRequest request = { std::wstring{ family }, weight, style };
        auto known = m_chainsByRequest.find(request);
        if (known != m_chainsByRequest.end())
            return *known->second;

        FcFontSet* fonts = sortFaces(request);
        std::unique_ptr<FaceChain>& chain = m_chainsByRequest[std::move(request)];
        chain = std::make_unique<FaceChain>(*this, fonts);
        return *chain;
    }

    FontFace& FontSet::openFace(const std::string& path, int faceIndex)
    {
        std::unique_ptr<FontFace>& face = m_facesByFile[FaceFile{ path, faceIndex }];
        if (!face)
            face = std::make_unique<FontFace>(m_library, path, faceIndex);
        return *face;
    }

    FcFontSet* FontSet::sortFaces(const FaceRequest& request) const
    {
        FcPattern* pattern = FcPatternCreate();
        const std::string family = toUtf8(request.family);
        FcPatternAddString(pattern, FC_FAMILY, reinterpret_cast<const FcChar8*>(family.c_str()));
        FcPatternAddInteger(pattern, FC_WEIGHT, fontconfigWeight(request.weight));
        FcPatternAddInteger(pattern, FC_SLANT, fontconfigSlant(request.style));
        // Outlines only. A bitmap face has its sizes and no others, and the rasterizer refuses
        // embedded bitmaps for the same reason. Colour faces are bitmaps too, so they stand last.
        FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);
        FcPatternAddBool(pattern, FC_COLOR, FcFalse);
        FcConfigSubstitute(m_config, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);

        // Trimmed: a face adding no character the faces before it lack is left out.
        FcResult result = FcResultNoMatch;
        FcFontSet* fonts = FcFontSort(m_config, pattern, FcTrue, nullptr, &result);
        FcPatternDestroy(pattern);
        if (!fonts || fonts->nfont == 0)
        {
            if (fonts)
                FcFontSetDestroy(fonts);
            unreachable("fontconfig has no face for " + family);
        }
        return fonts;
    }
}
