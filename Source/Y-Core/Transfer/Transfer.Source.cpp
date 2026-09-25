module ClaFi.Core.Transfer.Source;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    FormatList Source::nativeFormats() const
    {
        FormatList result{};
        result.reserve(m_slots.size());
        for (const SlotPtr& slot : m_slots)
        {
            result.push_back(slot->format);
        }

        return result;
    }

    FormatList Source::advertisedFormats() const
    {
        FormatList result = nativeFormats();
        const ConversionTable& table = ConversionTable::instance();
        for (const SlotPtr& slot : m_slots)
        {
            for (const Format& reachable : table.reachableFrom(slot->format))
            {
                if (std::ranges::find(result, reachable) == result.end())
                    result.push_back(reachable);
            }
        }

        return result;
    }

    bool Source::provides(const Format& format) const
    {
        if (findIn(m_slots, format))
            return true;

        const ConversionTable& table = ConversionTable::instance();
        for (const SlotPtr& slot : m_slots)
        {
            if (table.has(slot->format, format))
                return true;
        }

        return false;
    }

    const Payload* Source::content(const Format& format)
    {
        if (const Payload* held = findIn(m_slots, format))
            return held;

        if (const Payload* converted = findIn(m_converted, format))
            return converted;

        const ConversionTable& table = ConversionTable::instance();
        for (const SlotPtr& slot : m_slots)
        {
            const Converter converter = table.find(slot->format, format);
            if (!converter)
                continue;

            m_converted.push_back(std::make_unique<Slot>(Slot{ format, converter(slot->content) }));
            return &m_converted.back()->content;
        }

        return nullptr;
    }

    const Payload* Source::findIn(const SlotList& slots, const Format& format) const
    {
        for (const SlotPtr& slot : slots)
        {
            if (slot->format == format)
                return &slot->content;
        }

        return nullptr;
    }
}
