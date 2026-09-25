// =========================================================================
// ClaFi.Dom.Formats.Xml
// =========================================================================
//
// Not a general-purpose XML 1.0 parser — implements enough of XML to
// round-trip THIS engine's own schema-driven DOM. Known deviations:
//
// - No CDATA sections, XML comments ("<!-- -->"), DOCTYPE/DTD, namespaces,
//   or processing instructions other than the leading "<?xml ... ?>"
//   declaration (which is skipped unconditionally, not validated).
//   Encountering any of these mid-document will likely misparse, since
//   parseTagName's accepted character set (alnum, '_', '-', '.') doesn't
//   include '!', ':', or '?'.
// - Only double-quoted attribute values ("attr=\"value\"") are supported.
//   XML permits either double or single quotes for the same purpose;
//   single-quoted attributes ('attr=\'value\'') aren't recognized.
// - No numeric character references (&#65;, &#x41;) — only the five
//   predefined named entities (&amp; &lt; &gt; &quot; &apos;) are decoded.
// - No attribute-value whitespace normalization. The spec requires literal
//   tabs/newlines within an attribute value to be normalized to spaces by
//   a conformant processor; this reader preserves them verbatim.
// - No mixed content: an element's body is parsed as either pure text or
//   pure nested elements, never both together.
// - Schema-bound, not schema-inferring: every element/attribute name is
//   resolved against the pre-existing DOM schema via Section::child(). It
//   cannot represent an arbitrary XML document with no corresponding
//   schema; an unrecognized key emits a SchemaErrorEvent and is discarded
//   rather than preserved.
export module ClaFi.Dom.Formats.Xml;

import ClaFi.Core.DomEngine_Document;
import ClaFi.Core.DomEngine;
import ClaFi.StdLib;

namespace ClaFi::Dom::FileFormat
{
    export class Xml : public FileFormatBase
    {
    public:
        void writeHeader(std::wostream& stream) const override;
        void loadSectionFromStream(Section& section, std::wistream& stream) const override;
        void saveSectionToStream(const Section& section, std::wostream& stream) const override;

    private:
        void saveNode(std::wostream& stream, std::wstring_view tagName, const DomNodeBase& node, std::size_t indent) const;

        // Parses "<Name" — just enough for a parent to resolve the target
        // this element maps to — leaving `s` positioned right after the
        // name, before any attributes/'/'/'>' . Only the name needs
        // deferring like this; attributes are parsed and applied directly
        // in parseElementBody, one at a time, with no intermediate storage.
        static std::wstring parseElementName(std::wstring_view& s);
        static void parseElement(std::wstring_view& s, DomNodeBase* target);
        static void parseElementBody(std::wstring_view& s, DomNodeBase* target);

        static void skipWs(std::wstring_view& s);
        static bool match(std::wstring_view& s, wchar_t c);
        static bool matchLiteral(std::wstring_view& s, std::wstring_view literal);
        static std::wstring parseTagName(std::wstring_view& s);
        static std::wstring parseEscapedUntil(std::wstring_view& s, wchar_t terminator);
        static std::wstring escapeText(std::wstring_view sv);
        static std::wstring escapeAttribute(std::wstring_view sv);

    private:
        // Flip this to switch how scalars are written. When true, a
        // Section's scalar children are written as attributes on its own
        // opening tag instead of as separate child elements; a Section
        // with no remaining non-scalar children is self-closed. Reading
        // is unaffected by this flag either way — attributes are always
        // recognized on read, regardless of which mode wrote the file.
        static constexpr bool k_scalarsAsAttributes = true;
    };

    // ---------------------------------------------------------
    // Implementation
    // ---------------------------------------------------------

    void Xml::writeHeader(std::wostream& stream) const
    {
        stream << L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    }

    void Xml::loadSectionFromStream(Section& section, std::wistream& stream) const
    {
        std::wstringstream buffer = std::wstringstream{};
        buffer << stream.rdbuf();
        std::wstring content = buffer.str();

        std::wstring_view view = content;
        skipWs(view);

        // Skip an optional XML declaration: <?xml ... ?>
        // Not handled via readHeader(), same as ClaFi/Json: everything
        // about recognizing and discarding this prefix lives here, in the
        // one place that already reads and re-scans the whole stream.
        if (view.size() >= 2 && view[0] == L'<' && view[1] == L'?')
        {
            std::size_t end = view.find(L"?>");
            if (end != std::wstring_view::npos)
            {
                view.remove_prefix(end + 2);
            }
            skipWs(view);
        }

        if (view.empty() || view.front() != L'<')
        {
            return;
        }

        parseElement(view, &section);
    }

    void Xml::saveSectionToStream(const Section& section, std::wostream& stream) const
    {
        saveNode(stream, L"Document", section, 0);
        stream << L"\n";
    }

