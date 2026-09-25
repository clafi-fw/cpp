// =========================================================================
// ClaFi.Dom.Formats.Json
// =========================================================================
//
// Not a general-purpose RFC 8259 parser — implements enough of JSON to
// round-trip THIS engine's own schema-driven DOM. Known deviations:
//
// - No \uXXXX Unicode escape support. parseString's escape handling
//   recognizes \n, \t, \r, \\, and \" only; any other escape (including
//   \uXXXX) falls through to appending the character literally, dropping
//   the backslash — so \u00e9 becomes the five characters "u00e9", not é.
//   This is a real misparse of valid JSON, not just an unsupported case.
// - The root value must be a JSON object ("{...}"); loadSectionFromStream
//   returns immediately if the document doesn't start with '{'. A bare
//   top-level array or scalar, which RFC 8259 permits, isn't accepted.
// - Whitespace and number scanning are more permissive than the spec:
//   skipWs accepts any std::iswspace character (RFC 8259 only defines
//   space/tab/LF/CR as insignificant whitespace), and number tokens
//   aren't validated against the grammar (leading zeros, a lone
//   fractional part with no integer digit, etc. are all accepted rather
//   than rejected).
// - Schema-bound, not schema-inferring: every object key is resolved
//   against the pre-existing DOM schema via Section::child(). It cannot
//   represent an arbitrary JSON document with no corresponding schema; an
//   unrecognized key emits a SchemaErrorEvent and is discarded rather
//   than preserved.
export module ClaFi.Dom.Formats.Json;

import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine;
import ClaFi.StdLib;

namespace ClaFi::Dom::FileFormat
{
    export class Json : public FileFormatBase
    {
    public:
        void loadSectionFromStream(Section& section, std::wistream& stream) const override;
        void saveSectionToStream(const Section& section, std::wostream& stream) const override;

    private:
        void saveNode(std::wostream& stream, const DomNodeBase& node, std::size_t indent = 0) const;
        static void parseValue(std::wstring_view& s, DomNodeBase* target);

        static void skipWs(std::wstring_view& s);
        static bool match(std::wstring_view& s, wchar_t c);
        static std::wstring parseString(std::wstring_view& s);
        static std::wstring parsePrimitive(std::wstring_view& s);
        static bool isJsonPrimitive(std::wstring_view sv);
        static std::wstring escapeString(std::wstring_view sv);
    };

    // ---------------------------------------------------------
    // Implementation
    // ---------------------------------------------------------

    void Json::loadSectionFromStream(Section& section, std::wistream& stream) const
    {
        std::wstringstream buffer = std::wstringstream{};
        buffer << stream.rdbuf();
        std::wstring content = buffer.str();

        std::wstring_view view = content;
        skipWs(view);

        if (view.empty() || view.front() != L'{')
        {
            return;
        }

        parseValue(view, &section);
    }

    void Json::saveSectionToStream(const Section& section, std::wostream& stream) const
    {
        saveNode(stream, section, 0);
        stream << L"\n";
    }

    void Json::saveNode(std::wostream& stream, const DomNodeBase& node, std::size_t indent) const
    {
        auto writeIndentLocal = [&]()
            {
                stream << std::wstring(indent, L' ');
            };

        if (node.type() == DomNodeType::ScalarValue)
        {
            std::wstring raw = static_cast<const ScalarValueBase&>(node).getAsRaw();

            if (isJsonPrimitive(raw))
            {
                stream << raw;
            }
            else
            {
                stream << escapeString(raw);
            }
        }
        else if (node.type() == DomNodeType::Section || node.type() == DomNodeType::CompositeValue)
        {
            stream << L"{\n";
            auto& sec = static_cast<const Section&>(node);
            auto keys = sec.getKeys();

            for (std::size_t i = 0; i < keys.size(); ++i)
            {
                writeIndentLocal();
                stream << L"  " << escapeString(keys[i]) << L": ";
                saveNode(stream, *sec.child(keys[i]), indent + 2);

                if (i < keys.size() - 1)
                {
                    stream << L",";
                }
                stream << L"\n";
            }

            writeIndentLocal();
            stream << L"}";
        }
        else if (node.type() == DomNodeType::Sequence)
        {
            stream << L"[\n";
            auto& seq = static_cast<const SequenceBase&>(node);

            for (std::size_t i = 0; i < seq.size(); ++i)
            {
                writeIndentLocal();
                stream << L"  ";
                saveNode(stream, *seq.child(i), indent + 2);

                if (i < seq.size() - 1)
                {
                    stream << L",";
                }
                stream << L"\n";
            }

            writeIndentLocal();
            stream << L"]";
        }
    }

