export module TextEngine_App;

import ClaFi.Icons.Eye;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace TextEngine_App
{
    using namespace ::ClaFi;

    export Text buildTextEngineShowcase()
    {
        // The values section 5 writes.
        int demoInt = 42;
        float demoFloat = 3.14159f;
        unsigned int demoHex = 255;
        Color injectedColor{ 0, 255, 128, 255 };

        // An icon in the flow is any callable taking a DrawIconEvent.
        const DrawIconFunc& myIconPainter = Icons::Eye::draw;

        // Yellow is not set as a run of text: at the reading threshold on a light page the warm arc
        // collapses to brown, so the colour is carried by a shape instead.
        auto paintWarning = [](DrawIconEvent& event){
            event.canvas().fillCircle(event.iconCenter(), event.scaleF(5.0f),
                event.inkRgb(InkWell::Yellow));
            };

        // A bullet: no width, so it draws into the indent it stands at rather than occupying it.
        auto drawBullet = [](DrawIconEvent& event){
            FloatPoint center = event.iconCenter();
            center.x -= event.scaleF(16.0f);
            event.canvas().fillCircle(center, event.scaleF(3.0f), event.accentRgb(InkGrade::Strongest));
            };

        // One row of the ink table: the ink's name and the colour it is drawn from.
        struct InkRow
        {
            std::wstring_view name;
            InkColor color;
        };

        // One column of the ink table: the grade, its name and the stop the column starts at.
        struct GradeColumn
        {
            std::wstring_view name;
            InkGrade grade;
            float stop;
        };

        constexpr std::array k_inkRows{
            InkRow{ L"Text", InkColor::Text },
            InkRow{ L"Accent", InkColor::Accent },
            InkRow{ L"Spot", InkColor::Spot },
            InkRow{ L"Yellow", InkColor::Yellow },
            InkRow{ L"Green", InkColor::Green },
            InkRow{ L"Blue", InkColor::Blue },
            InkRow{ L"Red", InkColor::Red },
            InkRow{ L"Black", InkColor::Black },
            InkRow{ L"White", InkColor::White },
        };

        constexpr std::array k_gradeColumns{
            GradeColumn{ L"Faint", InkGrade::Faint, 80.0f },
            GradeColumn{ L"Subtle", InkGrade::Subtle, 170.0f },
            GradeColumn{ L"Muted", InkGrade::Muted, 260.0f },
            GradeColumn{ L"Strong", InkGrade::Strong, 350.0f },
            GradeColumn{ L"Strongest", InkGrade::Strongest, 440.0f },
        };

        Text text{};

        // ==========================================
        // 1. **Alignment & headings**
        // ==========================================
        text << TextAlign::Center;
        text << StyledText{ TextStyles::Title, L"ClaFi TextEngine" } << k_endLine;
        text << StyledText{ TextStyles::SubTitle, L"A comprehensive demonstration of all formatting features" };
        text << k_endLine;
        text << k_endLine; // the blank row under the subtitle is centred with it
        text << TextAlign::Left;

        // ==========================================
        // 2. **Text styles & typography**
        // ==========================================
        text << StyledText{ TextStyles::Heading, L"1. Typography & Hierarchy" } << k_endLine;
        text << L"The engine supports predefined text styles, such as " << BoldText{ L"title" };
        text << L" and " << BoldText{ L"subtitle" } << L" used above, alongside:" << k_endLine;
        text << StyledText{ TextStyles::Section, L"Section" } << L" ";
        text << StyledText{ TextStyles::SubSection, L"SubSection" } << L" ";
        text << StyledText{ TextStyles::Heading, L"Heading" } << L" ";
        text << StyledText{ TextStyles::SubHeading, L"SubHeading" } << L" ";
        text << StyledText{ TextStyles::Body, L"Body" } << L" ";
        text << StyledText{ TextStyles::SubBody, L"SubBody" } << L" ";
        text << StyledText{ TextStyles::Code, L"Code" } << k_endLine << k_endLine;

        text << StyledText{ TextStyles::Heading, L"2. Inline Formatting & Fonts" } << k_endLine;
        text << L"Standard formats include " << BoldText{ L"Bold" } << L", " << ItalicText{ L"Italic" };
        text << L", and " << BoldText{ ItalicText{ L"Bold Italic" } } << L"." << k_endLine;
        text << L"You can specify custom sizes like " << SizedText{ 24.0f, L"Large" };
        text << L" or " << SizedText{ 10.0f, L"Small" } << L", ";
        text << L"and override the font family dynamically to " << FamilyText{ L"Consolas", L"Consolas" };
        text << L"." << k_endLine << k_endLine;

        // ==========================================
        // 3. **Color system**
        // ==========================================
        text << StyledText{ TextStyles::Heading, L"3. Color System" } << k_endLine;
        text << L"Grades are steps between the surface and the ink the chain arrived at: ";
        text << ColoredText{ InkGrade::Strongest, L"Strongest" } << L", ";
        text << ColoredText{ InkGrade::Strong, L"Strong" } << L" and ";
        text << ColoredText{ InkGrade::Muted, L"Muted" } << L"." << k_endLine;
        text << L"The theme's two accents: " << ColoredText{ InkColor::Accent, L"Accent" };
        text << L" is what the interface answers with, " << ColoredText{ InkColor::Spot, L"Spot" };
        text << L" is what stands apart from it." << k_endLine;
        text << L"Semantic inks carry a meaning rather than emphasis: ";
        text << ColoredText{ InkColor::Green, L"Success" } << L", ";
        text << ColoredText{ InkColor::Blue, L"Information" } << L", ";
        text << ColoredText{ InkColor::Red, L"Error" } << L"." << k_endLine;
        text << L"Yellow is drawn rather than read, so a shape carries it: ";
        text << InTextIcon{ 14.0f, 14.0f, paintWarning } << L" warning." << k_endLine;
        text << L"Every color takes a grade as well: ";
        text << ColoredText{ InkWell::accentInk(InkGrade::Muted), L"Muted Accent" } << L", ";
        text << ColoredText{ InkWell::redInk(InkGrade::Strong), L"Strong Red" } << L"." << k_endLine;

        text << BoldText{ L"Color" };
        for (const GradeColumn& column : k_gradeColumns)
            text << TabTo{ column.stop } << BoldText{ column.name };
        text << k_endLine;
        for (const InkRow& row : k_inkRows)
        {
            text << row.name;
            for (const GradeColumn& column : k_gradeColumns)
            {
                Ink ink = InkWell::textInk(column.grade);
                ink.color = row.color;
                text << TabTo{ column.stop } << ColoredText{ ink, L"Sample" };
            }
            text << k_endLine;
        }
        text << k_endLine;

        text << L"A color stated outright is left alone by the theme: ";
        text << ColoredText{ Color{ 0xFF, 0x00, 0x55 }, L"Hex Color" } << L"." << k_endLine;
        text << L"...and one held in a variable: ";
        text << ColoredText{ injectedColor, L"Injected Argument Color" } << L"." << k_endLine << k_endLine;

        // ==========================================
        // 4. **Layout & spacing**
        // ==========================================
        text << StyledText{ TextStyles::Heading, L"4. Layout, Spacing & Alignment" } << k_endLine;
        text << TextAlign::Left << L"Left alignment." << k_endLine;
        text << TextAlign::Center << L"Center alignment." << k_endLine;
        text << TextAlign::Right << L"Right alignment." << k_endLine;
        text << TextAlign::Justified;
        text << L"Justified alignment spreads the text out to fill the available space, creating straight margins on both left and right edges. This typographic formatting alters the spacing between words and characters so that every line of text begins and ends at the same horizontal boundaries. In professional publishing and documentation, this layout creates a structured grid system. However, on wide displays or high-resolution monitors, short paragraphs can stretch or fail to wrap correctly, creating white space gaps known as 'rivers' throughout the block. To maintain rendering quality across wide display boundaries, the body text block requires sufficient length to fill multiple full rows.";
        text << k_endLine;
        text << k_endLine; // the blank row after the block is justified with it
        text << TextAlign::Left;

        text << L"Horizontal control:" << Space{ 40.0f } << L"40px fixed gap." << FlexSpace{};
        text << L"Flex space pushes this to the right." << k_endLine;
        text << L"Two flex spaces in one line share what is left over:" << FlexSpace{} << L"one,";
        text << FlexSpace{} << L"and two." << k_endLine;
        text << L"TabTo provides exact tabular layout:" << k_endLine;
        text << BoldText{ L"Component" } << TabTo{ 150.0f } << BoldText{ L"Status" };
        text << TabTo{ 250.0f } << BoldText{ L"Complexity" } << k_endLine;
        text << L"Parser" << TabTo{ 150.0f } << ColoredText{ InkColor::Green, L"Active" };
        text << TabTo{ 250.0f } << L"High" << k_endLine;
        text << L"Renderer" << TabTo{ 150.0f } << ColoredText{ InkGrade::Muted, L"Standby" };
        text << TabTo{ 250.0f } << L"Low" << k_endLine << k_endLine;

        text << L"A " << BoldText{ L"VSpace" };
        text << L" is an inline element that defines the minimum height of a row." << k_endLine;
        text << L"This is a standard line of text." << k_endLine;
        text << L"This line contains a " << VSpace{ 60.0f };
        text << L" VSpace, forcing this row height to a minimum of 60px." << k_endLine;
        text << L"If the specified VSpace height is less than the natural text height, it has no effect.";
        text << k_endLine << k_endLine;

        // ==========================================
        // 5. **Numbers**
        // ==========================================
        text << StyledText{ TextStyles::Heading, L"5. Numbers" } << k_endLine;
        text << L"Numbers are written with std::format, so its format specifiers apply:" << k_endLine;
        text << std::format(L"Integer: {}, Float: {:.2f}, Hex: 0x{:02X}", demoInt, demoFloat, demoHex);
        text << k_endLine << k_endLine;

        // ==========================================
        // 6. **In-text icons**
        // ==========================================
        text << StyledText{ TextStyles::Heading, L"6. In-Text Icons" } << k_endLine;
        text << L"Icons align with text. A width alone makes a square: ";
        text << InTextIcon{ 16.0f, myIconPainter } << L"." << k_endLine;
        text << L"Custom dimensions (W,H,B): " << InTextIcon{ 24.0f, 24.0f, 20.0f, myIconPainter };
        text << L" sizes the element relative to the layout." << k_endLine << k_endLine;

        // ==========================================
        // 7. **Lists & custom drawing**
        // ==========================================
        text << StyledText{ TextStyles::Heading, L"7. Lists & Custom Drawing\n" };

        const InTextIcon bullet{ 0.0f, 18.0f, drawBullet };
        Text list{};
        list << bullet << BoldText{ L"Lambda Drawables:" } << L" ";
        list << ColoredText{
            InkGrade::Muted,
            L"Inject C++ lambdas to render arbitrary graphics directly into the text flow."
        } << k_endLine;
        list << bullet << BoldText{ L"Baseline Alignment:" } << L" ";
        list << ColoredText{
            InkGrade::Muted,
            L"Icons and custom drawings share the baseline of the surrounding text block."
        } << k_endLine;
        list << bullet << BoldText{ L"Reusability:" } << L" ";
        list << ColoredText{
            InkGrade::Muted,
            L"Pass the same lambda instance multiple times to generate repeating UI elements like list bullets."
        } << k_endLine;
        list << bullet << BoldText{ L"Spacing Control:" } << L" ";
        list << ColoredText{
            InkGrade::Muted,
            L"Combine layout tags with icons to define structured list hierarchies."
        } << k_endLine;
        // The indent holds for every line of the list, the wrapped ones included, until it is
        // stated again.
        text << TextIndent{ 34.0f } << list;
        text << TextIndent{ 0.0f } << k_endLine;

        // ==========================================
        // 8. **Links**
        // ==========================================
        text << StyledText{ TextStyles::Heading, L"8. Links" } << k_endLine;
        text << L"A " << LinkText{ L"https://example.com", L"link" };
        text << L" is drawn in the accent ink and underlined under the pointer. ";
        text << L"A click follows it in a read-only box like this one, and a click with Ctrl held in a box the user types into.";
        text << k_endLine;
        text << L"The framework opens " << LinkText{ L"https://example.com", L"https" } << L" and ";
        text << LinkText{ L"mailto:someone@example.com", L"mailto" } << L" targets itself. ";
        text << L"Any other, such as " << LinkText{ L"clafi:ledger", L"this one" };
        text << L", goes to OnLinkClick and nowhere else." << k_endLine;
        text << L"Copy a link from here and paste it into the Emoji page to try it where the text can be edited.";
        text << k_endLine;
        return text;
    }
}