    void Xml::saveNode(std::wostream& stream, std::wstring_view tagName, const DomNodeBase& node, std::size_t indent) const
    {
        std::wstring pad = std::wstring(indent * 2, L' ');

        if (node.type() == DomNodeType::ScalarValue)
        {
            std::wstring raw = static_cast<const ScalarValueBase&>(node).getAsRaw();
            stream << pad << L"<" << tagName << L">" << escapeText(raw) << L"</" << tagName << L">\n";
        }
        else if (node.type() == DomNodeType::Section || node.type() == DomNodeType::CompositeValue)
        {
            auto& sec = static_cast<const Section&>(node);
            auto keys = sec.getKeys();

            std::vector<std::wstring_view> scalarKeys;
            std::vector<std::wstring_view> compositeKeys;

            for (const auto& key : keys)
            {
                const DomNodeBase* child = sec.child(key);
                if (k_scalarsAsAttributes && child->type() == DomNodeType::ScalarValue)
                {
                    scalarKeys.push_back(key);
                }
                else
                {
                    compositeKeys.push_back(key);
                }
            }

            stream << pad << L"<" << tagName;
            for (const auto& key : scalarKeys)
            {
                std::wstring raw = static_cast<const ScalarValueBase&>(*sec.child(key)).getAsRaw();
                stream << L" " << key << L"=\"" << escapeAttribute(raw) << L"\"";
            }

            if (compositeKeys.empty())
            {
                stream << L"/>\n";
                return;
            }

            stream << L">\n";
            for (const auto& key : compositeKeys)
            {
                saveNode(stream, key, *sec.child(key), indent + 1);
            }
            stream << pad << L"</" << tagName << L">\n";
        }
        else if (node.type() == DomNodeType::Sequence)
        {
            auto& seq = static_cast<const SequenceBase&>(node);

            stream << pad << L"<" << tagName << L">\n";
            for (std::size_t i = 0; i < seq.size(); ++i)
            {
                saveNode(stream, L"Item", *seq.child(i), indent + 1);
            }
            stream << pad << L"</" << tagName << L">\n";
        }
    }

    std::wstring Xml::parseElementName(std::wstring_view& s)
    {
        skipWs(s);
        match(s, L'<');
        return parseTagName(s);
    }

    void Xml::parseElement(std::wstring_view& s, DomNodeBase* target)
    {
        parseElementName(s); // consumed for syntax only — target is already known here
        parseElementBody(s, target);
    }

    // Parses an element's attributes, content, and closing tag into
    // `target`, given `s` positioned right after the name (the caller
    // already consumed "<Name" via parseElementName, either here or in a
    // parent's child loop). Attributes are parsed and applied one at a
    // time, directly — no intermediate collection. Tolerates target ==
    // nullptr throughout (an unrecognized key or a schema-mismatched
    // element): still fully consumes the element's tokens to stay in sync
    // with the stream, just doesn't store anything.
    void Xml::parseElementBody(std::wstring_view& s, DomNodeBase* target)
    {
        Section* sec = (target && (target->type() == DomNodeType::Section || target->type() == DomNodeType::CompositeValue))
            ? static_cast<Section*>(target)
            : nullptr;

        skipWs(s);
        while (!s.empty() && s.front() != L'/' && s.front() != L'>')
        {
            std::wstring attrName = parseTagName(s);
            match(s, L'=');
            match(s, L'"');
            std::wstring attrValue = parseEscapedUntil(s, L'"');
            match(s, L'"');
            skipWs(s);

            if (sec)
            {
                DomNodeBase* attrTarget = sec->child(attrName);
                if (attrTarget && attrTarget->type() == DomNodeType::ScalarValue)
                {
                    static_cast<ScalarValueBase*>(attrTarget)->setFromRaw(attrValue);
                }
                else if (!attrTarget)
                {
                    sec->emitSchemaError(*sec, L"Schema mismatch: Unknown XML attribute key.");
                }
            }
        }

        bool selfClosing = match(s, L'/');
        match(s, L'>');

        if (selfClosing)
        {
            if (target)
            {
                if (target->type() == DomNodeType::ScalarValue)
                {
                    static_cast<ScalarValueBase*>(target)->setFromRaw(L"");
                }
                else if (target->type() == DomNodeType::Sequence)
                {
                    static_cast<SequenceBase*>(target)->clear();
                }
            }
            return;
        }

        skipWs(s);

        bool startsWithTag = !s.empty() && s.front() == L'<';
        bool startsWithClosingTag = startsWithTag && s.size() >= 2 && s[1] == L'/';

        if (startsWithTag && !startsWithClosingTag)
        {
            // Nested child elements: target should be a Section/CompositeValue
            // (children looked up by tag name) or a Sequence (tag name
            // ignored, always appends a fresh item). `sec` was already
            // resolved above, alongside the attribute pass.
            SequenceBase* seq = nullptr;

            if (target)
            {
                if (target->type() == DomNodeType::Sequence)
                {
                    seq = static_cast<SequenceBase*>(target);
                    seq->clear();
                }
                else if (!sec)
                {
                    target->emitSchemaError(*target, L"Schema mismatch: XML expected a scalar value, nested elements found.");
                }
            }

            while (!s.empty() && !(s.front() == L'<' && s.size() >= 2 && s[1] == L'/'))
            {
                // Parse just the child's name — enough to resolve its
                // target — then continue parsing its attributes/content
                // via parseElementBody, starting from right after the name.
                std::wstring childName = parseElementName(s);
                DomNodeBase* childTarget = nullptr;

                if (sec)
                {
                    childTarget = sec->child(childName);
                    if (!childTarget)
                    {
                        sec->emitSchemaError(*sec, L"Schema mismatch: Unknown XML element key.");
                    }
                }
                else if (seq)
                {
                    childTarget = &seq->addNode();
                }

                parseElementBody(s, childTarget);
                skipWs(s);
            }
        }
        else if (startsWithClosingTag)
        {
            // "<Tag></Tag>" — immediately closed, empty content.
            if (target)
            {
                if (target->type() == DomNodeType::ScalarValue)
                {
                    static_cast<ScalarValueBase*>(target)->setFromRaw(L"");
                }
                else if (target->type() == DomNodeType::Sequence)
                {
                    static_cast<SequenceBase*>(target)->clear();
                }
                // Section/CompositeValue: nothing to do here — an empty
                // body just means none of this section's keys were given
                // as nested elements (they may still have arrived as
                // attributes, already applied above).
            }
        }
        else
        {
            // Plain text content.
            std::wstring text = parseEscapedUntil(s, L'<');
            if (target)
            {
                if (target->type() == DomNodeType::ScalarValue)
                {
                    static_cast<ScalarValueBase*>(target)->setFromRaw(text);
                }
                else
                {
                    target->emitSchemaError(*target, L"Schema mismatch: XML expected a composite or sequence, text found.");
                }
            }
        }

        // Consume the closing tag: </TagName>
        skipWs(s);
        match(s, L'<');
        match(s, L'/');
        parseTagName(s);
        skipWs(s);
        match(s, L'>');
    }

