module TextEngine_App.Math;

import ClaFi.Core.Context.DrawIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace TextEngine_App
{
    using namespace ::ClaFi;

    namespace
    {
        // Cambria for the notation: it carries a real italic face, which is what a variable is
        // set in, and DirectWrite falls back on its own for the few symbols it does not hold.
        // Naming a math family instead would buy those symbols and lose the italic.
        constexpr std::wstring_view k_mathFamily = L"Cambria";

        // What a formula standing on a row of its own is set at, a step above the prose that
        // introduces it.
        constexpr float k_displaySize = 17.0f;

        // A stacked row - a numerator, a denominator, a matrix row - is as tall as the tallest
        // thing in it, and a drawn piece is a thing in it. These are what the pieces are cut to,
        // so a bracket tiles into one unbroken stroke down the side of a matrix and a fraction
        // closes up to its bar.
        constexpr float k_ruleHeight = 5.0f;
        constexpr float k_ruleBaseline = 4.0f;
        constexpr float k_rowHeight = 22.0f;
        constexpr float k_rowBaseline = 16.0f;

        // The root sign and the bar over what it covers are two pieces meeting at a corner, so
        // both are cut to one height and one baseline. The baseline is what puts the bar above
        // the radicand, and the tallest thing a radicand holds is a superscript - the bar clears
        // one rather than resting on it.
        constexpr float k_radicalHeight = 18.0f;
        constexpr float k_radicalBaseline = 17.0f;
        constexpr float k_radicalWidth = 9.0f;
        constexpr float k_radicalStroke = 1.25f;

        // -----------------------------------------------------------------
        // The pieces a formula is drawn from
        // -----------------------------------------------------------------

        // A rule struck across the whole of the box it is given. Nothing else stands in the row a
        // fraction bar is written on, so the row is the height of this box.
        void drawRule(DrawIconEvent& event)
        {
            const FloatRect& box = event.iconRect();
            const float thickness = event.scaleF(1.25f);
            const float middle = box.center().y;
            event.canvas().fillRectangle(
                { box.left, middle - thickness * 0.5f, box.right, middle + thickness * 0.5f },
                event.textRgb(InkGrade::Strongest));
        }

        // The sign of a root: the short stroke down, the rise, and the arm it turns into. The arm
        // is laid the way the vinculum after it is laid - a fill hanging off the top of the box,
        // not a stroke centred on it - so the two are one bar rather than two a half stroke apart.
        // The rise ends at the middle of that bar, where a turn should meet it.
        void drawRadicalSign(DrawIconEvent& event)
        {
            const FloatRect& box = event.iconRect();
            const float stroke = event.scaleF(k_radicalStroke);
            const Color ink = event.textRgb(InkGrade::Strongest);
            const float width = box.right - box.left;
            const float height = box.bottom - box.top;
            const float armLeft = box.left + width * 0.72f;

            event.canvas().drawLine(
                { box.left, box.top + height * 0.62f },
                { box.left + width * 0.34f, box.bottom }, ink, stroke);
            event.canvas().drawLine(
                { box.left + width * 0.34f, box.bottom },
                { armLeft, box.top + stroke * 0.5f }, ink, stroke);
            event.canvas().fillRectangle({ armLeft, box.top, box.right, box.top + stroke }, ink);
        }

        // The bar of a root, drawn over the text that follows it. It has no width of its own, so
        // the radicand is not pushed along by it - the bar lies across what comes next. How far it
        // reaches is the page's own statement, because a Text is written before anything has been
        // shaped and there is nothing here to measure.
        [[nodiscard]] DrawIconFunc vinculum(float width)
        {
            return [width](DrawIconEvent& event){
                const FloatRect& box = event.iconRect();
                const float stroke = event.scaleF(k_radicalStroke);
                event.canvas().fillRectangle(
                    { box.left, box.top, box.left + event.scaleF(width), box.top + stroke },
                    event.textRgb(InkGrade::Strongest));
            };
        }

        enum class BracketSide
        {
            Left,
            Right
        };

        enum class BracketPart
        {
            Top,
            Middle,
            Bottom
        };

        // One row's worth of a bracket. The stem runs the whole height of the box, so the pieces
        // of the rows above and below meet it with no seam; the arm is what tells a top piece from
        // a middle one. A Middle piece on its own is the plain vertical bar a determinant is
        // written between.
        [[nodiscard]] DrawIconFunc bracketPiece(BracketSide side, BracketPart part)
        {
            return [side, part](DrawIconEvent& event){
                const FloatRect& box = event.iconRect();
                const float stroke = event.scaleF(1.25f);
                const Color ink = event.textRgb(InkGrade::Strongest);
                const float stem = side == BracketSide::Left ? box.left : box.right - stroke;

                event.canvas().fillRectangle({ stem, box.top, stem + stroke, box.bottom }, ink);

                if (part == BracketPart::Middle)
                    return;

                const float arm = part == BracketPart::Top ? box.top : box.bottom - stroke;
                event.canvas().fillRectangle({ box.left, arm, box.right, arm + stroke }, ink);
            };
        }

        // -----------------------------------------------------------------
        // The arrangements they are written into
        // -----------------------------------------------------------------

        // Rows standing on their own: set in the notation's family a step above the prose and
        // centred, then the blank row that parts every display block from the prose under it.
        void writeDisplay(Text& out, const Text& rows)
        {
            out << TextAlign::Center;
            out << FamilyText{ std::wstring{ k_mathFamily }, SizedText{ k_displaySize, rows } };
            out << TextAlign::Left;
            out << k_endLine;
        }

        // Rows set as a display is, standing where the page's stops put them rather than centred.
        void writeStoppedDisplay(Text& out, const Text& rows)
        {
            out << FamilyText{ std::wstring{ k_mathFamily }, SizedText{ k_displaySize, rows } };
        }

        // Notation inside a sentence: the family the display rows are set in, at the size of the
        // prose around it.
        void writeInline(Text& out, const Text& notation)
        {
            out << FamilyText{ std::wstring{ k_mathFamily }, notation };
        }

        // One row of a stack, and one row holding nothing but a rule. A stack is written out of
        // these rather than out of a fraction, because a fraction inside a fraction is not a
        // fraction holding another - it is five rows, and the rules across them are of two
        // different widths.
        void writeRow(Text& out, const Text& row)
        {
            out << row;
            out << k_endLine;
        }

        void writeRuleRow(Text& out, float ruleWidth)
        {
            out << InTextIcon{ ruleWidth, k_ruleHeight, k_ruleBaseline, drawRule };
            out << k_endLine;
        }

        // A fraction takes three rows: what is over the bar, the bar, and what is under it. The
        // two sides are centred rather than aligned to the bar, so the rule is written as wide as
        // the wider of them and the narrower one sits in the middle of it.
        void writeFraction(Text& out, const Text& numerator, const Text& denominator, float ruleWidth)
        {
            writeRow(out, numerator);
            writeRuleRow(out, ruleWidth);
            writeRow(out, denominator);
        }

        // One row of a bracketed block: a piece of the left bracket, the row, a tab stop, and a
        // piece of the right. Every row is stopped at the same column, so every row comes to the
        // same width and centring puts the columns of one under the columns of the next.
        void writeBracketedRow(Text& out, const Text& row, BracketPart part, float rightEdge)
        {
            out << InTextIcon{ 7.0f, k_rowHeight, k_rowBaseline, bracketPiece(BracketSide::Left, part) };
            out << Space{ 6.0f };
            out << row;
            out << TabTo{ rightEdge };
            out << InTextIcon{ 7.0f, k_rowHeight, k_rowBaseline, bracketPiece(BracketSide::Right, part) };
            out << k_endLine;
        }

        // A root: the sign, the bar as wide as the page says the root reaches, and the radicand
        // written under it.
        void writeRoot(Text& out, const Text& radicand, float coverWidth)
        {
            out << InTextIcon{ k_radicalWidth, k_radicalHeight, k_radicalBaseline, drawRadicalSign };
            out << InTextIcon{ 0.0f, k_radicalHeight, k_radicalBaseline, vinculum(coverWidth) };
            out << radicand;
        }
    }

    Text buildMathShowcase()
    {
        Text page;

        page << TextAlign::Center;
        page << StyledText{ TextStyles::Title, L"Mathematical Notation" } << k_endLine;
        page << StyledText{ TextStyles::SubTitle, L"Formulas written down, not drawn" } << k_endLine;
        page << k_endLine; // the blank row under the subtitle is centred with it
        page << TextAlign::Left;
        page << L"Every formula below is one Text. A raised run, a lowered run, a rule "
            L"drawn into the flow, a row centred against the row above it - the engine places all "
            L"of it, and none of it is a picture.";
        page << k_endLine << k_endLine;

        // -----------------------------------------------------------------
        // 1. Scripts
        // -----------------------------------------------------------------

        page << StyledText{ TextStyles::Heading, L"1. Scripts" } << k_endLine;
        page << L"A run is raised by " << BoldText{ L"SuperscriptText" } << L" and lowered by ";
        page << BoldText{ L"SubscriptText" } << L". A level takes the size down and steps the "
            L"baseline by a share of the size it was entered at, so a script inside a script "
            L"shrinks and steps again.";
        page << k_endLine << k_endLine;

        Text scripts{};
        scripts << ItalicText{ L"a" } << SuperscriptText{ L"2" } << L" + ";
        scripts << ItalicText{ L"b" } << SuperscriptText{ L"2" } << L" = ";
        scripts << ItalicText{ L"c" } << SuperscriptText{ L"2" };
        scripts << Space{ 44.0f };
        scripts << ItalicText{ L"x" } << SubscriptText{ L"i" } << SuperscriptText{ L"2" } << L" + ";
        scripts << ItalicText{ L"y" } << SubscriptText{ L"i" } << SuperscriptText{ L"2" };
        scripts << Space{ 44.0f };
        scripts << ItalicText{ L"e" };
        scripts << SuperscriptText{ L"−", ItalicText{ L"x" }, SuperscriptText{ L"2" }, L"/2" };
        scripts << Space{ 44.0f };
        scripts << L"2" << SuperscriptText{
            L"2",
            SuperscriptText{ L"2", SuperscriptText{ ItalicText{ L"n" } } }
        };
        scripts << k_endLine;
        writeDisplay(page, scripts);

        page << L"A subscript beside a superscript is the case that decides how a shift has to "
            L"travel. The two runs are the same size in the same family, and text of one format "
            L"is drawn in one piece - so the shift is carried on the drawing effect, which is "
            L"what DirectWrite breaks a piece at.";
        page << k_endLine;
        page << L"A line is not made taller for a script. A raised run keeps inside the ascent "
            L"of the run it stands on; a lowered one whose letters descend can reach below what "
            L"its line was measured at.";
        page << k_endLine << k_endLine;

        // -----------------------------------------------------------------
        // 2. Fractions
        // -----------------------------------------------------------------

        page << StyledText{ TextStyles::Heading, L"2. Fractions" } << k_endLine;
        page << L"Nothing stacks on its own, so a fraction is written as three rows: the "
            L"numerator, a rule, and the denominator, each centred. The rule is an inline object "
            L"and is the only thing in its row, so the row is as tall as the rule and the three "
            L"close up.";
        page << k_endLine << k_endLine;

        Text fraction{};
        writeFraction(fraction,
            Text{ ItalicText{ L"a" }, L" + ", ItalicText{ L"b" } },
            Text{ ItalicText{ L"c" }, L" + ", ItalicText{ L"d" } },
            64.0f);
        writeDisplay(page, fraction);

        page << L"A fraction inside a fraction is not one holding another. It is five rows with "
            L"two rules across them, and the rules are of different widths because the inner one "
            L"reaches over less:";
        page << k_endLine << k_endLine;

        Text nestedFraction{};
        writeRow(nestedFraction, Text{ L"1" });
        writeRuleRow(nestedFraction, 46.0f);
        writeRow(nestedFraction, Text{ L"1 + ", ItalicText{ L"x" } });
        writeRuleRow(nestedFraction, 64.0f);
        writeRow(nestedFraction, Text{ L"1 − ", ItalicText{ L"x" }, SuperscriptText{ L"2" } });
        writeDisplay(page, nestedFraction);

        // -----------------------------------------------------------------
        // 3. Roots
        // -----------------------------------------------------------------

        page << StyledText{ TextStyles::Heading, L"3. Roots" } << k_endLine;
        page << L"A root is two drawn pieces. The sign has a width and takes its place in the "
            L"line; the bar has none at all and lies across the text written after it, the way a "
            L"list bullet draws into the indent it stands in rather than occupying it. How far "
            L"the bar reaches is stated by the page - a Text is written before anything has been "
            L"shaped, so there is nothing yet to measure.";
        page << k_endLine << k_endLine;

        // The row carrying the rule carries the left hand side as well, so a space as wide as
        // that side is written after the rule: the row is then symmetric about the rule, and
        // centring the row centres the rule under the numerator above it.
        Text quadratic{};
        Text quadraticTop{ L"−", ItalicText{ L"b" }, L" ± " };
        writeRoot(quadraticTop,
            Text{ ItalicText{ L"b" }, SuperscriptText{ L"2" }, L" − 4", ItalicText{ L"ac" } },
            76.0f);
        writeRow(quadratic, quadraticTop);
        quadratic << ItalicText{ L"x" } << L" = ";
        quadratic << InTextIcon{ 132.0f, k_ruleHeight, k_ruleBaseline, drawRule };
        quadratic << Space{ 26.0f };
        quadratic << k_endLine;
        writeRow(quadratic, Text{ L"2", ItalicText{ L"a" } });
        writeDisplay(page, quadratic);

        // -----------------------------------------------------------------
        // 4. Operators that carry limits
        // -----------------------------------------------------------------

        page << StyledText{ TextStyles::Heading, L"4. Operators that carry limits" } << k_endLine;
        page << L"A limit written beside its operator is a script and needs nothing else:";
        page << k_endLine << k_endLine;

        Text besideLimits{};
        besideLimits << SizedText{ 26.0f, L"∑" };
        besideLimits << SubscriptText{ ItalicText{ L"i" }, L"=1" };
        besideLimits << SuperscriptText{ ItalicText{ L"n" } };
        besideLimits << L" " << ItalicText{ L"i" } << L" = " << ItalicText{ L"n" } << L"(";
        besideLimits << ItalicText{ L"n" } << L" + 1) / 2";
        besideLimits << Space{ 40.0f };
        besideLimits << SizedText{ 26.0f, L"∫" } << SubscriptText{ L"0" } << SuperscriptText{ L"∞" };
        besideLimits << L" " << ItalicText{ L"e" } << SuperscriptText{ L"−", ItalicText{ L"x" } };
        besideLimits << L" d" << ItalicText{ L"x" } << L" = 1";
        besideLimits << k_endLine;
        writeDisplay(page, besideLimits);

        page << L"A limit set over and under its operator is three rows again, and these rows "
            L"carry more than the stack - the operator has its summand beside it. Centring a row "
            L"would centre the whole of it, so the rows are left aligned and every column is a "
            L"stop the page names.";
        page << k_endLine << k_endLine;

        Text stackedLimits{};
        stackedLimits << Space{ 96.0f } << ItalicText{ L"n" } << k_endLine;
        stackedLimits << Space{ 80.0f } << SizedText{ 30.0f, L"∑" } << Space{ 12.0f };
        stackedLimits << ItalicText{ L"i" } << SuperscriptText{ L"2" } << L" = " << ItalicText{ L"n" };
        stackedLimits << L"(" << ItalicText{ L"n" } << L" + 1)(2" << ItalicText{ L"n" } << L" + 1) / 6";
        stackedLimits << k_endLine;
        stackedLimits << Space{ 84.0f } << ItalicText{ L"i" } << L"=1" << k_endLine << k_endLine;
        writeStoppedDisplay(page, stackedLimits);

        // -----------------------------------------------------------------
        // 5. Matrices and determinants
        // -----------------------------------------------------------------

        page << StyledText{ TextStyles::Heading, L"5. Matrices and determinants" } << k_endLine;
        page << L"A bracket does not grow to fit what it encloses, so it is drawn one row at a "
            L"time: a top piece, middle pieces, a bottom piece. The stem of each runs the whole "
            L"height of its box and every box is the height of its row, so the pieces meet with "
            L"no seam. A determinant is the same construction with the arms left off.";
        page << k_endLine << k_endLine;

        Text matrices{};
        writeBracketedRow(matrices,
            Text{
                ItalicText{ L"a" }, SubscriptText{ L"11" }, TabTo{ 62.0f },
                ItalicText{ L"a" }, SubscriptText{ L"12" }, TabTo{ 111.0f },
                ItalicText{ L"a" }, SubscriptText{ L"13" }
            },
            BracketPart::Top, 139.0f);
        writeBracketedRow(matrices,
            Text{
                ItalicText{ L"a" }, SubscriptText{ L"21" }, TabTo{ 62.0f },
                ItalicText{ L"a" }, SubscriptText{ L"22" }, TabTo{ 111.0f },
                ItalicText{ L"a" }, SubscriptText{ L"23" }
            },
            BracketPart::Middle, 139.0f);
        writeBracketedRow(matrices,
            Text{
                ItalicText{ L"a" }, SubscriptText{ L"31" }, TabTo{ 62.0f },
                ItalicText{ L"a" }, SubscriptText{ L"32" }, TabTo{ 111.0f },
                ItalicText{ L"a" }, SubscriptText{ L"33" }
            },
            BracketPart::Bottom, 139.0f);
        matrices << k_endLine;
        writeBracketedRow(matrices, Text{ ItalicText{ L"a" }, TabTo{ 49.0f }, ItalicText{ L"b" } },
            BracketPart::Middle, 65.0f);
        writeBracketedRow(matrices, Text{ ItalicText{ L"c" }, TabTo{ 49.0f }, ItalicText{ L"d" } },
            BracketPart::Middle, 65.0f);
        writeDisplay(page, matrices);

        page << L"The second of those is a determinant, and it comes to " << BoldText{ L"ad − bc" };
        page << L"." << k_endLine << k_endLine;

        // -----------------------------------------------------------------
        // 6. A definition by cases
        // -----------------------------------------------------------------

        page << StyledText{ TextStyles::Heading, L"6. A definition by cases" } << k_endLine;
        page << L"The same pieces with nothing on the right, and one row carrying the name. "
            L"Every row stops the bracket at the same column, so a row with a name in front of it "
            L"and a row with nothing in front of it put their piece in the same place.";
        page << k_endLine << k_endLine;

        Text cases{};
        cases << TabTo{ 96.0f };
        cases << InTextIcon{ 7.0f, k_rowHeight, k_rowBaseline, bracketPiece(BracketSide::Left, BracketPart::Top) };
        cases << Space{ 8.0f } << L"+1," << Space{ 16.0f } << ItalicText{ L"x" } << L" > 0" << k_endLine;
        cases << L"sgn " << ItalicText{ L"x" } << L" = ";
        cases << TabTo{ 96.0f };
        cases << InTextIcon{ 7.0f, k_rowHeight, k_rowBaseline, bracketPiece(BracketSide::Left, BracketPart::Middle) };
        cases << Space{ 8.0f } << L"0," << Space{ 16.0f } << ItalicText{ L"x" } << L" = 0" << k_endLine;
        cases << TabTo{ 96.0f };
        cases << InTextIcon{ 7.0f, k_rowHeight, k_rowBaseline, bracketPiece(BracketSide::Left, BracketPart::Bottom) };
        cases << Space{ 8.0f } << L"−1," << Space{ 16.0f } << ItalicText{ L"x" } << L" < 0";
        cases << k_endLine << k_endLine;
        writeStoppedDisplay(page, cases);

        // -----------------------------------------------------------------
        // 7. A page of them
        // -----------------------------------------------------------------

        page << StyledText{ TextStyles::Heading, L"7. A page of them" } << k_endLine;
        page << L"Each of these is a name, a tab stop, and one line of notation - no rows, no "
            L"drawn pieces except the roots.";
        page << k_endLine << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"Euler's identity" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{
            ItalicText{ L"e" }, SuperscriptText{ ItalicText{ L"i" }, L"π" }, L" + 1 = 0"
        });
        page << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"The Gaussian integral" };
        page << TabTo{ 230.0f };
        Text gaussian{
            SizedText{ 22.0f, L"∫" }, SubscriptText{ L"−∞" }, SuperscriptText{ L"∞" }, L" ",
            ItalicText{ L"e" },
            SuperscriptText{ L"−", ItalicText{ L"x" }, SuperscriptText{ L"2" } },
            L" d", ItalicText{ L"x" }, L" = "
        };
        writeRoot(gaussian, Text{ L"π" }, 12.0f);
        writeInline(page, gaussian);
        page << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"The binomial theorem" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{
            L"(", ItalicText{ L"x" }, L" + ", ItalicText{ L"y" }, L")",
            SuperscriptText{ ItalicText{ L"n" } }, L" = ",
            SizedText{ 22.0f, L"∑" },
            SubscriptText{ ItalicText{ L"k" }, L"=0" },
            SuperscriptText{ ItalicText{ L"n" } }, L" ",
            ItalicText{ L"C" }, SubscriptText{ ItalicText{ L"n" }, L",", ItalicText{ L"k" } }, L" ",
            ItalicText{ L"x" }, SuperscriptText{ ItalicText{ L"k" } }, L" ",
            ItalicText{ L"y" }, SuperscriptText{ ItalicText{ L"n" }, L"−", ItalicText{ L"k" } }
        });
        page << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"The Fourier transform" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{
            ItalicText{ L"F" }, L"(ξ) = ",
            SizedText{ 22.0f, L"∫" }, SubscriptText{ L"−∞" }, SuperscriptText{ L"∞" }, L" ",
            ItalicText{ L"f" }, L"(", ItalicText{ L"x" }, L") ",
            ItalicText{ L"e" }, SuperscriptText{ L"−2π", ItalicText{ L"ixξ" } },
            L" d", ItalicText{ L"x" }
        });
        page << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"The Schrödinger equation" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{
            ItalicText{ L"i" }, L"ħ ∂ψ / ∂", ItalicText{ L"t" }, L" = ", ItalicText{ L"H" }, L"ψ"
        });
        page << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"The Stokes theorem" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{
            SizedText{ 22.0f, L"∮" }, SubscriptText{ L"∂Σ" }, L" ",
            ItalicText{ L"F" }, L" · d", ItalicText{ L"r" }, L" = ",
            SizedText{ 22.0f, L"∬" }, SubscriptText{ L"Σ" },
            L" (∇ × ", ItalicText{ L"F" }, L") · d", ItalicText{ L"Σ" }
        });
        page << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"The Bayes rule" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{
            ItalicText{ L"P" }, L"(", ItalicText{ L"A" }, L" | ", ItalicText{ L"B" }, L") = ",
            ItalicText{ L"P" }, L"(", ItalicText{ L"B" }, L" | ", ItalicText{ L"A" }, L") ",
            ItalicText{ L"P" }, L"(", ItalicText{ L"A" }, L") / ",
            ItalicText{ L"P" }, L"(", ItalicText{ L"B" }, L")"
        });
        page << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"The Euler product" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{
            L"ζ(", ItalicText{ L"s" }, L") = ",
            SizedText{ 22.0f, L"∑" }, SubscriptText{ ItalicText{ L"n" }, L"=1" },
            SuperscriptText{ L"∞" }, L" 1 / ", ItalicText{ L"n" }, SuperscriptText{ ItalicText{ L"s" } },
            L" = ", SizedText{ 22.0f, L"∏" }, SubscriptText{ ItalicText{ L"p" } },
            L" (1 − ", ItalicText{ L"p" }, SuperscriptText{ L"−", ItalicText{ L"s" } }, L")",
            SuperscriptText{ L"−1" }
        });
        page << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"The Cauchy-Schwarz inequality" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{
            L"| ⟨", ItalicText{ L"u" }, L", ", ItalicText{ L"v" }, L"⟩ | ≤ ‖",
            ItalicText{ L"u" }, L"‖ ‖", ItalicText{ L"v" }, L"‖"
        });
        page << k_endLine;

        page << StyledText{ TextStyles::SubBody, L"The Taylor series" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{
            ItalicText{ L"f" }, L"(", ItalicText{ L"x" }, L") = ",
            SizedText{ 22.0f, L"∑" }, SubscriptText{ ItalicText{ L"n" }, L"=0" },
            SuperscriptText{ L"∞" }, L" ",
            ItalicText{ L"f" }, SuperscriptText{ L"(", ItalicText{ L"n" }, L")" },
            L"(", ItalicText{ L"a" }, L") / ", ItalicText{ L"n" }, L"! · (",
            ItalicText{ L"x" }, L" − ", ItalicText{ L"a" }, L")",
            SuperscriptText{ ItalicText{ L"n" } }
        });
        page << k_endLine;

        page << k_endLine;

        // -----------------------------------------------------------------
        // 8. A derivation
        // -----------------------------------------------------------------

        page << StyledText{ TextStyles::Heading, L"8. A derivation" } << k_endLine;
        page << L"Steps line up on one stop, which is what a tab stop is for: the equals signs "
            L"stand in a column whatever is written to the left of the first one.";
        page << k_endLine << k_endLine;

        Text derivation{};
        derivation << L"(" << ItalicText{ L"a" } << L" + " << ItalicText{ L"b" } << L")";
        derivation << SuperscriptText{ L"2" } << TabTo{ 120.0f } << L"= (";
        derivation << ItalicText{ L"a" } << L" + " << ItalicText{ L"b" } << L")(";
        derivation << ItalicText{ L"a" } << L" + " << ItalicText{ L"b" } << L")" << k_endLine;
        derivation << TabTo{ 120.0f } << L"= " << ItalicText{ L"a" } << SuperscriptText{ L"2" };
        derivation << L" + " << ItalicText{ L"ab" } << L" + " << ItalicText{ L"ba" } << L" + ";
        derivation << ItalicText{ L"b" } << SuperscriptText{ L"2" } << k_endLine;
        derivation << TabTo{ 120.0f } << L"= " << ItalicText{ L"a" } << SuperscriptText{ L"2" };
        derivation << L" + 2" << ItalicText{ L"ab" } << L" + " << ItalicText{ L"b" };
        derivation << SuperscriptText{ L"2" } << k_endLine << k_endLine;
        writeStoppedDisplay(page, derivation);

        // -----------------------------------------------------------------
        // 9. What the page states for itself
        // -----------------------------------------------------------------

        page << StyledText{ TextStyles::Heading, L"9. What the page states for itself" } << k_endLine;
        page << TextIndent{ 34.0f } << Text{
            BoldText{ L"Widths." },
            L" A rule is as wide as it is told and a bar reaches as far as it is "
            L"told. A Text is written before it has been shaped, so the page has nothing to "
            L"measure and states the numbers.\n",
            BoldText{ L"Brackets." },
            L" They are drawn a row at a time and do not grow. A block of a "
            L"different height is a different set of pieces.\n",
            BoldText{ L"Rows." },
            L" A stack costs whole rows, so a fraction with a bar cannot stand inside "
            L"a sentence. Inside a sentence a fraction is written with a slash.\n",
            BoldText{ L"Lines." },
            L" A script does not make its line taller, which is what keeps rows of "
            L"prose evenly spaced when one of them carries an exponent.\n"
        };
        page << TextIndent{ 0.0f };
        page << k_endLine;
        page << L"One more: the words of a Text are taken as they are written, so notation holding "
            L"square brackets and braces needs nothing said for it. The line below is written "
            L"in, braces and all:";
        page << k_endLine << k_endLine;

        page << TextAlign::Center;
        page << FamilyText{
            std::wstring{ k_mathFamily },
            SizedText{ k_displaySize, L"S = { x ∈ ℝ : x > 0 }" }
        };

        return page;
    }
}
