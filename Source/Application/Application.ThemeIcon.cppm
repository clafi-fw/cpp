export module ClaFi.App.ThemeIcon;

import ClaFi.Application.ThemesManager_Elements;

import ClaFi.Icons.ChannelTile;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.AppTheme_Colors;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.AppTheme_Theme;
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
        ThemeSampleColors(const AppTheme&, ColorMode);

        Color rootSurface{};

        Hsl titleHsl{}; // the title band as a surface, for a control standing on it
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

    // Two samples of the theme colors - one for dark and one for light mode
    export struct ThemeIconColors
    {
        ThemeIconColors() = default;
        explicit ThemeIconColors(const AppTheme&);
        [[nodiscard]] const ThemeSampleColors& sample(ColorMode) const;
        std::array<float, 3> paletteHues{};
        ThemeSampleColors darkSample{};
        ThemeSampleColors lightSample{};
    };

    export void drawThemeIcon(DrawIconEvent&, const ThemeIconColors&);
    export void drawThemeIcon(DrawIconEvent&, const AppTheme&);
    export void drawThemeBackground(Graphics::Canvas& canvas, const FloatRect& rect,
        float cornerRadius, const ThemeSampleColors& sample, float borderWidth);
    export void drawThemeIconOnly(Graphics::Canvas& canvas, const FloatRect& iconRect,
        const ThemeSampleColors& sample);


    //-----------------------------------------------------------------------------

    // ThemeSampleColors

    ThemeSampleColors::ThemeSampleColors(const AppTheme& theme, const ColorMode mode)
        :
        rootSurface{ theme.colors.rootSurface(mode).toColor() },

        titleHsl{ restingSurface(theme, UiElement::DialogTitle, mode) },
        title{ titleHsl.toColor() },
        dialog{ restingSurface(theme, UiElement::Dialog, mode).toColor() },
        toolBar{ restingSurface(theme, UiElement::ToolBar, mode).toColor() },
        page{ restingSurface(theme, UiElement::Page, mode).toColor() },
        section{ restingSurface(theme, UiElement::Section, mode).toColor() },

        border{ restingStroke(theme, UiElement::Dialog, mode).toColor() }
    {
        const ThemeColors& colors = theme.colors;
        // The page's inks, resolved the way ControlPaintContext::inkHsl resolves them.
        const Hsl pageInk = restingInk(theme, UiElement::Page, mode);
        constexpr float muted = gradeOf(InkGrade::Muted);
        const float grade = mode == ColorMode::Light
            ? lightGradeOf(muted, colors.darkModeFloor)
            : muted;

        Hsl dialogHsl = restingSurface(theme, UiElement::Dialog, mode);
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

    ThemeIconColors::ThemeIconColors(const AppTheme& theme)
        :
        paletteHues{ theme.colors.paletteHues },
        darkSample{ theme, ColorMode::Dark },
        lightSample{ theme, ColorMode::Light }
    {
    }

    const ThemeSampleColors& ThemeIconColors::sample(const ColorMode mode) const
    {
        return mode == ColorMode::Dark ? darkSample : lightSample;
    }

    // Painting

    void drawThemeBackground(Graphics::Canvas& canvas, const FloatRect& rect,
        float cornerRadius, const ThemeSampleColors& sample, float borderWidth)
    {
        constexpr float k_topShare = 0.3f;
        constexpr float k_bottomShare = 0.3f;

        const float titleBottom = rect.relativeY(k_topShare) + 0.5f;
        const float bodyTop = titleBottom - 1.0f;

        const float bottomBarTop = rect.relativeY(1.0f - k_bottomShare) - 0.5f;
        const float bodyBottom = bottomBarTop + 1.0f;

        // Title
        {
            RoundedRectangleParts backgroundPart{
                .bounds = rect,
                .radii = CornerRadii::TopSideRound(cornerRadius),
                .sides = RectSides::All()
            };
            backgroundPart.bounds.bottom = titleBottom + 1.0f;
            canvas.fillPartialRoundedRectangle(backgroundPart, sample.title);
        }

        // Middle background
        {
            RoundedRectangleParts backgroundPart{
                .bounds = {
                    rect.left,
                    bodyTop,
                    rect.right,
                    bodyBottom
                },
                .radii = CornerRadii::Square(),
                .sides = RectSides::All()
            };
            canvas.fillPartialRoundedRectangle(backgroundPart, sample.section);
        }

        // Bottom bar
        {
            RoundedRectangleParts backgroundPart{
                .bounds = rect,
                .radii = CornerRadii::BottomSideRound(cornerRadius),
                .sides = RectSides::All()
            };
            backgroundPart.bounds.top = bottomBarTop;
            canvas.fillPartialRoundedRectangle(backgroundPart, sample.toolBar);
        }

        // Border
        canvas.drawRoundedRectangle(rect, cornerRadius, cornerRadius,
            sample.border, borderWidth);
    }

    void drawThemeIconOnly(Graphics::Canvas& canvas, const FloatRect& iconRect, const ThemeSampleColors& sample )
    {
        // Rows
        {
            const std::array rowColors{ sample.mutedText, sample.accent, sample.spot };

            float rowSize = iconRect.height() / 6.0f;
            float rowMargin = rowSize / 2.0f;
            float rowPadding = 0.25f;
            float x = iconRect.relativeX(rowPadding);
            float x2 = iconRect.relativeX(1.0f - rowPadding);
            float boxRadius = rowSize / 2.0f;
            float rowRadius = rowSize / 2.0f;

            bool b = true;
            float y = iconRect.relativeY(0.2f);
            for (std::size_t i = 0; i != rowColors.size(); ++i)
            {
                FloatRect boxRect = { x, y,  b ? x2 : x + rowSize, y + rowSize };
                if (!b)
                {
                    // Box
                    canvas.fillRoundedRectangle(boxRect, boxRadius, boxRadius, sample.accent);
                    // Label
                    boxRect.left = boxRect.right + rowSize;
                    boxRect.right = x2;
                }
                canvas.fillRoundedRectangle(boxRect, rowRadius, rowRadius, rowColors[i]);
                y = boxRect.bottom + rowMargin;
            }
        }
    }

    void drawThemeIcon(DrawIconEvent& event, const AppTheme& theme)
    {
        drawThemeIcon(event, ThemeIconColors{ theme });
    }

    void drawThemeIcon(DrawIconEvent& event, const ThemeIconColors& colors)
    {
        const FloatRect backgroundRect = event.iconRect();
        const float cornersRadius = backgroundRect.height() / 8.0f;
        const ColorMode mode = colorModeOf(event.lightness());
        const ThemeSampleColors& sample = colors.sample(mode);
        drawThemeBackground(event.canvas(), backgroundRect, cornersRadius, sample,
            event.scaledStrokeWidth(Thickness::Thin));
        
        drawThemeIconOnly(event.canvas(), backgroundRect, sample);
    }

}
