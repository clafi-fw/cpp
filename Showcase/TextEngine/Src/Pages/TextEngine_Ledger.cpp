module ClaFi.Showcase.TextEngine.Rich;

import ClaFi.Core.Context.PaintIconEvent;
import ClaFi.Core.Context.FormContext;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Showcase::TextEngine
{
    namespace
    {
        // THE CAST. Five lists of PRIME length, each stepped by a stride of its own, so the tuple
        // a clue is written from repeats only after the product of the five - 323,323 clues, where
        // a round spends nine. A page of any length anyone will scroll holds no two alike.
        constexpr std::wstring_view k_persons[] = {
            L"Adele", L"Bruno", L"Clara", L"Dmitri", L"Elif", L"Farouk", L"Greta", L"Hideo",
            L"Ines", L"Jonas", L"Kira", L"Lars", L"Mira", L"Noor", L"Otto", L"Pia", L"Quentin"
        };

        constexpr std::wstring_view k_occupations[] = {
            L"archivist", L"botanist", L"cartographer", L"dentist", L"engraver", L"falconer",
            L"glassblower", L"herbalist", L"illustrator", L"jeweller", L"luthier", L"mason",
            L"notary"
        };

        // Without the article, which a column has no room for and a sentence writes itself. Every
        // one of them starts with a consonant, so the article prose writes is always "a".
        constexpr std::wstring_view k_items[] = {
            L"brass compass", L"folded map", L"tin whistle", L"wax seal", L"pocket loom",
            L"glass float", L"bone comb", L"paper crane", L"copper key", L"slate tile",
            L"silver thimble"
        };

        constexpr std::wstring_view k_states[] = {
            L"asleep", L"packing", L"fasting", L"humming", L"waiting", L"counting", L"whistling"
        };

        constexpr std::wstring_view k_rooms[] = {
            L"the blue landing", L"the west stair", L"the copper kitchen", L"the long gallery",
            L"the seed room", L"the clock closet", L"the north cellar", L"the glass porch",
            L"the map room", L"the dovecote", L"the salt pantry", L"the red study",
            L"the linen press", L"the bell tower", L"the drying loft", L"the fern court",
            L"the cold larder", L"the paper store", L"the lantern walk"
        };

        static_assert(std::size(k_persons) == 17);
        static_assert(std::size(k_occupations) == 13);
        static_assert(std::size(k_items) == 11);
        static_assert(std::size(k_states) == 7);
        static_assert(std::size(k_rooms) == 19);

        [[nodiscard]] std::wstring_view personFor(std::size_t clue)
        {
            return k_persons[(clue * 5) % std::size(k_persons)];
        }

        [[nodiscard]] std::wstring_view occupationFor(std::size_t clue)
        {
            return k_occupations[(clue * 3) % std::size(k_occupations)];
        }

        [[nodiscard]] std::wstring_view itemFor(std::size_t clue)
        {
            return k_items[(clue * 7) % std::size(k_items)];
        }

        [[nodiscard]] std::wstring_view stateFor(std::size_t clue)
        {
            return k_states[(clue * 2) % std::size(k_states)];
        }

        [[nodiscard]] std::wstring_view roomFor(std::size_t clue)
        {
            return k_rooms[(clue * 11) % std::size(k_rooms)];
        }

        // THE PAGE'S TYPE. Two voices, each set in one family, and a line kind stays in its voice
        // from the first round to the last: two lines of one kind are told apart by what they
        // say, never by what they are set in. The prose - the title, the headings, the clues, the
        // asides and the running text - is set in a serif. The apparatus a solver keeps beside it -
        // the lists, the grid, the tallies and the column tables - stays in the body family the
        // style set states, which is a sans. Both are proportional: what a line comes to depends
        // on which glyphs are in it, so a break point cannot be counted in characters and each
        // run has to be placed to find one, and the two break differently for the same words.
        constexpr std::wstring_view k_proseFamily = L"Georgia";

        // The size the prose is set in. A clue is set one size below the body so that its bold
        // number, its italic object and its coloured state read as marks inside a sentence rather
        // than as a heading each.
        constexpr float k_proseSize = 13.5f;

        // How far a clue in the list is pushed in. An indent belongs to a paragraph and stands
        // until another paragraph states a different one, so every line here states the one it
        // wants rather than a list stating it once for the lines inside it.
        constexpr float k_bulletIndent = 34.0f;

        // A bullet drawn into the indent its line stands at rather than into a width of its own.
        // A free function rather than a lambda: it is written into tens of thousands of paragraphs,
        // and a function pointer is what a PaintIconFunc holds without allocating.
        void paintBullet(PaintIconEvent& event)
        {
            FloatPoint center = event.iconCenter();
            center.x -= event.scaleF(16.0f);
            event.canvas().fillCircle(center, event.scaleF(3.0f), event.accentRgb(InkGrade::Strongest));
        }

        void paintCheck(PaintIconEvent& event)
        {
            FloatPoint center = event.iconCenter();
            center.x -= event.scaleF(16.0f);
            event.canvas().fillCircle(center, event.scaleF(3.0f), event.inkColor(InkWell::Green));
        }

        // The rule that closes a round: a hairline running in from each side of the box, and at the
        // middle of them a lozenge between two dots. The one mark on the page that is drawn rather
        // than typed, and the reason it is an icon: an icon states a design size and is scaled with
        // the text it stands under, where a glyph is whatever the family in force on that line
        // happens to have.
        //
        // Every measure is taken off the box the engine placed, so the ornament finds the middle of
        // the page the way the line it stands on does - the icon is written into a centred
        // paragraph, and the box arrives where that paragraph put it. The box is taller than the
        // drawing, so the air above and below the rule is the icon's own and costs no blank line.
        void paintOrnament(PaintIconEvent& event)
        {
            const FloatRect& box = event.iconRect();
            const FloatPoint center = box.center();
            const float gap = event.scaleF(13.0f);
            const Color ruleColor = event.textRgb(InkGrade::Subtle);
            const Color motifColor = event.accentRgb(InkGrade::Strong);
            const auto stroke = event.scaledStrokeWidth(Thickness::Thin);
            event.canvas().drawLine({ box.left, center.y }, { center.x - gap, center.y }, ruleColor, stroke);
            event.canvas().drawLine({ center.x + gap, center.y }, { box.right, center.y }, ruleColor, stroke);
            event.canvas().fillCircle({ center.x - gap, center.y }, event.scaleF(1.5f), motifColor);
            event.canvas().fillCircle({ center.x + gap, center.y }, event.scaleF(1.5f), motifColor);
            event.canvas().fillEllipse(center, event.scaleF(5.0f), event.scaleF(2.5f), motifColor);
        }

        // The number a clue is known by, written the way the text refers to it everywhere else.
        void writeClueNumber(Text& out, std::size_t clue)
        {
            out << L"Clue ";
            out << static_cast<int>(clue);
        }

        // The name a clue's anchor goes by, which a reference to the clue links to after a #.
        [[nodiscard]] std::wstring clueAnchor(std::size_t clue)
        {
            return L"clue-" + std::to_wstring(clue);
        }

        // A clue's number where the clue is stated, held by the anchor its references land on.
        void writeClueStatement(Text& out, std::size_t clue)
        {
            out << PushAnchor{ clueAnchor(clue) };
            writeClueNumber(out, clue);
            out << PopAnchor{};
        }

        // A clue's number where another line refers to it: a link to where the clue is stated, or
        // the number alone for a clue the ledger never reaches - a forward reference out of the
        // last round.
        void writeClueReference(Text& out, std::size_t clue, std::size_t lastClue)
        {
            if (clue == 0 || clue > lastClue)
            {
                writeClueNumber(out, clue);
                return;
            }
            out << PushLink{ L"#" + clueAnchor(clue) };
            writeClueNumber(out, clue);
            out << PopLink{};
        }

        // ---------------------------------------------------------------------------
        // The repeating part: a round of clues.
        // ---------------------------------------------------------------------------

        void writeRoundHeading(Text& out, std::size_t round, std::size_t line)
        {
            out << SetIndent{ 0.0f };
            out << TextStyleId::Heading;
            out << PushFontFamily{ std::wstring{ k_proseFamily } };
            out << L"Round ";
            out << static_cast<int>(round);
            out << L".";
            out << static_cast<int>(line);
            out << L" - what ";
            out << roomFor(round + line);
            out << L" gives away";
            out << PopFontFamily{};
            out << PopTextStyle{};
            out << L'\n';
        }

        // A clue as prose, long enough to break across lines. Six shapes, so a reader scrolling
        // past a hundred of them sees sentences rather than a filled-in template.
        void writeClue(Text& out, std::size_t clue, std::size_t lastClue)
        {
            out << SetIndent{ 0.0f };
            out << PushFontFamily{ std::wstring{ k_proseFamily } };
            out << PushFontSize{ k_proseSize };
            out << TextOp::PushBold;
            writeClueStatement(out, clue);
            out << L".";
            out << TextOp::PopBold;
            out << L" ";

            switch (clue % 6)
            {
                case 0:
                    out << L"The ";
                    out << occupationFor(clue);
                    out << L" who was seen carrying ";
                    out << TextOp::PushItalic;
                    out << L"a ";
                    out << itemFor(clue);
                    out << TextOp::PopItalic;
                    out << L" cannot have been the one found ";
                    out << InkWell::Blue;
                    out << stateFor(clue);
                    out << PopColor{};
                    out << L" in ";
                    out << roomFor(clue);
                    out << L", because the room was locked from the side nobody could reach without passing ";
                    out << TextOp::PushBold;
                    out << personFor(clue + 1);
                    out << TextOp::PopBold;
                    out << L".";
                    break;
                case 1:
                    out << TextOp::PushBold;
                    out << personFor(clue);
                    out << TextOp::PopBold;
                    out << L" stands exactly two doors from whoever was ";
                    out << InkWell::Blue;
                    out << stateFor(clue + 3);
                    out << PopColor{};
                    out << L", and neither of them is the ";
                    out << occupationFor(clue + 2);
                    out << L", who spent the whole of that hour in ";
                    out << roomFor(clue + 5);
                    out << L" with ";
                    out << TextOp::PushItalic;
                    out << L"a ";
                    out << itemFor(clue + 1);
                    out << TextOp::PopItalic;
                    out << L" in plain view.";
                    break;
                case 2:
                    out << L"Whoever holds ";
                    out << TextOp::PushItalic;
                    out << L"a ";
                    out << itemFor(clue);
                    out << TextOp::PopItalic;
                    out << L" answers to neither ";
                    out << TextOp::PushBold;
                    out << personFor(clue + 2);
                    out << TextOp::PopBold;
                    out << L" nor ";
                    out << TextOp::PushBold;
                    out << personFor(clue + 9);
                    out << TextOp::PopBold;
                    out << L", and was ";
                    out << InkWell::Blue;
                    out << stateFor(clue + 1);
                    out << PopColor{};
                    out << L" at the hour ";
                    out << roomFor(clue + 2);
                    out << L" was swept, which the ";
                    out << occupationFor(clue + 4);
                    out << L" swears to.";
                    break;
                case 3:
                    out << L"If ";
                    out << TextOp::PushBold;
                    out << personFor(clue);
                    out << TextOp::PopBold;
                    out << L" was ";
                    out << InkWell::Blue;
                    out << stateFor(clue);
                    out << PopColor{};
                    out << L", then ";
                    out << TextOp::PushItalic;
                    out << L"a ";
                    out << itemFor(clue + 3);
                    out << TextOp::PopItalic;
                    out << L" was left in ";
                    out << roomFor(clue + 1);
                    out << L" and the ";
                    out << occupationFor(clue + 1);
                    out << L" was not; if she was not, the same holds of ";
                    out << roomFor(clue + 7);
                    out << L" instead, and the ";
                    out << occupationFor(clue + 6);
                    out << L" says as much.";
                    break;
                case 4:
                    out << L"Between ";
                    out << roomFor(clue);
                    out << L" and ";
                    out << roomFor(clue + 4);
                    out << L" there is one person ";
                    out << InkWell::Blue;
                    out << stateFor(clue + 5);
                    out << PopColor{};
                    out << L" and one carrying ";
                    out << TextOp::PushItalic;
                    out << L"a ";
                    out << itemFor(clue + 2);
                    out << TextOp::PopItalic;
                    out << L", and they are not the same person - ";
                    out << TextOp::PushBold;
                    out << personFor(clue + 4);
                    out << TextOp::PopBold;
                    out << L" was counted in both places by the ";
                    out << occupationFor(clue + 3);
                    out << L", which cannot stand.";
                    break;
                default:
                    out << L"The one who was ";
                    out << InkWell::Blue;
                    out << stateFor(clue);
                    out << PopColor{};
                    out << L" in ";
                    out << roomFor(clue + 3);
                    out << L" gave ";
                    out << TextOp::PushItalic;
                    out << L"a ";
                    out << itemFor(clue + 5);
                    out << TextOp::PopItalic;
                    out << L" to ";
                    out << TextOp::PushBold;
                    out << personFor(clue + 6);
                    out << TextOp::PopBold;
                    out << L", who is not the ";
                    out << occupationFor(clue);
                    out << L" and never was, whatever ";
                    writeClueReference(out, clue > 3 ? clue - 3 : clue + 3, lastClue);
                    out << L" is taken to mean.";
                    break;
            }

            out << PopFontSize{};
            out << PopFontFamily{};
            out << L'\n';
        }

        // A clue as a list item: a bullet standing in the indent, the number in bold, the body
        // muted. Four shapes, shorter than the prose ones and still long enough to wrap.
        void writeClueBullet(Text& out, std::size_t clue)
        {
            out << SetIndent{ k_bulletIndent };
            out << InTextIcon{ 0.0f, 18.0f, paintBullet };
            out << TextOp::PushBold;
            writeClueStatement(out, clue);
            out << L":";
            out << TextOp::PopBold;
            out << L" ";
            out << InkWell::textInk(InkGrade::Muted);

            switch (clue % 4)
            {
                case 0:
                    out << L"nobody ";
                    out << stateFor(clue);
                    out << L" was ever in ";
                    out << roomFor(clue);
                    out << L" while ";
                    out << L"a ";
                    out << itemFor(clue + 1);
                    out << L" was on the table, and the ";
                    out << occupationFor(clue);
                    out << L" was the one who set the table.";
                    break;
                case 1:
                    out << personFor(clue);
                    out << L" is not the ";
                    out << occupationFor(clue + 5);
                    out << L", and whoever is was ";
                    out << stateFor(clue + 4);
                    out << L" two rooms along from ";
                    out << roomFor(clue + 6);
                    out << L".";
                    break;
                case 2:
                    out << L"a ";
                    out << itemFor(clue);
                    out << L" and ";
                    out << L"a ";
                    out << itemFor(clue + 4);
                    out << L" were never in one room, and neither was in ";
                    out << roomFor(clue + 2);
                    out << L", which is where ";
                    out << personFor(clue + 3);
                    out << L" was ";
                    out << stateFor(clue + 2);
                    out << L".";
                    break;
                default:
                    out << L"the ";
                    out << occupationFor(clue);
                    out << L" left ";
                    out << roomFor(clue + 8);
                    out << L" before ";
                    out << personFor(clue + 7);
                    out << L" arrived, carrying ";
                    out << L"a ";
                    out << itemFor(clue + 3);
                    out << L" and saying nothing at all.";
                    break;
            }

            out << PopColor{};
            out << L'\n';
        }

        // A row of the grid a solver keeps beside the clues: who, what, where, and how they were
        // found, on tab stops rather than in a sentence.
        //
        // A stop states a DESIGN x and is scaled with everything else, so the columns are sized
        // for the narrowest box this page is read in, and the cells drop the articles the prose
        // keeps - an item is "brass compass" in a column and "a brass compass" in a sentence.
        //
        // TWO WAYS A ROW CAN OUTGROW ITS COLUMNS. Wider than the box, it breaks at a stop: an
        // inline object states a neutral break condition and the character one stands on is a
        // contingent break, so half a row goes to a line of its own rather than being lost. Wider
        // than the gap to the next stop, it GLUES: a stop already behind the pen resolves to no
        // width at all, and the next column starts where this one ended.
        //
        // A row states an EARLIER clue rather than the one the round has reached, and takes its
        // distance from the line it stands on - so a run of rows walks back through the clues just
        // written instead of saying one of them over and over.
        void writeGridRow(Text& out, std::size_t reached, std::size_t line, std::size_t lastClue)
        {
            const std::size_t clue = reached > line ? reached - line : reached + line;
            const bool settled = (clue % 3) != 0;
            const Ink markInk = settled ? InkWell::Green : InkWell::Red;
            const std::wstring_view mark = settled ? L"fixed" : L"open";

            out << SetIndent{ 0.0f };
            out << personFor(clue);
            out << TabTo{ 95.0f };
            out << occupationFor(clue);
            out << TabTo{ 210.0f };
            out << itemFor(clue);
            out << TabTo{ 320.0f };
            out << markInk;
            out << mark;
            out << PopColor{};
            out << TabTo{ 365.0f };
            out << InkWell::textInk(InkGrade::Muted);
            out << L"by ";
            writeClueReference(out, clue, lastClue);
            out << PopColor{};
            out << L'\n';
        }

        // What the round came to, pushed to the right of the line by the flex space.
        void writeRoundTally(Text& out, std::size_t round, std::size_t clue, std::size_t lastClue)
        {
            out << SetIndent{ 0.0f };
            out << InkWell::textInk(InkGrade::Muted);
            out << L"Round ";
            out << static_cast<int>(round);
            out << L" leaves ";
            out << static_cast<int>(3 + (clue % 5));
            out << L" of the ninety-nine rooms unaccounted for";
            out << PopColor{};
            out << FlexSpace{};
            out << InkWell::accentInk();
            out << L"carried forward to ";
            writeClueReference(out, clue + 1, lastClue);
            out << PopColor{};
            out << L'\n';

            // The rule stands on a line of its own, and that line is centred: an icon states a
            // design width, and the alignment of the line it stands on states where that width
            // sits in the box. The alignment goes back to the left for what follows, since a
            // paragraph keeps the one in force when its newline is reached.
            out << TextAlign::Center;
            out << L'\n';
            out << InTextIcon{ 220.0f, 24.0f, paintOrnament };
            out << L'\n';
            out << TextAlign::Left;
            out << L'\n';
        }

        // An aside about the clues rather than a clue: the voice the puzzle keeps for itself.
        void writeAside(Text& out, std::size_t clue, std::size_t lastClue)
        {
            out << SetIndent{ 0.0f };
            out << PushFontFamily{ std::wstring{ k_proseFamily } };
            out << PushFontSize{ k_proseSize };
            out << InkWell::textInk(InkGrade::Muted);
            out << TextOp::PushItalic;

            switch (clue % 3)
            {
                case 0:
                    out << L"Note: ";
                    writeClueReference(out, clue, lastClue);
                    out << L" is the only one so far to name two rooms and no person, which is why it is read after ";
                    writeClueReference(out, clue > 11 ? clue - 11 : clue + 11, lastClue);
                    out << L" and not before it.";
                    break;
                case 1:
                    out << L"Note: the four clues above agree about ";
                    out << personFor(clue);
                    out << L" and disagree about ";
                    out << L"a ";
                    out << itemFor(clue);
                    out << L". The disagreement is the point of them.";
                    break;
                default:
                    out << L"Note: whoever was ";
                    out << stateFor(clue);
                    out << L" is named nowhere in this round, and is not meant to be.";
                    break;
            }

            out << TextOp::PopItalic;
            out << PopColor{};
            out << PopFontSize{};
            out << PopFontFamily{};
            out << L'\n';
        }

        // What one line of a round is. A round is a table of these rather than one string with
        // markers written into it: the line count has to be exact for a round page length to come
        // out round, and a table is the shape that can be counted at compile time.
        enum class LineKind
        {
            Heading,
            Clue,
            ClueBullet,
            GridRow,
            Tally,
            Aside,
            Blank
        };

        // One round, in the proportions the puzzle reads in: clues in the majority, a list and a
        // grid inside it, an aside where a solver would stop and mutter. A round ends in prose so
        // that the heading the next one opens with does not stand against one of this one's.
        constexpr LineKind k_roundLines[] = {
            LineKind::Heading,
            LineKind::Blank,
            LineKind::Clue,
            LineKind::Clue,
            LineKind::Blank,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::Blank,
            LineKind::Aside,
            LineKind::Blank,
            LineKind::GridRow,
            LineKind::GridRow,
            LineKind::GridRow,
            LineKind::GridRow,
            LineKind::Blank,
            LineKind::Clue,
            LineKind::Tally,
            LineKind::Blank,
            LineKind::Heading,
            LineKind::Blank,
            LineKind::Clue,
            LineKind::Clue,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::Blank,
            LineKind::Clue,
            LineKind::GridRow,
            LineKind::GridRow,
            LineKind::GridRow,
            LineKind::GridRow,
            LineKind::GridRow,
            LineKind::Blank,
            LineKind::Aside,
            LineKind::Blank,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::ClueBullet,
            LineKind::Blank,
            LineKind::Tally,
            LineKind::Clue,
            LineKind::Clue,
            LineKind::Blank
        };

        // A round block, so a round line count comes out exact. A line added here without a line
        // taken away puts the page past the count it was asked for by the whole of the last round.
        constexpr std::size_t k_roundLineCount = std::size(k_roundLines);
        static_assert(k_roundLineCount == 50, "A round block keeps a round line count exact.");

        // How many clues a round states, which is what lets the last clue be known before the
        // first round is written.
        constexpr std::size_t k_cluesPerRound = static_cast<std::size_t>(std::ranges::count_if(
            k_roundLines, [](LineKind kind){
                return kind == LineKind::Clue || kind == LineKind::ClueBullet;
            }));

        // The clue number runs across the whole document rather than restarting per round, which
        // is what lets a clue in round nine hundred refer to one in round four. lastClue is the
        // last one the ledger will state, and nothing links past it.
        void appendRound(Text& out, std::size_t round, std::size_t& clue, std::size_t lastClue)
        {
            for (std::size_t line = 0; line != k_roundLineCount; ++line)
            {
                switch (k_roundLines[line])
                {
                    case LineKind::Heading:
                        writeRoundHeading(out, round, line);
                        break;
                    case LineKind::Clue:
                        writeClue(out, ++clue, lastClue);
                        break;
                    case LineKind::ClueBullet:
                        writeClueBullet(out, ++clue);
                        break;
                    case LineKind::GridRow:
                        writeGridRow(out, clue, line, lastClue);
                        break;
                    case LineKind::Tally:
                        writeRoundTally(out, round, clue, lastClue);
                        break;
                    case LineKind::Aside:
                        writeAside(out, clue, lastClue);
                        break;
                    case LineKind::Blank:
                        out << L'\n';
                        break;
                }
            }
        }

        // ---------------------------------------------------------------------------
        // The two blocks that are written once. Both are built line by line rather than from a
        // table, and both count the lines they wrote: they are the parts whose length changes when
        // the words do, and nothing may guess it on their behalf.
        // ---------------------------------------------------------------------------

        std::size_t appendOpening(Text& out)
        {
            std::size_t lines = 0;
            auto endLine = [&out, &lines](){
                out << L'\n';
                ++lines;
                };

            out << PushFontFamily{ std::wstring{ k_proseFamily } };
            out << TextAlign::Center;
            out << TextStyleId::Title;
            out << L"The Ledger of Ninety-Nine Rooms";
            out << PopTextStyle{};
            endLine();

            out << TextStyleId::SubTitle;
            out << InkWell::textInk(InkGrade::Muted);
            out << L"A puzzle in the manner of Einstein's, at a length nobody asked for";
            out << PopColor{};
            out << PopTextStyle{};
            endLine();
            out << InTextIcon{ 220.0f, 24.0f, paintOrnament };
            endLine();

            out << TextAlign::Justified;
            out << PushFontSize{ k_proseSize };
            out << L"Seventeen people were in the house that evening, each with a trade, an object, "
                L"a way of passing the hour, and a room to pass it in. No two shared any of the four. "
                L"What follows is every remark anyone made about anyone else, numbered in the order "
                L"the ledger took them down rather than in any order that would help, and interrupted "
                L"by the grid a solver keeps beside such a thing. The clues do not contradict each "
                L"other. Whether they determine anything is a separate question, and one this ledger "
                L"declines to raise.";
            out << PopFontSize{};
            endLine();

            out << TextAlign::Left;
            endLine();

            out << TextStyleId::Section;
            out << L"What each clue is about";
            out << PopTextStyle{};
            out << PopFontFamily{};
            endLine();

            out << SetIndent{ k_bulletIndent };
            out << InTextIcon{ 0.0f, 18.0f, paintCheck };
            out << TextOp::PushBold;
            out << L"The people:";
            out << TextOp::PopBold;
            out << L" ";
            out << InkWell::textInk(InkGrade::Muted);
            out << L"seventeen of them, named once each in the list below and thereafter by trade "
                L"whenever a clue can get away with it.";
            out << PopColor{};
            endLine();

            out << InTextIcon{ 0.0f, 18.0f, paintCheck };
            out << TextOp::PushBold;
            out << L"The trades:";
            out << TextOp::PopBold;
            out << L" ";
            out << InkWell::textInk(InkGrade::Muted);
            out << L"thirteen, which is fewer than the people, and the ledger has never explained it.";
            out << PopColor{};
            endLine();

            out << InTextIcon{ 0.0f, 18.0f, paintCheck };
            out << TextOp::PushBold;
            out << L"The objects:";
            out << TextOp::PopBold;
            out << L" ";
            out << InkWell::textInk(InkGrade::Muted);
            out << L"eleven, each of which changed hands at least twice before anyone thought to "
                L"write any of it down.";
            out << PopColor{};
            endLine();

            out << InTextIcon{ 0.0f, 18.0f, paintCheck };
            out << TextOp::PushBold;
            out << L"The states:";
            out << TextOp::PopBold;
            out << L" ";
            out << InkWell::textInk(InkGrade::Muted);
            out << L"seven ways of spending an hour, one of which is being asleep and therefore "
                L"unable to confirm anything.";
            out << PopColor{};
            endLine();

            out << InTextIcon{ 0.0f, 18.0f, paintCheck };
            out << TextOp::PushBold;
            out << PushAnchor{ L"rooms" };
            out << L"The rooms:";
            out << PopAnchor{};
            out << TextOp::PopBold;
            out << L" ";
            out << InkWell::textInk(InkGrade::Muted);
            out << L"nineteen are named. The title says ninety-nine. Both are correct and the "
                L"reconciliation is left to ";
            out << PushLink{ L"#solution" };
            out << L"the closing note";
            out << PopLink{};
            out << L".";
            out << PopColor{};
            endLine();

            out << SetIndent{ 0.0f };
            endLine();

            out << PushFontFamily{ std::wstring{ k_proseFamily } };
            out << TextStyleId::Section;
            out << L"How to read a round";
            out << PopTextStyle{};
            out << PopFontFamily{};
            endLine();

            out << TextOp::PushBold;
            out << L"Column";
            out << TextOp::PopBold;
            out << TabTo{ 100.0f };
            out << TextOp::PushBold;
            out << L"What it holds";
            out << TextOp::PopBold;
            out << TabTo{ 290.0f };
            out << TextOp::PushBold;
            out << L"When it changes";
            out << TextOp::PopBold;
            endLine();

            out << L"Name";
            out << TabTo{ 100.0f };
            out << L"whoever it names outright";
            out << TabTo{ 290.0f };
            out << InkWell::textInk(InkGrade::Muted);
            out << L"never";
            out << PopColor{};
            endLine();

            out << L"Trade";
            out << TabTo{ 100.0f };
            out << L"the trade it rules out";
            out << TabTo{ 290.0f };
            out << InkWell::textInk(InkGrade::Muted);
            out << L"on any clue after it";
            out << PopColor{};
            endLine();

            out << L"Object";
            out << TabTo{ 100.0f };
            out << L"what was carried that hour";
            out << TabTo{ 290.0f };
            out << InkWell::textInk(InkGrade::Muted);
            out << L"twice a round";
            out << PopColor{};
            endLine();

            endLine();

            out << PushFontFamily{ std::wstring{ k_proseFamily } };
            out << PushFontSize{ k_proseSize };
            out << L"Clue numbers run from the first round to the last without restarting, so a "
                L"clue late in the ledger can send you back to one near the front, and several do. "
                L"Rounds are numbered separately, and a round's own number appears in its headings "
                L"and in nothing else.";
            out << PopFontSize{};
            out << PopFontFamily{};
            endLine();

            endLine();
            return lines;
        }

        std::size_t appendClosing(Text& out, std::size_t rounds, std::size_t clues)
        {
            std::size_t lines = 0;
            auto endLine = [&out, &lines](){
                out << L'\n';
                ++lines;
                };

            out << SetIndent{ 0.0f };
            out << TextAlign::Left;
            out << PushFontFamily{ std::wstring{ k_proseFamily } };
            out << TextStyleId::Section;
            out << PushAnchor{ L"solution" };
            out << L"The solution";
            out << PopAnchor{};
            out << PopTextStyle{};
            endLine();

            endLine();

            out << TextAlign::Justified;
            out << PushFontSize{ k_proseSize };
            out << L"Everything needed is above. Taking the ";
            out << static_cast<int>(clues);
            out << L" clues of the ";
            out << static_cast<int>(rounds);
            out << L" rounds in the order they were written down, each one removes at least one "
                L"possibility that the one before it left standing, and no two of them remove the "
                L"same possibility twice - which is the whole of the argument, and the reason it is "
                L"not set out here at length. The table below states the result in the form the "
                L"ledger keeps it, which is to say by reference. Each entry names the clue that "
                L"settles it rather than repeating what that clue already says.";
            out << PopFontSize{};
            out << PopFontFamily{};
            endLine();

            out << TextAlign::Left;
            endLine();

            out << TextOp::PushBold;
            out << L"Room";
            out << TextOp::PopBold;
            out << TabTo{ 130.0f };
            out << TextOp::PushBold;
            out << L"Who was in it";
            out << TextOp::PopBold;
            out << TabTo{ 285.0f };
            out << TextOp::PushBold;
            out << L"Carrying";
            out << TextOp::PopBold;
            out << TabTo{ 375.0f };
            out << TextOp::PushBold;
            out << L"Settled by";
            out << TextOp::PopBold;
            endLine();

            for (std::size_t row = 0; row != 5; ++row)
            {
                const std::size_t reference = clues > (row * 7 + 11) ? clues - (row * 7 + 11) : row + 1;

                out << k_rooms[row];
                out << TabTo{ 130.0f };
                out << InkWell::textInk(InkGrade::Muted);
                out << L"unnamed by ";
                writeClueReference(out, reference, clues);
                out << PopColor{};
                out << TabTo{ 285.0f };
                out << InkWell::textInk(InkGrade::Muted);
                out << L"nothing twice";
                out << PopColor{};
                out << TabTo{ 375.0f };
                writeClueReference(out, reference, clues);
                endLine();
            }

            endLine();

            out << PushFontFamily{ std::wstring{ k_proseFamily } };
            out << PushFontSize{ k_proseSize };
            out << L"The remaining ninety-four rooms follow by the same argument and are not "
                L"written out. The nineteen named rooms are the ones anyone entered; the other "
                L"eighty were locked that evening, which is ";
            out << PushLink{ L"#rooms" };
            out << L"the reconciliation the opening promised";
            out << PopLink{};
            out << L" and the only fact in this ledger that nobody disputes.";
            endLine();

            endLine();

            out << TextAlign::Right;
            out << InkWell::textInk(InkGrade::Muted);
            out << TextOp::PushItalic;
            out << L"- as set down after the fact, and not checked since";
            out << TextOp::PopItalic;
            out << PopColor{};
            out << PopFontSize{};
            out << PopFontFamily{};
            endLine();

            out << TextAlign::Left;
            return lines;
        }
    }

    Text buildRichShowcase(std::size_t lineCount)
    {
        Text result;

        const std::size_t written = appendOpening(result);

        // Whole rounds, so the count is reached or passed and never cut into. A round cut short
        // ends the page mid-list, which reads as damage rather than as a page that is long enough.
        const std::size_t rounds = written < lineCount
            ? (lineCount - written + k_roundLineCount - 1) / k_roundLineCount
            : 0;
        const std::size_t lastClue = rounds * k_cluesPerRound;
        std::size_t round = 0;
        std::size_t clue = 0;
        while (round != rounds)
        {
            ++round;
            appendRound(result, round, clue, lastClue);
        }

        appendClosing(result, round, clue);
        return result;
    }
}
