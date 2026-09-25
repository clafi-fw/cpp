export module ClaFi.App.ThemeIcon;

import ClaFi.Icons.ChannelTile;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.Graphics.Canvas;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    // What a theme's icon is drawn from - three palette hues and the form surface at either end.
    export struct ThemeIconColors
    {
        ThemeIconColors() = default;
        explicit ThemeIconColors(const ThemeColors&);
        [[nodiscard]] Hsl surface(ColorMode) const;

        std::array<float, 3> paletteHues{};
        Hsl darkSurface{};
        Hsl lightSurface{};
    };

    // The theme's own surface over the rect given. What a theme's sample sits on, at any size.
    export void paintThemeBackground(PaintIconEvent&, const ThemeIconColors&, const FloatRect&, float radius);
    export void paintThemeBackground(PaintIconEvent&, const ThemeColors&, const FloatRect&, float radius);
    // The theme's three palette colours as three blobs, cut from a rounded square by a Y. Three
    // flat regions carry a palette at a size where three swatches in a row would each be a sliver.
    export void paintPaletteTile(PaintIconEvent&, const ThemeIconColors&, const FloatRect&);
    export void paintPaletteTile(PaintIconEvent&, const ThemeColors&, const FloatRect&);
    // The same tile inline in a run of text, at a size given in design units. It carries no
    // surface, being written into text that is already drawn on the theme's own.
    export void paintPaletteTile(Text&, const ThemeColors&, float size);
    // The whole theme as one small square: its surface, in the mode the icon is shown in, with the
    // palette inset in it. Two themes can share a palette, and the surface tells them apart twice
    // over, being the ground they are shown on and part of what their shape is drawn from.
    export void paintThemeIcon(PaintIconEvent&, const ThemeIconColors&);
    export void paintThemeIcon(PaintIconEvent&, const ThemeColors&);

    class PaletteTile
    {
    public:
        static PaintIconFunc paint;
    };


    //-----------------------------------------------------------------------------


    namespace
    {
        // The icon's shape is taken from the theme's own colours, so a theme draws the same icon on
        // every run with nothing stored anywhere, and two themes share a shape only by sharing every
        // colour it is drawn from - the three palette hues and the surface they stand on.
        constexpr std::uint32_t k_seedBasis = 2166136261u;
        constexpr std::uint32_t k_seedStep = 16777619u;
        constexpr std::uint32_t k_seedSalt = 4u;
        constexpr float k_hueSteps = 65535.0f;

        constexpr float k_armOffset = k_2Pi * 14.0f / 360.0f;   // Either way, off the symmetric Y
        constexpr float k_marginRatio = 0.09f;      // The ground left showing around the parts
        constexpr float k_leastSoftening = 0.06f;
        constexpr float k_mostSoftening = 0.23f;

        struct TileShape
        {
            Icons::ChannelTile::ArmAngles arms{};
            Icons::ChannelTile::CutSoftening softening{};
        };

        // FNV-1a over the hues at sixteen bits apiece and the surface's three bytes. A hue moving by
        // less than one part in sixty-five thousand leaves the shape alone, which is what keeps a
        // slider being dragged from making the icon flicker between shapes.
        [[nodiscard]] std::uint32_t seedFrom(const ThemeIconColors& colors)
        {
            std::uint32_t hash = k_seedBasis;
            const auto fold = [&hash](std::uint32_t value){
                hash ^= value & 0xffu;
                hash *= k_seedStep;
            };

            for (const float hue : colors.paletteHues)
            {
                const std::uint32_t scaled =
                    static_cast<std::uint32_t>(std::lround(hue * k_hueSteps)) & 0xffffu;
                fold(scaled);
                fold(scaled >> 8);
            }

            // Read at the dark end whatever mode the icon is shown in, so the shape is the theme's.
            const Color surface = colors.darkSurface.toColor();
            fold(surface.red);
            fold(surface.green);
            fold(surface.blue);
            fold(k_seedSalt);
            return hash;
        }

        // Mulberry32, written out rather than taken from the standard library so that a theme draws
        // the same shape whatever <random> does on the platform it is running on.
        [[nodiscard]] float nextFrom(std::uint32_t& state)
        {
            state += 0x6d2b79f5u;
            std::uint32_t value = state;
            value = (value ^ (value >> 15)) * (value | 1u);
            value ^= value + (value ^ (value >> 7)) * (value | 61u);
            value ^= value >> 14;
            return static_cast<float>(static_cast<double>(value) / 4294967296.0);
        }

        [[nodiscard]] TileShape shapeFor(const ThemeIconColors& colors)
        {
            std::uint32_t state = seedFrom(colors);
            TileShape shape;

            for (std::size_t i = 0ull; i != shape.arms.size(); ++i)
            {
                const float offset = (nextFrom(state) * 2.0f - 1.0f) * k_armOffset;
                shape.arms[i] = Icons::ChannelTile::k_stemAngle
                    + k_2Pi * static_cast<float>(i) / Icons::ChannelTile::k_partCount + offset;
            }

            for (float& amount : shape.softening)
                amount = k_leastSoftening + nextFrom(state) * (k_mostSoftening - k_leastSoftening);

            return shape;
        }
    }

    // ThemeIconColors

    ThemeIconColors::ThemeIconColors(const ThemeColors& colors)
        :
        paletteHues{ colors.paletteHues },
        darkSurface{ colors.formSurface(ColorMode::Dark) },
        lightSurface{ colors.formSurface(ColorMode::Light) }
    {
    }

    Hsl ThemeIconColors::surface(const ColorMode mode) const
    {
        return mode == ColorMode::Dark ? darkSurface : lightSurface;
    }

    // Painting

    void paintThemeBackground(PaintIconEvent& event, const ThemeColors& colors, const FloatRect& rect, const float radius)
    {
        paintThemeBackground(event, ThemeIconColors{ colors }, rect, radius);
    }

    void paintThemeBackground(PaintIconEvent& event, const ThemeIconColors& colors, const FloatRect& rect, const float radius)
    {
        // The same width at every size: a border that scaled with the icon would read as a frame
        // on a tile and as nothing at all on a crumb.
        const ColorMode mode = colorModeOf(event.lightness());
        const Color surface = event.applyDisabledFactor(colors.surface(mode).toColor());
        event.canvas().fillRoundedRectangle(rect, radius, radius, surface);
    }

    void paintThemeIcon(PaintIconEvent& event, const ThemeColors& colors)
    {
        paintThemeIcon(event, ThemeIconColors{ colors });
    }

    void paintThemeIcon(PaintIconEvent& event, const ThemeIconColors& colors)
    {
        const float size = event.iconRect().height();
        const float radius = size * Icons::ChannelTile::k_cornerRatio;
        const FloatRect& iconRect = event.iconRect().centerRect(size);
        paintThemeBackground(event, colors, iconRect, radius);
        // THE SAME RECT THE BACKGROUND WAS GIVEN. The parts are brought in from it by their own
        // margin, cut from a shape whose arcs are concentric with this one, so the ground shows the
        // same width all round them, corners included. A rim taken by standing a second rounded
        // rect inside this one comes out uneven instead, the two arcs not running parallel through
        // a corner.
        paintPaletteTile(event, colors, iconRect);
    }

    void paintPaletteTile(PaintIconEvent& event, const ThemeColors& colors, const FloatRect& rect)
    {
        paintPaletteTile(event, ThemeIconColors{ colors }, rect);
    }

    void paintPaletteTile(PaintIconEvent& event, const ThemeIconColors& colors, const FloatRect& rect)
    {
        // A palette entry is a hue and nothing else, so the tile shows each one in the palette's
        // display pair. The shape is the theme's own: the same three hues over another surface come
        // out as a different set of blobs, so a theme is told apart by its sample as well as by the
        // ground the sample sits on.
        Icons::ChannelTile::TileColors tileColors{};
        for (std::size_t i = 0ull; i != tileColors.size(); ++i)
        {
            const Hsl paletteColor = paletteDisplayColor(colors.paletteHues[i]);
            tileColors[i] = event.applyDisabledFactor(paletteColor.toColor());
        }

        // The shape is held in fractions of the tile, so one answer serves every size and DPI the
        // tile is drawn at. Nothing is drawn in the divider, so the Y is whatever the tile sits on.
        const TileShape shape = shapeFor(colors);

        Icons::ChannelTile::drawBlobTile(event.canvas(), rect, tileColors, shape.arms,
            k_marginRatio, Icons::ChannelTile::k_channelDividerRatio, shape.softening);
    }

    void paintPaletteTile(Text& text, const ThemeColors& colors, const float size)
    {
        // Three colours do not fit in a Tag, so the tile is handed the theme they belong to. The
        // colours must outlive the text, which is baked and painted well after this returns.
        text << InTextIcon{ size, size, size * 0.85f, PaletteTile::paint, Tag{ &colors } };
    }

    PaintIconFunc PaletteTile::paint
    {
        [](PaintIconEvent& event)
        {
            paintPaletteTile(event, *event.tag().get<const ThemeColors*>(), event.iconRect());
        }
    };

}
