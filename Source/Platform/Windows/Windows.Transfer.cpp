module;
#include "Windows.Headers.h"
// WIN32_LEAN_AND_MEAN leaves OLE out of Windows.h, so the clipboard's own half is named here.
// shlobj.h is for SHCreateStdEnumFmtEtc, which spares this file a second COM class.
#include <ole2.h>
#include <ShlObj.h>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

// AN IMPLEMENTATION UNIT OF THE MODULE THAT DECLARED Clipboard, not of the Windows layer. A class
// member belongs to the module its class was declared in and must be defined there, so the bodies
// of Clipboard:: live here whatever platform supplies them.
module ClaFi.Core.Transfer.Clipboard;

import ClaFi.Platform.Windows.Window;

import ClaFi.Core.Transfer.Bytes;
import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Graphics.Dib;
import ClaFi.Core.Graphics.Png;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    namespace
    {
        using Microsoft::WRL::ComPtr;
        using PlatformImplementation::Windows::WinApiMsg;
        using PlatformImplementation::Windows::IMessageSink;
        using PlatformImplementation::Windows::IMessageWindowFactory;
        using PlatformImplementation::Windows::MessageWindow;
        using FormatEtcList = std::vector<FORMATETC>;

        // How a named format's payload says where it ends. GlobalSize may answer more than was
        // written, and a payload this framework owns has no other way to state its length. A wire
        // format's bytes came from another application and go back out exactly as they arrived.
        using PayloadLength = std::uint32_t;

        constexpr UINT_PTR k_retryTimer = 1;
        constexpr UINT k_retryDelay = 1000;
    }

    // THE CLIPBOARD AS OLE HAS IT: the data object this process stands behind, the generations
    // the held reader and the own copy were seen at, and the window that hears of a change. See
    // Transfer
    struct NativeClipboard : public IMessageSink
    {
        explicit NativeClipboard(Clipboard&);
        // RENDERS THE PACKAGE ONTO THE CLIPBOARD ON THE WAY OUT, while this process still owns
        // it, so the copy outlives the process. OLE refuses the flush from any other owner, so
        // it is asked first - and a package another application's copy replaced has already
        // been released with it.
        ~NativeClipboard() override;
        NativeClipboard(const NativeClipboard&) = delete;
        NativeClipboard& operator=(const NativeClipboard&) = delete;

        // The held package's content for a format, as the bytes this clipboard carries it as.
        [[nodiscard]] std::string bytesFor(const Format&);
        // Whether the held package provides a format, which is what QueryGetData answers.
        [[nodiscard]] bool provides(const Format&);
        // BRINGS THE HELD READER IN LINE WITH THE CLIPBOARD, or answers false while another holder
        // keeps it shut - in which case the last reading stands, its generation with it, and the
        // next call reads again. A reading that could not be made is not an empty clipboard.
        bool refreshOffer();
        // TAKES A LISTENER WINDOW FROM THE PLATFORM and puts it on the clipboard's format listener
        // list. AddClipboardFormatListener has to name a window, and WM_CLIPBOARDUPDATE is the
        // only message it exists to receive - see wndProc, which is where the window delivers.
        void startListening();
        // WM_CLIPBOARDUPDATE, and the retry's timer. Windows tells every listener, this process's
        // own copies included. That is right rather than wasteful: a cut changes what a paste
        // would give just as another application's copy does.
        void wndProc(WinApiMsg&) override;

        Clipboard& owner;
        ComPtr<IDataObject> dataObject{};
        // The clipboard generation the held reader was built for. Asked on every poll, which is
        // far more often than anything pastes, so the enumeration is not walked again until it
        // moves.
        DWORD offerSequence{ 0 };
        // The generation this process's own copy left behind. While the clipboard is still at that
        // number, nothing can have taken it.
        DWORD ownedSequence{ 0 };
        // Whether the listener window holds a retry: the next reading is the retry's to make, and
        // a caller in the meantime is answered the last one.
        bool retryPending{ false };
        // Asked of the platform by the first watch, so nothing listens in an application that
        // never asks.
        std::unique_ptr<MessageWindow> listener{};
    private:
        // A CHANGE IS ANNOUNCED ONCE IT HAS BEEN READ. Every listener in the session is told at
        // the same moment and one holder at a time is all the clipboard allows, so the read can
        // find it shut - see whileClipboardShut. A change that could not be read is read again a
        // second later, and the listeners hear of it then; until then the last reading stands.
        void readOrRetry();
    };

    namespace
    {
        // THE CLIPBOARD'S LINE END IS CRLF HERE, and the framework's is LF. The conversion belongs
        // in this layer: a caller made to think about it gets one of the two directions wrong.
        [[nodiscard]] std::wstring toCrLf(const std::wstring_view text)
        {
            std::wstring result{};
            result.reserve(text.size());
            for (const wchar_t letter : text)
            {
                if (letter == L'\n' && (result.empty() || result.back() != L'\r'))
                    result.push_back(L'\r');

                result.push_back(letter);
            }

            return result;
        }

        [[nodiscard]] std::wstring toLf(const std::wstring_view text)
        {
            std::wstring result{};
            result.reserve(text.size());
            for (const wchar_t letter : text)
            {
                if (letter != L'\r')
                    result.push_back(letter);
            }

            return result;
        }

        [[nodiscard]] std::string wideBytes(const std::wstring_view text)
        {
            // TERMINATED, which is what CF_UNICODETEXT is defined as.
            std::string bytes((text.size() + 1) * sizeof(wchar_t), '\0');
            std::memcpy(bytes.data(), text.data(), text.size() * sizeof(wchar_t));
            return bytes;
        }

        [[nodiscard]] std::wstring fromWideBytes(const std::string_view bytes)
        {
            const std::size_t characters = bytes.size() / sizeof(wchar_t);
            std::wstring text(characters, L'\0');
            std::memcpy(text.data(), bytes.data(), characters * sizeof(wchar_t));

            // UP TO THE TERMINATOR. The block can be larger than what was written into it, and
            // the rest of it is whatever was there before.
            const std::size_t end = text.find(L'\0');
            if (end != std::wstring::npos)
                text.resize(end);

            return text;
        }

        [[nodiscard]] bool sameIgnoringCase(const std::wstring_view first,
            const std::wstring_view second)
        {
            if (first.size() != second.size())
                return false;

            for (std::size_t i = 0; i < first.size(); ++i)
            {
                wchar_t left = first[i];
                wchar_t right = second[i];
                if (left >= L'A' && left <= L'Z')
                    left = static_cast<wchar_t>(left - L'A' + L'a');
                if (right >= L'A' && right <= L'Z')
                    right = static_cast<wchar_t>(right - L'A' + L'a');
                if (left != right)
                    return false;
            }

            return true;
        }

        // WHAT THIS LAYER CALLS THE FORMATS THAT HAVE NO NAME OF THEIR OWN. A predefined format
        // answers nothing to GetClipboardFormatName, and a clipboard the user can see must not
        // look emptier than it is, so the names are this platform's to give - as they are for
        // every other standard spelling.
        struct PredefinedFormat
        {
            CLIPFORMAT clipFormat;
            std::wstring_view name;
        };

        constexpr PredefinedFormat k_predefinedFormats[] = {
            { CF_UNICODETEXT, L"CF_UNICODETEXT" },
            { CF_TEXT, L"CF_TEXT" },
            { CF_DIB, L"CF_DIB" },
            { CF_DIBV5, L"CF_DIBV5" },
            { CF_BITMAP, L"CF_BITMAP" },
            { CF_PALETTE, L"CF_PALETTE" },
            { CF_HDROP, L"CF_HDROP" },
            { CF_ENHMETAFILE, L"CF_ENHMETAFILE" },
            { CF_METAFILEPICT, L"CF_METAFILEPICT" },
            { CF_OEMTEXT, L"CF_OEMTEXT" },
            { CF_LOCALE, L"CF_LOCALE" },
            { CF_RIFF, L"CF_RIFF" },
            { CF_WAVE, L"CF_WAVE" },
            { CF_TIFF, L"CF_TIFF" }
        };

        // THE PNG'S NAMES ON THIS CLIPBOARD: the one nearly every source registers, and the MIME
        // name a few register instead.
        constexpr std::wstring_view k_pngName = L"PNG";
        constexpr std::wstring_view k_pngMimeName = L"image/png";

        // WHICH PLATFORM FORMATS SERVE A FRAMEWORK FORMAT, best first. Text is CF_UNICODETEXT
        // alone: Windows synthesises it from CF_TEXT, so it is there whenever text is. A picture
        // is the PNG where the source placed one - lossless, its alpha intact - else the DIB
        // with its alpha mask, else the plain DIB, else the GDI bitmap read as one: OLE lists
        // what the source placed, and a source that placed CF_BITMAP alone lists no DIB.
        constexpr std::wstring_view k_textSpellings[] = { L"CF_UNICODETEXT" };
        constexpr std::wstring_view k_pictureSpellings[] = {
            k_pngName,
            k_pngMimeName,
            L"CF_DIBV5",
            L"CF_DIB",
            L"CF_BITMAP"
        };

        [[nodiscard]] std::span<const std::wstring_view> spellingsOf(const StandardFormat standard)
        {
            switch (standard)
            {
                case StandardFormat::Text:
                    return k_textSpellings;
                case StandardFormat::Picture:
                    return k_pictureSpellings;
            }

            return {};
        }

        [[nodiscard]] bool isPngName(const std::wstring_view name)
        {
            return sameIgnoringCase(name, k_pngName) || sameIgnoringCase(name, k_pngMimeName);
        }

        // A FORMAT WHOSE CLIPBOARD ENTRY IS A HANDLE AND NOT BYTES: a GDI bitmap, a palette, an
        // enhanced metafile. No memory or stream medium carries one and nothing here spells one, so
        // asked for as bytes it answers nothing rather than an error - what it has is a number, and
        // Offer::readHandle is where that is asked for.
        //
        // CF_METAFILEPICT IS NOT ONE OF THESE. Its entry is memory holding a METAFILEPICT, whose
        // last member is the handle, and those bytes are as readable as any other format's - under
        // a medium of its own, which is the whole of what makes it look different.
        [[nodiscard]] bool isHandleFormat(const CLIPFORMAT clipFormat)
        {
            return clipFormat == CF_BITMAP || clipFormat == CF_PALETTE
                || clipFormat == CF_ENHMETAFILE;
        }

        // WHICH MEDIUM CARRIES A HANDLE FORMAT: a metafile comes on its own, a bitmap and a palette
        // are both GDI objects.
        [[nodiscard]] DWORD handleMediumOf(const CLIPFORMAT clipFormat)
        {
            if (clipFormat == CF_ENHMETAFILE)
                return TYMED_ENHMF;

            return TYMED_GDI;
        }

        // The handle a medium of one of those carries, out of the member its own medium names.
        [[nodiscard]] HANDLE handleOf(const STGMEDIUM& medium)
        {
            if (medium.tymed == TYMED_ENHMF)
                return medium.hEnhMetaFile;

            return medium.hBitmap;
        }

        // THE DIB A GDI BITMAP'S PIXELS MAKE: the block CF_DIB carries, an info header and 32-bit
        // rows top down, so the bytes read as the picture and decode as one. A GDI bitmap has no
        // alpha to speak of, and BI_RGB says so.
        //
        // THIS IS A READING OF THE OBJECT AND NOT THE FORMAT'S BYTES. CF_BITMAP carries a handle;
        // these bytes were made here, never travelled, and are only ever handed out as a decoded
        // picture - see ClipboardOffer::read.
        [[nodiscard]] std::string dibOf(const HBITMAP bitmap)
        {
            BITMAP shape{};
            if (!::GetObjectW(bitmap, sizeof(shape), &shape))
                return {};
            if (shape.bmWidth <= 0 || shape.bmHeight <= 0)
                return {};

            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = shape.bmWidth;
            info.bmiHeader.biHeight = -shape.bmHeight;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            const std::size_t rowBytes = static_cast<std::size_t>(shape.bmWidth) * 4;
            const std::size_t pixelBytes = rowBytes * static_cast<std::size_t>(shape.bmHeight);
            info.bmiHeader.biSizeImage = static_cast<DWORD>(pixelBytes);

            std::string bytes(sizeof(BITMAPINFOHEADER) + pixelBytes, '\0');
            const HDC screen = ::GetDC(nullptr);
            if (!screen)
                return {};
            const int rows = ::GetDIBits(screen, bitmap, 0, static_cast<UINT>(shape.bmHeight),
                bytes.data() + sizeof(BITMAPINFOHEADER), &info, DIB_RGB_COLORS);
            ::ReleaseDC(nullptr, screen);
            if (rows != shape.bmHeight)
                return {};

            std::memcpy(bytes.data(), &info.bmiHeader, sizeof(BITMAPINFOHEADER));
            return bytes;
        }

        // A REGISTERED NAME CAN COME BACK IN ANOTHER SPELLING - the OS folds case and answers with
        // whichever spelling reached it first - so a name is matched against what this application
        // declared rather than trusted as it arrives.
        [[nodiscard]] Format declaredOrNamed(std::wstring name)
        {
            for (const Format& known : ByteSpellingTable::instance().knownFormats())
            {
                if (known.kind() != Format::Kind::Custom)
                    continue;
                if (sameIgnoringCase(known.name(), name))
                    return known;
            }

            return Format::custom(std::move(name));
        }

        // EVERY PLATFORM FORMAT UNDER ITS PLATFORM NAME, the synthesised ones included: Windows
        // makes CF_TEXT from CF_UNICODETEXT and CF_DIBV5 and CF_BITMAP from CF_DIB, and each
        // answers different bytes. Never a framework format - see Offer::platformFormat.
        [[nodiscard]] Format formatFor(const CLIPFORMAT clipFormat)
        {
            wchar_t name[256]{};
            const int length = ::GetClipboardFormatNameW(clipFormat, name,
                static_cast<int>(std::size(name)));
            if (length > 0)
                return declaredOrNamed(std::wstring{ name, static_cast<std::size_t>(length) });

            for (const PredefinedFormat& predefined : k_predefinedFormats)
            {
                if (predefined.clipFormat == clipFormat)
                    return Format::custom(std::wstring{ predefined.name });
            }

            return Format::custom(L"CF_" + std::to_wstring(clipFormat));
        }

        [[nodiscard]] CLIPFORMAT clipFormatFor(const Format& format)
        {
            switch (format.kind())
            {
                case Format::Kind::Standard:
                    // CF_UNICODETEXT ALONE. Windows synthesises CF_TEXT and CF_OEMTEXT from it,
                    // and offering those as well would make every format look explicitly placed
                    // to whoever enumerates the clipboard - the order is the only thing that tells
                    // a placed format from a convertible one.
                    if (format.standard() == StandardFormat::Text)
                        return CF_UNICODETEXT;
                    if (format.standard() == StandardFormat::Picture)
                        return CF_DIB;

                    return 0;
                case Format::Kind::Custom:
                    for (const PredefinedFormat& predefined : k_predefinedFormats)
                    {
                        if (sameIgnoringCase(predefined.name, format.name()))
                            return predefined.clipFormat;
                    }

                    return static_cast<CLIPFORMAT>(
                        ::RegisterClipboardFormatW(format.name().c_str()));
                default:
                    return 0;
            }
        }

        constexpr StandardFormat k_standardFormats[] = {
            StandardFormat::Text,
            StandardFormat::Picture
        };

        // THE PACKAGE'S OWN NAME FOR WHAT A PASTE ASKS BY THE PLATFORM'S. A package holds a
        // framework format under the framework's name and this platform places it under its
        // own, so the clip format a framework format is placed as is asked of the package as
        // that format; anything else is asked for by its platform name.
        [[nodiscard]] Format heldFormatFor(const CLIPFORMAT clipFormat)
        {
            for (const StandardFormat standard : k_standardFormats)
            {
                if (clipFormatFor(Format::of(standard)) == clipFormat)
                    return Format::of(standard);
            }

            return formatFor(clipFormat);
        }

        [[nodiscard]] FormatEtcList formatEtcFor(const Source& source)
        {
            FormatEtcList result{};
            for (const Format& format : source.advertisedFormats())
            {
                const CLIPFORMAT clipFormat = clipFormatFor(format);
                if (!clipFormat)
                    continue;

                FORMATETC entry{};
                entry.cfFormat = clipFormat;
                entry.ptd = nullptr;
                entry.dwAspect = DVASPECT_CONTENT;
                entry.lindex = -1;
                entry.tymed = TYMED_HGLOBAL;
                result.push_back(entry);
            }

            return result;
        }

        // ONE HOLDER AT A TIME IS ALL WINDOWS ALLOWS, and every listener in the session is told of
        // a change at once - so the readers that lose the race for it are answered with a
        // failure to read, which is not an empty clipboard; the two are told apart by asking
        // again while whoever holds it finishes. EVERY CALL THAT OPENS THE CLIPBOARD CAN LOSE
        // THAT RACE: taking the data object, enumerating its formats and reading one all open
        // it, the last two through OLE's own object, so all of them go through here.
        //
        // TWO CODES SAY SHUT. A direct open and GetData answer CLIPBRD_E_CANT_OPEN; OLE's object
        // answers a shut clipboard from EnumFormatEtc as E_OUTOFMEMORY, the enumerator it could
        // not fill - every enumeration that failed under a change out of a Chromium application
        // came back so, and every one read on the next try.
        //
        // The wait is on the thread the notification arrived on, and short: a holder that keeps
        // the clipboard past it is waited for off the thread, on the listener window's timer.
        constexpr int k_clipboardReadAttempts = 5;
        constexpr DWORD k_clipboardReadPause = 10;

        [[nodiscard]] bool saysShut(const HRESULT result)
        {
            return result == CLIPBRD_E_CANT_OPEN || result == E_OUTOFMEMORY;
        }

        template<typename Call>
        [[nodiscard]] HRESULT whileClipboardShut(Call&& call)
        {
            HRESULT result = call();
            for (int attempt = 1; attempt < k_clipboardReadAttempts; ++attempt)
            {
                if (!saysShut(result))
                    break;

                ::Sleep(k_clipboardReadPause);
                result = call();
            }

            return result;
        }

        // A read that failed, said into the debugger's output with its code: the code is what
        // tells a clipboard that was shut from a format that was not there.
        void reportFailedRead(const std::wstring_view call, const HRESULT result,
            const std::wstring_view subject = {})
        {
            const std::wstring line = std::format(
                L"ClaFi clipboard: {} failed with {:#010x} {}\n",
                call, static_cast<unsigned long>(result), subject);
            ::OutputDebugStringW(line.c_str());
        }

        // What a report names a format by: its own name, or the standard set's number.
        [[nodiscard]] std::wstring labelOf(const Format& format, const CLIPFORMAT clipFormat)
        {
            if (format.kind() == Format::Kind::Custom)
                return format.name();

            return std::format(L"standard format {}", clipFormat);
        }

        // The whole of a stream, from its start - wherever the source left it standing.
        [[nodiscard]] std::string readStream(IStream& stream)
        {
            const LARGE_INTEGER start{};
            [[maybe_unused]] const HRESULT sought = stream.Seek(start, STREAM_SEEK_SET, nullptr);

            std::string bytes{};
            char buffer[4096]{};
            while (true)
            {
                ULONG count = 0;
                const HRESULT read = stream.Read(buffer, sizeof(buffer), &count);
                if (FAILED(read) || count == 0)
                    break;

                bytes.append(buffer, count);
            }

            return bytes;
        }

        // Whether the clipboard opens for this process now, tried as often as a read is: what
        // every read off the held object needs, since OLE's object opens it again for each.
        [[nodiscard]] bool clipboardOpens(const HWND owner)
        {
            for (int attempt = 0; attempt < k_clipboardReadAttempts; ++attempt)
            {
                if (attempt != 0)
                    ::Sleep(k_clipboardReadPause);

                if (::OpenClipboard(owner))
                {
                    ::CloseClipboard();
                    return true;
                }
            }

            reportFailedRead(L"OpenClipboard", HRESULT_FROM_WIN32(::GetLastError()));
            return false;
        }

        // The formats the object advertises, or nothing while the clipboard is shut against the
        // enumeration - which is not an object advertising no formats.
        [[nodiscard]] std::optional<FormatList> formatsOn(IDataObject& data)
        {
            ComPtr<IEnumFORMATETC> enumerator;
            const HRESULT enumerated = whileClipboardShut([&data, &enumerator]() {
                return data.EnumFormatEtc(DATADIR_GET, enumerator.GetAddressOf());
            });
            if (FAILED(enumerated) || !enumerator)
            {
                reportFailedRead(L"EnumFormatEtc", enumerated);
                return std::nullopt;
            }

            FormatList result{};
            FORMATETC entry{};
            while (enumerator->Next(1, &entry, nullptr) == S_OK)
            {
                // A TARGET DEVICE IS THE CALLER'S TO FREE, and nothing here reads one.
                if (entry.ptd)
                    ::CoTaskMemFree(entry.ptd);

                const Format format = formatFor(entry.cfFormat);
                if (std::ranges::find(result, format) != result.end())
                    continue;

                result.push_back(format);
            }

            return result;
        }

        [[nodiscard]] bool readClipboard(ComPtr<IDataObject>& data)
        {
            const HRESULT taken = whileClipboardShut([&data]() {
                return ::OleGetClipboard(data.GetAddressOf());
            });
            if (FAILED(taken) || !data)
            {
                reportFailedRead(L"OleGetClipboard", taken);
                return false;
            }

            return true;
        }

        // WHAT THE CLIPBOARD HOLDS ON THIS PROCESS'S BEHALF. OLE keeps a reference of its own and
        // asks for a format only when somebody pastes, which is the pull contract the whole
        // subsystem is built on - and the one DoDragDrop takes, so a drag will reuse this.
        //
        // The formats are settled when the object is made. It lives exactly as long as the package
        // it was made for: a later copy replaces both together.
        //
        // THE CLIPBOARD IT SERVES OUTLIVES EVERY CALL INTO IT. OLE calls in on the thread that
        // owns the apartment, from its message loop or from the flush the clipboard's own
        // destructor makes, and the apartment is closed after that destructor has run.
        class DataObject : public IDataObject
        {
        public:
            DataObject(NativeClipboard& native, FormatEtcList formats);
            virtual ~DataObject() = default;

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** result) override;
            ULONG STDMETHODCALLTYPE AddRef() override;
            ULONG STDMETHODCALLTYPE Release() override;

            HRESULT STDMETHODCALLTYPE GetData(FORMATETC* requested, STGMEDIUM* medium) override;
            HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC* requested, STGMEDIUM* medium) override;
            HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* requested) override;
            HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC* requested,
                FORMATETC* canonical) override;
            HRESULT STDMETHODCALLTYPE SetData(FORMATETC* requested, STGMEDIUM* medium,
                BOOL release) override;
            HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD direction,
                IEnumFORMATETC** result) override;
            HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC* requested, DWORD flags, IAdviseSink*,
                DWORD* connection) override;
            HRESULT STDMETHODCALLTYPE DUnadvise(DWORD connection) override;
            HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA** result) override;
        private:
            NativeClipboard& m_native;
            FormatEtcList m_formats;
            LONG m_references{ 1 };
        };

        // READS THE CLIPBOARD THROUGH OLE. Built for one data object and replaced when the
        // clipboard generation moves.
        class ClipboardOffer : public Offer
        {
        public:
            ClipboardOffer(ComPtr<IDataObject> data, FormatList formats);

            [[nodiscard]] FormatList advertisedFormats() const override;
            [[nodiscard]] const Format* platformFormat(StandardFormat) const override;
            [[nodiscard]] Payload read(const Format&) override;
            [[nodiscard]] std::string readBytes(const Format&) override;
            [[nodiscard]] std::uint64_t readHandle(const Format&) override;
        private:
            // One native format's bytes off the object, as they arrive. Empty where it answers
            // nothing, and empty for a handle format, which has none.
            [[nodiscard]] std::string readNative(CLIPFORMAT, const Format&);
            // THE PICTURE A CF_BITMAP OFFERS, read off the object its handle names. A decode rather
            // than a read: the DIB it goes through is made here - see dibOf - so it is served as a
            // Payload and never as the format's bytes.
            [[nodiscard]] Payload readBitmapPicture(const Format&);
        private:
            ComPtr<IDataObject> m_data;
            FormatList m_formats;
        };

        DataObject::DataObject(NativeClipboard& native, FormatEtcList formats)
            :
            m_native{ native },
            m_formats{ std::move(formats) }
        {
        }

        HRESULT DataObject::QueryInterface(REFIID id, void** result)
        {
            if (!result)
                return E_INVALIDARG;

            *result = nullptr;
            if (id == __uuidof(IUnknown) || id == __uuidof(IDataObject))
            {
                *result = static_cast<IDataObject*>(this);
                AddRef();
                return S_OK;
            }

            return E_NOINTERFACE;
        }

        ULONG DataObject::AddRef()
        {
            return static_cast<ULONG>(::InterlockedIncrement(&m_references));
        }

        ULONG DataObject::Release()
        {
            const LONG remaining = ::InterlockedDecrement(&m_references);
            if (remaining == 0)
                delete this;

            return static_cast<ULONG>(remaining);
        }

        HRESULT DataObject::GetData(FORMATETC* requested, STGMEDIUM* medium)
        {
            if (!requested || !medium)
                return E_INVALIDARG;

            *medium = {};
            if (!(requested->tymed & TYMED_HGLOBAL))
                return DV_E_TYMED;
            if (requested->dwAspect != DVASPECT_CONTENT)
                return DV_E_DVASPECT;

            const Format format = heldFormatFor(requested->cfFormat);
            const std::string bytes = m_native.bytesFor(format);
            if (bytes.empty())
                return DV_E_FORMATETC;

            HGLOBAL block = ::GlobalAlloc(GMEM_MOVEABLE, bytes.size());
            if (!block)
                return E_OUTOFMEMORY;

            void* target = ::GlobalLock(block);
            if (!target)
            {
                ::GlobalFree(block);
                return E_OUTOFMEMORY;
            }

            std::memcpy(target, bytes.data(), bytes.size());
            ::GlobalUnlock(block);

            medium->tymed = TYMED_HGLOBAL;
            medium->hGlobal = block;
            medium->pUnkForRelease = nullptr;
            return S_OK;
        }

        // For a caller that has allocated the medium itself, which only the stream and storage
        // media need. Everything here travels as an HGLOBAL this object allocates.
        HRESULT DataObject::GetDataHere(FORMATETC*, STGMEDIUM*)
        {
            return E_NOTIMPL;
        }

        HRESULT DataObject::QueryGetData(FORMATETC* requested)
        {
            if (!requested)
                return E_INVALIDARG;
            if (!(requested->tymed & TYMED_HGLOBAL))
                return DV_E_TYMED;
            if (requested->dwAspect != DVASPECT_CONTENT)
                return DV_E_DVASPECT;

            if (!m_native.provides(heldFormatFor(requested->cfFormat)))
                return DV_E_FORMATETC;

            return S_OK;
        }

        HRESULT DataObject::GetCanonicalFormatEtc(FORMATETC*, FORMATETC* canonical)
        {
            if (!canonical)
                return E_INVALIDARG;

            *canonical = {};
            return E_NOTIMPL;
        }

        // The clipboard is written by putting a package on it, not by a caller reaching into the
        // object that is already there.
        HRESULT DataObject::SetData(FORMATETC*, STGMEDIUM*, BOOL)
        {
            return E_NOTIMPL;
        }

        HRESULT DataObject::EnumFormatEtc(const DWORD direction, IEnumFORMATETC** result)
        {
            if (!result)
                return E_INVALIDARG;

            *result = nullptr;
            if (direction != DATADIR_GET)
                return E_NOTIMPL;
            if (m_formats.empty())
                return E_FAIL;

            return ::SHCreateStdEnumFmtEtc(static_cast<UINT>(m_formats.size()), m_formats.data(),
                result);
        }

        // A package does not change while it is the clipboard's, so there is nothing to notify.
        HRESULT DataObject::DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*)
        {
            return OLE_E_ADVISENOTSUPPORTED;
        }

        HRESULT DataObject::DUnadvise(DWORD)
        {
            return OLE_E_ADVISENOTSUPPORTED;
        }

        HRESULT DataObject::EnumDAdvise(IEnumSTATDATA**)
        {
            return OLE_E_ADVISENOTSUPPORTED;
        }

        ClipboardOffer::ClipboardOffer(ComPtr<IDataObject> data, FormatList formats)
            :
            m_data{ std::move(data) },
            m_formats{ std::move(formats) }
        {
        }

        FormatList ClipboardOffer::advertisedFormats() const
        {
            return m_formats;
        }

        const Format* ClipboardOffer::platformFormat(const StandardFormat standard) const
        {
            for (const std::wstring_view spelling : spellingsOf(standard))
            {
                for (const Format& listed : m_formats)
                {
                    if (listed.kind() == Format::Kind::Custom
                        && sameIgnoringCase(listed.name(), spelling))
                    {
                        return &listed;
                    }
                }
            }

            return nullptr;
        }

        Payload ClipboardOffer::read(const Format& format)
        {
            if (format.kind() != Format::Kind::Standard)
            {
                // THE ONE PLATFORM FORMAT THIS LAYER DECODES FOR ITSELF. CF_BITMAP carries a
                // handle, so there are no bytes to spell and readBytes answers none; its picture
                // is read off the object, and a decode is what this call is for.
                if (clipFormatFor(format) == CF_BITMAP)
                    return readBitmapPicture(format);

                const std::string bytes = readBytes(format);
                return bytes.empty() ? Payload{} : fromBytes(format, bytes);
            }

            const Format* served = platformFormat(format.standard());
            if (!served)
                return {};

            // A PICTURE SERVED FROM CF_BITMAP TAKES THE SAME ROUTE, which is the last spelling in
            // the picture list and the only one a source may place alone.
            if (clipFormatFor(*served) == CF_BITMAP)
                return readBitmapPicture(*served);

            const std::string bytes = readNative(clipFormatFor(*served), *served);
            if (bytes.empty())
                return {};

            switch (format.standard())
            {
                case StandardFormat::Text:
                    return Payload{ toLf(fromWideBytes(bytes)) };
                case StandardFormat::Picture:
                {
                    std::optional<Graphics::Bitmap> picture = isPngName(served->name())
                        ? Graphics::decodePng(bytes)
                        : Graphics::decodeDib(bytes);
                    if (!picture)
                        return {};

                    return Payload{ std::move(*picture) };
                }
            }

            return {};
        }

        std::string ClipboardOffer::readBytes(const Format& format)
        {
            if (format.kind() == Format::Kind::Standard)
            {
                const Format* served = platformFormat(format.standard());
                return served ? readNative(clipFormatFor(*served), *served) : std::string{};
            }

            return readNative(clipFormatFor(format), format);
        }

        std::uint64_t ClipboardOffer::readHandle(const Format& format)
        {
            const CLIPFORMAT clipFormat = clipFormatFor(format);
            if (!clipFormat || !m_data || !isHandleFormat(clipFormat))
                return 0;

            FORMATETC requested{};
            requested.cfFormat = clipFormat;
            requested.ptd = nullptr;
            requested.dwAspect = DVASPECT_CONTENT;
            requested.lindex = -1;
            requested.tymed = handleMediumOf(clipFormat);

            STGMEDIUM medium{};
            const HRESULT read = whileClipboardShut([this, &requested, &medium]() {
                return m_data->GetData(&requested, &medium);
            });
            if (FAILED(read))
            {
                reportFailedRead(L"GetData", read, labelOf(format, clipFormat));
                return 0;
            }

            // READ BEFORE THE MEDIUM GOES BACK, which is where OLE takes away the handle it made
            // for this reader. What is answered is therefore a value that was live and is not any
            // more, which is the whole of what a handle can honestly say from here.
            const std::uint64_t value{ reinterpret_cast<std::uintptr_t>(handleOf(medium)) };
            ::ReleaseStgMedium(&medium);
            return value;
        }

        std::string ClipboardOffer::readNative(const CLIPFORMAT clipFormat, const Format& format)
        {
            if (!clipFormat || !m_data || isHandleFormat(clipFormat))
                return {};

            // EITHER MEDIUM, since the source chooses: memory is what the clipboard itself hands
            // over, and a stream is what some sources serve a format as, HTML among them. A
            // metafile picture is memory as well, under a medium of its own, and its bytes are the
            // METAFILEPICT it carries.
            FORMATETC requested{};
            requested.cfFormat = clipFormat;
            requested.ptd = nullptr;
            requested.dwAspect = DVASPECT_CONTENT;
            requested.lindex = -1;
            requested.tymed = TYMED_HGLOBAL | TYMED_ISTREAM;
            if (clipFormat == CF_METAFILEPICT)
                requested.tymed |= TYMED_MFPICT;

            STGMEDIUM medium{};
            const HRESULT read = whileClipboardShut([this, &requested, &medium]() {
                return m_data->GetData(&requested, &medium);
            });
            if (FAILED(read))
            {
                reportFailedRead(L"GetData", read, labelOf(format, clipFormat));
                return {};
            }

            std::string bytes{};
            // A METAFILEPICT ARRIVES ON ITS OWN MEDIUM AND IS STILL AN HGLOBAL, which is why one
            // branch reads both: the union's members alias, and GlobalSize answers for either.
            if ((medium.tymed == TYMED_HGLOBAL || medium.tymed == TYMED_MFPICT) && medium.hGlobal)
            {
                const SIZE_T size = ::GlobalSize(medium.hGlobal);
                const void* source = ::GlobalLock(medium.hGlobal);
                if (source)
                {
                    bytes.assign(static_cast<const char*>(source), size);
                    ::GlobalUnlock(medium.hGlobal);
                }
            }
            else if (medium.tymed == TYMED_ISTREAM && medium.pstm)
            {
                bytes = readStream(*medium.pstm);
            }

            ::ReleaseStgMedium(&medium);

            // THE LENGTH IN FRONT IS THIS APPLICATION'S ENVELOPE, not the content. A format with
            // a byte spelling travels with its length written ahead of it - see the writer, which
            // puts it there - because an HGLOBAL is rounded up and the reader cannot otherwise
            // tell the payload from the padding. The other platform sends the payload alone, and
            // the two have to answer the same bytes.
            if (!ByteSpellingTable::instance().find(format))
                return bytes;

            if (bytes.size() < sizeof(PayloadLength))
                return {};

            PayloadLength length = 0;
            std::memcpy(&length, bytes.data(), sizeof(length));
            if (length > bytes.size() - sizeof(length))
                return {};

            return bytes.substr(sizeof(PayloadLength), length);
        }

        Payload ClipboardOffer::readBitmapPicture(const Format& format)
        {
            FORMATETC requested{};
            requested.cfFormat = CF_BITMAP;
            requested.ptd = nullptr;
            requested.dwAspect = DVASPECT_CONTENT;
            requested.lindex = -1;
            requested.tymed = TYMED_GDI;

            STGMEDIUM medium{};
            const HRESULT read = whileClipboardShut([this, &requested, &medium]() {
                return m_data->GetData(&requested, &medium);
            });
            if (FAILED(read))
            {
                reportFailedRead(L"GetData", read, labelOf(format, CF_BITMAP));
                return {};
            }

            std::string dib{};
            if (medium.tymed == TYMED_GDI && medium.hBitmap)
                dib = dibOf(medium.hBitmap);

            ::ReleaseStgMedium(&medium);
            if (dib.empty())
                return {};

            std::optional<Graphics::Bitmap> picture = Graphics::decodeDib(dib);
            if (!picture)
                return {};

            return Payload{ std::move(*picture) };
        }
    }

    // UTF-16, CRLF AND THE TERMINATOR, which is what CF_UNICODETEXT is defined as and what a
    // reader on the other side of this clipboard is handed.
    std::string nativeBytes(const Format& format, const Payload& content)
    {
        if (format.kind() == Format::Kind::Standard)
        {
            switch (format.standard())
            {
                case StandardFormat::Text:
                {
                    const std::wstring* text = std::any_cast<std::wstring>(&content);
                    return text ? wideBytes(toCrLf(*text)) : std::string{};
                }
                case StandardFormat::Picture:
                    // TODO: a picture held in a package is not spelled yet. Which header does a
                    // copy out carry, is the alpha written as a V5 mask, and does a PNG go
                    // beside it - the encoder is WIC's, as the icon writer's is?
                    return {};
            }
        }

        return toBytes(format, content);
    }

    Format platformFormatOf(const StandardFormat standard)
    {
        return formatFor(clipFormatFor(Format::of(standard)));
    }

    // Clipboard

    Clipboard::Clipboard(IPlatformServices& platform)
        :
        m_platform{ platform },
        m_native{ std::make_unique<NativeClipboard>(*this) }
    {
    }

    Clipboard::~Clipboard() = default;

    // THE STAMP IS NOT READ HERE. OLE authorises nothing against an input event, so a copy this
    // application made for itself takes the clipboard exactly as a user's does. The other platform
    // refuses that request, and the difference belongs to the platforms rather than to this
    // framework.
    void Clipboard::set(Source&& source, InputStamp)
    {
        holdSource(std::move(source));

        const Source* held = heldSource();
        if (!held)
            return;

        // Attached rather than assigned: the object is born with the one reference this holds, and
        // OleSetClipboard takes another of its own.
        m_native->dataObject.Attach(new DataObject{ *m_native, formatEtcFor(*held) });
        ::OleSetClipboard(m_native->dataObject.Get());
        // READ AFTER THE CALL, which moves it.
        m_native->ownedSequence = ::GetClipboardSequenceNumber();

        // The cached reader is over whatever was on the clipboard before this.
        m_native->offerSequence = 0;
        dropOffer();
    }

    Offer* Clipboard::offer()
    {
        // While a retry is armed the next reading is the listener window's to make, and a caller
        // in the meantime is answered the last one.
        if (!m_native->retryPending)
            m_native->refreshOffer();

        if (Offer* own = ownSelectionOffer())
            return own;

        return heldOffer();
    }

    // The clipboard itself is left as it is. What this drops is the package this process was
    // standing behind, which is a different thing from emptying what the user copied.
    void Clipboard::release()
    {
        dropSource();
        m_native->dataObject.Reset();
    }

    void Clipboard::startWatching()
    {
        m_native->startListening();
    }

    // NativeClipboard

    NativeClipboard::NativeClipboard(Clipboard& clipboard)
        :
        owner{ clipboard }
    {
    }

    NativeClipboard::~NativeClipboard()
    {
        if (dataObject && ::OleIsCurrentClipboard(dataObject.Get()) == S_OK)
            ::OleFlushClipboard();

        if (listener)
            ::RemoveClipboardFormatListener(listener->handle());
    }

    std::string NativeClipboard::bytesFor(const Format& format)
    {
        Source* held = owner.heldSource();
        if (!held)
            return {};

        const Payload* content = held->content(format);
        if (!content)
            return {};

        // FRAMED ONLY WHERE THIS APPLICATION SPELLS THE FORMAT ITSELF. A format nothing is
        // registered for is bytes that arrived from elsewhere, and they go back out as they
        // came.
        const bool framed = ByteSpellingTable::instance().find(format) != nullptr;
        const std::string payload = nativeBytes(format, *content);
        if (!framed)
            return payload;

        const PayloadLength length = static_cast<PayloadLength>(payload.size());
        std::string result(sizeof(length), '\0');
        std::memcpy(result.data(), &length, sizeof(length));
        result.append(payload);
        return result;
    }

    bool NativeClipboard::provides(const Format& format)
    {
        const Source* held = owner.heldSource();
        return held && held->provides(format);
    }

    bool NativeClipboard::refreshOffer()
    {
        const DWORD sequence = ::GetClipboardSequenceNumber();

        // WHETHER THIS PROCESS STILL OWNS THE CLIPBOARD. Nothing arrives to say it changed
        // hands, and a package held here would shadow a copy made in another application. OLE
        // is asked only once the generation has moved: until then nothing can have taken it,
        // and this is read on every poll of the paste action.
        if (owner.heldSource() && sequence != ownedSequence
            && ::OleIsCurrentClipboard(dataObject.Get()) != S_OK)
        {
            owner.release();
        }

        if (owner.ownSelectionOffer())
            return true;

        if (sequence == offerSequence && owner.heldOffer())
            return true;

        ComPtr<IDataObject> data;
        if (!readClipboard(data))
            return false;

        std::optional<FormatList> formats = formatsOn(*data.Get());
        if (!formats.has_value())
            return false;

        offerSequence = sequence;
        owner.holdOffer(std::make_unique<ClipboardOffer>(data, std::move(formats.value())));
        return true;
    }

    // THE PLATFORM BEHIND THE NAME IS THE WINDOWS ONE, the only platform of this build, and it
    // derives from the factory - see Win32Platform.
    void NativeClipboard::startListening()
    {
        IMessageWindowFactory& platform = static_cast<IMessageWindowFactory&>(owner.m_platform);
        listener = platform.createMessageWindow();
        listener->setSink(this);
        ::AddClipboardFormatListener(listener->handle());
    }

    void NativeClipboard::wndProc(WinApiMsg& msg)
    {
        if (msg.msg == WM_CLIPBOARDUPDATE)
        {
            readOrRetry();
            msg.handled = true;
        }
        else if (msg.msg == WM_TIMER && msg.wParam == k_retryTimer)
        {
            readOrRetry();
            msg.handled = true;
        }
    }

    void NativeClipboard::readOrRetry()
    {
        // A retry still armed is this reading's now, whether the timer or a second change
        // brought it here: one announcement per reading.
        const HWND handle = listener->handle();
        ::KillTimer(handle, k_retryTimer);
        retryPending = false;

        // Read, and then make sure the listeners' own reads can follow at once: they read
        // through OLE's object, which opens the clipboard again for every format they ask.
        if (refreshOffer() && clipboardOpens(handle))
        {
            owner.clipboardChanged();
            return;
        }

        retryPending = true;
        ::SetTimer(handle, k_retryTimer, k_retryDelay, nullptr);
    }
}
