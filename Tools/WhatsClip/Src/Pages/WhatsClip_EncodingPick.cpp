module ClaFi.Tools.WhatsClip.EncodingPick;

import ClaFi.Tools.WhatsClip.AutoPick;
import ClaFi.Tools.WhatsClip.Encodings;

import ClaFi.Core.Foundation;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    std::optional<EncodingEntry> EncodingPick::pickedEncoding() const
    {
        const std::optional<std::size_t> picked = pickedIndex();
        if (!picked)
            return std::nullopt;

        return m_encodings[*picked];
    }

    std::wstring EncodingPick::decode(const std::wstring_view formatName,
        const std::string_view bytes)
    {
        std::optional<EncodingEntry> entry = pickedEncoding();
        if (!entry)
        {
            entry = detectEncoding(m_encodings, formatName, bytes);
            setFoundName(entry ? entry->name : std::wstring_view{});
        }

        return entry ? entry->decode(bytes) : fromUtf8(bytes);
    }

    AutoPicker::Names EncodingPick::namesOf(const EncodingList& encodings)
    {
        Names names{};
        names.reserve(encodings.size());
        for (const EncodingEntry& entry : encodings)
            names.push_back(entry.name);

        return names;
    }
}
