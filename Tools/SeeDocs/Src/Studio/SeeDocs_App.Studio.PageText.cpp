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
        constexpr float k_levelIndent = 24.0f;    // what one level of nesting stands in by
        constexpr float k_bulletIndent = 18.0f;
        constexpr float k_columnGap = 16.0f;      // the least room between two cells of a row
        constexpr float k_minColumn = 60.0f;
        constexpr float k_maxColumn = 300.0f;
        constexpr float k_entryGap = 6.0f;        // the room under an entry, as an empty line
        // What a character is taken to measure, for placing the columns; the layout measures
        // the words themselves.
        constexpr float k_codeCharWidth = 8.0f;
        constexpr float k_textCharWidth = 6.5f;
        constexpr std::wstring_view k_bullet = L"\u2022 ";

        using Stops = std::vector<float>;             // where each column of a table begins
        using GroupStops = std::map<std::size_t, Stops>;

        [[nodiscard]] float estimatedWidth(const Runs& runs)
        {
            float width = 0.0f;
            for (const Run& run : runs)
            {
                const bool code = run.style == RunStyle::Code;
                const float perChar = code ? k_codeCharWidth : k_textCharWidth;
                width += static_cast<float>(run.text.size()) * perChar;
            }
            return width;
        }

        // A column begins where the widest cell of the column before it ends, that width capped,
        // plus a gap - measured over every table of the group on the page.
        [[nodiscard]] GroupStops columnStopsOf(const Page& page)
        {
            std::map<std::size_t, std::vector<float>> widest;
            for (const Block& block : page.blocks)
            {
                if (block.kind != BlockKind::Table && block.kind != BlockKind::Entry)
                    continue;
                std::vector<float>& widths = widest[block.group];
                for (const Cells& row : block.rows)
                {
                    if (widths.size() < row.size())
                        widths.resize(row.size(), 0.0f);
                    for (std::size_t i = 0; i != row.size(); ++i)
                        widths[i] = std::max(widths[i], estimatedWidth(row[i]));
                }
            }
            GroupStops stops;
            for (const auto& [group, widths] : widest)
            {
                Stops& columns = stops[group];
                columns.push_back(0.0f);
                for (std::size_t i = 1; i < widths.size(); ++i)
                {
                    const float width = std::clamp(widths[i - 1], k_minColumn, k_maxColumn);
                    columns.push_back(columns.back() + width + k_columnGap);
                }
            }
            return stops;
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

        void writeRow(Text& text, const Cells& row, const Stops& columns, const float indent)
        {
            text << ParaIndent{ indent };
            for (std::size_t i = 0; i != row.size(); ++i)
            {
                if (i != 0 && i < columns.size())
                    text << Space{ k_columnGap } << TabTo{ columns[i] };
                writeRuns(text, row[i]);
            }
            text << k_endLine;
        }

        void writeStyled(Text& text, const TextStyleId style, const Runs& runs, const float indent)
        {
            text << ParaIndent{ indent } << style;
            writeRuns(text, runs);
            text << PopTextStyle{} << k_endLine;
        }

        void writeBlock(Text& text, const Block& block, const GroupStops& stops)
        {
            const float indent = static_cast<float>(block.level) * k_levelIndent;
            switch (block.kind)
            {
                case BlockKind::Title:
                    writeStyled(text, TextStyleId::Title, block.runs, indent);
                    break;
                case BlockKind::Lead:
                    writeStyled(text, TextStyleId::SubTitle, block.runs, indent);
                    break;
                case BlockKind::Meta:
                    text << ParaIndent{ indent };
                    writeRuns(text, block.runs);
                    text << k_endLine;
                    break;
                case BlockKind::Heading:
                    text << ParaIndent{ indent } << k_endLine;
                    writeStyled(text, TextStyleId::Heading, block.runs, indent);
                    break;
                case BlockKind::SubHeading:
                    writeStyled(text, TextStyleId::Section, block.runs, indent);
                    break;
                case BlockKind::Paragraph:
                    text << ParaIndent{ indent };
                    writeRuns(text, block.runs);
                    text << k_endLine;
                    break;
                case BlockKind::Code:
                    for (const std::wstring& line : block.lines)
                    {
                        text << ParaIndent{ indent + k_bulletIndent } << TextStyleId::Code << line
                            << PopTextStyle{} << k_endLine;
                    }
                    break;
                case BlockKind::Bullets:
                    for (const Runs& item : block.items)
                    {
                        text << ParaIndent{ indent + k_bulletIndent } << k_bullet;
                        writeRuns(text, item);
                        text << k_endLine;
                    }
                    break;
                case BlockKind::Table:
                    for (const Cells& row : block.rows)
                        writeRow(text, row, stops.at(block.group), indent);
                    break;
                case BlockKind::Entry:
                    // The term on its line, the hint under it, one level in; a short empty line
                    // keeps the next entry apart.
                    for (const Cells& row : block.rows)
                        writeRow(text, row, stops.at(block.group), indent);
                    text << ParaIndent{ indent + k_levelIndent };
                    writeRuns(text, block.runs);
                    text << k_endLine;
                    text << PushFontSize{ k_entryGap } << k_endLine << PopFontSize{};
                    break;
                case BlockKind::Source:
                    text << ParaIndent{ indent } << k_endLine;
                    writeStyled(text, TextStyleId::SubBody, block.runs, indent);
                    break;
            }
        }
    }

    Text textOf(const Page& page)
    {
        const GroupStops stops = columnStopsOf(page);
        Text text;
        std::size_t previousLevel = 0;
        for (const Block& block : page.blocks)
        {
            // A note's blocks stand in; the first block back at the margin takes a blank line.
            if (block.level < previousLevel)
                text << ParaIndent{ 0.0f } << k_endLine;
            previousLevel = block.level;
            writeBlock(text, block, stops);
        }
        return text;
    }
}
