module ClaFi.Core.Transfer.Bytes;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    std::string toBytes(const Format& format, const Payload& payload)
    {
        if (const ByteSpelling* spelling = ByteSpellingTable::instance().find(format))
            return spelling->toBytes(payload);

        // Nothing is registered, so the content is the bytes themselves.
        const std::string* bytes = std::any_cast<std::string>(&payload);
        return bytes ? *bytes : std::string{};
    }

    Payload fromBytes(const Format& format, const std::string_view bytes)
    {
        if (const ByteSpelling* spelling = ByteSpellingTable::instance().find(format))
            return spelling->fromBytes(bytes);

        return Payload{ std::string{ bytes } };
    }

    ByteSpellingTable& ByteSpellingTable::instance()
    {
        static ByteSpellingTable table{};
        return table;
    }

    void ByteSpellingTable::add(Format format, const ByteSpelling spelling)
    {
        m_entries.push_back(Entry{ std::move(format), spelling });
    }

    const ByteSpelling* ByteSpellingTable::find(const Format& format) const
    {
        for (const Entry& entry : m_entries)
        {
            if (entry.format == format)
                return &entry.spelling;
        }

        return nullptr;
    }

    FormatList ByteSpellingTable::knownFormats() const
    {
        FormatList result{};
        result.reserve(m_entries.size());
        for (const Entry& entry : m_entries)
        {
            result.push_back(entry.format);
        }

        return result;
    }
}
