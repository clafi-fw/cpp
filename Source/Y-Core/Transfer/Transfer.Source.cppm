export module ClaFi.Core.Transfer.Source;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Formats;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    // What leaves the application: the content captured when the user copied. See Transfer
    export class Source
    {
    public:
        Source() = default;
        Source(const Source&) = delete;
        Source& operator=(const Source&) = delete;
        Source(Source&&) = default;
        Source& operator=(Source&&) = default;

        template<IsTransferFormat F>
            void add(typename F::Content content);

        [[nodiscard]] bool empty() const { return m_slots.empty(); }
        // Held outright, in the order they were added.
        [[nodiscard]] FormatList nativeFormats() const;
        // The native formats, then everything one conversion away from one of them. This is what
        // a context advertises: without it a conversion would only ever serve ClaFi talking to
        // itself, because nobody else would know to ask.
        [[nodiscard]] FormatList advertisedFormats() const;
        [[nodiscard]] bool provides(const Format&) const;
        // Converts on the first ask and keeps the result, so a second paste of the same format
        // converts once. Null for a format this does not provide.
        [[nodiscard]] const Payload* content(const Format&);
    private:
        struct Slot
        {
            Format format;
            Payload content;
        };

        // Held by pointer so that a list growing after a read cannot move what the read handed
        // out. A platform serves one paste at a time from pointers taken earlier.
        using SlotPtr = std::unique_ptr<Slot>;
        using SlotList = std::vector<SlotPtr>;

        [[nodiscard]] const Payload* findIn(const SlotList&, const Format&) const;

        SlotList m_slots;
        // Conversion results. Not part of what is native - nativeFormats answers from m_slots
        // alone, and advertise order depends on that staying true.
        SlotList m_converted;
    };
}

namespace ClaFi::Transfer
{
    template<IsTransferFormat F>
        void Source::add(typename F::Content content)
    {
        m_slots.push_back(std::make_unique<Slot>(Slot{ F::format(), Payload{ std::move(content) } }));
    }
}
