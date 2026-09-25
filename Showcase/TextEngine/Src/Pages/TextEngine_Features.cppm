export module ClaFi.Showcase.TextEngine;

import ClaFi.Icons.Eye;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

namespace ClaFi::Showcase::TextEngine
{
    export Text buildTextEngineShowcase()
    {
        // Arguments the format string draws from, in the order it consumes them.
        int demoInt = 42;
        float demoFloat = 3.14159f;
        unsigned int demoHex = 255;
        Color injectedColor{ 0, 255, 128, 255 };

        // An icon in the flow is any callable taking a PaintIconEvent.
        const PaintIconFunc& myIconPainter = Icons::Eye::paint;

        // Yellow has no format tag: at the reading threshold on a light page the warm arc collapses
        // to brown, so the colour is carried by a shape instead of by a run of text.
        auto paintWarning = [](PaintIconEvent& event){
            event.canvas().fillCircle(event.iconCenter(), event.scaleF(5.0f),
                event.inkColor(InkWell::Yellow));
            };

        // A bullet: no width, so it draws into the indent it stands at rather than occupying it.
        auto paintBullet = [](PaintIconEvent& event){
            FloatPoint center = event.iconCenter();
            center.x -= event.scaleF(16.0f);
            event.canvas().fillCircle(center, event.scaleF(3.0f), event.accentRgb(InkGrade::Strongest));
            };

        return Text{ Fmt{
            // ==========================================
            // 1. ALIGNMENT & HEADINGS
            // ==========================================
            L"[center][title]ClaFi TextEngine[/title][n]"
            L"[subtitle]A comprehensive demonstration of all formatting features[/subtitle][n][n]"

            // ==========================================
            // 2. STRUCTURAL TAGS & TYPOGRAPHY
            // ==========================================
            L"[left][heading]1. Typography & Hierarchy[/heading][n]"
            L"[body]The engine supports predefined structural tags, such as [b]title[/b] and [b]subtitle[/b] used above, alongside:[/body][n]"
            L"[section]Section[/section] [subsection]SubSection[/subsection] [heading]Heading[/heading] "
            L"[subheading]SubHeading[/subheading] [body]Body[/body] [subbody]SubBody[/subbody] [code]Code[/code][n][n]"

            L"[heading]2. Inline Formatting & Fonts[/heading][n]"
            L"[body]Standard formats include [b]Bold[/b], [i]Italic[/i], and [b][i]Bold Italic[/i][/b].[/body][n]"
            L"[body]You can specify custom sizes like [size 24]Large[/size] or [size 10]Small[/size],[/body] "
            L"[body]and override the font family dynamically to [font Consolas]Consolas[/font].[/body][n][n]"

            // ==========================================
            // 3. COLOR SYSTEM
            // ==========================================
            L"[heading]3. Color System[/heading][n]"
            L"[body]Grades are steps between the surface and the ink the chain arrived at: "
            L"[color strongest]Strongest[/color], [color strong]Strong[/color] and [color muted]Muted[/color].[/body][n]"
            L"[body]The theme's two accents: [color accent]Accent[/color] is what the interface answers with, "
            L"[color spot]Spot[/color] is what stands apart from it.[/body][n]"
            L"[body]Semantic inks carry a meaning rather than emphasis: "
            L"[color green]Success[/color], [color blue]Information[/color], [color red]Error[/color].[/body][n]"
            L"[body]Yellow is drawn rather than read, so a shape carries it: [icon {} 14,14] warning.[/body][n]"
            L"[body]Every color takes a grade as well: [color accent muted]Muted Accent[/color], [color red strong]Strong Red[/color].[/body][n]"
            L"[b]Color[/b][tabto 80][b]Faint[/b][tabto 170][b]Subtle[/b][tabto 260][b]Muted[/b][tabto 350][b]Strong[/b][tabto 440][b]Strongest[/b][n]"
            L"Text[tabto 80][color text faint]Sample[/color][tabto 170][color text subtle]Sample[/color][tabto 260][color text muted]Sample[/color][tabto 350][color text strong]Sample[/color][tabto 440][color text strongest]Sample[/color][n]"
            L"Accent[tabto 80][color accent faint]Sample[/color][tabto 170][color accent subtle]Sample[/color][tabto 260][color accent muted]Sample[/color][tabto 350][color accent strong]Sample[/color][tabto 440][color accent strongest]Sample[/color][n]"
            L"Spot[tabto 80][color spot faint]Sample[/color][tabto 170][color spot subtle]Sample[/color][tabto 260][color spot muted]Sample[/color][tabto 350][color spot strong]Sample[/color][tabto 440][color spot strongest]Sample[/color][n]"
            L"Yellow[tabto 80][color yellow faint]Sample[/color][tabto 170][color yellow subtle]Sample[/color][tabto 260][color yellow muted]Sample[/color][tabto 350][color yellow strong]Sample[/color][tabto 440][color yellow strongest]Sample[/color][n]"
            L"Green[tabto 80][color green faint]Sample[/color][tabto 170][color green subtle]Sample[/color][tabto 260][color green muted]Sample[/color][tabto 350][color green strong]Sample[/color][tabto 440][color green strongest]Sample[/color][n]"
            L"Blue[tabto 80][color blue faint]Sample[/color][tabto 170][color blue subtle]Sample[/color][tabto 260][color blue muted]Sample[/color][tabto 350][color blue strong]Sample[/color][tabto 440][color blue strongest]Sample[/color][n]"
            L"Red[tabto 80][color red faint]Sample[/color][tabto 170][color red subtle]Sample[/color][tabto 260][color red muted]Sample[/color][tabto 350][color red strong]Sample[/color][tabto 440][color red strongest]Sample[/color][n]"
            L"Black[tabto 80][color black faint]Sample[/color][tabto 170][color black subtle]Sample[/color][tabto 260][color black muted]Sample[/color][tabto 350][color black strong]Sample[/color][tabto 440][color black strongest]Sample[/color][n]"
            L"White[tabto 80][color white faint]Sample[/color][tabto 170][color white subtle]Sample[/color][tabto 260][color white muted]Sample[/color][tabto 350][color white strong]Sample[/color][tabto 440][color white strongest]Sample[/color][n][n]"
            L"[body]A color stated outright is left alone by the theme: [color #FF0055]Hex Color[/color].[/body][n]"
            L"[body]...and one handed in as an argument: [color {}]Injected Argument Color[/color].[/body][n][n]"

            // ==========================================
            // 4. LAYOUT & SPACING
            // ==========================================
            L"[heading]4. Layout, Spacing & Alignment[/heading][n]"
            L"[left]Left alignment.[end]"
            L"[center]Center alignment.[end]"
            L"[right]Right alignment.[end]"
            L"[justified]Justified alignment spreads the text out to fill the available space, creating straight margins on both left and right edges. This typographic formatting alters the spacing between words and characters so that every line of text begins and ends at the same horizontal boundaries. In professional publishing and documentation, this layout creates a structured grid system. However, on wide displays or high-resolution monitors, short paragraphs can stretch or fail to wrap correctly, creating white space gaps known as 'rivers' throughout the block. To maintain rendering quality across wide display boundaries, the body text block requires sufficient length to fill multiple full rows.[/justified][n][n]"

            L"[left][body]Horizontal control:[space 40]40px fixed gap.[flex]Flex space pushes this to the right.[end]"
            L"[body]The same space under its other names:[space flex]space flex,[fill]and fill.[end]"
            L"[body]TabTo provides exact tabular layout:[/body][n]"
            L"[b]Component[/b][tabto 150][b]Status[/b][tabto 250][b]Complexity[/b][n]"
            L"Parser[tabto 150][color green]Active[/color][tabto 250]High[n]"
            L"Renderer[tabto 150][color muted]Standby[/color][tabto 250]Low[n][n]"

            L"[body]The [b]vspace[/b] tag is an inline element that defines the minimum height of a row.[/body][n]"
            L"This is a standard line of text.[n]"
            L"This line contains a [vspace 60] tag, forcing this row height to a minimum of 60px.[n]"
            L"If the specified vspace height is less than the natural text height, it has no effect.[n][n]"

            // ==========================================
            // 5. VARIABLE INJECTION
            // ==========================================
            L"[heading]5. Dynamic Variable Injection[/heading][n]"
            L"[body]Because Fmt executes over std::vformat, standard format specifiers are supported natively:[/body][n]"
            L"Integer: {}, Float: {:.2f}, Hex: 0x{:02X}[n][n]"

            // ==========================================
            // 6. IN-TEXT ICONS
            // ==========================================
            L"[heading]6. In-Text Icons[/heading][n]"
            L"[body]Icons align with text. Default size fallback: [icon {}].[/body][n]"
            L"[body]Custom dimensions (W,H,B): [icon {} 24,24,20] sizes the element relative to the layout.[/body][n][n]"

            // ==========================================
            // 7. LISTS & CUSTOM DRAWING
            // ==========================================
            L"[heading]7. Lists & Custom Drawing\n[/heading]"

            L"[indent 34]" // Push this entire block (including wrapped lines) to the right
            L"[icon {} 0,18][b]Lambda Drawables:[/b]"  L" [color muted]Inject C++ lambdas to render arbitrary graphics directly into the text flow.[/color]\n"
            L"[icon {} 0,18][b]Baseline Alignment:[/b]" L" [color muted]Icons and custom drawings share the baseline of the surrounding text block.[/color]\n"
            L"[icon {} 0,18][b]Reusability:[/b]"       L" [color muted]Pass the same lambda instance multiple times to generate repeating UI elements like list bullets.[/color]\n"
            L"[icon {} 0,18][b]Spacing Control:[/b]"   L" [color muted]Combine layout tags with icons to define structured list hierarchies.[/color]\n"

            L"[indent 0][n]" // Reset the indent back to normal for the following note

            // ==========================================
            // 8. LINKS
            // ==========================================
            L"[heading]8. Links[/heading][n]"
            L"[body]A [link https://example.com]link[/link] is drawn in the accent ink and underlined under the pointer. "
            L"A click follows it in a read-only box like this one, and a click with Ctrl held in a box the user types into.[/body][n]"
            L"[body]The framework opens [link https://example.com]https[/link] and [link mailto:someone@example.com]mailto[/link] targets itself. "
            L"Any other, such as [link clafi:ledger]this one[/link], goes to OnLinkClick and nowhere else.[/body][n]"
            L"[body]Copy a link from here and paste it into the Emoji page to try it where the text can be edited.[/body][n]"
            ,
            // One queue serves both syntaxes, so these stand in the order the string reaches them.
            paintWarning,       // Arg 1  - the yellow mark in section 3
            injectedColor,      // Arg 2  - [color {}]
            demoInt,            // Arg 3
            demoFloat,          // Arg 4
            demoHex,            // Arg 5
            myIconPainter,      // Arg 6
            myIconPainter,      // Arg 7

            // Bullets
            paintBullet,        // Arg 8
            paintBullet,        // Arg 9
            paintBullet,        // Arg 10
            paintBullet         // Arg 11
            } };
    }
}
