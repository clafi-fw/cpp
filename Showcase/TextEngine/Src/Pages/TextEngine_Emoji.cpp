module ClaFi.Showcase.TextEngine.Emoji;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.TextEngine.Fmt;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Showcase::TextEngine
{
    namespace
    {
        // The set every family row is asked for. The first four are basic-plane characters a text
        // face may well carry itself, so they say whether the family answered at all; the last
        // three are above that plane and are what an emoji family is named for.
        constexpr std::wstring_view k_sampleSet = L"✓ ✈ ⚠ ❤ 😀 🚀 🐈";

        // The columns of the two tables. Stated rather than left to the widest entry, because a
        // column that measured itself would stand somewhere else on a machine carrying one family
        // more than another, and the two tables would stop lining up.
        constexpr float k_familySampleStop = 176.0f;
        constexpr float k_familyNoteStop = 448.0f;

        constexpr float k_sequenceSampleStop = 176.0f;
        constexpr float k_sequenceCodePointStop = 300.0f;
        constexpr float k_sequenceUnitStop = 380.0f;
        constexpr float k_sequenceNoteStop = 460.0f;

        // What section 5 sets its second reading at, far enough above the prose that a line grown
        // to fit an emoji is grown visibly.
        constexpr float k_largeSize = 28.0f;

        struct FamilyRow
        {
            std::wstring_view family;
            std::wstring_view note;
        };

        // Named for both platforms. A row whose family this machine does not have is not a gap in
        // the table - fontconfig answers an unmatched name with the configured default and
        // DirectWrite answers it with the system fallback, so the row shows what that default put
        // on the screen.
        constexpr std::array k_families{
            FamilyRow{ L"Segoe UI Emoji", L"Windows - colour carried in COLR layers" },
            FamilyRow{ L"Segoe UI Symbol", L"Windows - outlines, with no colour to lose" },
            FamilyRow{ L"Noto Color Emoji", L"Linux - colour carried in bitmap strikes" },
            FamilyRow{ L"Noto Emoji", L"Linux - outlines, one ink by design" },
            FamilyRow{ L"Twemoji Mozilla", L"COLR layers again, where it is installed" },
            FamilyRow{ L"Apple Color Emoji", L"Named so a miss can be read on both targets" },
            FamilyRow{ L"Symbola", L"Outlines over a wide range, rarely installed" },
            FamilyRow{ L"Cambria", L"A text face - what it lacks is fallen back for" },
            FamilyRow{ L"Consolas", L"A monospace text face, asked the same thing" },
        };

        struct Sequence
        {
            std::wstring_view name;
            std::wstring_view sample;
            std::wstring_view note;
        };

        // One picture each, at lengths from one code point to seven. What separates them is where
        // the picture is decided: a modifier and a joiner are read by the shaper, a variation
        // selector picks between two presentations of one character, and a tag sequence spells a
        // region out in characters that draw nothing on their own.
        constexpr std::array k_sequences{
            Sequence{ L"Plain emoji", L"😀", L"One code point, above the basic plane" },
            Sequence{ L"Skin tone", L"👍🏽", L"A base and a modifier after it" },
            Sequence{ L"Joined pair", L"👩‍💻", L"Two, joined by U+200D" },
            Sequence{ L"Joined family", L"👨‍👩‍👧‍👦", L"Four, joined by three" },
            Sequence{ L"Regional flag", L"🇯🇵", L"Two regional indicators" },
            Sequence{ L"Keycap", L"1️⃣", L"A digit, a selector, an enclosing mark" },
            Sequence{ L"Tag flag", L"🏴󠁧󠁢󠁳󠁣󠁴󠁿", L"A base and six tag characters" },
            Sequence{ L"Text presentation", L"❤", L"The selector left off" },
            Sequence{ L"Emoji presentation", L"❤️", L"The same, with U+FE0F after it" },
        };

        // The code points a literal holds, which is not its size where wchar_t is 16 bits: a
        // character above the basic plane stands there as a surrogate pair and counts twice. Where
        // wchar_t is 32 bits no surrogate ever appears and the two numbers agree.
        [[nodiscard]] std::size_t codePointCount(std::wstring_view text)
        {
            std::size_t count = 0;
            for (std::size_t i = 0; i != text.size(); ++i)
            {
                const char32_t unit = static_cast<char32_t>(text[i]);
                const bool leadsAPair = unit >= 0xD800 && unit <= 0xDBFF && i + 1 != text.size();
                if (leadsAPair)
                    ++i;
                ++count;
            }
            return count;
        }

        void writeBold(Text& out, std::wstring_view heading)
        {
            out << TextOp::PushBold;
            out << heading;
            out << TextOp::PopBold;
        }

        // A column heading, set where the column it names begins. The stops are read from the same
        // constants the rows are written with, so a heading cannot drift off the column under it.
        void writeHeading(Text& out, std::wstring_view heading, float stop)
        {
            out << TabTo{ stop };
            writeBold(out, heading);
        }

        void writeNote(Text& out, std::wstring_view note)
        {
            out << TextStyleId::SubBody;
            out << InkGrade::Muted;
            out << note;
            out << PopColor{};
            out << PopTextStyle{};
            out << TextOp::EndLine;
        }

        // A family, the set drawn in it, and what the family is. The name is set in the monospace
        // style so that the column reads as a list of names rather than as prose, and the set is
        // the only run on the line the family applies to.
        void writeFamilyRow(Text& out, const FamilyRow& row)
        {
            out << TextStyleId::Code;
            out << row.family;
            out << PopTextStyle{};
            out << TabTo{ k_familySampleStop };
            out << PushFontFamily{ std::wstring{ row.family } };
            out << k_sampleSet;
            out << PopFontFamily{};
            out << TabTo{ k_familyNoteStop };
            writeNote(out, row.note);
        }

        void writeFamilyHeader(Text& out)
        {
            writeBold(out, L"Family");
            writeHeading(out, L"The set", k_familySampleStop);
            writeHeading(out, L"What it is", k_familyNoteStop);
            out << TextOp::EndLine;
        }

        // A sequence, the picture it comes to, and the two counts. The counts are taken from the
        // literal at run time, so the line states what this build actually holds - which is the
        // whole point of the column, since the wchar_t figure is the one the caret steps by.
        void writeSequenceRow(Text& out, const Sequence& sequence)
        {
            out << sequence.name;
            out << TabTo{ k_sequenceSampleStop };
            out << sequence.sample;
            out << TabTo{ k_sequenceCodePointStop };
            out << codePointCount(sequence.sample);
            out << TabTo{ k_sequenceUnitStop };
            out << sequence.sample.size();
            out << TabTo{ k_sequenceNoteStop };
            writeNote(out, sequence.note);
        }

        void writeSequenceHeader(Text& out)
        {
            writeBold(out, L"Sequence");
            writeHeading(out, L"Drawn", k_sequenceSampleStop);
            writeHeading(out, L"Points", k_sequenceCodePointStop);
            writeHeading(out, L"Units", k_sequenceUnitStop);
            writeHeading(out, L"What it is", k_sequenceNoteStop);
            out << TextOp::EndLine;
        }
    }

    Text buildEmojiShowcase()
    {
        Text page{};

        page << Fmt{
            L"[center][title]Emoji[/title][n]"
            L"[subtitle]One set of characters, one family after another[/subtitle][n][n]"
            L"[left][body]Nothing on this page is a picture the showcase supplies. Every mark "
            L"below is a character in the text, shaped by whatever face the platform found for it, "
            L"and what the page states is which face that was and what it could draw.[/body][n][n]"
        };

        // -----------------------------------------------------------------
        // 1. What answers when nothing is named
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]1. What answers when nothing is named[/heading][n]"
            L"[body]A style names a text family, and a text family has no glyph for an emoji. The "
            L"character is drawn all the same, because neither platform stops at the family the run "
            L"states: DirectWrite runs its own fallback over the system list, and the Linux side "
            L"walks the chain fontconfig sorted for that family, taking the first face with a glyph "
            L"for the character. That sort asks for scalable faces and against colour ones, so an "
            L"outline emoji face is reached before a colour one and a colour face stands last of "
            L"all. The style below is the only thing stated on each line.[/body][n][n]"
            L"[body]Body[space 24]The 🦊 jumped over the 🐕 at 🕗 and left a 📄.[/body][n]"
            L"[subbody]SubBody[space 24]The 🦊 jumped over the 🐕 at 🕗 and left a 📄.[/subbody][n]"
            L"[code]Code[space 24]The 🦊 jumped over the 🐕 at 🕗 and left a 📄.[/code][n]"
            L"[heading]Heading[space 24]The 🦊 and the 🐕[/heading][n][n]"
            L"[body]The four lines are drawn by two faces each: the family's own for the letters, "
            L"the fallback's for the emoji. A line whose emoji are wider or taller than its letters "
            L"is the second face's metrics showing through the first face's line.[/body][n][n]"
        };

        // -----------------------------------------------------------------
        // 2. One set, many families
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]2. One set, many families[/heading][n]"
            L"[body]The same seven characters, each row naming its family outright with the "
            L"[b]font[/b] tag. The first four are basic-plane characters a text face may carry "
            L"itself; the last three are above that plane and belong to an emoji face alone. "
            L"Every row draws all seven, because neither platform leaves a character undrawn - so "
            L"what separates the rows is which face each character came from. The shapes differ, "
            L"and so do the advances, which is why a row that fell back stands wider or narrower "
            L"than one that did not.[/body][n][n]"
        };

        writeFamilyHeader(page);

        for (const FamilyRow& row : k_families)
            writeFamilyRow(page, row);

        page << Fmt{
            L"[n][body]A name nothing matches is not an error on either platform. fontconfig "
            L"answers it with the configured default and DirectWrite answers it with the system "
            L"fallback, so every row here draws something - and rows that look alike are rows that "
            L"landed on the same face.[/body][n]"
            L"[body]Which is most of them. A machine has one emoji face it prefers, and both "
            L"platforms reach that face for anything the named family lacks - so the rows that "
            L"differ are the ones naming a family this machine actually has.[/body][n][n]"
        };

        // -----------------------------------------------------------------
        // 3. Colour
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]3. Colour[/heading][n]"
            L"[body]A colour emoji face carries its colour one of two ways: as layers of outlines "
            L"with a palette entry each, or as a bitmap strike per size. Both backends here draw a "
            L"run with a single brush - Direct2D through DrawGlyphRun, the CPU backend by "
            L"accumulating one coverage mask and compositing the brush through it - so a face's "
            L"colour reaches neither.[/body][n][n]"
            L"[indent 34]"
            L"[b]Layered faces[/b] draw their base layer alone, in the text ink. That layer is "
            L"usually the silhouette, so the emoji arrives as a solid shape.[n]"
            L"[b]Strike-carrying faces[/b] have no outline behind the bitmap, and the Linux "
            L"rasterizer loads with FT_LOAD_NO_BITMAP, so a strike would be refused and the glyph "
            L"left blank. Reaching one takes more than naming it: the sort carries FC_COLOR "
            L"FcFalse alongside the family, so a row drawing outlines under a colour family's "
            L"name is that preference winning over the name.[n]"
            L"[b]Outline faces[/b] - Noto Emoji, Symbola, Segoe UI Symbol - lose nothing to the "
            L"brush, because one ink is all they ever had. Grey is still theirs to arrive in: a "
            L"face drawing its emoji as hairlines covers no whole pixel at body size, so it peaks "
            L"short of full ink before the coverage curve thins what is left.[n]"
            L"[indent 0][n]"
            L"[body]Which is why section 2 is worth reading twice: a family that draws nothing is "
            L"not a family that was missed, and a family that draws a black silhouette is not a "
            L"monochrome family.[/body][n][n]"
        };

        // -----------------------------------------------------------------
        // 4. Many code points, one picture
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]4. Many code points, one picture[/heading][n]"
            L"[body]A picture the reader takes for one character is often several, put together by "
            L"the shaper. The two count columns are what the page holds against what the screen "
            L"shows: code points, and the wchar_t units they occupy on this build. Where a row "
            L"draws its parts side by side instead of one picture, the face has no glyph for the "
            L"sequence and the shaper had nothing to ligate.[/body][n][n]"
        };

        writeSequenceHeader(page);

        for (const Sequence& sequence : k_sequences)
            writeSequenceRow(page, sequence);

        page << Fmt{ L"[n][body]wchar_t is " };
        page << sizeof(wchar_t);
        page << Fmt{
            L" bytes here, which is what parts the two columns. The engine holds no notion of a "
            L"cluster: a length, an offset and a caret column are all counts of units, and the "
            L"Units column is what those counts run over.[/body][n][n]"
        };

        // -----------------------------------------------------------------
        // 5. What a line is measured at
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]5. What a line is measured at[/heading][n]"
            L"[body]An emoji face is square, and its ascent is taller than the text face's at the "
            L"same em size. A line carrying one is measured over both faces, so it stands taller "
            L"than the lines around it. The three below are one sentence, with an emoji in the "
            L"middle one.[/body][n][n]"
            L"The quick brown fox jumps over the lazy dog.[n]"
            L"The quick brown fox 🦊 jumps over the lazy dog.[n]"
            L"The quick brown fox jumps over the lazy dog.[n][n]"
            L"[body]The same at a size where the difference is not a matter of a pixel:[/body][n][n]"
        };

        page << PushFontSize{ k_largeSize };
        page << L"Fox, and the same 🦊 again";
        page << TextOp::EndLine;
        page << PopFontSize{};
        page << TextOp::EndLine;

        // -----------------------------------------------------------------
        // 6. The caret
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]6. The caret[/heading][n]"
            L"[body]This page is editable, which is what the section is for. The readout in the "
            L"corner names the line and the column the caret stands at, and a column is a count of "
            L"wchar_t - so walking the line below with the arrow keys is a reading of the Units "
            L"column above.[/body][n][n]"
            L"[indent 34]"
            L"[b]Where wchar_t is 16 bits[/b] the column steps by two over a plain emoji and by "
            L"eleven over the joined family, so a press lands the position inside a surrogate "
            L"pair.[n]"
            L"[b]Where it is 32 bits[/b] a plain emoji is one press, and the joined family is still "
            L"seven - the joiners are units of their own and the caret stops between them.[n]"
            L"[b]Selecting[/b] a picture and typing over it is the same question asked of a range "
            L"rather than of a position.[n]"
            L"[indent 0][n]"
            L"[body]Walk this line:[/body][n][n]"
            L"a 😀 b 👍🏽 c 👩‍💻 d 👨‍👩‍👧‍👦 e 🇯🇵 f[n][n]"
            L"[body]And this one, which is the same line with the pictures spelled out so the count "
            L"can be followed:[/body][n][n]"
            L"[subbody][color muted]a | 1 emoji | b | base+modifier | c | 2 joined | d | 4 joined | "
            L"e | 2 indicators | f[/color][/subbody][n][n]"
        };

        // -----------------------------------------------------------------
        // 7. What the page found out
        // -----------------------------------------------------------------

        page << Fmt{
            L"[heading]7. What the page found out[/heading][n]"
            L"[indent 34]"
            L"[b]Fallback works and is invisible.[/b] A run states a family and can be drawn by "
            L"another, with nothing in the text saying so.[n]"
            L"[b]Colour does not survive the run.[/b] One brush per run is a decision both backends "
            L"make, and it is what a colour face is lost to.[n]"
            L"[b]A cluster is not a unit.[/b] Every count the engine keeps is a count of wchar_t, "
            L"so a caret, a selection and a column all step through the inside of a picture.[n]"
            L"[b]The two platforms count differently.[/b] The same document is a different number "
            L"of units on each, so an offset saved on one does not name the same place on the "
            L"other.[n]"
            L"[indent 0]"
        };

        return page;
    }
}
