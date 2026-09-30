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

    // The mode the application is drawn in, as the user chose it. See UI-Types
    export enum class ColorModeSetting
    {
        Auto,
        Dark,
        Light
    };

    // Which of the theme's semantic hues an ink is drawn from. See UI-Types
    export enum class Pigment : std::size_t
    {
        First = 0,
        Yellow = 0,
        Green,
        Blue,
        Red,
        Count
    };

    export constexpr std::size_t k_pigmentsCount = static_cast<std::size_t>(Pigment::Count);

    // How colourful a colour is and where it sits. See UI-Types
    export struct InkTone
    {
        float saturation{ 1.0f };
        float luminosity{ 0.5f };
        bool operator==(const InkTone&) const = default;
    };

    // One colour stated once for each side of the theme. See UI-Types
    export struct PigmentTones
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

    // The named steps' shares, as a dark surface reads them. See UI-Types#inkgrade
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

    // A share as a light surface reads it: the steps counted from the ink. See UI-Types#inkgrade
    export [[nodiscard]] constexpr float lightGradeOf(float grade, float luminosityFloor)
    {
        if (grade <= 0.0f || grade >= 1.0f)
            return grade;

        // The steps with the surface's end before them - the last step is the ink's end.
        std::array<float, k_inkGradesCount + 1> ladder{};
        for (std::size_t index = 0; index != k_inkGradesCount; ++index)
            ladder[index + 1] = k_inkGrades[index];

        // The segment the share falls in, and the same segment counted from the ink's end - every
        // step but the ink itself stopped short of it by the floor.
        const std::size_t last = k_inkGradesCount;
        const float reach = 1.0f - luminosityFloor;
        std::size_t index = 1;
        while (grade > ladder[index])
            ++index;
        const float from = (1.0f - ladder[last - index + 1]) * reach;
        const float to = index == last ? 1.0f : (1.0f - ladder[last - index]) * reach;
        const float along = (grade - ladder[index - 1]) / (ladder[index] - ladder[index - 1]);
        return from + (to - from) * along;
    }

    // How an ink's grade becomes a colour. See UI-Types
    export enum class InkColor : std::size_t
    {
        Text,   // a shade of the ink the paint chain arrived at. See UI-Types
        Accent, // the hue the theme's own live states are drawn from. See UI-Types
        Spot,   // the second accent, for what stands apart from the interface. See UI-Types
        Yellow, // the yellow pigment, at InkWell's tone. See UI-Types#yellow-green-blue-red
        Green,  // the green pigment, at InkWell's tone. See UI-Types#yellow-green-blue-red
        Blue,   // the blue pigment, at InkWell's tone. See UI-Types#yellow-green-blue-red
        Red,    // the red pigment, at InkWell's tone. See UI-Types#yellow-green-blue-red
        Black,  // pure black, the same on either side of the theme. See UI-Types#black-white
        White,  // pure white, the same on either side of the theme. See UI-Types#black-white
        Count
    };

    // The pigment one of the semantic colours is drawn from, and none for the others.
    export [[nodiscard]] constexpr std::optional<Pigment> pigmentOf(InkColor color)
    {
        switch (color)
        {
            case InkColor::Yellow:
                return Pigment::Yellow;
            case InkColor::Green:
                return Pigment::Green;
            case InkColor::Blue:
                return Pigment::Blue;
            case InkColor::Red:
                return Pigment::Red;
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
        // The share of the gap between the surface and the ink, as a dark surface reads it.
        float grade{ gradeOf(InkGrade::Strongest) };
    };

}
