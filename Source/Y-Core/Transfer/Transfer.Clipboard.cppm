export module ClaFi.Core.Transfer.Clipboard;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.UiTypes;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    // THE SELECTION MOVED ON, and that is the whole of what it says. See Transfer
    export struct ClipboardChangeEvent : public Event
    {
    };

    // The platform's half of the clipboard, defined in the platform's own unit. See Transfer
    struct NativeClipboard;

    // The one way on and off the system clipboard, owned by the platform. See Transfer
    export class Clipboard
    {
        friend struct NativeClipboard;
    public:
        // Takes the platform under the one name this can see it by. See Transfer
        explicit Clipboard(IPlatformServices&);
        ~Clipboard();
        Clipboard(const Clipboard&) = delete;
        Clipboard& operator=(const Clipboard&) = delete;
    public:
        // Takes the package and stands behind it. The stamp is the input event that asked for the
        // copy: a display server that authorises the request refuses one naming no event, so a
        // copy made outside an input event cannot take the clipboard.
        void set(Source&& source, InputStamp);
        // What the clipboard holds now, or null while there is nothing to read. Owned here.
        [[nodiscard]] Offer* offer();
        // WHAT IS ON THE CLIPBOARD NOW, AS A NUMBER TO COMPARE. It moves whenever the content
        // does and never otherwise, so anything holding a view of the clipboard knows whether to
        // rebuild it. Zero while nothing has been on it.
        //
        // AN OFFER'S ADDRESS IS NOT ITS IDENTITY, in either direction. The offer over this
        // process's own package is one object held here, so two copies made here in a row share
        // an address; a freed offer's address is handed to the next one, so two packages can
        // share one across a real change.
        [[nodiscard]] std::uint64_t generation();
        // Whether the clipboard answers any of these, directly or by one conversion. Asked
        // whenever a paste action is polled for its state, which is far more often than anything
        // pastes, so it reads advertised names and never content.
        [[nodiscard]] std::optional<Match> accepts(const FormatList& accepted);
        // Drops the outgoing package. Called from the platform's half when the selection is taken
        // elsewhere.
        void release();
        // WHAT TO CONNECT TO INSTEAD OF ASKING ON A TIMER:
        //
        //     clipboard.events().connect<ClipboardChangeEvent>([](ClipboardChangeEvent&) { ... });
        //
        // The first connect is what starts the platform watching, so nothing is registered with
        // the display server in an application that never asks.
        //
        // A connection made by anything shorter-lived than the platform must be scoped: the
        // dispatcher is this object's, and the platform keeps it up past every window, which is
        // the safe direction for a ScopedEventConnection and the only direction that is safe at
        // all.
        [[nodiscard]] EventDispatcher& events();
    private:
        // THE PACKAGE AS ANOTHER APPLICATION WOULD SEE IT: a framework format the package holds
        // is listed under the platform's name for it, and read back as the bytes the platform
        // would send. The content itself is answered for the framework format alone.
        class SourceOffer : public Offer
        {
        public:
            explicit SourceOffer(Clipboard& owner);

            [[nodiscard]] FormatList advertisedFormats() const override;
            [[nodiscard]] const Format* platformFormat(StandardFormat) const override;
            [[nodiscard]] Payload read(const Format&) override;
            [[nodiscard]] std::string readBytes(const Format&) override;
        private:
            // The framework format a platform name in the list stands for, if it stands for one.
            [[nodiscard]] static std::optional<StandardFormat> standardBehind(const Format&);
            // The platform's names, held so they can be handed out by reference.
            [[nodiscard]] const Format& nameOf(StandardFormat) const;
        private:
            Clipboard& m_owner;
            mutable std::map<StandardFormat, Format> m_names{};
        };

        using NativePtr = std::unique_ptr<NativeClipboard>;
    private:
        // THE CONTENT OWNER, one for every platform. The package is reached from this module's
        // own implementation units, the platform's included, and from nowhere above the platform
        // layer. What differs per platform is the protocol object beside it - a wl_data_source
        // held by the display, an IDataObject OLE holds a reference to - and that lives in
        // NativeClipboard.
        void holdSource(Source&&);
        [[nodiscard]] Source* heldSource();
        void dropSource();
        void holdOffer(std::unique_ptr<Offer>);
        [[nodiscard]] Offer* heldOffer();
        void dropOffer();
        // AN OFFER OVER THE PACKAGE THIS PROCESS IS STANDING BEHIND, or null while it holds none.
        // Reading one's own selection back through the display server would deadlock on Wayland
        // - the write that answers the read happens on the event loop the read is blocking - and
        // is pointless on either platform, the content being in hand and in its richest form.
        [[nodiscard]] Offer* ownSelectionOffer();
        // THE SELECTION MOVED ON, said by the platform's half when it notices.
        void clipboardChanged();
        // Whatever the platform needs in order to notice at all - a handler on the data device
        // under Wayland, a clipboard format listener on a message-only window under Windows.
        // Called once, from the first connect. Defined by the platform.
        void startWatching();
    private:
        IPlatformServices& m_platform;
        std::unique_ptr<Source> m_source{};
        std::unique_ptr<Offer> m_offer{};
        SourceOffer m_sourceOffer{ *this };
        EventDispatcher m_events{};
        bool m_watching{ false };
        // MOVED WHERE THE CONTENT DOES. Every hold is a package that was not there before, even
        // one whose bytes match the last; a drop counts only where something was there to drop,
        // a platform being free to ask for one at any time.
        std::uint64_t m_generation{ 0 };
        // LAST, SO THAT IT IS THE FIRST TO GO: a platform that renders the package on the way
        // out reads it from the members above, which are still standing then.
        NativePtr m_native;
    };

    // THE BYTES A CONTENT TRAVELS AS ON THIS PLATFORM. The standard set states no spelling of its
    // own - it is UTF-16 and CRLF on one side, UTF-8 and LF on the other - so the platform layer
    // is the only side that can write one. A format that spells itself is spelled by toBytes,
    // whichever platform is asked.
    //
    // THE CONTENT'S BYTES AND NOT THE ENVELOPE AROUND THEM. A platform that frames a payload for
    // its own carriage puts the frame on where it writes it, so this answers what another
    // application reads - see Offer::readBytes, which answers the same bytes off the wire.
    [[nodiscard]] std::string nativeBytes(const Format&, const Payload&);

    // THE PLATFORM FORMAT THIS PLATFORM PLACES A FRAMEWORK FORMAT UNDER, by its platform name -
    // CF_UNICODETEXT and CF_DIB on one side, text/plain;charset=utf-8 and image/bmp on the other.
    // What the own-selection offer lists in place of the framework format, so a viewer of this
    // process's own package sees what another application would.
    [[nodiscard]] Format platformFormatOf(StandardFormat);
}
