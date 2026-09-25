module ClaFi.Showcase.TextEngine.Math;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Showcase::TextEngine
{
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
        void paintRule(PaintIconEvent& event)
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
        void paintRadicalSign(PaintIconEvent& event)
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
        [[nodiscard]] PaintIconFunc vinculum(float width)
        {
            return [width](PaintIconEvent& event){
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
        [[nodiscard]] PaintIconFunc bracketPiece(BracketSide side, BracketPart part)
        {
            return [side, part](PaintIconEvent& event){
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

        // Everything a formula on a row of its own is set in. Stated as a pair, because the
        // family, the size and the alignment all stand until they are taken back.
        void openDisplay(Text& out)
        {
            out << TextAlign::Center;
            out << PushFontFamily{ std::wstring{ k_mathFamily } };
            out << PushFontSize{ k_displaySize };
        }

        // The blank row a block is followed by is closed here as well, so every display block is
        // parted from the prose under it by the same gap however its last row was written.
        void closeDisplay(Text& out)
        {
            out << PopFontSize{};
            out << PopFontFamily{};
            out << TextAlign::Left;
            out << TextOp::EndLine;
        }

        // Notation inside a sentence: the family the display rows are set in, at the size of the
        // prose around it.
        void writeInline(Text& out, const Text& notation)
        {
            out << PushFontFamily{ std::wstring{ k_mathFamily } };
            out << notation;
            out << PopFontFamily{};
        }

        // One row of a stack, and one row holding nothing but a rule. A stack is written out of
        // these rather than out of a fraction, because a fraction inside a fraction is not a
        // fraction holding another - it is five rows, and the rules across them are of two
        // different widths.
        void writeRow(Text& out, const Text& row)
        {
            out << row;
            out << TextOp::EndLine;
        }

        void writeRuleRow(Text& out, float ruleWidth)
        {
            out << InTextIcon{ ruleWidth, k_ruleHeight, k_ruleBaseline, paintRule };
            out << TextOp::EndLine;
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
            out << TextOp::EndLine;
        }

        // A root: the sign, the bar as wide as the page says the root reaches, and the radicand
        // written under it.
        void writeRoot(Text& out, const Text& radicand, float coverWidth)
        {
            out << InTextIcon{ k_radicalWidth, k_radicalHeight, k_radicalBaseline, paintRadicalSign };
            out << InTextIcon{ 0.0f, k_radicalHeight, k_radicalBaseline, vinculum(coverWidth) };
            out << radicand;
        }
    }

    Text buildMathShowcase()
    {
        Text page;

        page << Fmt{
            L"[center][title]Mathematical Notation[/title][n]"
            L"[subtitle]Formulas written down, not drawn[/subtitle][n][n]"
            L"[left][body]Every formula below is one Text. A raised run, a lowered run, a rule "
            L"drawn into the flow, a row centred against the row above it - the engine places all "
            L"of it, and none of it is a picture.[/body][n][n]"
        };

        // -----------------------------------------------------------------
        // 1. Scripts
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]1. Scripts[/heading][n]"
            L"[body]A run is raised by the [b]sup[/b] tag and lowered by [b]sub[/b], both written "
            L"in square brackets like every other tag. A level takes the size down and steps the "
            L"baseline by a share of the size it was entered at, so a script inside a script "
            L"shrinks and steps again. One instruction closes either: a closing script tag ends "
            L"the nearest script still open.[/body][n][n]"
        };

        openDisplay(page);
        page << Fmt{
            L"[i]a[/i][sup]2[/sup] + [i]b[/i][sup]2[/sup] = [i]c[/i][sup]2[/sup]"
            L"[space 44][i]x[/i][sub]i[/sub][sup]2[/sup] + [i]y[/i][sub]i[/sub][sup]2[/sup]"
            L"[space 44][i]e[/i][sup]−[i]x[/i][sup]2[/sup]/2[/sup]"
            L"[space 44]2[sup]2[sup]2[sup][i]n[/i][/sup][/sup][/sup][n]"
        };
        closeDisplay(page);

        page << Fmt{
            L"[body]A subscript beside a superscript is the case that decides how a shift has to "
            L"travel. The two runs are the same size in the same family, and text of one format "
            L"is drawn in one piece - so the shift is carried on the drawing effect, which is "
            L"what DirectWrite breaks a piece at.[/body][n]"
            L"[body]A line is not made taller for a script. A raised run keeps inside the ascent "
            L"of the run it stands on; a lowered one whose letters descend can reach below what "
            L"its line was measured at.[/body][n][n]"
        };

        // -----------------------------------------------------------------
        // 2. Fractions
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]2. Fractions[/heading][n]"
            L"[body]Nothing stacks on its own, so a fraction is written as three rows: the "
            L"numerator, a rule, and the denominator, each centred. The rule is an inline object "
            L"and is the only thing in its row, so the row is as tall as the rule and the three "
            L"close up.[/body][n][n]"
        };

        openDisplay(page);
        writeFraction(page,
            Text{ Fmt{ L"[i]a[/i] + [i]b[/i]" } },
            Text{ Fmt{ L"[i]c[/i] + [i]d[/i]" } },
            64.0f);
        closeDisplay(page);

        page << Fmt{
            L"[body]A fraction inside a fraction is not one holding another. It is five rows with "
            L"two rules across them, and the rules are of different widths because the inner one "
            L"reaches over less:[/body][n][n]"
        };

        openDisplay(page);
        writeRow(page, Text{ Fmt{ L"1" } });
        writeRuleRow(page, 46.0f);
        writeRow(page, Text{ Fmt{ L"1 + [i]x[/i]" } });
        writeRuleRow(page, 64.0f);
        writeRow(page, Text{ Fmt{ L"1 − [i]x[/i][sup]2[/sup]" } });
        closeDisplay(page);

        // -----------------------------------------------------------------
        // 3. Roots
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]3. Roots[/heading][n]"
            L"[body]A root is two drawn pieces. The sign has a width and takes its place in the "
            L"line; the bar has none at all and lies across the text written after it, the way a "
            L"list bullet draws into the indent it stands in rather than occupying it. How far "
            L"the bar reaches is stated by the page - a Text is written before anything has been "
            L"shaped, so there is nothing yet to measure.[/body][n][n]"
        };

        // The row carrying the rule carries the left hand side as well, so a space as wide as
        // that side is written after the rule: the row is then symmetric about the rule, and
        // centring the row centres the rule under the numerator above it.
        openDisplay(page);
        Text quadraticTop{ Fmt{ L"−[i]b[/i] ± " } };
        writeRoot(quadraticTop, Text{ Fmt{ L"[i]b[/i][sup]2[/sup] − 4[i]ac[/i]" } }, 76.0f);
        writeRow(page, quadraticTop);
        page << Fmt{ L"[i]x[/i] = " };
        page << InTextIcon{ 132.0f, k_ruleHeight, k_ruleBaseline, paintRule };
        page << Space{ 26.0f };
        page << TextOp::EndLine;
        writeRow(page, Text{ Fmt{ L"2[i]a[/i]" } });
        closeDisplay(page);

        // -----------------------------------------------------------------
        // 4. Operators that carry limits
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]4. Operators that carry limits[/heading][n]"
            L"[body]A limit written beside its operator is a script and needs nothing "
            L"else:[/body][n][n]"
        };

        openDisplay(page);
        page << Fmt{
            L"[size 26]∑[/size][sub][i]i[/i]=1[/sub][sup][i]n[/i][/sup] [i]i[/i] = "
            L"[i]n[/i]([i]n[/i] + 1) / 2"
            L"[space 40][size 26]∫[/size][sub]0[/sub][sup]∞[/sup] [i]e[/i][sup]−[i]x[/i][/sup] "
            L"d[i]x[/i] = 1[n]"
        };
        closeDisplay(page);

        page << Fmt{
            L"[body]A limit set over and under its operator is three rows again, and these rows "
            L"carry more than the stack - the operator has its summand beside it. Centring a row "
            L"would centre the whole of it, so the rows are left aligned and every column is a "
            L"stop the page names.[/body][n][n]"
        };

        page << PushFontFamily{ std::wstring{ k_mathFamily } };
        page << PushFontSize{ k_displaySize };
        page << Fmt{
            L"[space 96][i]n[/i][n]"
            L"[space 80][size 30]∑[/size][space 12][i]i[/i][sup]2[/sup] = "
            L"[i]n[/i]([i]n[/i] + 1)(2[i]n[/i] + 1) / 6[n]"
            L"[space 84][i]i[/i]=1[n][n]"
        };
        page << PopFontSize{};
        page << PopFontFamily{};

        // -----------------------------------------------------------------
        // 5. Matrices and determinants
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]5. Matrices and determinants[/heading][n]"
            L"[body]A bracket does not grow to fit what it encloses, so it is drawn one row at a "
            L"time: a top piece, middle pieces, a bottom piece. The stem of each runs the whole "
            L"height of its box and every box is the height of its row, so the pieces meet with "
            L"no seam. A determinant is the same construction with the arms left off.[/body][n][n]"
        };

        openDisplay(page);
        writeBracketedRow(page,
            Text{ Fmt{ L"[i]a[/i][sub]11[/sub][tabto 62][i]a[/i][sub]12[/sub][tabto 111][i]a[/i][sub]13[/sub]" } },
            BracketPart::Top, 139.0f);
        writeBracketedRow(page,
            Text{ Fmt{ L"[i]a[/i][sub]21[/sub][tabto 62][i]a[/i][sub]22[/sub][tabto 111][i]a[/i][sub]23[/sub]" } },
            BracketPart::Middle, 139.0f);
        writeBracketedRow(page,
            Text{ Fmt{ L"[i]a[/i][sub]31[/sub][tabto 62][i]a[/i][sub]32[/sub][tabto 111][i]a[/i][sub]33[/sub]" } },
            BracketPart::Bottom, 139.0f);
        page << TextOp::EndLine;
        writeBracketedRow(page, Text{ Fmt{ L"[i]a[/i][tabto 49][i]b[/i]" } },
            BracketPart::Middle, 65.0f);
        writeBracketedRow(page, Text{ Fmt{ L"[i]c[/i][tabto 49][i]d[/i]" } },
            BracketPart::Middle, 65.0f);
        closeDisplay(page);

        page << Fmt{
            L"[body]The second of those is a determinant, and it comes to [b]ad − bc[/b].[/body][n][n]"
        };

        // -----------------------------------------------------------------
        // 6. A definition by cases
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]6. A definition by cases[/heading][n]"
            L"[body]The same pieces with nothing on the right, and one row carrying the name. "
            L"Every row stops the bracket at the same column, so a row with a name in front of it "
            L"and a row with nothing in front of it put their piece in the same place.[/body][n][n]"
        };

        page << PushFontFamily{ std::wstring{ k_mathFamily } };
        page << PushFontSize{ k_displaySize };
        page << TabTo{ 96.0f };
        page << InTextIcon{ 7.0f, k_rowHeight, k_rowBaseline, bracketPiece(BracketSide::Left, BracketPart::Top) };
        page << Fmt{ L"[space 8]+1,[space 16][i]x[/i] > 0[n]" };
        page << Fmt{ L"sgn [i]x[/i] = " };
        page << TabTo{ 96.0f };
        page << InTextIcon{ 7.0f, k_rowHeight, k_rowBaseline, bracketPiece(BracketSide::Left, BracketPart::Middle) };
        page << Fmt{ L"[space 8]0,[space 16][i]x[/i] = 0[n]" };
        page << TabTo{ 96.0f };
        page << InTextIcon{ 7.0f, k_rowHeight, k_rowBaseline, bracketPiece(BracketSide::Left, BracketPart::Bottom) };
        page << Fmt{ L"[space 8]−1,[space 16][i]x[/i] < 0[n][n]" };
        page << PopFontSize{};
        page << PopFontFamily{};

        // -----------------------------------------------------------------
        // 7. A page of them
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]7. A page of them[/heading][n]"
            L"[body]Each of these is a name, a tab stop, and one line of notation - no rows, no "
            L"drawn pieces except the roots.[/body][n][n]"
        };

        page << Fmt{ L"[subbody]Euler's identity[/subbody]" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{ Fmt{ L"[i]e[/i][sup][i]i[/i]π[/sup] + 1 = 0" } });
        page << TextOp::EndLine;

        page << Fmt{ L"[subbody]The Gaussian integral[/subbody]" };
        page << TabTo{ 230.0f };
        Text gaussian{ Fmt{
            L"[size 22]∫[/size][sub]−∞[/sub][sup]∞[/sup] [i]e[/i][sup]−[i]x[/i][sup]2[/sup][/sup] "
            L"d[i]x[/i] = " } };
        writeRoot(gaussian, Text{ Fmt{ L"π" } }, 12.0f);
        writeInline(page, gaussian);
        page << TextOp::EndLine;

        page << Fmt{ L"[subbody]The binomial theorem[/subbody]" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{ Fmt{
            L"([i]x[/i] + [i]y[/i])[sup][i]n[/i][/sup] = [size 22]∑[/size][sub][i]k[/i]=0[/sub]"
            L"[sup][i]n[/i][/sup] [i]C[/i][sub][i]n[/i],[i]k[/i][/sub] "
            L"[i]x[/i][sup][i]k[/i][/sup] [i]y[/i][sup][i]n[/i]−[i]k[/i][/sup]" } });
        page << TextOp::EndLine;

        page << Fmt{ L"[subbody]The Fourier transform[/subbody]" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{ Fmt{
            L"[i]F[/i](ξ) = [size 22]∫[/size][sub]−∞[/sub][sup]∞[/sup] [i]f[/i]([i]x[/i]) "
            L"[i]e[/i][sup]−2π[i]ixξ[/i][/sup] d[i]x[/i]" } });
        page << TextOp::EndLine;

        page << Fmt{ L"[subbody]The Schrödinger equation[/subbody]" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{ Fmt{
            L"[i]i[/i]ħ ∂ψ / ∂[i]t[/i] = [i]H[/i]ψ" } });
        page << TextOp::EndLine;

        page << Fmt{ L"[subbody]The Stokes theorem[/subbody]" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{ Fmt{
            L"[size 22]∮[/size][sub]∂Σ[/sub] [i]F[/i] · d[i]r[/i] = [size 22]∬[/size][sub]Σ[/sub] "
            L"(∇ × [i]F[/i]) · d[i]Σ[/i]" } });
        page << TextOp::EndLine;

        page << Fmt{ L"[subbody]The Bayes rule[/subbody]" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{ Fmt{
            L"[i]P[/i]([i]A[/i] | [i]B[/i]) = [i]P[/i]([i]B[/i] | [i]A[/i]) [i]P[/i]([i]A[/i]) / "
            L"[i]P[/i]([i]B[/i])" } });
        page << TextOp::EndLine;

        page << Fmt{ L"[subbody]The Euler product[/subbody]" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{ Fmt{
            L"ζ([i]s[/i]) = [size 22]∑[/size][sub][i]n[/i]=1[/sub][sup]∞[/sup] 1 / "
            L"[i]n[/i][sup][i]s[/i][/sup] = [size 22]∏[/size][sub][i]p[/i][/sub] "
            L"(1 − [i]p[/i][sup]−[i]s[/i][/sup])[sup]−1[/sup]" } });
        page << TextOp::EndLine;

        page << Fmt{ L"[subbody]The Cauchy-Schwarz inequality[/subbody]" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{ Fmt{
            L"| ⟨[i]u[/i], [i]v[/i]⟩ | ≤ ‖[i]u[/i]‖ ‖[i]v[/i]‖" } });
        page << TextOp::EndLine;

        page << Fmt{ L"[subbody]The Taylor series[/subbody]" };
        page << TabTo{ 230.0f };
        writeInline(page, Text{ Fmt{
            L"[i]f[/i]([i]x[/i]) = [size 22]∑[/size][sub][i]n[/i]=0[/sub][sup]∞[/sup] "
            L"[i]f[/i][sup]([i]n[/i])[/sup]([i]a[/i]) / [i]n[/i]! · "
            L"([i]x[/i] − [i]a[/i])[sup][i]n[/i][/sup]" } });
        page << TextOp::EndLine;

        page << TextOp::EndLine;

        // -----------------------------------------------------------------
        // 8. A derivation
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]8. A derivation[/heading][n]"
            L"[body]Steps line up on one stop, which is what a tab stop is for: the equals signs "
            L"stand in a column whatever is written to the left of the first one.[/body][n][n]"
        };

        page << PushFontFamily{ std::wstring{ k_mathFamily } };
        page << PushFontSize{ k_displaySize };
        page << Fmt{
            L"([i]a[/i] + [i]b[/i])[sup]2[/sup][tabto 120]= ([i]a[/i] + [i]b[/i])"
            L"([i]a[/i] + [i]b[/i])[n]"
            L"[tabto 120]= [i]a[/i][sup]2[/sup] + [i]ab[/i] + [i]ba[/i] + [i]b[/i][sup]2[/sup][n]"
            L"[tabto 120]= [i]a[/i][sup]2[/sup] + 2[i]ab[/i] + [i]b[/i][sup]2[/sup][n][n]"
        };
        page << PopFontSize{};
        page << PopFontFamily{};

        // -----------------------------------------------------------------
        // 9. What the page states for itself
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]9. What the page states for itself[/heading][n]"
            L"[indent 34]"
            L"[b]Widths.[/b] A rule is as wide as it is told and a bar reaches as far as it is "
            L"told. A Text is written before it has been shaped, so the page has nothing to "
            L"measure and states the numbers.[n]"
            L"[b]Brackets.[/b] They are drawn a row at a time and do not grow. A block of a "
            L"different height is a different set of pieces.[n]"
            L"[b]Rows.[/b] A stack costs whole rows, so a fraction with a bar cannot stand inside "
            L"a sentence. Inside a sentence a fraction is written with a slash.[n]"
            L"[b]Lines.[/b] A script does not make its line taller, which is what keeps rows of "
            L"prose evenly spaced when one of them carries an exponent.[n]"
            L"[indent 0][n]"
            L"[body]One more: a format string reads a square bracket as the opening of a tag and "
            L"a brace as the opening of an argument, so notation wanting either is written "
            L"straight into the text instead of through a format string. The line below is "
            L"streamed in, braces and all:[/body][n][n]"
        };

        page << TextAlign::Center;
        page << PushFontFamily{ std::wstring{ k_mathFamily } };
        page << PushFontSize{ k_displaySize };
        page << L"S = { x ∈ ℝ : x > 0 }";
        page << TextOp::EndLine;
        page << PopFontSize{};
        page << PopFontFamily{};
        page << TextAlign::Left;

        return page;
    }
}
