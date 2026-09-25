module ClaFi.Core.Transfer.Clipboard;

import ClaFi.Core.Transfer.Conversion;
import ClaFi.Core.Transfer.Offer;
import ClaFi.Core.Transfer.Source;
import ClaFi.Core.Transfer.Formats;
import ClaFi.Core.System.Events;
import ClaFi.StdLib;

namespace ClaFi::Transfer
{
    namespace
    {
        constexpr StandardFormat k_standardFormats[] = {
            StandardFormat::Text,
            StandardFormat::Picture
        };
    }

    std::uint64_t Clipboard::generation()
    {
        // ASKED THROUGH offer(), because that is where a platform notices. Windows reads its
        // sequence counter there and Wayland compares the selection handle, so until one of them
        // has been asked nothing here has been told the clipboard moved.
        [[maybe_unused]] const Offer* current = offer();
        return m_generation;
    }

    std::optional<Match> Clipboard::accepts(const FormatList& accepted)
    {
        Offer* current = offer();
        if (!current)
            return std::nullopt;

        return current->match(accepted);
    }

    EventDispatcher& Clipboard::events()
    {
        if (!m_watching)
        {
            // Raised before the call rather than after it. A platform that notices a selection
            // while it is starting up reports it through clipboardChanged, and an application
            // connecting from that handler would come back through here and start twice.
            m_watching = true;
            startWatching();
        }

        return m_events;
    }

    // Clipboard::SourceOffer

    Clipboard::SourceOffer::SourceOffer(Clipboard& owner)
        :
        m_owner{ owner }
    {
    }

    FormatList Clipboard::SourceOffer::advertisedFormats() const
    {
        const Source* source = m_owner.m_source.get();
        if (!source)
            return {};

        FormatList result{};
        for (const Format& format : source->advertisedFormats())
        {
            const Format listed = format.kind() == Format::Kind::Standard
                ? nameOf(format.standard())
                : format;
            if (std::ranges::find(result, listed) == result.end())
                result.push_back(listed);
        }

        return result;
    }

    const Format* Clipboard::SourceOffer::platformFormat(const StandardFormat standard) const
    {
        const Source* source = m_owner.m_source.get();
        if (!source || !source->provides(Format::of(standard)))
            return nullptr;

        return &nameOf(standard);
    }

    Payload Clipboard::SourceOffer::read(const Format& format)
    {
        Source* source = m_owner.m_source.get();
        if (!source)
            return {};

        if (format.kind() == Format::Kind::Standard)
        {
            const Payload* content = source->content(format);
            return content ? *content : Payload{};
        }

        // A platform name is answered as the bytes it would carry, the way the other offers
        // answer a name nothing here decodes.
        if (standardBehind(format))
            return Payload{ readBytes(format) };

        const Payload* content = source->content(format);
        return content ? *content : Payload{};
    }

    // WHAT THIS PACKAGE WOULD SEND. Nothing is sent while this process holds the selection,
    // so the bytes are written here rather than read - the platform's spelling for a
    // framework format, under its own name or the platform's, the format's own for the rest.
    // A reader is handed what another application would receive, whoever owns the clipboard.
    std::string Clipboard::SourceOffer::readBytes(const Format& format)
    {
        Source* source = m_owner.m_source.get();
        if (!source)
            return {};

        const std::optional<StandardFormat> standard = standardBehind(format);
        const Format held = standard ? Format::of(*standard) : format;
        const Payload* content = source->content(held);
        if (!content)
            return {};

        return nativeBytes(held, *content);
    }

    std::optional<StandardFormat> Clipboard::SourceOffer::standardBehind(const Format& format)
    {
        if (format.kind() != Format::Kind::Custom)
            return std::nullopt;

        for (const StandardFormat standard : k_standardFormats)
        {
            if (platformFormatOf(standard) == format)
                return standard;
        }

        return std::nullopt;
    }

    const Format& Clipboard::SourceOffer::nameOf(const StandardFormat standard) const
    {
        const auto found = m_names.find(standard);
        if (found != m_names.end())
            return found->second;

        return m_names.emplace(standard, platformFormatOf(standard)).first->second;
    }

    void Clipboard::holdSource(Source&& source)
    {
        m_source = std::make_unique<Source>(std::move(source));
        ++m_generation;
    }

    Source* Clipboard::heldSource()
    {
        return m_source.get();
    }

    void Clipboard::dropSource()
    {
        if (!m_source)
            return;

        m_source.reset();
        ++m_generation;
    }

    void Clipboard::holdOffer(std::unique_ptr<Offer> offer)
    {
        m_offer = std::move(offer);
        ++m_generation;
    }

    Offer* Clipboard::heldOffer()
    {
        return m_offer.get();
    }

    void Clipboard::dropOffer()
    {
        if (!m_offer)
            return;

        m_offer.reset();
        ++m_generation;
    }

    Offer* Clipboard::ownSelectionOffer()
    {
        return m_source ? &m_sourceOffer : nullptr;
    }

    void Clipboard::clipboardChanged()
    {
        m_events.emit<ClipboardChangeEvent>();
    }
}
