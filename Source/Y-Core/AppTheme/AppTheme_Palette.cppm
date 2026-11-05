export module ClaFi.Core.AppTheme_Palette;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;
import ClaFi.Core.System.UiTypes;

namespace ClaFi
{
    export class PaletteColor
    {
    public:
        explicit PaletteColor(Hsl hsl);
        //
        Hsl hsl() const { return m_hsl; }
        Color rgb() const { return m_rgb; }
        float hue() const { return m_hsl.hue; }
        int hueDegree() const { return m_hsl.hueDegree(); }
    private:
        void updateRgb();
        Hsl m_hsl;
        Color m_rgb;
    };

    export using PaletteColors = std::vector<PaletteColor>;

    export using PaletteMap = std::array<std::size_t, 3ull>;

    // How the palette's further hues are placed against the first.
    export enum class ColorHarmonyKind : std::size_t {
        First = 0,
        Monochromatic = 0,
        TwoAnalogous,
        Complementary,
        ThreeAnalogous,
        Triadic,
        SplitComplementary,
        Count
    };

    export ColorHarmonyKind& operator++(ColorHarmonyKind& value)
    {
        std::size_t& i = reinterpret_cast<std::size_t&>(value);
        return reinterpret_cast<ColorHarmonyKind&>(++i);
    }
    export constexpr ColorHarmonyKind k_defaultHarmonyKind = ColorHarmonyKind::Monochromatic;

    export constexpr std::array<std::wstring_view, static_cast<std::size_t>(ColorHarmonyKind::Count)> k_colorHarmonyTitles{
        L"Monochromatic",
        L"Two analogous colors",
        L"Complementary",
        L"Three analogous colors",
        L"Triadic",
        L"Split complementary"
    };

    export constexpr std::array<std::wstring_view, static_cast<std::size_t>(ColorHarmonyKind::Count)> k_harmonyKeys{
        L"Monochromatic",
        L"TwoAnalogous",
        L"Complementary",
        L"ThreeAnalogous",
        L"Triadic",
        L"SplitComplementary"
    };

    constexpr std::array<std::size_t, static_cast<std::size_t>(ColorHarmonyKind::Count)> k_colorHarmonyColorsCount{
        1ull,
        2ull,
        2ull,
        3ull,
        3ull,
        3ull
    };

    // Where each pigment's name sits, in Pigment order: the hue that still reads as that name at
    // the luminosities pigments are painted at. It is the middle of the colour, not the sRGB corner
    // of it - sRGB's blue carries hue 264, which paints a periwinkle.
    constexpr std::array<int, k_pigmentsCount> k_pigmentReferenceDegrees{
        90,
        148,
        245,
        25,
        305,
        57
    };

    // The theme's grid: twelve hues this far apart, the first at the anchor. Every harmony colour
    // stands on it, and each pigment is drawn at the grid hue nearest its reference.
    constexpr int k_pigmentGridDegrees = 30;

    // Whether every two references stand more than a grid step apart, which keeps two pigments off
    // one grid hue.
    constexpr bool pigmentReferenceGapsExceedGridStep()
    {
        for (std::size_t pigment = 0ull; pigment != k_pigmentsCount; ++pigment)
        {
            for (std::size_t other = 0ull; other != k_pigmentsCount; ++other)
            {
                const int from = k_pigmentReferenceDegrees[pigment];
                const int to = k_pigmentReferenceDegrees[other];
                if (other != pigment && (to - from + 360) % 360 <= k_pigmentGridDegrees)
                    return false;
            }
        }
        return true;
    }

    static_assert(pigmentReferenceGapsExceedGridStep());

    // A hue wrapped into the half open turn a hue is held in.
    [[nodiscard]] float wrappedHue(float value);

    // Where a hue stands relative to another, going the shorter way round: negative below it,
    // positive above.
    [[nodiscard]] float hueOffset(float hue, float reference);

    // A palette entry shown as a colour. An entry is a hue and nothing else, so anything that
    // draws one has to be given the other two from somewhere, and this is that somewhere. The pair
    // is the same in every theme, so a swatch shows the hue itself rather than how that hue
    // happens to land in the theme being looked at.
    //
    // Nothing resolved is drawn from these: BakedColors::pigmentHsl takes the hue of a palette
    // colour and states the other two itself.
    export constexpr float k_paletteDisplaySaturation = 0.75f;
    export constexpr float k_paletteDisplayLuminosity = 0.66f;

    export [[nodiscard]] Hsl paletteDisplayColor(float hue);

