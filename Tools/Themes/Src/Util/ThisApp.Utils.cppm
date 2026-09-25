export module ThisApp.Utils;

import ClaFi;
import ClaFi.App.ThemeIcon;
import ClaFi.Application.ThemesManager;
import ClaFi.Diagnostic.Log;
import ClaFi.Browser;

import ThisApp.Consts;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.AppTheme_Palette;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.Graphics.Canvas;

import ClaFi.Core.System.InkWell;

import ClaFi.Core.System.Utils;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ThisApp
{
    using namespace ::ClaFi;
    using namespace ::ClaFi::Controls;
    using namespace ::ClaFi::Browser;

    //export BrowserSettings* settings{};

    export void paintColorSpot(Text&, Color);
    // One colour as a round mark the height of a line, where paintColorSpot writes a wider one.
    // A hue that stands beside a palette rather than in it is written with this, so the two kinds
    // of mark are told apart by shape before they are read.
    export void paintColorDot(Text&, Color);

    /// Whether a name is one of the built-ins' and so cannot be given to a user theme. Case is not
    /// part of the answer: the file system does not tell `dark` from `Dark`, so the two would take
    /// the same file.
    export [[nodiscard]] bool isReservedThemeName(std::wstring_view stem);

    /// What a question is about, written into the sentence that puts it in the spot ink at full
    /// strength: the name of the one theme, or how many there are where the question is about
    /// several.
    ///
    /// @note SAID IN ONE PLACE so that two questions about a theme never mark it differently. A
    /// question is put about ONE thing, and this is what says which words in it are that thing -
    /// the spot rule being the accent kept for what stands apart from the interface rather than
    /// answering to it, which a theme, in an application about themes, is.
    ///
    /// @note IT IS NOT ALWAYS A NAME. A question about several themes names none of them: the
    /// count is what the user has to weigh there, and it stands in the sentence where a name
    /// would. Reading out a list of names instead would be a sentence nobody weighs.
    export [[nodiscard]] Text themeInQuestionText(std::wstring_view subject);

    /// Refuses a theme name that is no name at all, whatever it is being given to. Whether the
    /// name is FREE is the caller's: a rename may land on its own file and a new theme may not, so
    /// the two ask that question differently.
    export void checkThemeNameShape(AcceptEditEvent&, std::wstring_view stem);

    /// Renames a theme file to the stem an editor was left with, or refuses the name and says why.
    /// Answers THE FILE'S NEW NAME - the stem with the extension a theme file carries, which is
    /// what a page or a tile is known by - and nothing where the file did not move: a name that was
    /// refused, and a name the file already had, are both a theme still standing where it stood.
    ///
    /// @note Given the path rather than whatever holds it, because it runs WHILE THE EDITOR IS UP:
    /// a list of themes can be rebuilt under it, and a tile or a page of that list with it.
    export [[nodiscard]] std::wstring renameThemeFile(const std::filesystem::path&, AcceptEditEvent&);

    // The browser page a theme path opens - see AppTheme. A page is a file name under the home
    // page and a theme path is a root over a bare name, so the two spellings name one theme.
    export [[nodiscard]] std::wstring pagePathOfTheme(std::wstring_view themePath);
    // The theme a browser page stands for, by the extension the page name carries.
    export [[nodiscard]] std::wstring themePathOfPage(std::wstring_view pagePath);
    export void paintColorCell(Text&, Color, bool extendedMode = false);
    export void paintPaletteMap(Text&, const ColorHarmony&, const PaletteMap&);
    export void paintPaletteMap(PaintIconEvent&, const ColorHarmony&, const PaletteMap&);


    export inline auto disabledState = [](GetStateEvent& event) {
        event.state.enabled = false;
        };

    export struct NamedColor
    {
        const std::wstring_view name;
        Color* colorLink;
    };

    class ColorSpot
    {
    public:
        static PaintIconFunc paint;
    };


    //-------------------------------------------------------------------------

    constexpr float scale = 1.2f;
    constexpr float k_markH = 12.0f * scale;
    constexpr float k_markW = 24.0f * scale;
    export constexpr FixedSize k_harmonyItemSize{ 60.0f * scale, 48.0f * scale };

    // Square. The three parts meet at the centre and leave along their own arms, so a tile wider
    // than it is tall gives the width to the top part and the height to the other two.
    export constexpr float k_paletteTileSize = k_markW;

    // Design units, so both survive the scale down to a tab.
    constexpr float k_themeBackgroundRadius = 4.0f;
    constexpr float k_themeIconPadding = 2.0f;

    void paintColorSpot(Text& text, Color color)
    {
        text << InTextIcon{ k_markW, k_markH, k_markH * 0.85, ColorSpot::paint, Tag{ color.asUint() } };
    }

    void paintColorDot(Text& text, Color color)
    {
        // Square, so the rounding ColorSpot applies - half the height - closes into a circle.
        text << InTextIcon{ k_markH, k_markH, k_markH * 0.85f, ColorSpot::paint, Tag{ color.asUint() } };
    }

    void paintColorCell(Text& tt, Color color, bool extendedMode)
    {
        if (color.alpha)
        {
            paintColorSpot(tt, color);
            if (extendedMode)
                tt << L' '; // Space{ extSpace };
            else
                tt << TextOp::EndLine << TextStyleId::SubBody;
            std::wstring hexString = toHex(color.asUint() & 0xFFFFFF);
            while (hexString.size() < 6ull)
                hexString = L'0' + hexString;
            tt << TextStyleId::SubBody << L'#' << hexString;
        }
        else
        {
            tt << InTextIcon{ 1.0f, k_markH, k_markH * 0.85, nullptr, nullptr };
            if (!extendedMode)
                tt << TextOp::EndLine << TextStyleId::SubBody;
            tt << L' ';
        };
    }

    void paintPaletteMap(Text& tt, const ColorHarmony& harmony, const PaletteMap& map)
    {
        tt << TextAlign::Center << TextStyleId::SubBody;
        for (std::size_t i = 0ull; i != map.size(); ++i)
        {
            std::size_t index = map[i];
            if (!tt.empty())
                tt << TextOp::EndLine;
            paintColorSpot(tt, harmony.color(index).rgb());
        }
    }

    void paintPaletteMap(PaintIconEvent& event, const ColorHarmony& harmony, const PaletteMap& map)
    {
        RoundedRectangleParts parts{};
        const float radius = event.iconWidth() / 8.0f;

        float deflate = -event.scaleF(0.66f);
        float strokeWidth = event.scaledStrokeWidth(Thickness::Thin);
        Color strokeColor = event.textRgb(InkGrade::Muted);

        constexpr auto outline = false;

        parts.radii = { radius, radius, 0.0f, 0.0f };
        parts.bounds = event.iconRect().relativeRect({ 0.0f, 0.0f }, { 1.0f, 0.333f }).inflated(deflate);
        event.canvas().fillPartialRoundedRectangle(parts, harmony.color(map[0]).rgb());
        if (outline)
            event.canvas().drawPartialRoundedRectangle(parts, strokeColor, strokeWidth);

        parts.bounds = event.iconRect().relativeRect({ 0.0f, 0.333f }, { 1.0f, 0.666f }).inflated(deflate);
        event.canvas().fillRectangle(parts.bounds, harmony.color(map[1]).rgb());
        if (outline)
            event.canvas().drawRectangle(parts.bounds, strokeColor, strokeWidth);

        parts.bounds = event.iconRect().relativeRect({ 0.0f, 0.666f }, { 1.0f, 1.0f }).inflated(deflate);
        parts.radii = { 0.0f, 0.0f, radius, radius };
        event.canvas().fillPartialRoundedRectangle(parts, harmony.color(map[2]).rgb());
        if (outline)
            event.canvas().drawPartialRoundedRectangle(parts, strokeColor, strokeWidth);
    }

    PaintIconFunc ColorSpot::paint
    {
        [](PaintIconEvent& event)
        {
            const Scaler& scaler = event.scaler();
            FloatRect rect = event.iconRect();
            rect.inflate(-scaler.scaleF(2.0f));
            const float radius = rect.height() / 2.0f;
            event.canvas().fillRoundedRectangle(rect, radius, radius, event.tag().get<Color>());
            // event.canvas().drawRoundedRectangle(rect, radius, radius, event.textRgb(InkGrade::Muted));
        }
    };





    // Drop-in replacement for ThisApp.Utils::paintPaletteMap, plus the slant enum it takes.
    //
    // Needs, on top of what the module already imports:
    //     import ClaFi.Core.Graphics.Types;     // PixelPath, Matrix3x2, FloatRect
    //     import ClaFi.StdLib;                  // std::array, std::sqrt
    //
    // The straight version could stack three rounded rectangles because the splits were horizontal.
    // A leaning split cannot be a rectangle, so the bands are built as quads and the tile's rounded
    // outline comes from a clip instead - which also means the corner rounding no longer has to be
    // spelled out per band.

        // Six ways for the two inner lines to lean. Every one of them splits the tile into three equal
        // areas; what differs is the shape. Named rather than numbered so a caller spreading them over
        // a list is choosing something it can read back later.
    export enum class PaletteMapSlant
    {
        Converging,     // Lines lean towards each other - the middle band narrows to the right
        ConvergingLow,  // The same, with most of the lean carried by the lower line
        Diverging,      // The middle band widens to the right
        DivergingHigh,  // The same, with most of the lean carried by the upper line
        Falling,        // Both lines lean down to the right
        Rising,         // Both lean up to the right
        Count
    };

    export void paintPaletteMap(PaintIconEvent&, const ColorHarmony&, const PaletteMap&, PaletteMapSlant);

    // A slant for an arbitrary index, for handing every theme in a list a different one. Wraps, so
    // the caller can pass a row number or a hash without range-checking it.
    export PaletteMapSlant slantForIndex(std::size_t index);


    //----------------------------------------------------------------------------


    namespace
    {
        constexpr int k_bandCount = 3;

        // 1 - pi/4: what a quarter circle leaves behind in the square that boxes it, which is what
        // one rounded corner takes out of the tile.
        constexpr float k_quarterCircleDeficit = 1.0f - k_2Pi / 8.0f;

        // How far each inner line leans across the full width, as a fraction of the tile height.
        // Positive leans down to the right.
        struct SlantPair
        {
            float upper;
            float lower;
        };

        constexpr auto k_leanFactor = 0.15f;

        // Deliberately a narrow spread. The six only have to be told apart from each other at a
        // glance in a list; leaning harder than this makes each tile look like a statement rather
        // than a variation, and the middle band starts to read as a wedge.
        constexpr std::array<SlantPair, static_cast<std::size_t>(PaletteMapSlant::Count)> k_slants = {
            SlantPair{ -0.13f * k_leanFactor,  0.05f * k_leanFactor },    // DivergingHigh
            SlantPair{  0.12f * k_leanFactor, -0.07f * k_leanFactor },    // Converging
            SlantPair{  0.10f * k_leanFactor,  0.15f * k_leanFactor },    // Falling
            SlantPair{  0.06f * k_leanFactor, -0.14f * k_leanFactor },    // ConvergingLow
            SlantPair{ -0.06f * k_leanFactor,  0.15f * k_leanFactor },    // Diverging
            SlantPair{ -0.15f * k_leanFactor, -0.08f * k_leanFactor },     // Rising
        };

        // An inner line, as where it crosses the tile's vertical centreline and how steeply it runs.
        struct Divider
        {
            float centreY;
            float slope;
        };

        // Height at which the upper line crosses the tile's vertical centreline, as a fraction of
        // the tile height. The lower line is the mirror of it, the tile being symmetric top to
        // bottom.
        //
        // This does not depend on the lean, which is what lets one pair of crossings serve all six
        // variants: the tile is symmetric left to right, so a leaning line adds exactly as much area
        // on one side of the centreline as it takes from the other. That cancellation is exact while
        // the line stays clear of the corner arcs, which needs |lean| <= 2 * (crossing - radius /
        // height) - about 0.42 at a radius of an eighth, nearly three times the steepest lean used.
        //
        // The value is the plain one-third split, pushed down by what the corners remove: each
        // corner costs radius^2 * (1 - pi/4), the top band gives up two of them and the middle band
        // none, so the line has to drop to make the top band's share back up.
        float upperCrossing(float width, float height, float radius)
        {
            float cornerLoss = radius * radius * k_quarterCircleDeficit;
            return 1.0f / 3.0f + 2.0f / 3.0f * cornerLoss / (width * height);
        }

        // Where a line sits at a given x once it has been pushed half a gap to one side. The push
        // follows the line's normal rather than straight down, so a leaning line leaves the same gap
        // as a level one instead of a slightly wider one.
        float dividerEdge(const Divider& divider, float x, float halfGap, float towards)
        {
            float normalScale = std::sqrt(1.0f + divider.slope * divider.slope);
            return divider.centreY + divider.slope * x + towards * halfGap * normalScale;
        }

        // One band as a quad. It reaches past the tile on every side that is not a line, so the clip
        // decides the outline and the band only has to get its lines right.
        void addBand(Graphics::PixelPath& path, int band, const std::array<Divider, k_bandCount - 1>& dividers,
                     float outerX, float reach, float halfGap)
        {
            bool hasLineAbove = band > 0;
            bool hasLineBelow = band < k_bandCount - 1;

            float topLeft = hasLineAbove ? dividerEdge(dividers[band - 1], -outerX, halfGap, 1.0f) : -reach;
            float topRight = hasLineAbove ? dividerEdge(dividers[band - 1], outerX, halfGap, 1.0f) : -reach;
            float bottomLeft = hasLineBelow ? dividerEdge(dividers[band], -outerX, halfGap, -1.0f) : reach;
            float bottomRight = hasLineBelow ? dividerEdge(dividers[band], outerX, halfGap, -1.0f) : reach;

            path.clear();
            path.moveTo(-outerX, topLeft);
            path.lineTo(outerX, topRight);
            path.lineTo(outerX, bottomRight);
            path.lineTo(-outerX, bottomLeft);
            path.close();
        }
    }

    PaletteMapSlant slantForIndex(std::size_t index)
    {
        return static_cast<PaletteMapSlant>(index % static_cast<std::size_t>(PaletteMapSlant::Count));
    }

    void paintPaletteMap(PaintIconEvent& event, const ColorHarmony& harmony, const PaletteMap& map,
        PaletteMapSlant slant)
    {
        Graphics::Canvas& canvas = event.canvas();

        // The old deflate did two jobs and still does both: it is the inset of the whole tile, and
        // half the gap left between bands.
        float inset = event.scaleF(0.66f);
        FloatRect mapRect = event.iconRect().inflated(-inset);
        float width = mapRect.width();
        float height = mapRect.height();
        float radius = event.iconWidth() / 8.0f;

        const SlantPair& lean = k_slants[static_cast<std::size_t>(slant)];
        float crossing = upperCrossing(width, height, radius);

        // A lean is given per tile height but applied per x, so it converts on the way in. On a
        // square tile the two are the same and this is a multiply by one.
        float slopeScale = height / width;
        std::array<Divider, k_bandCount - 1> dividers = {
            Divider{ (crossing - 0.5f) * height, lean.upper * slopeScale },
            Divider{ (0.5f - crossing) * height, lean.lower * slopeScale }
        };

        // Built around the origin, the way drawRoundedRect builds, and placed by one matrix.
        Graphics::Matrix3x2 transform = Graphics::Matrix3x2::translation(mapRect.center());
        float outerX = width;       // Past the clip on the left and right
        float reach = height;       // Past the clip above the first band and below the last

        Graphics::PixelPath path;
        path.drawRoundedRect(width, height, radius);
        canvas.pushClip(path, &transform);

        for (int band = 0; band < k_bandCount; ++band)
        {
            addBand(path, band, dividers, outerX, reach, inset);
            canvas.fillPath(path, harmony.color(map[band]).rgb(), &transform);
            //canvas.drawPath(path, textInk(InkGrade::Muted).toColor(event), event.scaleBorder(1.0f), &transform);
        }

        canvas.popClip();
    }

    void checkThemeNameShape(AcceptEditEvent& event, const std::wstring_view stem)
    {
        if (stem.empty())
        {
            event.refuse(L"A theme needs a name.");
            return;
        }
        if (stem.find_first_of(L"\\/:*?\"<>|") != std::wstring_view::npos)
        {
            event.refuse(L"A name cannot contain any of \\ / : * ? \" < > |");
            return;
        }
        if (isReservedThemeName(stem))
            event.refuse(L"That name belongs to a built-in theme.");
    }

    std::wstring renameThemeFile(const std::filesystem::path& oldPath, AcceptEditEvent& event)
    {
        const std::wstring newStem{ event.text.plainText() };
        checkThemeNameShape(event, newStem);
        if (event.refused())
            return {};
        // Nothing to do is not a refusal - the user looked at the name and left it alone.
        if (newStem == oldPath.stem().wstring())
            return {};

        std::wstring newName = newStem;
        newName.append(k_themeFileExtension);
        std::filesystem::path newPath = oldPath;
        newPath.replace_filename(newName);

        std::error_code errorCode;
        // The name that is already taken may be this file's own: the stem comparison above is
        // case sensitive and the file system is not, so `dark` renamed to `Dark` reaches here
        // and finds itself. equivalent() asks whether the two paths name one file, which is the
        // question, and lets a change of capitalisation through.
        if (std::filesystem::exists(newPath, errorCode)
            && !std::filesystem::equivalent(oldPath, newPath, errorCode))
        {
            event.refuse(L"There is already a theme with that name.");
            return {};
        }

        errorCode.clear();
        std::filesystem::rename(oldPath, newPath, errorCode);
        if (errorCode)
        {
            event.refuse(L"The file could not be renamed.");
            return {};
        }

        return newName;
    }

    Text themeInQuestionText(const std::wstring_view subject)
    {
        Text result{};
        result << PushThemeColor{ InkWell::spotInk() } << subject << PopColor{};
        return result;
    }

    bool isReservedThemeName(const std::wstring_view stem)
    {
        return std::ranges::any_of(k_reservedThemeNames, [stem](const std::wstring_view reserved) {
            return std::ranges::equal(stem, reserved, [](const wchar_t alpha, const wchar_t beta) {
                return std::towlower(alpha) == std::towlower(beta);
            });
        });
    }

    // THE EXTENSION IS WHICH ROOT THE THEME STANDS UNDER, and it is all that tells them apart
    // here: a built-in page is named with the extension the compiled-in themes go by, a user
    // page with the one a theme file carries.
    std::wstring pagePathOfTheme(const std::wstring_view themePath)
    {
        if (themePath.empty())
            return {};
        std::wstring result{ Browser::ConfigNames::homePath };
        result.append(themePathName(themePath));
        result.append(themePathRoot(themePath) == ThemeRoots::builtIn
            ? k_builtInThemeExtension
            : k_themeFileExtension);
        return result;
    }

    std::wstring themePathOfPage(const std::wstring_view pagePath)
    {
        if (pagePath.empty())
            return {};
        const std::filesystem::path page{ pagePath };
        const std::wstring_view root = page.extension() == k_builtInThemeExtension
            ? ThemeRoots::builtIn
            : ThemeRoots::user;
        return themePathOf(root, page.stem().wstring());
    }
}
