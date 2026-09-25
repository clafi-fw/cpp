export module ClaFi.Core.Transfer.Conversion;

import ClaFi.Core.Transfer.Formats;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    // A captured value held without its type. Every payload holds the Content type of the format
    // it sits under, which is what makes reading it back by that type sound.
    export using Payload = std::any;

    // Declared and specialised, never defined: a conversion exists where somebody wrote one.
    export template<IsTransferFormat From, IsTransferFormat To>
        struct FormatConversion;

    export template<typename From, typename To>
        concept IsConvertibleFormat = IsTransferFormat<From> && IsTransferFormat<To>
            && requires (const typename From::Content& content)
        {
            { FormatConversion<From, To>::convert(content) } -> std::same_as<typename To::Content>;
        };

    export using Converter = Payload (*)(const Payload&);

    // WHICH FORMATS REACH WHICH, and how. See Transfer
    export class ConversionTable
    {
    public:
        [[nodiscard]] static ConversionTable& instance();

        void add(Format from, Format to, Converter);
        // Null where nothing is registered for the pair.
        [[nodiscard]] Converter find(const Format& from, const Format& to) const;
        [[nodiscard]] bool has(const Format& from, const Format& to) const;
        // Everything one step from this format, in registration order.
        [[nodiscard]] FormatList reachableFrom(const Format&) const;
    private:
        struct Entry
        {
            Format from;
            Format to;
            Converter converter;
        };

        using EntryList = std::vector<Entry>;

        // EVERY ENTRY IS PUT HERE BY A CALL. A static library drops a translation unit nothing
        // references, so self-registration at static init would leave this table full in some
        // builds and empty in others.
        ConversionTable() = default;

        EntryList m_entries;
    };

    export template<IsTransferFormat From, IsTransferFormat To>
        requires IsConvertibleFormat<From, To>
    [[nodiscard]] Converter converterFor();

    export template<IsTransferFormat From, IsTransferFormat To>
        requires IsConvertibleFormat<From, To>
    void registerConversion();

}

namespace ClaFi::Transfer
{
    template<IsTransferFormat From, IsTransferFormat To>
        requires IsConvertibleFormat<From, To>
    Converter converterFor()
    {
        return [](const Payload& payload) -> Payload {
            const typename From::Content& content
                = std::any_cast<const typename From::Content&>(payload);
            return Payload{ FormatConversion<From, To>::convert(content) };
        };
    }

    template<IsTransferFormat From, IsTransferFormat To>
        requires IsConvertibleFormat<From, To>
    void registerConversion()
    {
        ConversionTable::instance().add(From::format(), To::format(), converterFor<From, To>());
    }
}
