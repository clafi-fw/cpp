module TextEngine_App.Scripts;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace TextEngine_App
{
    using namespace ::ClaFi;

    namespace
    {
        // Wide enough for the longest name the table carries - Canadian Aboriginal - so no row
        // pushes its own characters along and the column reads as one edge.
        constexpr float k_sampleStop = 190.0f;
        constexpr float k_pointStop = 300.0f;

        struct ScriptRow
        {
            std::wstring_view name;
            std::wstring_view sample;
        };

        // Every script the corpus states, in its order, less the two that are laid out right to
        // left - those stand in a block of their own at the end.
        constexpr std::array k_scripts{
            ScriptRow{ L"Latin", L"A z é" },
            ScriptRow{ L"Cyrillic", L"А Ж я" },
            ScriptRow{ L"Greek", L"Α Ω β" },
            ScriptRow{ L"Hiragana", L"あ き ん" },
            ScriptRow{ L"Katakana", L"ア キ ン" },
            ScriptRow{ L"Hangul", L"가 한 국" },
            ScriptRow{ L"CJK Unified", L"一 汉 字" },
            ScriptRow{ L"Devanagari", L"अ क ह" },
            ScriptRow{ L"Bengali", L"অ ক হ" },
            ScriptRow{ L"Gurmukhi", L"ਅ ਕ ਹ" },
            ScriptRow{ L"Gujarati", L"અ ક હ" },
            ScriptRow{ L"Oriya", L"ଅ କ ହ" },
            ScriptRow{ L"Tamil", L"அ க ஹ" },
            ScriptRow{ L"Telugu", L"అ క హ" },
            ScriptRow{ L"Kannada", L"ಅ ಕ ಹ" },
            ScriptRow{ L"Malayalam", L"അ ക ഹ" },
            ScriptRow{ L"Sinhala", L"අ ක හ" },
            ScriptRow{ L"Thai", L"ก ท ส" },
            ScriptRow{ L"Lao", L"ກ ທ ສ" },
            ScriptRow{ L"Myanmar", L"က ခ ဂ" },
            ScriptRow{ L"Khmer", L"ក ខ គ" },
            ScriptRow{ L"Armenian", L"Ա Բ ա" },
            ScriptRow{ L"Georgian", L"ა ბ გ" },
            ScriptRow{ L"Ethiopic", L"ሀ ሁ ሂ" },
            ScriptRow{ L"Tibetan", L"ཀ ཁ ག" },
            ScriptRow{ L"Mongolian", L"ᠠ ᠡ ᠢ" },
            ScriptRow{ L"Canadian Aboriginal", L"ᐁ ᐃ ᐅ" },
            ScriptRow{ L"Cherokee", L"Ꭰ Ꭱ Ꭲ" },
            // U+1680 is OGHAM SPACE MARK - a space, and invisible in this file. Written as an
            // escape so an editor cannot lose it, and drawn as the blank it is.
            ScriptRow{ L"Ogham", L"\u1680 ᚁ ᚂ" },
            ScriptRow{ L"Runic", L"ᚠ ᚢ ᚦ" },
            ScriptRow{ L"Tagalog", L"ᜀ ᜁ ᜂ" },
            ScriptRow{ L"Hanunoo", L"ᜠ ᜡ ᜢ" },
            ScriptRow{ L"Buhid", L"ᝀ ᝁ ᝂ" },
            ScriptRow{ L"Tagbanwa", L"ᝠ ᝡ ᝢ" },
            ScriptRow{ L"Limbu", L"ᤀ ᤁ ᤂ" },
            ScriptRow{ L"Tai Le", L"ᥐ ᥑ ᥒ" },
            ScriptRow{ L"New Tai Lue", L"ᦀ ᦁ ᦂ" },
            ScriptRow{ L"Buginese", L"ᨀ ᨁ ᨂ" },
            ScriptRow{ L"Tai Tham", L"ᨠ ᨡ ᨢ" },
            ScriptRow{ L"Balinese", L"ᬅ ᬓ ᬔ" },
            ScriptRow{ L"Sundanese", L"ᮃ ᮊ ᮋ" },
            ScriptRow{ L"Lepcha", L"ᰀ ᰁ ᰂ" },
            ScriptRow{ L"Ol Chiki", L"᱐ ᱚ ᱛ" },
            ScriptRow{ L"Vai", L"ꔀ ꔁ ꔂ" },
            ScriptRow{ L"Bamum", L"ꚠ ꚡ ꚢ" },
            ScriptRow{ L"Syloti Nagri", L"ꠀ ꠁ ꠃ" },
            ScriptRow{ L"Javanese", L"ꦄ ꦏ ꦐ" },
            ScriptRow{ L"Cham", L"ꨀ ꨁ ꨂ" },
            ScriptRow{ L"Tai Viet", L"ꪀ ꪁ ꪂ" },
            ScriptRow{ L"Meetei Mayek", L"ꯀ ꯁ ꯂ" },
            ScriptRow{ L"Mathematical", L"∀ ∞ ∑" },
            ScriptRow{ L"Emoji", L"😀 👍 😂" },
            ScriptRow{ L"Symbols", L"☀ ❤ ✓" },
            ScriptRow{ L"Braille", L"⠀ ⠁ ⠿" },
        };

        // Right to left, which the engine does not do. Kept together and headed as what they are.
        constexpr std::array k_rightToLeft{
            ScriptRow{ L"Arabic", L"ا ب م" },
            ScriptRow{ L"Hebrew", L"א ש ל" },
        };

        // The code points a literal holds. Where wchar_t is 16 bits a character above the basic
        // plane stands as a surrogate pair, and the pair names one code point.
        void writeCodePoints(Text& out, std::wstring_view text)
        {
            for (std::size_t i = 0; i != text.size(); ++i)
            {
                char32_t codePoint = static_cast<char32_t>(text[i]);
                const bool leadsAPair = codePoint >= 0xD800 && codePoint <= 0xDBFF && i + 1 != text.size();
                if (leadsAPair)
                {
                    const char32_t trail = static_cast<char32_t>(text[i + 1]);
                    codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (trail - 0xDC00);
                    ++i;
                }
                if (codePoint == U' ')
                    continue;
                out << std::format(L"U+{:04X} ", static_cast<std::uint32_t>(codePoint));
            }
        }

        void writeScriptRow(Text& out, const ScriptRow& row)
        {
            out << row.name;
            out << TabTo{ k_sampleStop };
            out << row.sample;
            out << TabTo{ k_pointStop };
            Text codePoints{};
            writeCodePoints(codePoints, row.sample);
            out << StyledText{ TextStyles::SubBody, ColoredText{ InkGrade::Muted, codePoints } };
            out << k_endLine;
        }

        void writeHeader(Text& out)
        {
            out << BoldText{
                L"Script",
                TabTo{ k_sampleStop },
                L"Characters",
                TabTo{ k_pointStop },
                L"Code points"
            };
            out << k_endLine;
        }
    }

    Text buildScriptsShowcase()
    {
        Text page{};

        page << ParagraphText{
            TextAlign::Center,
            StyledText{ TextStyles::Title, L"Writing Systems" },
            k_endLine,
            StyledText{ TextStyles::SubTitle, L"Fifty-six of them, three characters each" },
            k_endLine,
            k_endLine // the blank row under the subtitle is centred with it
        };
        page << L"No family is named anywhere on this page. Every row is set in the same "
            L"style as the prose around it, so what draws each character is the platform's own "
            L"fallback and nothing the page chose - which is the question being put.";
        page << k_endLine << k_endLine;

        page << StyledText{ TextStyles::Heading, L"1. How to read a row" } << k_endLine;
        page << ParagraphText{
            ParaIndent{ 34.0f },
            BoldText{ L"Characters drawn" },
            L" means a face was found and had them.\n",
            BoldText{ L"Boxes" },
            L" mean a face was found and did not have them, so its .notdef stood in. "
            L"Noto and DejaVu draw .notdef as a box.\n",
            BoldText{ L"Blanks" },
            L" mean the same miss from a face whose .notdef is empty, which is what a "
            L"text face answers with for a script it never carried.\n"
        };
        page << k_endLine;
        page << L"Only the first is coverage. A box and a blank are one miss wearing different "
            L"clothes, and which of the two appears says more about the face that was reached than "
            L"about the script that was asked for.";
        page << k_endLine << k_endLine;

        page << StyledText{ TextStyles::Heading, L"2. The scripts" } << k_endLine;
        page << L"Each row is a paragraph of its own. The shaper guesses a script per item, and a "
            L"line carrying fifty-six of them would be guessed once and wrongly - so a row holds "
            L"one script, and the guess it is asked for is one it can answer.";
        page << k_endLine << k_endLine;

        writeHeader(page);

        for (const ScriptRow& row : k_scripts)
            writeScriptRow(page, row);

        page << k_endLine;
        page << StyledText{ TextStyles::Heading, L"3. Not claimed yet" } << k_endLine;
        page << L"These two are laid out right to left and the engine does not do that. The "
            L"shaper is handed an item and guesses its direction from the contents, so the glyphs "
            L"are chosen and joined correctly - and then the line is broken and placed left to "
            L"right over them. Both rows below are wrong on purpose, and are what bidi will be "
            L"measured against when it arrives.";
        page << k_endLine << k_endLine;

        writeHeader(page);

        for (const ScriptRow& row : k_rightToLeft)
            writeScriptRow(page, row);

        page << k_endLine;
        page << StyledText{ TextStyles::Heading, L"4. What the page leaves out" } << k_endLine;
        page << ParagraphText{
            ParaIndent{ 34.0f },
            BoldText{ L"Joining and reordering." },
            L" Three characters standing alone ask a face for three "
            L"glyphs. A script whose letters join, reorder or stack - Devanagari, Khmer, Myanmar, "
            L"Tibetan - is only half asked by a row like this; coverage is not shaping.\n",
            BoldText{ L"Which face answered." },
            L" The page shows what was drawn, never what drew it. A row "
            L"of boxes and a row of blanks both found a face; the probe beside this showcase is "
            L"what names it.\n",
            BoldText{ L"One line per script." },
            L" Nothing here puts two scripts in one paragraph, which is "
            L"the case the item guess is weakest at.\n"
        };

        return page;
    }
}
