module;
#include "Wayland.Headers.h"
// FOR THE MACROS, which no module can export: SIGPIPE, SIG_IGN and SIG_ERR from the first,
// F_SETFD and FD_CLOEXEC from the second.
#include <csignal>
#include <fcntl.h>

// AN IMPLEMENTATION UNIT OF THE MODULE THAT DECLARED Clipboard, not of the Wayland layer. A class
// member belongs to the module its class was declared in and must be defined there, so the bodies
// of Clipboard:: live here whatever platform supplies them.
module ClaFi.Core.Transfer.Clipboard;

import ClaFi.Platform.Wayland.Display;

import ClaFi.Core.Transfer.Bytes;
import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.Graphics.Dib;
import ClaFi.Core.Graphics.Png;
import ClaFi.Core.Graphics.Types;
import ClaFi.Core.System.UiTypes;
import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    namespace
    {
        using PlatformImplementation::Wayland::DisplayManager;
        using PlatformImplementation::Wayland::IDisplayAccess;
        using PlatformImplementation::Wayland::SelectionOffer;

        constexpr std::string_view k_textMime = "text/plain;charset=utf-8";
        // The short spelling, offered beside the qualified one because clients ask for both. A
        // framework text holds LF line ends already, so neither needs the bytes rewritten.
        constexpr std::string_view k_shortTextMime = "text/plain";
        // What a client coming through XWayland offers for the same thing.
        constexpr std::string_view k_legacyTextMime = "UTF8_STRING";
        // A PICTURE'S TWO SPELLINGS: a BMP file, a DIB behind a file header, which is what this
        // application places, and a PNG, which is what most sources place.
        constexpr std::string_view k_bmpMime = "image/bmp";
        constexpr std::string_view k_pngMime = "image/png";

        // WHICH PLATFORM FORMATS SERVE A FRAMEWORK FORMAT, best first. The three text names carry
        // the same UTF-8; a picture is the PNG where the source placed one - lossless, its alpha
        // intact - else the BMP.
        constexpr std::string_view k_textSpellings[] = {
            k_textMime,
            k_shortTextMime,
            k_legacyTextMime
        };
        constexpr std::string_view k_pictureSpellings[] = {
            k_pngMime,
            k_bmpMime
        };
        constexpr std::string_view k_namedMimePrefix = "application/x-";

        // How long a pipe may stall before the transfer is abandoned. A paste is served on the
        // event loop, so this is the longest the application can be unresponsive for one.
        constexpr int k_pipeTimeoutMs = 1000;
    }

    // THE CLIPBOARD AS THE COMPOSITOR HAS IT: the source standing behind the selection this
    // process took, and the offer object the held reader was built for. See Transfer
    struct NativeClipboard
    {
        explicit NativeClipboard(Clipboard&);
        // The selection this process holds goes with it, and the display is told there is nobody
        // left to report a change to.
        ~NativeClipboard();
        NativeClipboard(const NativeClipboard&) = delete;
        NativeClipboard& operator=(const NativeClipboard&) = delete;

        // The held package's content for a format, as the bytes that format travels as.
        [[nodiscard]] std::string bytesFor(const Format&);
        // SOMEBODY IS PASTING. The fd is this process's to write and to close, and the content
        // is produced now rather than at the copy - which is the whole shape of a selection here.
        void send(const char* mimeName, std::int32_t fd);
        // ANOTHER CLIENT TOOK THE SELECTION, and the package this process was standing behind
        // goes with it.
        //
        // TESTED AGAINST THE STANDING SOURCE rather than assumed to be it. A cancel for a source
        // a later copy already replaced would otherwise drop the package behind the current one,
        // and what that looks like is a copy that works and then stops a moment later with
        // nothing to see. A source cancelled is a source finished, so one that is not the
        // standing one is destroyed here and nowhere else.
        void cancelled(wl_data_source*);
        void destroyDataSource();
        // THE DISPLAY BEHIND THE NAME the clipboard holds the platform by - the Wayland one, the
        // only platform of this build, which derives from IDisplayAccess. See Transfer
        [[nodiscard]] DisplayManager& display() const;

        Clipboard& owner;
        wl_data_source* dataSource{ nullptr };
        SelectionOffer offerHandle{};
    };

    namespace
    {
        [[nodiscard]] bool sameIgnoringCase(const std::string_view first,
            const std::string_view second)
        {
            if (first.size() != second.size())
                return false;

            for (std::size_t i = 0; i < first.size(); ++i)
            {
                char left = first[i];
                char right = second[i];
                if (left >= 'A' && left <= 'Z')
                    left = static_cast<char>(left - 'A' + 'a');
                if (right >= 'A' && right <= 'Z')
                    right = static_cast<char>(right - 'A' + 'a');
                if (left != right)
                    return false;
            }

            return true;
        }

        [[nodiscard]] std::string loweredUtf8(const std::wstring_view name)
        {
            std::wstring folded{ name };
            for (wchar_t& letter : folded)
            {
                if (letter >= L'A' && letter <= L'Z')
                    letter = static_cast<wchar_t>(letter - L'A' + L'a');
            }

            return toUtf8(folded);
        }

        // Every MIME name one format is offered under, best first.
        [[nodiscard]] std::vector<std::string> mimeNamesFor(const Format& format)
        {
            switch (format.kind())
            {
                case Format::Kind::Standard:
                    if (format.standard() == StandardFormat::Text)
                        return { std::string{ k_textMime }, std::string{ k_shortTextMime } };
                    if (format.standard() == StandardFormat::Picture)
                        return { std::string{ k_bmpMime } };

                    return {};
                case Format::Kind::Custom:
                    // A DECLARED FORMAT IS SPELLED FROM ITS NAME. Anything else already carries a
                    // spelling, that being the only name it has.
                    if (ByteSpellingTable::instance().find(format))
                        return { std::string{ k_namedMimePrefix } + loweredUtf8(format.name()) };

                    return { toUtf8(format.name()) };
                default:
                    return {};
            }
        }

        // The format a MIME name names: every platform format under its platform name, never a
        // framework format - see Offer::platformFormat. ASKED OF WHAT THIS APPLICATION DECLARED
        // rather than guessed from the name: the prefix a declared format is spelled with is not
        // this framework's alone, so a name that merely looks like one of ours is not one.
        [[nodiscard]] Format formatForMimeName(const std::string& mimeName)
        {
            for (const Format& known : ByteSpellingTable::instance().knownFormats())
            {
                for (const std::string& candidate : mimeNamesFor(known))
                {
                    if (sameIgnoringCase(candidate, mimeName))
                        return known;
                }
            }

            // Nothing here knows it, so its own spelling is its name. That is what lets a viewer
            // list it and read the bytes it arrived as.
            return Format::custom(fromUtf8(mimeName));
        }

        // A READER THAT CLOSES EARLY MUST NOT KILL THE PROCESS. Writing into a pipe nobody is
        // reading raises SIGPIPE, whose default action is termination, and a paste the user
        // abandoned does exactly that. The disposition belongs to the process, so it is set once.
        void ignoreBrokenPipe()
        {
            [[maybe_unused]] static const bool s_ignored
                = std::signal(SIGPIPE, SIG_IGN) != SIG_ERR;
        }

        [[nodiscard]] bool makePipe(int& readEnd, int& writeEnd)
        {
            int ends[2] = { -1, -1 };
            if (::pipe(ends) != 0)
                return false;

            // NOT INHERITED BY A CHILD PROCESS. A process started while a transfer is open would
            // hold the write end, and the reader would wait for an end of data that never comes.
            ::fcntl(ends[0], F_SETFD, FD_CLOEXEC);
            ::fcntl(ends[1], F_SETFD, FD_CLOEXEC);
            readEnd = ends[0];
            writeEnd = ends[1];
            return true;
        }

        [[nodiscard]] std::string readAll(const int fd)
        {
            std::string bytes{};
            char buffer[4096]{};
            while (true)
            {
                pollfd waiting{};
                waiting.fd = fd;
                waiting.events = POLLIN;
                if (::poll(&waiting, 1, k_pipeTimeoutMs) <= 0)
                    break;

                const ssize_t count = ::read(fd, buffer, sizeof(buffer));
                if (count <= 0)
                    break;

                bytes.append(buffer, static_cast<std::size_t>(count));
            }

            return bytes;
        }

        void writeAll(const int fd, const std::string_view bytes)
        {
            std::size_t written = 0;
            while (written < bytes.size())
            {
                pollfd waiting{};
                waiting.fd = fd;
                waiting.events = POLLOUT;
                if (::poll(&waiting, 1, k_pipeTimeoutMs) <= 0)
                    return;

                const ssize_t count = ::write(fd, bytes.data() + written, bytes.size() - written);
                if (count <= 0)
                    return;

                written += static_cast<std::size_t>(count);
            }
        }

        constexpr StandardFormat k_standardFormats[] = {
            StandardFormat::Text,
            StandardFormat::Picture
        };

        // THE PACKAGE'S OWN NAME FOR WHAT A PASTE ASKS BY THE PLATFORM'S. A package holds a
        // framework format under the framework's name and this platform offers it under its
        // own MIME names, so a name a framework format is offered as is asked of the package as
        // that format; anything else is asked for by its platform name.
        [[nodiscard]] Format heldFormatForMimeName(const std::string& mimeName)
        {
            for (const StandardFormat standard : k_standardFormats)
            {
                for (const std::string& candidate : mimeNamesFor(Format::of(standard)))
                {
                    if (sameIgnoringCase(candidate, mimeName))
                        return Format::of(standard);
                }
            }

            return formatForMimeName(mimeName);
        }

        void onSourceTarget(void*, wl_data_source*, const char*)
        {
        }

        // The listener's data is the clipboard's native half, given when the source was made.
        void onSourceSend(void* data, wl_data_source*, const char* mimeName, const std::int32_t fd)
        {
            static_cast<NativeClipboard*>(data)->send(mimeName, fd);
        }

        void onSourceCancelled(void* data, wl_data_source* source)
        {
            static_cast<NativeClipboard*>(data)->cancelled(source);
        }

        // READS THE CLIPBOARD THROUGH THE COMPOSITOR. Built for one wl_data_offer and replaced
        // when the selection changes.
        class CompositorOffer : public Offer
        {
        public:
            CompositorOffer(DisplayManager&, SelectionOffer, std::vector<std::string> mimeNames);

            [[nodiscard]] FormatList advertisedFormats() const override;
            [[nodiscard]] const Format* platformFormat(StandardFormat) const override;
            [[nodiscard]] Payload read(const Format&) override;
            [[nodiscard]] std::string readBytes(const Format&) override;
        private:
            // The MIME name a listed format is asked for under, or null for one not listed.
            [[nodiscard]] const std::string* mimeNameOf(const Format&) const;
        private:
            // Owned by the display manager, which destroys it when the selection changes. Tested
            // against the current selection before every read, so a handle left behind by a
            // change this object did not see is never touched. WHICH CHANNEL IT CAME ON travels
            // with it, the two spelling the read request differently.
            DisplayManager& m_display;
            SelectionOffer m_offer{};
            std::vector<std::string> m_mimeNames;
            FormatList m_formats;
        };

        CompositorOffer::CompositorOffer(DisplayManager& display, const SelectionOffer offer,
            std::vector<std::string> mimeNames)
            :
            m_display{ display },
            m_offer{ offer },
            m_mimeNames{ std::move(mimeNames) }
        {
            for (const std::string& mimeName : m_mimeNames)
            {
                const Format format = formatForMimeName(mimeName);
                if (std::ranges::find(m_formats, format) == m_formats.end())
                    m_formats.push_back(format);
            }
        }

        FormatList CompositorOffer::advertisedFormats() const
        {
            return m_formats;
        }

        const Format* CompositorOffer::platformFormat(const StandardFormat standard) const
        {
            std::span<const std::string_view> spellings{};
            switch (standard)
            {
                case StandardFormat::Text:
                    spellings = k_textSpellings;
                    break;
                case StandardFormat::Picture:
                    spellings = k_pictureSpellings;
                    break;
            }

            for (const std::string_view spelling : spellings)
            {
                for (const Format& listed : m_formats)
                {
                    if (listed.kind() == Format::Kind::Custom
                        && sameIgnoringCase(toUtf8(listed.name()), spelling))
                    {
                        return &listed;
                    }
                }
            }

            return nullptr;
        }

        Payload CompositorOffer::read(const Format& format)
        {
            if (format.kind() != Format::Kind::Standard)
            {
                const std::string bytes = readBytes(format);
                return bytes.empty() ? Payload{} : fromBytes(format, bytes);
            }

            const Format* served = platformFormat(format.standard());
            if (!served)
                return {};

            const std::string bytes = readBytes(*served);
            if (bytes.empty())
                return {};

            switch (format.standard())
            {
                case StandardFormat::Text:
                    return Payload{ fromUtf8(bytes) };
                case StandardFormat::Picture:
                {
                    const bool png = sameIgnoringCase(toUtf8(served->name()), k_pngMime);
                    std::optional<Graphics::Bitmap> picture = png
                        ? Graphics::decodePng(bytes)
                        : Graphics::decodeBmpFile(bytes);
                    if (!picture)
                        return {};

                    return Payload{ std::move(*picture) };
                }
            }

            return {};
        }

        std::string CompositorOffer::readBytes(const Format& format)
        {
            DisplayManager& display = m_display;
            if (m_offer.empty() || display.selectionOffer() != m_offer)
                return {};

            if (format.kind() == Format::Kind::Standard)
            {
                const Format* served = platformFormat(format.standard());
                return served ? readBytes(*served) : std::string{};
            }

            const std::string* mimeName = mimeNameOf(format);
            if (!mimeName)
                return {};

            int readEnd = -1;
            int writeEnd = -1;
            if (!makePipe(readEnd, writeEnd))
                return {};

            display.receiveSelection(m_offer, mimeName->c_str(), writeEnd);
            // THE REQUEST REACHES THE COMPOSITOR BEFORE THE READ. Requests are buffered
            // client-side, and a read waiting on a pipe whose other end was never asked for waits
            // out the timeout and comes back empty.
            ::wl_display_flush(display.display());
            ::close(writeEnd);

            const std::string bytes = readAll(readEnd);
            ::close(readEnd);

            // NO ENVELOPE. A compositor hands over a stream that ends when the writer closes it,
            // so the payload's length is the read's own answer and nothing is written in front of
            // it - see the Windows twin, which cannot do that.
            return bytes;
        }

        const std::string* CompositorOffer::mimeNameOf(const Format& format) const
        {
            for (const std::string& candidate : m_mimeNames)
            {
                if (formatForMimeName(candidate) == format)
                    return &candidate;
            }

            return nullptr;
        }
    }

    // UTF-8 AND LF, which is what text/plain;charset=utf-8 carries. Nothing is written around it:
    // a compositor's stream ends where the payload does.
    std::string nativeBytes(const Format& format, const Payload& content)
    {
        if (format.kind() == Format::Kind::Standard)
        {
            switch (format.standard())
            {
                case StandardFormat::Text:
                {
                    const std::wstring* text = std::any_cast<std::wstring>(&content);
                    return text ? toUtf8(*text) : std::string{};
                }
                case StandardFormat::Picture:
                    // TODO: a picture held in a package is not spelled yet. Is a BMP file what a
                    // copy out carries, and does it need image/png beside it to be taken - libpng
                    // writes as well as it reads?
                    return {};
            }
        }

        return toBytes(format, content);
    }

    Format platformFormatOf(const StandardFormat standard)
    {
        return formatForMimeName(mimeNamesFor(Format::of(standard)).front());
    }

    // Clipboard

    Clipboard::Clipboard(IPlatformServices& platform)
        :
        m_platform{ platform },
        m_native{ std::make_unique<NativeClipboard>(*this) }
    {
    }

    Clipboard::~Clipboard() = default;

    void Clipboard::set(Source&& source, const InputStamp stamp)
    {
        DisplayManager& display = m_native->display();
        wl_data_device* device = display.dataDevice();

        // NOTHING IS HELD THAT CANNOT BE OFFERED. A compositor refuses a selection request naming
        // no input event, and a package kept anyway would answer pastes inside this process with
        // content the rest of the desktop cannot see.
        if (!device || stamp.empty())
        {
            release();
            return;
        }

        holdSource(std::move(source));
        const Source* held = heldSource();

        static const wl_data_source_listener k_sourceListener = {
            .target = &onSourceTarget,
            .send = &onSourceSend,
            .cancelled = &onSourceCancelled
        };
        wl_data_source* previous = m_native->dataSource;
        m_native->dataSource
            = ::wl_data_device_manager_create_data_source(display.dataDeviceManager());
        ::wl_data_source_add_listener(m_native->dataSource, &k_sourceListener, m_native.get());

        // WHAT THE PACKAGE CAN ANSWER, not only what it holds. Without the reachable formats a
        // conversion would serve ClaFi talking to itself, no other application knowing to ask.
        std::vector<std::string> offered{};
        for (const Format& format : held->advertisedFormats())
        {
            for (const std::string& mimeName : mimeNamesFor(format))
            {
                if (std::ranges::find(offered, mimeName) != offered.end())
                    continue;

                offered.push_back(mimeName);
                ::wl_data_source_offer(m_native->dataSource, mimeName.c_str());
            }
        }

        ::wl_data_device_set_selection(device, m_native->dataSource, stamp.serial);

        // THE NEW SELECTION IS IN PLACE FIRST. Destroying the source behind a selection clears
        // it, and doing that before this one is set would leave the clipboard empty in between.
        if (previous)
            ::wl_data_source_destroy(previous);
    }

    Offer* Clipboard::offer()
    {
        // THIS PROCESS'S OWN SELECTION IS READ WITHOUT THE COMPOSITOR. The write that answers a
        // receive happens on the event loop, so a read going out and back would be waiting on
        // the loop it is blocking.
        if (Offer* own = ownSelectionOffer())
            return own;

        DisplayManager& display = m_native->display();
        const SelectionOffer current = display.selectionOffer();
        if (current.empty())
        {
            m_native->offerHandle = {};
            dropOffer();
            return nullptr;
        }

        if (m_native->offerHandle == current)
            return heldOffer();

        m_native->offerHandle = current;
        holdOffer(std::make_unique<CompositorOffer>(
            display, current, display.selectionMimeTypes()));
        return heldOffer();
    }

    void Clipboard::release()
    {
        dropSource();
        m_native->destroyDataSource();
    }

    // The listener is already on the data device - see DisplayManager::createDataDevice, which
    // puts it there before the first selection event can arrive, earlier than anything above the
    // platform layer exists. All that is missing until now is somewhere for it to report to.
    void Clipboard::startWatching()
    {
        m_native->display().setSelectionHandler([this]() {
            clipboardChanged();
        });
    }

    // NativeClipboard

    NativeClipboard::NativeClipboard(Clipboard& clipboard)
        :
        owner{ clipboard }
    {
    }

    NativeClipboard::~NativeClipboard()
    {
        destroyDataSource();
        if (owner.m_watching)
            display().setSelectionHandler({});
    }

    std::string NativeClipboard::bytesFor(const Format& format)
    {
        Source* held = owner.heldSource();
        if (!held)
            return {};

        const Payload* content = held->content(format);
        if (!content)
            return {};

        return nativeBytes(format, *content);
    }

    void NativeClipboard::send(const char* mimeName, const std::int32_t fd)
    {
        ignoreBrokenPipe();

        const std::string bytes = bytesFor(heldFormatForMimeName(std::string{ mimeName }));
        writeAll(fd, bytes);
        ::close(fd);
    }

    void NativeClipboard::cancelled(wl_data_source* source)
    {
        if (source != dataSource)
        {
            ::wl_data_source_destroy(source);
            return;
        }

        owner.release();
    }

    void NativeClipboard::destroyDataSource()
    {
        if (dataSource)
            ::wl_data_source_destroy(dataSource);

        dataSource = nullptr;
    }

    DisplayManager& NativeClipboard::display() const
    {
        return static_cast<IDisplayAccess&>(owner.m_platform).display();
    }
}
