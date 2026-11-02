export module ClaFi.Tools.WhatsClip.TextPage;

import ClaFi.Tools.WhatsClip.Page;
import ClaFi.Tools.WhatsClip.Encodings;

import ClaFi.Controls.MessageBar;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Foundation;

import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    // Plain text, shown as the text it is.
    export class TextPage : public TextPageBase
    {
    public:
        TextPage(const CreateParams&, const PickLists&);
    public:
        [[nodiscard]] std::optional<ByteSelection> byteSelection() const override;
        void setByteSelection(const ByteSelection&) override;
    protected:
        void showContent(Transfer::Offer&, const Transfer::Format&) override;
        void encodingPicked() override;
    private:
        // The bytes kept, as the text they read as in the encoding picked - or, while they are
        // held back, the question whether to read them at all.
        void showBytes();
        // The band asks before a large binary block is read, and the box stays empty.
        void showHeldBack();
        // The band's answer: the block is read as text after all.
        void readHeldBack();
        // The count into the strip - and how much of the format it is, where the limit cut it -
        // and the text into the box, its byte starts kept. None where the read failed.
        void showReading(std::optional<DecodedText>&&, std::size_t wholeSize,
            std::wstring_view unit);
        // The last place in the text at or before a byte boundary, never between a pair's units.
        [[nodiscard]] std::size_t positionAtOrBefore(std::size_t byteBoundary) const;
        // The first place in the text at or after a byte boundary - the end, where none is.
        [[nodiscard]] std::size_t positionAtOrAfter(std::size_t byteBoundary) const;
        // **The most of a format read as text.** The engine shapes a document whole before its
        // first paint, and a screenshot is tens of megabytes of bytes that are not text; the
        // byte view shows the rest. Room for a source tree's worth of text.
        static constexpr std::size_t k_textLimit = 4 * 1024 * 1024;
        static_assert(k_textLimit <= std::numeric_limits<ByteStarts::value_type>::max(),
            "a reading's byte starts must reach every byte read as text");
        // **A binary format larger than this waits to be asked for.** Bytes that are no text can
        // lay out as one line of millions of characters, which takes minutes; a smaller block is
        // read.
        static constexpr std::size_t k_askAbove = 64 * 1024;
    private:
        // The format's first k_textLimit bytes at most, kept so that a new encoding reads them
        // again without a second transfer out of the clipboard. Empty after a read that
        // answered nothing.
        std::string m_bytes{};
        std::size_t m_byteCount{ 0 };   // the whole format's, for the strip
        // The byte in m_bytes each unit of the box's text starts at - empty while it shows none.
        ByteStarts m_byteStarts{};
        // The format this page shows, once it has been asked to show one.
        std::optional<Transfer::Format> m_format{};
        bool m_binary{ false }; // whether the bytes kept were judged binary - see k_askAbove
        // Whether the bytes kept wait for the band's question to be answered - see k_askAbove.
        bool m_heldBack{ false };
        Controls::MessageBar& m_question;   // shown while the bytes are held back
    };
}
