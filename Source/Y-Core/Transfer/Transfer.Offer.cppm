export module ClaFi.Core.Transfer.Offer;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    // WHICH FORMAT TO ASK FOR, and what it becomes once read.
    export struct Match
    {
        Format offered;
        Format accepted;
    };

    // What arrives from outside: the formats advertised, and a way to ask for one. See Transfer
    export class Offer
    {
    public:
        virtual ~Offer() = default;

        // THE PLATFORM'S FORMATS, EVERY ONE UNDER ITS PLATFORM NAME - CF_UNICODETEXT beside
        // CF_TEXT, image/png beside image/bmp - so a viewer of the list sees what is on the
        // clipboard. The framework's own formats are never in it: they are asked for through
        // platformFormat.
        [[nodiscard]] virtual FormatList advertisedFormats() const = 0;
        // THE PLATFORM FORMAT A FRAMEWORK FORMAT IS SERVED FROM: one of the advertised formats, by
        // reference, or null where none of them can serve it. A paste asks for Text or Picture,
        // and this is the platform deciding which of its formats fits - a PNG ahead of a DIB.
        [[nodiscard]] virtual const Format* platformFormat(StandardFormat) const = 0;
        // Decoded: a framework format as its Content, read from the platform format that serves
        // it; a format with a byte spelling as that; anything else as its bytes.
        [[nodiscard]] virtual Payload read(const Format&) = 0;
        // THE BYTES BEHIND A FORMAT, WITH NO SPELLING APPLIED. `read` decodes: a format this
        // application knows comes back as its Content and the bytes it travelled as are gone. A
        // reader that wants to show what actually arrived asks here instead.
        //
        // What comes back is the CONTENT's bytes and not the envelope around them: a platform
        // that wraps a payload for its own carriage unwraps it here, so both platforms answer the
        // same bytes for the same clipboard.
        //
        // A FRAMEWORK FORMAT'S BYTES ARE THOSE OF THE PLATFORM FORMAT SERVING IT - UTF-16 and CRLF
        // on one side, UTF-8 and LF on the other, a PNG file where that is what the picture came
        // as. Nothing normalises them, because what they are is the thing being looked at.
        //
        // Empty where the read failed, which is what a null Payload says on the other call, AND
        // EMPTY WHERE THE FORMAT CARRIES NO BYTES - a handle format has none, and this call will
        // not invent any. What it has is read through readHandle, which is what tells the two
        // apart.
        [[nodiscard]] virtual std::string readBytes(const Format&) = 0;
        // THE HANDLE A FORMAT CARRIES, WHERE WHAT IS OFFERED IS NOT BYTES AT ALL. A platform may
        // advertise a format whose entry is a handle to an object it holds - a bitmap, a palette, a
        // metafile - and such a format has nothing on the wire to hand over. Anything made out of
        // the object instead would be this framework's bytes rather than the clipboard's, so the
        // number is what there is, and a reader that wants to say what is offered says it.
        //
        // THE VALUE IS THE ONE THIS READ WAS HANDED and no more than that: a platform that copies a
        // handle per reader answers a different number to each of them, and the object can be gone
        // by the time the number is looked at. It says what kind of thing is offered, never where
        // to find it.
        //
        // Zero where the format carries bytes, and zero where the read failed - a caller that knows
        // the format is a handle format is the one that can tell those apart. A platform holding no
        // handle formats at all answers zero throughout, which is the default here.
        [[nodiscard]] virtual std::uint64_t readHandle(const Format&) { return 0; }

        // The first accepted format this offer answers - a framework format through the platform
        // format serving it, any other directly or by one conversion. Walked in the target's
        // preference order, and a direct answer wins over a converted one for the same format.
        // Nothing where the two sets meet nowhere, which is what a paste action reads for its
        // state.
        [[nodiscard]] std::optional<Match> match(const FormatList& accepted) const;
        // Reads what the match names and converts it to the accepted format. Empty where the read
        // answered nothing.
        [[nodiscard]] Payload take(const Match&);
    };

    export template<IsTransferFormat F>
        [[nodiscard]] std::optional<typename F::Content> take(Offer&);
}

namespace ClaFi::Transfer
{
    template<IsTransferFormat F>
        std::optional<typename F::Content> take(Offer& offer)
    {
        const std::optional<Match> found = offer.match(FormatList{ F::format() });
        if (!found)
            return std::nullopt;

        const Payload payload = offer.take(*found);
        if (!payload.has_value())
            return std::nullopt;

        return std::any_cast<const typename F::Content&>(payload);
    }
}
