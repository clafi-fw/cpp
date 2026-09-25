module ClaFi.Core.Transfer.Conversion;

import ClaFi.Core.Transfer.Formats;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    ConversionTable& ConversionTable::instance()
    {
        static ConversionTable table{};
        return table;
    }

    void ConversionTable::add(Format from, Format to, const Converter converter)
    {
        m_entries.push_back(Entry{ std::move(from), std::move(to), converter });
    }

    Converter ConversionTable::find(const Format& from, const Format& to) const
    {
        for (const Entry& entry : m_entries)
        {
            if (entry.from == from && entry.to == to)
                return entry.converter;
        }

        return nullptr;
    }

    bool ConversionTable::has(const Format& from, const Format& to) const
    {
        return find(from, to) != nullptr;
    }

    FormatList ConversionTable::reachableFrom(const Format& from) const
    {
        FormatList result{};
        for (const Entry& entry : m_entries)
        {
            if (entry.from == from)
                result.push_back(entry.to);
        }

        return result;
    }
}