    void Xml::skipWs(std::wstring_view& s)
    {
        while (!s.empty() && std::iswspace(s.front()))
        {
            s.remove_prefix(1);
        }
    }

    bool Xml::match(std::wstring_view& s, wchar_t c)
    {
        skipWs(s);
        if (!s.empty() && s.front() == c)
        {
            s.remove_prefix(1);
            return true;
        }
        return false;
    }

    bool Xml::matchLiteral(std::wstring_view& s, std::wstring_view literal)
    {
        if (s.size() >= literal.size() && s.substr(0, literal.size()) == literal)
        {
            s.remove_prefix(literal.size());
            return true;
        }
        return false;
    }

    std::wstring Xml::parseTagName(std::wstring_view& s)
    {
        std::wstring name = std::wstring{};
        while (!s.empty() && (std::iswalnum(s.front()) || s.front() == L'_' || s.front() == L'-' || s.front() == L'.'))
        {
            name += s.front();
            s.remove_prefix(1);
        }
        return name;
    }

    std::wstring Xml::parseEscapedUntil(std::wstring_view& s, wchar_t terminator)
    {
        std::wstring res = std::wstring{};
        while (!s.empty() && s.front() != terminator)
        {
            if (s.front() == L'&')
            {
                if (matchLiteral(s, L"&amp;")) { res += L'&'; continue; }
                if (matchLiteral(s, L"&lt;")) { res += L'<'; continue; }
                if (matchLiteral(s, L"&gt;")) { res += L'>'; continue; }
                if (matchLiteral(s, L"&quot;")) { res += L'"'; continue; }
                if (matchLiteral(s, L"&apos;")) { res += L'\''; continue; }
            }
            res += s.front();
            s.remove_prefix(1);
        }
        return res;
    }

    std::wstring Xml::escapeText(std::wstring_view sv)
    {
        std::wstring res = std::wstring{};
        for (wchar_t c : sv)
        {
            if (c == L'&')
            {
                res += L"&amp;";
            }
            else if (c == L'<')
            {
                res += L"&lt;";
            }
            else if (c == L'>')
            {
                res += L"&gt;";
            }
            else
            {
                res += c;
            }
        }
        return res;
    }

    std::wstring Xml::escapeAttribute(std::wstring_view sv)
    {
        std::wstring res = std::wstring{};
        for (wchar_t c : sv)
        {
            if (c == L'&')
            {
                res += L"&amp;";
            }
            else if (c == L'<')
            {
                res += L"&lt;";
            }
            else if (c == L'"')
            {
                res += L"&quot;";
            }
            else
            {
                res += c;
            }
        }
        return res;
    }
}
