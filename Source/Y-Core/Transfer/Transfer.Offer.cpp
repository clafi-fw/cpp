module ClaFi.Core.Transfer.Offer;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    std::optional<Match> Offer::match(const FormatList& accepted) const
    {
        const FormatList advertised = advertisedFormats();
        const ConversionTable& table = ConversionTable::instance();
        for (const Format& wanted : accepted)
        {
            const bool served = wanted.kind() == Format::Kind::Standard
                ? platformFormat(wanted.standard()) != nullptr
                : std::ranges::find(advertised, wanted) != advertised.end();
            if (served)
                return Match{ wanted, wanted };

            for (const Format& offered : advertised)
            {
                if (table.has(offered, wanted))
                    return Match{ offered, wanted };
            }
        }

        return std::nullopt;
    }

    Payload Offer::take(const Match& match)
    {
        Payload payload = read(match.offered);
        if (!payload.has_value())
            return {};

        if (match.offered == match.accepted)
            return payload;

        const Converter converter = ConversionTable::instance().find(match.offered, match.accepted);
        if (!converter)
            return {};

        return converter(payload);
    }
}
