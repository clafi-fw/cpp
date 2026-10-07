module SeeDocs_App.Studio.PageText;

import SeeDocs_App.Pages;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.InkWell;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace SeeDocs_App
{
    using namespace ClaFi;

    namespace
    {
        // Design units, as every measure a Text states is.
        constexpr float k_codeIndent = 18.0f;
        constexpr float k_bulletIndent = 18.0f;
        constexpr float k_footnoteGap = 6.0f;   // the room under a footnote, as an empty line
        constexpr std::wstring_view k_bullet = L"\u2022 ";

        void writeBlock(Text& text, const Block& block)
        {
            switch (block.kind)
            {
                case BlockKind::Paragraph:
                    text << ParaIndent{ 0.0f };
                    writeRuns(text, block.runs);
                    text << k_endLine;
                    break;
                case BlockKind::SubHeading:
                    text << ParaIndent{ 0.0f } << TextStyleId::Section;
                    writeRuns(text, block.runs);
                    text << PopTextStyle{} << k_endLine;
                    break;
                case BlockKind::Code:
                    for (const std::wstring& line : block.lines)
                    {
                        text << ParaIndent{ k_codeIndent } << TextStyleId::Code << line
                            << PopTextStyle{} << k_endLine;
                    }
                    break;
                case BlockKind::Bullets:
                    for (const Runs& item : block.items)
                    {
                        text << ParaIndent{ k_bulletIndent } << k_bullet;
                        writeRuns(text, item);
                        text << k_endLine;
                    }
                    break;
            }
        }
    }

    void writeRuns(Text& text, const Runs& runs)
    {
        for (const Run& run : runs)
        {
            const bool linked = !run.link.empty();
            if (linked)
                text << PushLink{ run.link };
            switch (run.style)
            {
                case RunStyle::Plain:
                    text << run.text;
                    break;
                case RunStyle::Code:
                    text << TextStyleId::Code << run.text << PopTextStyle{};
                    break;
                case RunStyle::Muted:
                    text << InkGrade::Muted << run.text << PopColor{};
                    break;
                case RunStyle::Bold:
                    text << TextOp::PushBold << run.text << TextOp::PopBold;
                    break;
                case RunStyle::Missing:
                    text << InkWell::redInk(InkGrade::Muted) << TextOp::PushItalic << run.text
                        << TextOp::PopItalic << PopColor{};
                    break;
            }
            if (linked)
                text << PopLink{};
        }
    }

    Text textOf(const Runs& runs)
    {
        Text text;
        writeRuns(text, runs);
        return text;
    }

    Text textOf(const Blocks& blocks)
    {
        Text text;
        for (const Block& block : blocks)
            writeBlock(text, block);
        return text;
    }

    void writeFootnote(Text& text, const std::wstring_view anchor, const std::wstring_view mark,
        const Runs& name, const Excerpt& excerpt)
    {
        text << ParaIndent{ 0.0f } << PushAnchor{ std::wstring{ anchor } } << InkGrade::Muted
            << mark << PopColor{} << L" ";
        writeRuns(text, name);
        text << PopAnchor{} << L"  ";
        writeRuns(text, excerpt.source);
        text << k_endLine;
        for (const Block& block : excerpt.blocks)
            writeBlock(text, block);
        text << PushFontSize{ k_footnoteGap } << k_endLine << PopFontSize{};
    }
}
