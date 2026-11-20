export module ClaFi.Core.System.UiTypes :Color;

import ClaFi.StdLib;

namespace ClaFi
{
    export using ColorByte = std::uint8_t;
    export using ColorAsUint = std::uint32_t;
    export using Opacity = float; // 0.0f to 1.0f

    export struct Color
    {
    public:
        ColorByte blue;
        ColorByte green;
        ColorByte red;
        ColorByte alpha;
    public:
        constexpr Color(ColorByte red, ColorByte green, ColorByte blue, ColorByte alpha);
        constexpr Color(ColorByte red, ColorByte green, ColorByte blue);
        constexpr Color(ColorAsUint c);
        Color(Color c1, Color c2, float k2);
        constexpr Color();
        ~Color() = default;
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
        /// @brief Blends another color **over** this color, assuming this color is already opaque (Alpha = 255).
        /// @note This is extremely fast (zero divisions, zero branches in hot paths, processes channels in parallel).
        constexpr void paintOverOpaque(const Color& other) noexcept
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
        /// @brief Blends another color **over** this color, mathematically handling arbitrary alphas for both colors.
        /// @note Slightly slower than paint_over_opaque due to a dynamic integer division, but fully general.
        constexpr void paintOverGeneral(const Color& other) noexcept
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
        void clear() { blue = green = red = alpha = 0; }
        bool isOpaque() const { return alpha != 0; }
        ColorAsUint asUint() const { return std::bit_cast<const ColorAsUint>(*this); }
        Color fullyOpaque() const { return asUint() | 0xFF000000; }
        Color withOpacity(Opacity opacity) const {
            Color result = *this;
            result.alpha = static_cast<ColorByte>(255 * opacity);
            return result;
        }
        [[nodiscard]] Color faded(float share) const; // this color at a share of its own alpha
        inline constexpr void blend0256(Color to, int k2_fixed)
        {
            // Assumed k2_fixed is pre-validated (0 <= k2_fixed <= 256) by the caller
            const int k1_fixed = 256 - k2_fixed;

            // Use integer math and bit shifting (much faster than float operations)
#define BLEND_CHANNEL(channel) \
            this->channel = static_cast<ColorByte>( \
                (this->channel * k1_fixed + to.channel * k2_fixed) >> 8 \
            );

            BLEND_CHANNEL(blue);
            BLEND_CHANNEL(green);
            BLEND_CHANNEL(red);
            BLEND_CHANNEL(alpha);

#undef BLEND_CHANNEL
        }
        void constexpr blend(Color to, float share)
        {
            // It's not actually the float arithmetics we're fighting here, by using blend_fixed -
            // there's no speed difference between integer and float operations on any modern cpu's.
            // But integer to float and back conversions may be slow - and here we have only one
            return blend0256(to, static_cast<int>(share * 256.0f));

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
        static constexpr Color between(Color from, Color to, float share);
        void setOpacity(Opacity value) { alpha = static_cast<ColorByte>(255 * value); }
        float luminance() const;
        // The WCAG contrast ratio of two colors, 1 for two alike to 21 for black on white.
        static float contrastBetween(Color one, Color other);
    };

    float Color::luminance() const
    {
        const auto linear = [](ColorByte channel) {
            const float value = static_cast<float>(channel) / 255.0f;
            return value <= 0.03928f
                ? value / 12.92f
                : std::pow((value + 0.055f) / 1.055f, 2.4f);
            };
        return 0.2126f * linear(red) + 0.7152f * linear(green)
            + 0.0722f * linear(blue);
    }

    float Color::contrastBetween(const Color one, const Color other)
    {
        const float lighter = std::max(one.luminance(), other.luminance());
        const float darker = std::min(one.luminance(), other.luminance());
        return (lighter + 0.05f) / (darker + 0.05f);
    }

    export const Color k_noColor{};
    export const Color k_ColorFuchsia{ 0xFF'662366 };


    // A hue is held as a fraction of a full turn, and named in degrees wherever a person reads
    // or types one. These two are the whole of that conversion, so a hue kept as a bare float
    // converts the same way one inside an Hsl does.
    export constexpr float hueFromDegree(int value) { return static_cast<float>(value) / 360.0f; }
    export [[nodiscard]] constexpr int hueDegreeOf(float hue) { return static_cast<int>(hue * 360.0f); }

    // A color on three channels, each a fraction of its range, over Oklab. See UI-Types
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
        Hsl withLuminosity(float value) const { return { hue, saturation, value }; }
    public:
        float hue;
        float saturation;
        float luminosity;
    private:
        static void normalizeValue(float& value);
        static void clampValue(float& value);
    };

    // A color per key - a new key takes the next hue round the wheel, a known key the one it has.
    export template <typename Key>
    class ColorCodes
    {
    public:
        ColorCodes() = default;
        ColorCodes(Hsl firstColor, float hueStep);
    public:
        [[nodiscard]] Color colorOf(const Key&);   // given on the first ask, kept after
        void clear();
    private:
        using Colors = std::unordered_map<Key, Color>;
    private:
        Colors m_colors{};
        Hsl m_first{ 0.6f, 1.0f, 0.66f };
        Hsl m_next{ m_first };
        float m_hueStep{ 1.0f / 6.0f };
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

    Color constexpr Color::between(Color from, Color to, float share)
    {
        Color result = from;
        result.blend(to, share);
        return result;
    }


    // ColorCodes

    template <typename Key>
    ColorCodes<Key>::ColorCodes(const Hsl firstColor, const float hueStep)
        :
        m_first{ firstColor },
        m_next{ firstColor },
        m_hueStep{ hueStep }
    {
    }

    template <typename Key>
    Color ColorCodes<Key>::colorOf(const Key& key)
    {
        const auto found = m_colors.find(key);
        if (found != m_colors.end())
            return found->second;
        const Color color = m_next.toColor();
        m_colors.emplace(key, color);
        m_next.offsetHue(m_hueStep);
        return color;
    }

    template <typename Key>
    void ColorCodes<Key>::clear()
    {
        m_colors.clear();
        m_next = m_first;
    }
}
