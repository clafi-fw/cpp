module ClaFi.Core.Context.UpdateCheck;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.WebFetch;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    namespace
    {
        // A version as major.minor.patch, compared part by part.
        struct Version
        {
            std::uint32_t major{};
            std::uint32_t minor{};
            std::uint32_t patch{};
            [[nodiscard]] auto operator<=>(const Version&) const = default;
        };

        // The three numbers of a major.minor.patch spelling, or none for any other spelling.
        [[nodiscard]] std::optional<Version> parseVersion(std::wstring_view spelling);
        // The versions every tag in a matching-refs answer carries after the prefix.
        [[nodiscard]] std::optional<std::vector<std::wstring>> taggedVersions(
            std::string_view answer, std::wstring_view tagPrefix);

        // Where GitHub answers questions about a repository, and where it shows one.
        constexpr std::wstring_view k_apiAddress{ L"https://api.github.com/repos/" };
        constexpr std::wstring_view k_siteAddress{ L"https://github.com/" };
        constexpr int k_statusOk{ 200 };
    }

    // UpdateCheckEvent

    UpdateCheckEvent::UpdateCheckEvent(UpdateCheck& sender)
        :
        EventOf<UpdateCheck>{ sender }
    {
    }

    // UpdateCheck

    UpdateCheck::UpdateCheck(const UpdateSource& source, const std::wstring_view version)
        :
        m_source{ source },
        m_version{ version }
    {
    }

    bool UpdateCheck::available() const
    {
        return !m_source.repository.empty() && parseVersion(m_version).has_value();
    }

    std::wstring UpdateCheck::releaseAddress() const
    {
        if (m_state != UpdateState::Newer)
            return {};
        std::wstring address{ k_siteAddress };
        address += m_source.repository;
        address += L"/releases/tag/";
        address += m_source.tagPrefix;
        address += m_newerVersion;
        return address;
    }

    // NOT FROM INSIDE AN UpdateCheckEvent HANDLER: the fetch replaced here may be the one whose
    // done event raised it.
    void UpdateCheck::start()
    {
        if (!available() || m_state == UpdateState::Checking)
            return;
        m_fetch = std::make_unique<WebFetch>(tagsAddress());
        m_fetch->onDone([this](WebFetchEvent& event) {
            finish(event.sender());
        });
        setState(UpdateState::Checking);
    }

    std::wstring UpdateCheck::tagsAddress() const
    {
        std::wstring address{ k_apiAddress };
        address += m_source.repository;
        address += L"/git/matching-refs/tags/";
        address += m_source.tagPrefix;
        return address;
    }

    // A RUNNING VERSION AHEAD OF EVERY TAG IS THE LATEST - one bumped and not yet released.
    void UpdateCheck::finish(const WebFetch& fetch)
    {
        const std::optional<std::vector<std::wstring>> versions = fetch.status() == k_statusOk
            ? taggedVersions(fetch.body(), m_source.tagPrefix)
            : std::nullopt;
        if (!versions)
        {
            setState(UpdateState::Failed);
            return;
        }
        Version newest = parseVersion(m_version).value_or(Version{});
        std::wstring newestSpelling{};
        for (const std::wstring& spelling : versions.value())
        {
            const std::optional<Version> tagged = parseVersion(spelling);
            if (tagged && tagged.value() > newest)
            {
                newest = tagged.value();
                newestSpelling = spelling;
            }
        }
        m_newerVersion = std::move(newestSpelling);
        setState(m_newerVersion.empty() ? UpdateState::Latest : UpdateState::Newer);
    }

    void UpdateCheck::setState(const UpdateState state)
    {
        m_state = state;
        emitEvent<UpdateCheckEvent>(*this);
    }

    // parseVersion, taggedVersions

    namespace
    {
        std::optional<Version> parseVersion(const std::wstring_view spelling)
        {
            std::array<std::uint32_t, 3> parts{};
            std::size_t part = 0;
            bool digitSeen = false;
            for (const wchar_t chr : spelling)
            {
                if (chr == L'.')
                {
                    if (!digitSeen || part + 1 == parts.size())
                        return std::nullopt;
                    ++part;
                    digitSeen = false;
                }
                else if (chr >= L'0' && chr <= L'9')
                {
                    const std::uint32_t digit = static_cast<std::uint32_t>(chr - L'0');
                    if (parts[part] > (std::numeric_limits<std::uint32_t>::max() - digit) / 10u)
                        return std::nullopt;
                    parts[part] = parts[part] * 10u + digit;
                    digitSeen = true;
                }
                else
                {
                    return std::nullopt;
                }
            }
            if (!digitSeen || part + 1 != parts.size())
                return std::nullopt;
            return Version{ parts[0], parts[1], parts[2] };
        }

        // THE ANSWER IS A JSON ARRAY OF REFS, and a ref name is the only string in it that starts
        // with refs/tags/ - every address in it starts with its scheme - so a quote followed by
        // that and the prefix opens one. Anything that is not an array is no answer at all.
        std::optional<std::vector<std::wstring>> taggedVersions(const std::string_view answer,
            const std::wstring_view tagPrefix)
        {
            const std::size_t first = answer.find_first_not_of(" \t\r\n");
            if (first == std::string_view::npos || answer[first] != '[')
                return std::nullopt;
            const std::string opening = std::string{ "\"refs/tags/" } + toUtf8(tagPrefix);
            std::vector<std::wstring> versions{};
            std::size_t pos = answer.find(opening);
            while (pos != std::string_view::npos)
            {
                const std::size_t start = pos + opening.size();
                const std::size_t end = answer.find('"', start);
                if (end == std::string_view::npos)
                    break;
                versions.push_back(fromUtf8(answer.substr(start, end - start)));
                pos = answer.find(opening, end);
            }
            return versions;
        }
    }
}
