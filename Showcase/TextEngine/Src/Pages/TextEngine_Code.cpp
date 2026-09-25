module ClaFi.Showcase.TextEngine.Code;

import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.StdLib;

namespace ClaFi::Showcase::TextEngine
{
    namespace
    {
        // Where the repetition number is written into a line.
        constexpr std::wstring_view k_indexMarker = L"$N$";

        // One repetition of the block the page is filled with, written to look like the framework
        // it ships with rather than like generated filler. What the page measures is how many
        // tokens a real line carries, and filler carries fewer.
        constexpr std::wstring_view k_blockLines[] = {
            L"// ---------------------------------------------------------------------------",
            L"// Section $N$ - what a held buffer costs, and what it does not.",
            L"// A buffer refilled per paragraph allocates once and is reused for every",
            L"// paragraph after it, so the number below counts paragraphs and not frames.",
            L"",
            L"export class SpanCollector$N$ : public CollectorBase",
            L"{",
            L"public:",
            L"    explicit SpanCollector$N$(const BuildParams& params);",
            L"    ~SpanCollector$N$() override = default;",
            L"    [[nodiscard]] size_t collected() const { return m_collected; }",
            L"    [[nodiscard]] bool ordered() const noexcept { return m_ordered; }",
            L"    void collect(std::span<const ColorSpan> spans, size_t start, size_t end);",
            L"    void reset();",
            L"private:",
            L"    static constexpr size_t k_reserve$N$ = 64;",
            L"    static constexpr float k_growth$N$ = 1.5f;",
            L"    std::vector<ColorSpan> m_spans;",
            L"    size_t m_collected{ 0 };",
            L"    bool m_ordered{ true };",
            L"};",
            L"",
            L"void SpanCollector$N$::collect(std::span<const ColorSpan> spans, size_t start, size_t end)",
            L"{",
            L"    m_spans.clear();",
            L"    m_spans.reserve(spans.size() + 2);",
            L"",
            L"    for (const ColorSpan& span : spans)",
            L"    {",
            L"        // Ordered, so the first span past this paragraph ends the answer.",
            L"        if (span.range.start >= end)",
            L"            break;",
            L"",
            L"        const size_t first = std::max(start, span.range.start);",
            L"        const size_t last = std::min(end, span.range.end());",
            L"        if (last <= first)",
            L"            continue;",
            L"",
            L"        m_spans.push_back({ { first - start, last - first }, span.value });",
            L"        ++m_collected;",
            L"    }",
            L"    trace(L\"section $N$ collected \", m_collected, L\" spans\\n\");",
            L"}",
            L"",
            L"void SpanCollector$N$::reset()",
            L"{",
            L"    m_spans.clear();",
            L"    m_collected = 0;",
            L"    m_ordered = true;",
            L"}"
        };

        // A round block, so a round line count comes out exact and the last one still ends
        // where it should. A line added here without a line taken away puts the truncation back.
        constexpr std::size_t k_blockLineCount = std::size(k_blockLines);
        static_assert(k_blockLineCount == 50, "A round block keeps a round line count exact.");

        void appendLine(std::wstring& out, std::wstring_view line, std::wstring_view number)
        {
            std::size_t start = 0;
            for (;;)
            {
                const std::size_t marker = line.find(k_indexMarker, start);
                if (marker == std::wstring_view::npos)
                    break;

                out += line.substr(start, marker - start);
                out += number;
                start = marker + k_indexMarker.size();
            }

            out += line.substr(start);
            out += L'\n';
        }
    }

    std::wstring buildCppSource(std::size_t lineCount)
    {
        // Enough for the block as written; a longer one grows the string a few times more and
        // costs nothing that shows next to the shaping this exists to provoke.
        constexpr std::size_t k_averageLineLength = 56;

        std::wstring result;
        result.reserve(lineCount * k_averageLineLength);

        // Whole blocks, so the count is reached or passed and never cut into. An exact count
        // stops the last block wherever it falls - mid-function, mid-comment - and a page of
        // source that breaks off reads as damage rather than as a page that is long enough.
        std::size_t written = 0;
        std::size_t repetition = 0;
        while (written < lineCount)
        {
            ++repetition;
            const std::wstring number = std::to_wstring(repetition);
            for (const std::wstring_view line : k_blockLines)
            {
                appendLine(result, line, number);
                ++written;
            }
        }

        return result;
    }

    Text buildCodeShowcase(std::size_t lineCount)
    {
        return Text{ TextStyleId::Code, buildCppSource(lineCount), PopTextStyle{} };
    }
}
