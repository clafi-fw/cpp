module ClaFi.Tools.WhatsClip.HandlePage;

import ClaFi.Tools.WhatsClip.Page;

import ClaFi.Controls.ScrollBox;
import ClaFi.Controls.TextBox;
import ClaFi.Controls.Base.PanelBase;

import ClaFi.Core.Foundation;

import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.TextEngine.Text;
import ClaFi.Core.TextEngine.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Props;
import ClaFi.StdLib;

namespace ClaFi::Tools::WhatsClip
{
    namespace
    {
        constexpr std::wstring_view k_hexDigits = L"0123456789ABCDEF";

        // WHAT THE NUMBER IS WORTH, on the page rather than left for a reader to assume.
        constexpr std::wstring_view k_handleNote =
            L"A handle names an object rather than an address, and no bytes travelled for this "
            L"format. The value is the one this read was handed - the clipboard hands each reader "
            L"its own, and it was void again the moment the read was done with it.";

        // A platform format whose entry is a handle: its name, the type the handle is of, and what
        // the object is.
        struct HandleFormat
        {
            std::wstring_view name;
            std::wstring_view handleType;
            std::wstring_view holds;
        };

        // WINDOWS SPELLINGS, since Windows is the platform that offers an object where the other
        // offers a pipe. A metafile picture is not among them: its entry is memory holding the
        // handle, and memory is bytes like anything else.
        constexpr HandleFormat k_handleFormats[] = {
            { L"CF_BITMAP", L"HBITMAP", L"a GDI bitmap" },
            { L"CF_PALETTE", L"HPALETTE", L"a GDI palette" },
            { L"CF_ENHMETAFILE", L"HENHMETAFILE", L"an enhanced metafile" }
        };

        [[nodiscard]] const HandleFormat* entryFor(const Transfer::Format& format)
        {
            if (format.kind() != Transfer::Format::Kind::Custom)
                return nullptr;

            for (const HandleFormat& entry : k_handleFormats)
            {
                if (entry.name == format.name())
                    return &entry;
            }

            return nullptr;
        }

        // WRITTEN TO A WIDTH AND NOT TRIMMED TO ITS DIGITS: a handle is a place in a table rather
        // than a quantity, so a leading zero of it is as much of the value as any other digit.
        [[nodiscard]] std::wstring hexOf(const std::uint64_t value)
        {
            const std::size_t digits = value > 0xFFFFFFFFull ? 16u : 8u;

            std::wstring result{ L"0x" };
            for (std::size_t shift = digits * 4u; shift > 0u; shift -= 4u)
            {
                result += k_hexDigits[(value >> (shift - 4u)) & 0xFull];
            }

            return result;
        }
    }

    HandlePage::HandlePage(const CreateParams& params)
        :
        HandlePageBase{
            params,
            HostProps{ ScrollBars::Both },
            BodyProps{ ReadOnly::Yes, WordWrap::Yes, Padding{ 12.0f } }
        }
    {
    }

    bool HandlePage::carriesHandle(const Transfer::Format& format)
    {
        return entryFor(format) != nullptr;
    }

    // A HANDLE OF ZERO IS A FAILED READ HERE. The page is only ever offered for a format whose
    // entry is a handle, so nothing else a zero could mean is reachable, and the sentence every
    // other page says of a clipboard that held its peace is the right one.
    void HandlePage::showContent(Transfer::Offer& offer, const Transfer::Format& format)
    {
        const HandleFormat* entry = entryFor(format);
        const std::uint64_t value = offer.readHandle(format);
        const bool answered = entry && value;

        Text status{};
        status << TextStyleId::SubBody;
        if (answered)
        {
            status << L"A handle to ";
            status << entry->holds;
            status << L" - this format carries no bytes.";
        }
        else
        {
            status << k_noAnswer;
        }

        writeStatus(std::move(status));

        Text reading{};
        if (answered)
        {
            reading << TextStyleId::Code;
            reading << entry->handleType;
            reading << L"  ";
            reading << hexOf(value);
            reading << PopTextStyle{};
            reading << L"\n\n";
            reading << InkGrade::Muted;
            reading << k_handleNote;
            reading << PopColor{};
        }

        body().text() = std::move(reading);
        body().invalidateFormAlign();
        // Nothing on this page reads where the caret is, but a selection left in the last value
        // would paint over this one.
        body().setCaretPos(0);
    }
}
