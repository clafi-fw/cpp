module;
#include <immintrin.h>
export module ClaFi.Core.System.UiTypes :Color;

import ClaFi.StdLib;

namespace ClaFi
{
    export using ColorByte = std::uint8_t;
    export using ColorAsUint = std::uint32_t;
    export using Opacity = float; // 0.0f to 1.0f

    // The order the channels are written in a hex colour.
    export enum class HexFormat {
        RGBA,  // Web standard (#RRGGBBAA)
        ARGB   // Microsoft/Android standard (#AARRGGBB)
    };

    export struct Color
    {
    public:
        ColorByte blue;
        ColorByte green;
        ColorByte red;
        ColorByte alpha;
    public:
        /// @brief Blends another color OVER this color, assuming this color is already opaque (Alpha = 255).
        /// @note This is extremely fast (zero divisions, zero branches in hot paths, processes channels in parallel).
        constexpr void paint_over_opaque(const Color& other) noexcept
        {
            const std::uint32_t a = other.alpha;
            if (a == 0) [[unlikely]] return; // Transparent source is a no-op
            if (a == 255) [[unlikely]] {
                *this = other; // Fully opaque source overwrites destination
                return;
            }

            const std::uint32_t inv_a = 255 - a;

            // Reinterpret the 4-byte structs as 32-bit integers at zero cost
            const std::uint32_t src_u32 = std::bit_cast<std::uint32_t>(other);
            const std::uint32_t dst_u32 = std::bit_cast<std::uint32_t>(*this);

            // --- SWAR: Blend Red and Blue channels in parallel ---
            // Masks out Green and Alpha to prevent overflow: 0x00RR00BB
            const std::uint32_t src_rb = src_u32 & 0x00FF00FF;
            const std::uint32_t dst_rb = dst_u32 & 0x00FF00FF;

            std::uint32_t blended_rb = src_rb * a + dst_rb * inv_a;
            // Fast division by 255 for packed channels: (val + 128) / 255
            blended_rb = (blended_rb + 0x00800080);
            blended_rb = (blended_rb + ((blended_rb >> 8) & 0x00FF00FF)) >> 8;
            blended_rb &= 0x00FF00FF; // Clear division remainders from the gap

            // --- Scalar: Blend Green channel ---
            const std::uint32_t src_g = (src_u32 >> 8) & 0x000000FF;
            const std::uint32_t dst_g = (dst_u32 >> 8) & 0x000000FF;

            std::uint32_t blended_g = src_g * a + dst_g * inv_a;
            blended_g = (blended_g + 128);
            blended_g = (blended_g + (blended_g >> 8)) >> 8;

            // Pack channels back together, keeping Alpha at 255 (since destination is opaque)
            const std::uint32_t result = blended_rb | (blended_g << 8) | 0xFF000000;

            *this = std::bit_cast<Color>(result);
        }

        /// @brief Blends another color OVER this color, mathematically handling arbitrary alphas for both colors.
        /// @note Slightly slower than paint_over_opaque due to a dynamic integer division, but fully general.
        constexpr void paint_over_general(const Color& other) noexcept
        {
            const std::uint32_t a_src = other.alpha;
            if (a_src == 0) [[unlikely]] return;

            const std::uint32_t a_dst = this->alpha;
            if (a_src == 255 || a_dst == 0) [[unlikely]] {
                *this = other;
                // If the source is opaque or destination is fully transparent, simply copy
                return;
            }

            const std::uint32_t inv_a_src = 255 - a_src;

            // Compute pre-multiplied weight factors
            const std::uint32_t w_src = a_src * 255;
            const std::uint32_t w_dst = a_dst * inv_a_src;
            const std::uint32_t w_sum = w_src + w_dst; // Max value: 65,025

            // Calculate the resulting alpha
            const std::uint32_t a_out = (w_sum + 127) / 255;

            // Standard Porter-Duff "Over" operator using 1 dynamic division instead of 3
            this->red = static_cast<ColorByte>((other.red * w_src + this->red * w_dst + w_sum / 2) / w_sum);
            this->green = static_cast<ColorByte>((other.green * w_src + this->green * w_dst + w_sum / 2) / w_sum);
            this->blue = static_cast<ColorByte>((other.blue * w_src + this->blue * w_dst + w_sum / 2) / w_sum);
            this->alpha = static_cast<ColorByte>(a_out);
        }
    public:
        constexpr Color(ColorByte red, ColorByte green, ColorByte blue, ColorByte alpha);
        constexpr Color(ColorByte red, ColorByte green, ColorByte blue);
        constexpr Color(ColorAsUint c);
        Color(Color c1, Color c2, float k2);
        constexpr Color();
        ~Color() = default;
    public:
        // from/to hexStr
        constexpr Color(std::wstring_view, HexFormat = HexFormat::RGBA);
        std::wstring toStr(HexFormat format = HexFormat::RGBA) const;
    public:
        bool operator == (const Color& second) const
        {
            // return ((b== second.b) && (g == second.g) && (r == second.r));
            return asUint() == second.asUint();
        }
        bool operator!=(const Color& second) const
        {
            return asUint() != second.asUint();
        }
    public:
        void clear() { blue = green = red = alpha = 0; }
        bool isOpaque() const { return alpha != 0; }
        ColorAsUint asUint() const { return std::bit_cast<const ColorAsUint>(*this); }
        Color fullyOpaque() const { return asUint() | 0xFF000000; }
        Color withOpacity(Opacity opacity) const {
            Color result = *this;
            result.alpha = static_cast<ColorByte>(255 * opacity);
            return result;
        }
        //ColorAsUint toGdiColor() const {
        //  return static_cast<ColorAsUint>((red | (static_cast<uint16_t>(green) << 8)) | (static_cast<uint16_t>(blue) << 16));
        //}
        void blendRgb(Color second, const float k2)
        {
            for (int offset = 0; offset < 3; ++offset)
                mixByte(reinterpret_cast<ColorByte*>(this)[offset], (&reinterpret_cast<const ColorByte&>(second))[offset], k2);
        }