    void Json::parseValue(std::wstring_view& s, DomNodeBase* target)
    {
        skipWs(s);
        if (s.empty())
        {
            return;
        }

        if (s.front() == L'{')
        {
            s.remove_prefix(1);
            skipWs(s);

            Section* sec = nullptr;
            if (target && (target->type() == DomNodeType::Section || target->type() == DomNodeType::CompositeValue))
            {
                sec = static_cast<Section*>(target);
            }

            while (!s.empty() && s.front() != L'}')
            {
                std::wstring key = parseString(s);
                match(s, L':');

                DomNodeBase* childPtr = sec ? sec->child(key) : nullptr;
                if (sec && !childPtr)
                {
                    sec->emitSchemaError(*sec, L"Schema mismatch: Unknown JSON object key.");
                }

                parseValue(s, childPtr);
                match(s, L',');
                skipWs(s);
            }
            match(s, L'}');
        }
        else if (s.front() == L'[')
        {
            s.remove_prefix(1);
            skipWs(s);

            SequenceBase* seq = nullptr;
            if (target && target->type() == DomNodeType::Sequence)
            {
                seq = static_cast<SequenceBase*>(target);
            }

            if (seq)
            {
                seq->clear();
            }

            while (!s.empty() && s.front() != L']')
            {
                DomNodeBase* child = seq ? &seq->addNode() : nullptr;
                parseValue(s, child);
                match(s, L',');
                skipWs(s);
            }
            match(s, L']');
        }
        else if (s.front() == L'"')
        {
            std::wstring val = parseString(s);
            if (target)
            {
                if (target->type() == DomNodeType::ScalarValue)
                {
                    static_cast<ScalarValueBase*>(target)->setFromRaw(val);
                }
                else
                {
                    target->emitSchemaError(*target, L"Schema mismatch: JSON expected composite structure, string found.");
                }
            }
        }
        else
        {
            std::wstring val = parsePrimitive(s);
            if (target)
            {
                if (target->type() == DomNodeType::ScalarValue)
                {
                    static_cast<ScalarValueBase*>(target)->setFromRaw(val);
                }
                else
                {
                    target->emitSchemaError(*target, L"Schema mismatch: JSON expected composite structure, primitive found.");
                }
            }
        }
    }

    void Json::skipWs(std::wstring_view& s)
    {
        while (!s.empty() && std::iswspace(s.front()))
        {
            s.remove_prefix(1);
        }
    }

    bool Json::match(std::wstring_view& s, wchar_t c)
    {
        skipWs(s);
        if (!s.empty() && s.front() == c)
        {
            s.remove_prefix(1);
            return true;
        }
        return false;
    }

    std::wstring Json::parseString(std::wstring_view& s)
    {
        match(s, L'"');
        std::wstring res = std::wstring{};

        while (!s.empty() && s.front() != L'"')
        {
            if (s.front() == L'\\')
            {
                s.remove_prefix(1);
                if (s.empty())
                {
                    break;
                }

                wchar_t c = s.front();
                if (c == L'n')
                {
                    res += L'\n';
                }
                else if (c == L't')
                {
                    res += L'\t';
                }
                else if (c == L'r')
                {
                    res += L'\r';
                }
                else
                {
                    res += c;
                }
            }
            else
            {
                res += s.front();
            }
            s.remove_prefix(1);
        }

        if (!s.empty())
        {
            s.remove_prefix(1);
        }
        return res;
    }

    std::wstring Json::parsePrimitive(std::wstring_view& s)
    {
        std::wstring res = std::wstring{};
        while (!s.empty() && !std::iswspace(s.front()) && s.front() != L',' && s.front() != L']' && s.front() != L'}')
        {
            res += s.front();
            s.remove_prefix(1);
        }
        return res;
    }

    bool Json::isJsonPrimitive(std::wstring_view sv)
    {
        if (sv == L"true" || sv == L"false" || sv == L"null")
        {
            return true;
        }
        if (sv.empty())
        {
            return false;
        }

        std::size_t start = (sv.front() == L'-') ? 1 : 0;
        bool hasDot = false;

        for (std::size_t i = start; i < sv.size(); ++i)
        {
            if (sv[i] == L'.')
            {
                if (hasDot)
                {
                    return false;
                }
                hasDot = true;
            }
            else if (!std::iswdigit(sv[i]))
            {
                return false;
            }
        }
        return true;
    }

    std::wstring Json::escapeString(std::wstring_view sv)
    {
        std::wstring res = L"\"";
        for (wchar_t c : sv)
        {
            if (c == L'"')
            {
                res += L"\\\"";
            }
            else if (c == L'\\')
            {
                res += L"\\\\";
            }
            else if (c == L'\n')
            {
                res += L"\\n";
            }
            else if (c == L'\t')
            {
                res += L"\\t";
            }
            else if (c == L'\r')
            {
                res += L"\\r";
            }
            else
            {
                res += c;
            }
        }
        res += L"\"";
        return res;
    }
}