    export class ColorHarmony
    {
    public:
        ColorHarmony(float& anchorHue, ColorHarmonyKind);
        virtual ~ColorHarmony() = default;
        static constexpr std::size_t noHue = static_cast<std::size_t>(-1);
    public:
        ColorHarmonyKind kind() const { return m_kind; }
        const std::wstring_view& name() const { return m_name; }
        void anchorChanged();
        std::vector<PaletteMap>& maps() { return m_maps; }
        void addMap(const PaletteMap&);
        void selectMap(PaletteMap&);
        PaletteMap* findMapByHueDegrees(std::array<int, 3ull> hues);
        PaletteMap* selectedMap() const { return m_selectedMap; }
        const PaletteColor& color(std::size_t index) const;
        std::size_t colorsNum() const { return m_colors.size(); }
        bool useColorOnBg() const { return (*m_selectedMap)[0] != noHue; }
        // The pigments, each at the hue of the anchor's grid nearest its reference.
        [[nodiscard]] const PaletteColors& pigmentColors() const { return m_pigmentColors; }
        [[nodiscard]] const PaletteColor& pigmentColor(Pigment pigment) const { return m_pigmentColors[static_cast<std::size_t>(pigment)]; }
    protected:
        PaletteColors& colors() { return m_colors; }
        // Places the pigments on the anchor's grid; an anchorChanged override ends by calling it.
        void fillPigmentColors();
    protected:
        PaletteColor m_gray{ { 0.0f, 0.0f, 0.5f } };
    protected:
        // The hue every colour here is turned from. Held by reference so that a drag moving it
        // reaches this without anything having to be handed back in.
        float& m_anchor;
        ColorHarmonyKind m_kind;
        const std::wstring_view& m_name;
        PaletteColors m_colors{};
        PaletteColors m_pigmentColors{};
        PaletteMap* m_selectedMap;
        std::vector<PaletteMap> m_maps;
    };

    export class ColorHarmonySelector
    {
    public:
        ColorHarmonySelector(float& anchorHue);
        void anchorChanged();
    public:
        std::size_t count();
        ColorHarmony& harmony(std::size_t);
    private:
        float& m_anchorHue;
        std::array<std::unique_ptr<ColorHarmony>, static_cast<std::size_t>(ColorHarmonyKind::Count)> m_harmonies{};
    };


    //-------------------------------------------------------------------------


    // PaletteColor

    PaletteColor::PaletteColor(Hsl hsl)
        :
        m_hsl{ hsl }
    {
        updateRgb();
    }

    void PaletteColor::updateRgb()
    {
        m_rgb = m_hsl.toColor();
    }

    float wrappedHue(float value)
    {
        float hue = value - std::floor(value);

        // A value a hair below zero floors a whole turn back, and the sum rounds up to a whole
        // turn from there. A whole turn is the hue the wheel starts at, and the range is half open.
        if (hue >= 1.0f)
            hue = 0.0f;
        return hue;
    }

    float hueOffset(float hue, float reference)
    {
        float offset = wrappedHue(hue - reference);
        return offset > 0.5f ? offset - 1.0f : offset;
    }

    Hsl paletteDisplayColor(float hue)
    {
        return { hue, k_paletteDisplaySaturation, k_paletteDisplayLuminosity };
    }

    // ColorHarmony