        inline constexpr void blend_fixed(Color second, int k2_fixed)
        {
            // Assumed k2_fixed is pre-validated (0 <= k2_fixed <= 256) by the caller
            const int k1_fixed = 256 - k2_fixed;

            // Use integer math and bit shifting (much faster than float operations)
#define BLEND_CHANNEL(channel) \
            this->channel = static_cast<ColorByte>( \
                (this->channel * k1_fixed + second.channel * k2_fixed) >> 8 \
            );

            BLEND_CHANNEL(blue);
            BLEND_CHANNEL(green);
            BLEND_CHANNEL(red);
            BLEND_CHANNEL(alpha);

#undef BLEND_CHANNEL
        }

        void constexpr blend(Color second, float k2)
        {
            // upd: it's not actually the float ariphmetics we fighting here,
            // by using blend_fixed (there's no performance difference between integer and float calculations on modern cpu's).
            // But integer to float and back conversions required for float calculations, they can be slow
            return blend_fixed(second, static_cast<int>(k2 * 256.0f));

            //ColorByte* p = &b; // blue byte (first)
            //const ColorByte* p2 = &second.b;
            //ColorByte* end = std::next(&q); // last byte
            //do
            //{
            //  *p += static_cast<ColorByte>(k2 * (*p2 - *p));
            //  ++p;
            //  ++p2;
            //} while (p != end);
        }

        static constexpr Color blend(Color c1, Color c2, float k2);

        // SIMD 8-pixel blend
        static inline __m256i blend8(__m256i dest, __m256i src, __m256i alpha) {
            const __m256i rb_mask = _mm256_set1_epi32(0x00FF00FF);
            const __m256i ga_mask = _mm256_set1_epi32(0xFF00FF00);

            // 1. Prepare differences for both passes at once
            __m256i d_rb = _mm256_and_si256(dest, rb_mask);
            __m256i s_rb = _mm256_and_si256(src, rb_mask);
            __m256i diff_rb = _mm256_sub_epi32(s_rb, d_rb);

            __m256i d_ga = _mm256_and_si256(_mm256_srli_epi32(dest, 8), rb_mask);
            __m256i s_ga = _mm256_and_si256(_mm256_srli_epi32(src, 8), rb_mask);
            __m256i diff_ga = _mm256_sub_epi32(s_ga, d_ga);

            // 2. Fire both multiplications
            // Modern CPUs can often pipeline these two independent mullo instructions
            __m256i prod_rb = _mm256_mullo_epi32(diff_rb, alpha);
            __m256i prod_ga = _mm256_mullo_epi32(diff_ga, alpha);

            // 3. Shift and reconstruct
            __m256i res_rb = _mm256_and_si256(_mm256_add_epi32(d_rb, _mm256_srai_epi32(prod_rb, 8)), rb_mask);
            __m256i res_ga = _mm256_slli_epi32(_mm256_add_epi32(d_ga, _mm256_srai_epi32(prod_ga, 8)), 8);

            // Blend using the ga_mask to finalize the combine
            // This is often faster than a separate final mask + OR
            return _mm256_blendv_epi8(res_rb, res_ga, ga_mask);
        }

