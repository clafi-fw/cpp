export module ClaFi.Core.System.UiTypes :TextTypes;

import :Color;

import ClaFi.StdLib;

namespace ClaFi{

    // Which side of the theme something is drawn on. See UI-Types
    export enum class ColorMode
    {
        Dark,
        Light
    };

    // The other side of the theme. An element the theme carries across itself - a dark strip on a
    // light theme - stands in the flipped mode, and everything drawn from that surface answers to
    // it. See ControlColorRules::flip.
    export [[nodiscard]] constexpr ColorMode flipped(ColorMode value)
    {
        return value == ColorMode::Dark ? ColorMode::Light : ColorMode::Dark;
    }

    // The mode the application is drawn in, as the user chose it. See UI-Types
    export enum class ColorModeSetting
    {
        Auto,
        Dark,
        Light
    };

    // Which of the theme's semantic hues an ink is drawn from. See UI-Types
    export enum class ColorSlot : std::size_t
    {
        First = 0,
        Yellowish = 0,
        Greenish,
        Blueish,
        Reddish,
        Count
    };

    export constexpr std::size_t k_colorSlotsCount = static_cast<std::size_t>(ColorSlot::Count);

    // How colourful a colour is and where it sits. See UI-Types
    export struct InkTone
    {
        float saturation{ 1.0f };
        float luminosity{ 0.5f };
        bool operator==(const InkTone&) const = default;
    };

    // One colour stated once for each side of the theme. See UI-Types
    export struct SlotTones
    {
        InkTone dark{};
        InkTone light{};
    };

    // A shade of the ink, as a share of the gap between the surface and the ink. See UI-Types
    export enum class InkGrade : std::size_t
    {
        First = 0,
        Faint = 0,
        Subtle,
        Muted,
        Strong,
        Strongest,
        Count
    };

    export constexpr std::size_t k_inkGradesCount = static_cast<std::size_t>(InkGrade::Count);

    export constexpr std::array<float, k_inkGradesCount> k_inkGrades{
        0.12f,
        0.27f,
        0.60f,
        0.80f,
        1.00f
    };

    // The share one of the named steps stands at. A grade is a share of the gap between the
    // surface and the ink, not a place on the luminosity axis: elevationOf names the second.
    export [[nodiscard]] constexpr float gradeOf(InkGrade value) { return k_inkGrades[static_cast<std::size_t>(value)]; }

    // How an ink's grade becomes a colour. See UI-Types
    export enum class InkColor : std::size_t
    {
        Text,   // a shade of the ink the paint chain arrived at. See UI-Types
        Accent, // the hue the theme's own live states are drawn from. See UI-Types
        Spot,   // the second accent, for what stands apart from the interface. See UI-Types
        // The palette's semantic hues, at the tones InkWell states. See UI-Types
        Yellow,
        Green,
        Blue,
        Red,
        // Pure black and white, the same on either side of the theme. See UI-Types
        Black,
        White,
        //
        Count
    };

    // The palette slot one of the semantic colours is drawn from, and none for the others.
    export [[nodiscard]] constexpr std::optional<ColorSlot> slotOf(InkColor color)
    {
        switch (color)
        {
            case InkColor::Yellow:
                return ColorSlot::Yellowish;
            case InkColor::Green:
                return ColorSlot::Greenish;
            case InkColor::Blue:
                return ColorSlot::Blueish;
            case InkColor::Red:
                return ColorSlot::Reddish;
            case InkColor::Text:
            case InkColor::Accent:
            case InkColor::Spot:
            case InkColor::Black:
            case InkColor::White:
            case InkColor::Count:
                break;
        }
        return std::nullopt;
    }

    // A colour named rather than stated. See UI-Types
    export struct Ink
    {
    public:
        constexpr Ink() = default;
        bool operator==(const Ink&) const = default;
        InkColor color{ InkColor::Text };
        // The share of the gap between the surface and the ink this stands at.
        float grade{ gradeOf(InkGrade::Strongest) };
    };

}