    ColorHarmony::ColorHarmony(float& anchorHue, ColorHarmonyKind kind)
        :
        m_anchor{ anchorHue },
        m_kind{ kind },
        m_name{ k_colorHarmonyTitles[static_cast<std::size_t>(kind)] }
    {
        m_colors.reserve(k_colorHarmonyColorsCount[static_cast<std::size_t>(kind)]);
        switch (m_kind)
        {
        case ColorHarmonyKind::Monochromatic:
            addMap({ 0ull, 0ull, 0ull });
            break;
        case ColorHarmonyKind::TwoAnalogous:
            addMap({ 0ull, 1ull, 1ull });
            addMap({ 0ull, 0ull, 1ull });
            addMap({ 1ull, 0ull, 0ull });
            addMap({ 1ull, 1ull, 0ull });
            addMap({ 1ull, 0ull, 1ull });
            addMap({ 0ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::Complementary:
            addMap({ 0ull, 0ull, 1ull });
            addMap({ 0ull, 1ull, 1ull });
            addMap({ 1ull, 0ull, 0ull });
            addMap({ 1ull, 1ull, 0ull });
            addMap({ 1ull, 0ull, 1ull });
            addMap({ 0ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::ThreeAnalogous:
            addMap({ 0ull, 1ull, 2ull });
            addMap({ 0ull, 2ull, 1ull });
            addMap({ 1ull, 0ull, 2ull });
            addMap({ 1ull, 2ull, 0ull });
            addMap({ 2ull, 0ull, 1ull });
            addMap({ 2ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::Triadic:
            addMap({ 0ull, 1ull, 2ull });
            addMap({ 0ull, 2ull, 1ull });
            addMap({ 1ull, 0ull, 2ull });
            addMap({ 1ull, 2ull, 0ull });
            addMap({ 2ull, 0ull, 1ull });
            addMap({ 2ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::SplitComplementary:
            addMap({ 0ull, 1ull, 2ull });
            addMap({ 0ull, 2ull, 1ull });
            addMap({ 1ull, 0ull, 2ull });
            addMap({ 1ull, 2ull, 0ull });
            addMap({ 2ull, 0ull, 1ull });
            addMap({ 2ull, 1ull, 0ull });
            break;
        case ColorHarmonyKind::Count:
            break;
        }
        anchorChanged();
    }

    void ColorHarmony::anchorChanged()
    {
        m_colors.clear();
        m_colors.emplace_back(paletteDisplayColor(m_anchor));
        switch (m_kind)
        {
        case ColorHarmonyKind::Monochromatic:
            break;
        case ColorHarmonyKind::TwoAnalogous:
        {
            Hsl secondColor = paletteDisplayColor(m_anchor);
            secondColor.offsetHue(1.0f / 12.0f);
            m_colors.emplace_back(secondColor);
            break;
        }
        case ColorHarmonyKind::Complementary:
        {
            Hsl secondColor = paletteDisplayColor(m_anchor);
            secondColor.offsetHue(0.5f);
            m_colors.emplace_back(secondColor);
            break;
        }
        case ColorHarmonyKind::ThreeAnalogous:
        {
            Hsl otherColor = paletteDisplayColor(m_anchor);
            otherColor.offsetHue(1.0f / 12.0f);
            m_colors.emplace_back(otherColor);
            otherColor.offsetHue(1.0f / 12.0f);
            m_colors.emplace_back(otherColor);
            break;
        }
        case ColorHarmonyKind::Triadic:
        {
            Hsl otherColor = paletteDisplayColor(m_anchor);
            otherColor.offsetHue(1.0f / 3.0f);
            m_colors.emplace_back(otherColor);
            otherColor.offsetHue(1.0f / 3.0f);
            m_colors.emplace_back(otherColor);
            break;
        }
        case ColorHarmonyKind::SplitComplementary:
        {
            Hsl otherColor = paletteDisplayColor(m_anchor);
            otherColor.offsetHue(5.0f / 12.0f);
            m_colors.emplace_back(otherColor);
            otherColor.offsetHue(2.0f / 12.0f);
            m_colors.emplace_back(otherColor);
            break;
        }
        case ColorHarmonyKind::Count:
            break;
        }
        fillPigmentColors();
    }

    void ColorHarmony::addMap(const PaletteMap& value)
    {
        m_maps.push_back(value);
        m_selectedMap = &m_maps.front();
    }

    void ColorHarmony::selectMap(PaletteMap& value)
    {
        m_selectedMap = &value;
    }

    PaletteMap* ColorHarmony::findMapByHueDegrees(std::array<int, 3ull> hueDegrees)
    {
        for (PaletteMap& map : m_maps)
            if (map.size() == hueDegrees.size())
                if (color(map[0ull]).hueDegree() == hueDegrees[0ull]
                    && color(map[1ull]).hueDegree() == hueDegrees[1ull]
                    && color(map[2ull]).hueDegree() == hueDegrees[2ull])
                    return &map;
        return nullptr;
    }

    const PaletteColor& ColorHarmony::color(std::size_t index) const
    {
        switch (index)
        {
        case noHue: return m_gray;
        default: return m_colors[index];
        }
    }

    void ColorHarmony::fillPigmentColors()
    {
        const float gridStep = hueFromDegree(k_pigmentGridDegrees);

        // Only the hue is the harmony's, and only the hue leaves here for anything painted: the
        // resolver states the other two itself. These carry the palette's display pair so that a
        // pigment shown in the editor and a palette swatch beside it read as one set.
        m_pigmentColors.clear();
        for (const int referenceDegree : k_pigmentReferenceDegrees)
        {
            const float referenceOffset = hueOffset(hueFromDegree(referenceDegree), m_anchor);
            const float gridSteps = std::round(referenceOffset / gridStep);
            const float hue = wrappedHue(m_anchor + gridSteps * gridStep);
            m_pigmentColors.emplace_back(paletteDisplayColor(hue));
        }
    }

    // ColorHarmonySelector

    ColorHarmonySelector::ColorHarmonySelector(float& anchorHue)
        :
        m_anchorHue{ anchorHue }
    {
        for (ColorHarmonyKind kind = ColorHarmonyKind::First; kind != ColorHarmonyKind::Count; ++reinterpret_cast<std::size_t&>(kind))
            m_harmonies[static_cast<std::size_t>(kind)] = std::make_unique<ColorHarmony>(m_anchorHue, kind);
    }

    void ColorHarmonySelector::anchorChanged()
    {
        for (ColorHarmonyKind kind = ColorHarmonyKind::First; kind != ColorHarmonyKind::Count; ++reinterpret_cast<std::size_t&>(kind))
            m_harmonies[static_cast<std::size_t>(kind)]->anchorChanged();
    }

    std::size_t ColorHarmonySelector::count()
    {
        return m_harmonies.size();
    }

    ColorHarmony& ColorHarmonySelector::harmony(std::size_t index)
    {
        return *m_harmonies[index];
    }
}
