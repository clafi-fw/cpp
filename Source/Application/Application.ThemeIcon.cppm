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
    export void paintThemeBackground(Graphics::Canvas& canvas, const FloatRect& rect,
        float cornerRadius, const ThemeSampleColors& sample, float borderWidth);
    export void paintThemeIconOnly(Graphics::Canvas& canvas, const FloatRect& iconRect,
        bool isSmall, const ThemeSampleColors& sample);


    //-----------------------------------------------------------------------------

    // ThemeSampleColors

    ThemeSampleColors::ThemeSampleColors(const ThemeColors& colors, const ColorMode mode)
        :
        rootSurface{ colors.rootSurface(mode).toColor() },

        titleHsl{ restingSurface(colors, UiElement::DialogTitle, mode) },
        title{ titleHsl.toColor() },
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

    void paintThemeBackground(Graphics::Canvas& canvas, const FloatRect& rect,
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
                .radii = CornerRadii::topSideRound(cornerRadius),
                .sides = RectSides::all()
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
                .radii = CornerRadii::square(),
                .sides = RectSides::all()
            };
            canvas.fillPartialRoundedRectangle(backgroundPart, sample.section);
        }

        // Bottom bar
        {
            RoundedRectangleParts backgroundPart{
                .bounds = rect,
                .radii = CornerRadii::bottomSideRound(cornerRadius),
                .sides = RectSides::all()
            };
            backgroundPart.bounds.top = bottomBarTop;
            canvas.fillPartialRoundedRectangle(backgroundPart, sample.toolBar);
        }

        // Border
        canvas.drawRoundedRectangle(rect, cornerRadius, cornerRadius,
            sample.border, borderWidth);
    }

    void paintThemeIconOnly(Graphics::Canvas& canvas, const FloatRect& iconRect, bool isSmall, const ThemeSampleColors& sample )
    {
        // Rows
        {
            const std::array rowColors{ sample.mutedText, sample.accent, sample.spot };

            float rowSize = iconRect.height() / 6.0f;
            float rowMargin = rowSize / 2.0;
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

    void paintThemeIcon(PaintIconEvent& event, const ThemeColors& colors)
    {
        paintThemeIcon(event, ThemeIconColors{ colors });
    }

    void paintThemeIcon(PaintIconEvent& event, const ThemeIconColors& colors)
    {
        const FloatRect backgroundRect = event.iconRect();
        const float iconSize = backgroundRect.width();
        const float cornersRadius = backgroundRect.height() / 8.0f;
        const ColorMode mode = colorModeOf(event.lightness());
        const ThemeSampleColors& sample = colors.sample(mode);
        paintThemeBackground(event.canvas(), backgroundRect, cornersRadius, sample,
            event.scaledStrokeWidth(Thickness::Thin));
        
        paintThemeIconOnly(event.canvas(), backgroundRect, true, sample);
    }

}
