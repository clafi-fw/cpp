export module ClaFi.Core.Transfer.Bytes;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    // A FORMAT THAT STATES ITS OWN BYTES. A named format does: what one process writes is read
    // back by another running the same application, and no display server has a say in the
    // spelling.
    //
    // The standard set states none. Its bytes ARE the platform's - UTF-16 and CRLF on one side,
    // UTF-8 and LF on the other - and the platform layer answers for those itself, in
    // nativeBytes.
    export template<typename F>
        concept HasByteSpelling = IsTransferFormat<F>
            && requires (const typename F::Content& content, std::string_view bytes)
        {
            { F::toBytes(content) } -> std::same_as<std::string>;
            { F::fromBytes(bytes) } -> std::same_as<typename F::Content>;
        };

    export struct ByteSpelling
    {
        std::string (*toBytes)(const Payload&);
        Payload (*fromBytes)(std::string_view);
    };

    // WHICH FORMATS THIS APPLICATION KNOWS HOW TO DEAL WITH, and how. See Transfer
    export class ByteSpellingTable
    {
    public:
        [[nodiscard]] static ByteSpellingTable& instance();

        void add(Format, ByteSpelling);
        // Null where nothing is registered for the format. The bytes are still readable - see
        // toBytes and fromBytes, which fall back to handing them through unchanged.
        [[nodiscard]] const ByteSpelling* find(const Format&) const;
        // Every format registered here, in registration order. What a platform walks to answer
        // which of its own spellings names a format this application declared.
        [[nodiscard]] FormatList knownFormats() const;
    private:
        struct Entry
        {
            Format format;
            ByteSpelling spelling;
        };

        using EntryList = std::vector<Entry>;

        ByteSpellingTable() = default;

        EntryList m_entries;
    };

    export template<HasByteSpelling F>
        void registerByteSpelling();

    // THE BYTES A FORMAT TRAVELS AS, WHERE THIS APPLICATION IS THE ONE SPELLING IT. A format
    // nothing is registered for converts nothing: its content IS the bytes, which is what lets a
    // clipboard viewer read a format this application has never heard of. So both of these always
    // answer, and neither has a "no" to test for.
    //
    // THE STANDARD SET IS NOT SPELLED HERE. Nothing is registered for it and its content is not a
    // byte string, so these answer nothing for it - nativeBytes is what spells it.
    export [[nodiscard]] std::string toBytes(const Format&, const Payload&);
    export [[nodiscard]] Payload fromBytes(const Format&, std::string_view bytes);
}

namespace ClaFi::Transfer
{
    template<HasByteSpelling F>
        void registerByteSpelling()
    {
        ByteSpellingTable::instance().add(F::format(), ByteSpelling{
            .toBytes = [](const Payload& payload) -> std::string {
                return F::toBytes(std::any_cast<const typename F::Content&>(payload));
            },
            .fromBytes = [](const std::string_view bytes) -> Payload {
                return Payload{ F::fromBytes(bytes) };
            }
        });
    }
}
