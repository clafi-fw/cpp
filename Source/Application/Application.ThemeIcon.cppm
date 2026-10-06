export module ClaFi.App.ThemeIcon;

import ClaFi.Application.ThemesManager_Elements;

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
    // A theme's window in one mode, at rest, as its icon draws it.
    export struct ThemeSampleColors
    {
        ThemeSampleColors() = default;
        ThemeSampleColors(const ThemeColors&, ColorMode);

        Color rootSurface{};
        
        Color title{};
        Color dialog{};
        Color toolBar{};
        Color page{};
        Color section{};
        Color border{};

        Color mutedText{};
        Color accent{};
        Color spot{};
    };

    // What a theme's icon is drawn from - three palette hues, and the surface and window per mode.
    export struct ThemeIconColors
    {
        ThemeIconColors() = default;
        explicit ThemeIconColors(const ThemeColors&);
        [[nodiscard]] const ThemeSampleColors& sample(ColorMode) const;
        std::array<float, 3> paletteHues{};
        ThemeSampleColors darkSample{};
        ThemeSampleColors lightSample{};
    };

    export void paintThemeIcon(PaintIconEvent&, const ThemeIconColors&);
    export void paintThemeIcon(PaintIconEvent&, const ThemeColors&);


    //-----------------------------------------------------------------------------

    // ThemeSampleColors

    ThemeSampleColors::ThemeSampleColors(const ThemeColors& colors, const ColorMode mode)
        :
        rootSurface{ colors.rootSurface(mode).toColor() },

        title{ restingSurface(colors, UiElement::DialogTitle, mode).toColor() },
        dialog{ restingSurface(colors, UiElement::Dialog, mode).toColor() },
        toolBar{ restingSurface(colors, UiElement::ToolBar, mode).toColor() },
        page{ restingSurface(colors, UiElement::Page, mode).toColor() },
        section{ restingSurface(colors, UiElement::Section, mode).toColor() },

        border{ restingStroke(colors, UiElement::Dialog, mode).toColor() }
    {
        // The page's inks, resolved the way ControlPaintContext::inkHsl resolves them.
        const Hsl pageInk = restingInk(colors, UiElement::Page, mode);
        constexpr float muted = gradeOf(InkGrade::Muted);
        const float grade = mode == ColorMode::Light
            ? lightGradeOf(muted, colors.darkModeFloor)
            : muted;

        Hsl dialogHsl = restingSurface(colors, UiElement::Dialog, mode);
        Hsl tmpHsl = Hsl{ dialogHsl, pageInk, grade };
        mutedText = tmpHsl.toColor();
        
        tmpHsl = dialogHsl;
        colors.accent.applyTo(tmpHsl, 1.0f, colors, mode);
        accent = tmpHsl.toColor();

        tmpHsl = dialogHsl;
        colors.spot.applyTo(tmpHsl, 1.0f, colors, mode);
        spot = tmpHsl.toColor();
    }

    // ThemeIconColors

    ThemeIconColors::ThemeIconColors(const ThemeColors& colors)
        :
        paletteHues{ colors.paletteHues },
        darkSample{ colors, ColorMode::Dark },
        lightSample{ colors, ColorMode::Light }
    {
    }

    const ThemeSampleColors& ThemeIconColors::sample(const ColorMode mode) const
    {
        return mode == ColorMode::Dark ? darkSample : lightSample;
    }

    // Painting

    void paintThemeIcon(PaintIconEvent& event, const ThemeColors& colors)
    {
        paintThemeIcon(event, ThemeIconColors{ colors });
    }

    void paintThemeIcon(PaintIconEvent& event, const ThemeIconColors& colors)
    {
        const ColorMode mode = colorModeOf(event.lightness());
        float iconSize = event.iconWidth();
        bool isSmall = event.unScale(iconSize) < 32.0f;

        const FloatRect backgroundRect = event.iconRect();
        const float cornersRadius = std::min(
            backgroundRect.height() / 8.0f,
            event.scaleF(4.0f)
            );
        
        const ThemeSampleColors& sample = colors.sample(mode);
        const Color titleColor = event.applyDisabledFactor(sample.title);
        const Color dialogColor = event.applyDisabledFactor(sample.dialog);
        const Color pageColor = event.applyDisabledFactor(sample.page);
        const Color sectionColor = event.applyDisabledFactor(sample.section);

        const Color barColor = event.applyDisabledFactor(sample.toolBar);
        const Color mutedText = event.applyDisabledFactor(sample.mutedText);
        const Color borderColor = event.applyDisabledFactor(sample.border);
        const Color accentColor = event.applyDisabledFactor(sample.accent);
        const Color spotColor = event.applyDisabledFactor(sample.spot);

        constexpr float k_titleShare = 0.25f;
        constexpr float k_stripShare = 0.28f;
        constexpr float k_barShare = 0.25f;
        
        const float titleBottom = backgroundRect.relativeY(k_titleShare) + 0.5f;
        const float bodyTop = titleBottom - 1.0f;

        const float bottomBarTop = backgroundRect.relativeY(1.0f - k_barShare) - 0.5f;
        const float bodyBottom = bottomBarTop + 1.0f;
        const float bodyLeft = backgroundRect.relativeX(k_stripShare);
        const float bodyRight = backgroundRect.relativeX(1.0f - k_stripShare);

        // Title
        {
            RoundedRectangleParts backgroundPart{
                .bounds = backgroundRect,
                .radii = CornerRadii::topSideRound(cornersRadius),
                .sides = RectSides::all()
            };
            backgroundPart.bounds.bottom = titleBottom + 1.0f;
            event.canvas().fillPartialRoundedRectangle(backgroundPart, titleColor);
        }

        // Left strip background
        {
            RoundedRectangleParts backgroundPart{
                .bounds = {
                    backgroundRect.left,
                    bodyTop,
                    bodyLeft + 0.5f,
                    bodyBottom
                },
                .radii = CornerRadii::square(),
                .sides = RectSides::all()
            };
            event.canvas().fillPartialRoundedRectangle(backgroundPart, dialogColor);
        }
        
        // Middle page background
        {
            RoundedRectangleParts backgroundPart{
                .bounds = {
                    bodyLeft - 0.5f,
                    bodyTop,
                    bodyRight + 0.5f,
                    bodyBottom
                },
                .radii = CornerRadii::square(),
                .sides = RectSides::all()
            };
            event.canvas().fillPartialRoundedRectangle(backgroundPart, pageColor);
        }

        // Right strip background
        {
            RoundedRectangleParts backgroundPart{
                .bounds = {
                    bodyRight - 0.5f,
                    bodyTop,
                    backgroundRect.right,
                    bodyBottom
                },
                .radii = CornerRadii::square(),
                .sides = RectSides::all()
            };
            event.canvas().fillPartialRoundedRectangle(backgroundPart, sectionColor);
        }

        // Bottom bar
        {
            RoundedRectangleParts backgroundPart{
                .bounds = backgroundRect,
                .radii = CornerRadii::bottomSideRound(cornersRadius),
                .sides = RectSides::all()
            };
            backgroundPart.bounds.top = bottomBarTop;
            event.canvas().fillPartialRoundedRectangle(backgroundPart, barColor);
        }

        // Rows
        {
            const std::array rowColors{ spotColor, mutedText, mutedText };

            float rowSize = backgroundRect.height() / 8.0f;
            float rowMargin = rowSize;
            float rowPadding = 0.25f;
            float x = backgroundRect.relativeX(rowPadding);
            float x2 = backgroundRect.relativeX(1.0f - rowPadding);
            float boxRadius = rowSize / 2.0f;
            float rowRadius = rowSize / 2.0f;

            float y = backgroundRect.relativeY(0.2f);
            for (std::size_t i = 0; i != rowColors.size(); ++i)
            {
                FloatRect boxRect = { x, y,  isSmall ? x2 : x + rowSize, y + rowSize };
                if (!isSmall)
                {
                    // Box
                    event.canvas().fillRoundedRectangle(boxRect, boxRadius, boxRadius, accentColor);
                    // Label
                    boxRect.left = boxRect.right + rowSize;
                    boxRect.right = x2;
                }
                event.canvas().fillRoundedRectangle(boxRect, rowRadius, rowRadius, rowColors[i]);
                y = boxRect.bottom + rowMargin;
            }
        }

        // Border
        event.canvas().drawRoundedRectangle(backgroundRect, cornersRadius, cornersRadius,
            borderColor, event.scaledStrokeWidth(Thickness::Thin));
    }

}
