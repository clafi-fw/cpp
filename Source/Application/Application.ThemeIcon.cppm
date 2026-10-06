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

        Hsl rootSurface{};
        
        Hsl title{};
        Hsl dialog{};
        Hsl toolBar{};
        Hsl section{};
        Hsl border{};

        Hsl mutedText{};
        Hsl accent{};
        Hsl spot{};
    };

    // What a theme's icon is drawn from - three palette hues, and the surface and window per mode.
    export struct ThemeIconColors
    {
        ThemeIconColors() = default;
        explicit ThemeIconColors(const ThemeColors&);
        [[nodiscard]] Hsl surface2(ColorMode) const;
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
        rootSurface{ colors.rootSurface(mode) },
        title{ restingSurface(colors, UiElement::DialogTitle, mode) },
        dialog{ restingSurface(colors, UiElement::Dialog, mode) },
        toolBar{ restingSurface(colors, UiElement::ToolBar, mode) },
        section{ restingSurface(colors, UiElement::Section, mode) },
        border{ restingStroke(colors, UiElement::Dialog, mode) }
    {
        // The page's inks, resolved the way ControlPaintContext::inkHsl resolves them.
        const Hsl ink = restingInk(colors, UiElement::Page, mode);
        constexpr float muted = gradeOf(InkGrade::Muted);
        const float grade = mode == ColorMode::Light
            ? lightGradeOf(muted, colors.darkModeFloor)
            : muted;
        mutedText = Hsl{ dialog, ink, grade };
        accent = ink;
        colors.accent.applyTo(accent, 1.0f, colors, mode);
        spot = ink;
        colors.spot.applyTo(spot, 1.0f, colors, mode);
    }

    // ThemeIconColors

    ThemeIconColors::ThemeIconColors(const ThemeColors& colors)
        :
        paletteHues{ colors.paletteHues },
        darkSample{ colors, ColorMode::Dark },
        lightSample{ colors, ColorMode::Light }
    {
    }

    Hsl ThemeIconColors::surface2(const ColorMode mode) const
    {
        return mode == ColorMode::Dark ? darkSample.rootSurface : lightSample.rootSurface;
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
        const Color titleColor = event.applyDisabledFactor(sample.title.toColor());
        const Color dialogColor = event.applyDisabledFactor(sample.dialog.toColor());
        const Color pageColor = event.applyDisabledFactor(sample.dialog.toColor());
        const Color barColor = event.applyDisabledFactor(sample.toolBar.toColor());
        const Color mutedText = event.applyDisabledFactor(sample.mutedText.toColor());
        const Color borderColor = event.applyDisabledFactor(sample.border.toColor());
        const Color accentColor = event.applyDisabledFactor(sample.accent.toColor());
        const Color spotColor = event.applyDisabledFactor(sample.spot.toColor());

        constexpr float k_titleShare = 0.25f;
        constexpr float k_barShare = 0.25f;

        // Title
        {
            RoundedRectangleParts backgroundPart{
                .bounds = backgroundRect,
                .radii = CornerRadii::topSideRound(cornersRadius),
                .sides = RectSides::all()
            };
            backgroundPart.bounds.bottom = backgroundRect.relativeY(k_titleShare) + 1.0f;

            event.canvas().fillPartialRoundedRectangle(backgroundPart, titleColor);
        }

        // Left strip Background
        {
            RoundedRectangleParts backgroundPart{
                .bounds = backgroundRect,
                .radii = CornerRadii::oneRound(Corner::BottomLeft, cornersRadius),
                .sides = RectSides::all()
            };
            backgroundPart.bounds.top = backgroundRect.relativeY(k_titleShare) - 1.0f;
            backgroundPart.bounds.right = backgroundRect.relativeX(0.33f) + 1.0f;

            event.canvas().fillPartialRoundedRectangle(backgroundPart, dialogColor);
        }
        
        // Page background
        {
            RoundedRectangleParts backgroundPart{
                .bounds = backgroundRect,
                .radii = CornerRadii::rightSideRound(cornersRadius),
                .sides = RectSides::all()
            };
            backgroundPart.bounds.top = backgroundRect.relativeY(k_titleShare) - 1.0f;
            backgroundPart.bounds.left = backgroundRect.relativeX(0.33f) - 1.0f;

            event.canvas().fillPartialRoundedRectangle(backgroundPart, pageColor);
        }

        // Bottom bar
        {
            RoundedRectangleParts backgroundPart{
                .bounds = backgroundRect,
                .radii = CornerRadii::bottomSideRound(cornersRadius),
                .sides = RectSides::all()
            };
            backgroundPart.bounds.top = backgroundRect.relativeY(1.0f - k_barShare);

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