        void setOpacity(Opacity value) { alpha = static_cast<ColorByte>(255 * value); }

    private:
        inline static void mixByte(ColorByte& v1, const ColorByte v2, float k2)
        {
            v1 += static_cast<ColorByte>(k2 * (v2 - v1));
        }
    };

    export const Color k_noColor{};
    export const Color k_ColorFuchsia{ 0xFF'662366 };


    // A hue is held as a fraction of a full turn, and named in degrees wherever a person reads
    // or types one. These two are the whole of that conversion, so a hue kept as a bare float
    // converts the same way one inside an Hsl does.
    export constexpr float hueFromDegree(int value) { return static_cast<float>(value) / 360.0f; }
    export [[nodiscard]] constexpr int hueDegreeOf(float hue) { return static_cast<int>(hue * 360.0f); }

    // A colour on three channels, each a fraction of its range, over Oklab. See UI-Types
    export struct Hsl
    {
    public:
        Hsl() = default;
        Hsl(float hue, float saturation, float luminosity);
        Hsl(int hueDegree, int saturationPercent, int lumPercent);
        Hsl(Hsl color1, Hsl color2, float factor);
        explicit Hsl(const Color&);
        Color toColor() const;
        void offsetHue(float value);
        void scaleSaturation(float k);
        void scaleLuminosity(float k);
        int hueDegree() const { return hueDegreeOf(hue); }
        void setHueDegree(int value) { hue = hueFromDegree(value); }
    public:
        float hue;
        float saturation;
        float luminosity;
    private:
        static void normalizeValue(float& value);
        static void clampValue(float& value);
    };


    //----------------------------------------------------------------------------


    // Color

    constexpr Color::Color(ColorByte red, ColorByte green, ColorByte blue, ColorByte alpha)
        :
        red{ red },
        green{ green },
        blue{ blue },
        alpha{ alpha }
    {
    }

    constexpr Color::Color(ColorByte red, ColorByte green, ColorByte blue)
        :
        Color{ red, green, blue, 255u }
    {
    }

    constexpr Color::Color(ColorAsUint c)
        :
        blue{ static_cast<ColorByte>(c & 0xFFu) },
        green{ static_cast<ColorByte>((c >> 8) & 0xFFu) },
        red{ static_cast<ColorByte>((c >> 16) & 0xFFu) },
        alpha{ static_cast<ColorByte>((c >> 24) & 0xFFu) }
    {
    }

    constexpr Color::Color()
        :
        Color{ 0u, 0u, 0u, 0u }
    {
    }

    constexpr Color::Color(std::wstring_view hexStr, HexFormat format)
        :
        Color{ 0, 0, 0, 0 }
    {
        if (hexStr.empty())
            return;

        const std::size_t start = (hexStr[0] == L'#') ? 1 : 0;
        std::uint32_t val = 0;

        for (std::size_t i = start; i < hexStr.size(); ++i) {
            const wchar_t ch = hexStr[i];
            std::uint32_t digit;
            if (ch >= L'0' && ch <= L'9')
                digit = ch - L'0';
            else if (ch >= L'a' && ch <= L'f')
                digit = ch - L'a' + 10;
            else if (ch >= L'A' && ch <= L'F')
                digit = ch - L'A' + 10;
            else
                return; // Invalid character fallback

            val = (val << 4) | digit;
        }

        std::size_t len = hexStr.size() - start;
        if (len == 6) {
            alpha = 255;
            red = (val >> 16) & 0xFF;
            green = (val >> 8) & 0xFF;
            blue = val & 0xFF;
        }
        else if (len == 8) {
            if (format == HexFormat::RGBA) {
                red = (val >> 24) & 0xFF;
                green = (val >> 16) & 0xFF;
                blue = (val >> 8) & 0xFF;
                alpha = val & 0xFF;
            }
            else { // HexFormat::ARGB
                alpha = (val >> 24) & 0xFF;
                red = (val >> 16) & 0xFF;
                green = (val >> 8) & 0xFF;
                blue = val & 0xFF;
            }
        }
    }

    Color constexpr Color::blend(Color c1, Color c2, float k2)
    {
        Color result = c1;
        result.blend(c2, k2);
        return result;
    }


    // Color

}
