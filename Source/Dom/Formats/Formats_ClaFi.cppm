// =========================================================================
// ClaFi.Dom.Formats.ClaFi
// =========================================================================
//
// Implements ClaFi v1.0 (see the ClaFi Handbook) for THIS engine's
// schema-driven DOM — not a general-purpose, standalone ClaFi parser.
// Known deviations from the spec:
//
// - Schema-bound, not schema-inferring: the spec describes disambiguating
//   a section from an array purely from a file's own content (does it
//   contain key=value lines, or =value lines?). This reader instead
//   resolves every key against the pre-existing DOM schema and relies on
//   the schema to know which is which. It cannot represent an arbitrary
//   ClaFi file with no corresponding schema; an unrecognized key emits a
//   SchemaErrorEvent and is discarded rather than preserved.
// - Malformed input (a non-reserved line with no '=', an unknown key, a
//   composite/scalar type mismatch) degrades via SchemaErrorEvent and
//   continues parsing, rather than treating it as a hard syntax error.
export module ClaFi.Dom.Formats.ClaFi;

import ClaFi.Dom;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Dom::FileFormat
{
    export class ClaFi : public FileFormatBase
    {
    public:
        void writeHeader(std::wostream& stream) const override;
        void loadSectionFromStream(Section& section, std::wistream& stream) const override;
        void saveSectionToStream(const Section& section, std::wostream& stream) const override;

    private:
        void saveSectionContent(const Section& section, std::wostream& stream, std::size_t indent) const;
        void saveNodeRecursive(std::wostream& stream, const DomNodeBase& node, std::size_t indent = 0) const;
    };

    // ---------------------------------------------------------
    // Implementation
    // ---------------------------------------------------------

    void ClaFi::writeHeader(std::wostream& stream) const
    {
        stream <<
            L"@ClaFi 1.0\n"
            L"@encoding utf-8\n\n";
    }

    void ClaFi::loadSectionFromStream(Section& section, std::wistream& stream) const
    {
        std::vector<DomNodeBase*> stack = std::vector<DomNodeBase*>{ &section };
        ScalarValueBase* lastValue = nullptr;
        std::wstring pendingRaw = std::wstring{};
        std::wstring line = std::wstring{};

        Section dummy = Section{ nullptr };

        // A value and its continuation lines are ONE assignment, made when the run ends. Writing
        // each line as it arrives would read the whole accumulated value back and rewrite it,
        // which is quadratic in the lines a value spans - a document's plain text is one value.
        auto writePending = [&](){
            if (!lastValue)
            {
                return;
            }

            lastValue->setFromRaw(pendingRaw);
            lastValue = nullptr;
            pendingRaw.clear();
        };

        while (std::getline(stream, line))
        {
            trimLeft(line);

            if (line.empty() || line.front() == L';' || line.front() == L'@')
            {
                continue;
            }

            if (line == L"]")
            {
                if (stack.size() > 1)
                {
                    stack.pop_back();
                }
                continue;
            }

            const bool addNewline = line.front() == L'|';
            if (addNewline || line.front() == L'+')
            {
                if (lastValue)
                {
                    std::wstring_view chunk = std::wstring_view{ line.data() + 1, line.size() - 1 };
                    trim(chunk);

                    if (chunk.size() >= 2 && chunk.front() == L'"' && chunk.back() == L'"')
                    {
                        chunk = chunk.substr(1, chunk.size() - 2);
                    }

                    if (addNewline)
                    {
                        pendingRaw += L'\n';
                    }

                    pendingRaw += chunk;
                }
                continue;
            }

            writePending();

            std::size_t eqPos = line.find(L'=');
            if (eqPos != std::wstring::npos)
            {
                std::wstring_view key = std::wstring_view{ line.data(), eqPos };
                trimRight(key);

                std::wstring_view val = std::wstring_view{ line.data() + eqPos + 1, line.size() - eqPos - 1 };
                trim(val);

                DomNodeBase* parent = stack.back();
                DomNodeBase* targetChild = nullptr;

                if (parent == &dummy)
                {
                    targetChild = &dummy;
                }
                else
                {
                    const bool parentIsSequence = (parent->type() == DomNodeType::Sequence);

                    if (val == L"[")
                    {
                        if (key.empty() && parentIsSequence)
                        {
                            targetChild = &static_cast<SequenceBase*>(parent)->addNode();
                        }
                        else if (parent->type() == DomNodeType::Section || parent->type() == DomNodeType::CompositeValue)
                        {
                            targetChild = static_cast<Section*>(parent)->child(key);

                            if (!targetChild)
                            {
                                parent->emitSchemaError(*parent, L"Schema mismatch: Undefined section key.");
                                targetChild = &dummy;
                            }
                            else if (targetChild->type() != DomNodeType::Section &&
                                targetChild->type() != DomNodeType::Sequence &&
                                targetChild->type() != DomNodeType::CompositeValue)
                            {
                                parent->emitSchemaError(*parent, L"Schema mismatch: Key expects a composite, but '[' found.");
                                targetChild = &dummy;
                            }
                        }
                    }
                    else
                    {
                        if (val.size() >= 2 && val.front() == L'"' && val.back() == L'"')
                        {
                            val = val.substr(1, val.size() - 2);
                        }

                        if (key.empty() && parentIsSequence)
                        {
                            targetChild = &static_cast<SequenceBase*>(parent)->addNode();
                        }
                        else if (parent->type() == DomNodeType::Section || parent->type() == DomNodeType::CompositeValue)
                        {
                            targetChild = static_cast<Section*>(parent)->child(key);

                            if (!targetChild)
                            {
                                parent->emitSchemaError(*parent, L"Schema mismatch: Undefined scalar key.");
                            }
                        }

                        if (targetChild)
                        {
                            if (targetChild->type() == DomNodeType::ScalarValue)
                            {
                                lastValue = static_cast<ScalarValueBase*>(targetChild);
                                pendingRaw = val;
                            }
                            else
                            {
                                parent->emitSchemaError(*parent, L"Schema mismatch: Key expects a composite, but scalar value found.");
                            }
                        }
                    }
                }

                if (val == L"[")
                {
                    if (targetChild)
                    {
                        // A list in the file is the whole list. The schema arrives carrying the
                        // member's default entries, so anything parsed out of the file lands behind
                        // them; a fixed-size read - std::array, which fills by position - would then
                        // hand back the defaults and drop what the file said.
                        if (targetChild->type() == DomNodeType::Sequence)
                        {
                            static_cast<SequenceBase*>(targetChild)->clear();
                        }
                        stack.push_back(targetChild);
                    }
                    else
                    {
                        stack.push_back(&dummy);
                    }
                }
            }
            else
            {
                DomNodeBase* parent = stack.back();
                if (parent != &dummy)
                {
                    parent->emitSchemaError(*parent, L"Syntax error: Line missing '=' assignment operator.");
                }
            }
        }

        writePending();
    }

    void ClaFi::saveSectionToStream(const Section& section, std::wostream& stream) const
    {
        saveSectionContent(section, stream, 0);
    }

    void ClaFi::saveSectionContent(const Section& section, std::wostream& stream, std::size_t indent) const
    {
        for (const auto& key : section.getKeys())
        {
            writeIndent(stream, L'\t', indent);
            stream << key << L"=";
            saveNodeRecursive(stream, *section.child(key), indent);
        }
    }

    void ClaFi::saveNodeRecursive(std::wostream& stream, const DomNodeBase& node, std::size_t indent) const
    {
        auto writeIndentLocal = [&]()
            {
                stream << std::wstring(indent, L'\t');
            };

        if (node.type() == DomNodeType::ScalarValue)
        {
            std::wstring val = static_cast<const ScalarValueBase&>(node).getAsRaw();

            if (val.find(L'\n') != std::wstring::npos)
            {
                // Every line the newlines cut the value into, the last one included when it is
                // empty: a value ending in a newline is written with an empty continuation, which
                // is what puts that newline back on reading.
                const std::wstring_view whole = val;
                std::size_t lineStart = 0;
                while (true)
                {
                    const std::size_t lineEnd = whole.find(L'\n', lineStart);
                    const std::size_t lineLength = lineEnd == std::wstring_view::npos
                        ? std::wstring_view::npos
                        : lineEnd - lineStart;
                    if (lineStart != 0)
                    {
                        stream << L"\n";
                        writeIndentLocal();
                        stream << L"|";
                    }
                    stream << L"\"" << whole.substr(lineStart, lineLength) << L"\"";
                    if (lineEnd == std::wstring_view::npos)
                    {
                        break;
                    }
                    lineStart = lineEnd + 1;
                }
            }
            else
            {
                const bool isOpeningBracket = (val == L"[");
                const bool isAlreadyBookended = (val.size() >= 2 && val.front() == L'"' && val.back() == L'"');
                const bool hasBoundaryWhitespace = (!val.empty() && (std::iswspace(val.front()) || std::iswspace(val.back())));

                if (isOpeningBracket || isAlreadyBookended || hasBoundaryWhitespace)
                {
                    stream << L"\"" << val << L"\"";
                }
                else
                {
                    stream << val;
                }
            }
            stream << L"\n";
        }
        else if (node.type() == DomNodeType::Section || node.type() == DomNodeType::CompositeValue)
        {
            stream << L"[\n";
            auto& section = static_cast<const Section&>(node);
            saveSectionContent(section, stream, indent + 1);
            writeIndentLocal();
            stream << L"]\n";
        }
        else if (node.type() == DomNodeType::Sequence)
        {
            stream << L"[\n";
            auto& seq = static_cast<const SequenceBase&>(node);
            for (std::size_t i = 0; i < seq.size(); ++i)
            {
                writeIndentLocal();
                stream << L"\t=";
                saveNodeRecursive(stream, *seq.child(i), indent + 1);
            }
            writeIndentLocal();
            stream << L"]\n";
        }
    }
}
