module ClaFi.Core.Context.UpdateCheck;

import ClaFi.Core.Dom_StdSerializers;
import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
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
        // A moment as the config keeps it - UTC to the second, as 2026-10-04T09:20:34Z.
        [[nodiscard]] std::wstring utcSpelling(std::chrono::sys_seconds moment);
        // The moment a spelling names, or none for anything utcSpelling would not have written.
        [[nodiscard]] UpdateCheck::AnswerTime parseUtcSpelling(std::wstring_view spelling);

        // Both values of the section an answer is kept in.
        using RecordNode = Dom::Value<std::wstring>;

        // The names of the two values in that section.
        namespace RecordNodes
        {
            constexpr std::wstring_view answeredAt = L"AnsweredAt";
            constexpr std::wstring_view newestVersion = L"NewestVersion";
        }

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

    // THE NEWEST VERSION IS KEPT WHETHER OR NOT IT WAS NEWER, as its tag spells it, and empty
    // where no tag was published. Empty values are no answer at all.
    Dom::Dt::Section UpdateCheck::createConfigSchema()
    {
        return {
            Dom::Dt::Value{ RecordNodes::answeredAt, std::wstring{} },
            Dom::Dt::Value{ RecordNodes::newestVersion, std::wstring{} }
        };
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

    // THE SECTION'S OWN CHANGE IS WHAT A STORED ANSWER ARRIVES ON. The config is read after the
    // context is built, inside one transaction, so the section is told once with both values in.
    void UpdateCheck::keepIn(Dom::Section& record)
    {
        m_record = &record;
        m_recordConnection = ScopedEventConnection{ record.connectEvent(
            [this](Dom::NestedChangeEvent&) {
                takeRecord();
            }) };
        takeRecord();
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

    // A failure leaves the last answer where it is, so its time still says when one arrived.
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
        std::optional<Version> newest{};
        std::wstring newestSpelling{};
        for (const std::wstring& spelling : versions.value())
        {
            const std::optional<Version> tagged = parseVersion(spelling);
            if (tagged && (!newest || tagged.value() > newest.value()))
            {
                newest = tagged;
                newestSpelling = spelling;
            }
        }
        const std::chrono::sys_seconds answeredAt =
            std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        storeAnswer(answeredAt, newestSpelling);
        takeAnswer(answeredAt, newestSpelling);
    }

    // ONE CHANGE FOR THE TWO VALUES. The section's handler reads both, and a new time beside the
    // previous answer's version would be an answer nobody gave.
    void UpdateCheck::storeAnswer(const std::chrono::sys_seconds answeredAt,
        const std::wstring_view newestVersion)
    {
        if (!m_record)
            return;
        const Dom::Transaction transaction = m_record->startTransaction();
        RecordNode& answeredAtNode = (*m_record / RecordNodes::answeredAt).as<RecordNode>();
        RecordNode& newestVersionNode = (*m_record / RecordNodes::newestVersion).as<RecordNode>();
        answeredAtNode.set(utcSpelling(answeredAt));
        newestVersionNode.set(std::wstring{ newestVersion });
    }

    void UpdateCheck::takeRecord()
    {
        const RecordNode& answeredAtNode = (*m_record / RecordNodes::answeredAt).as<RecordNode>();
        const RecordNode& newestVersionNode =
            (*m_record / RecordNodes::newestVersion).as<RecordNode>();
        const AnswerTime moment = parseUtcSpelling(answeredAtNode.get());
        if (!moment)
            return;
        takeAnswer(moment.value(), newestVersionNode.get());
    }

    // A RUNNING VERSION AHEAD OF EVERY TAG IS THE LATEST - one bumped and not yet released. An
    // answer kept by an earlier run is compared against the version running now, so an update
    // installed since then reads as the latest.
    void UpdateCheck::takeAnswer(const std::chrono::sys_seconds answeredAt,
        const std::wstring_view newestVersion)
    {
        const std::optional<Version> newest = parseVersion(newestVersion);
        const bool newer = newest && newest.value() > parseVersion(m_version).value_or(Version{});
        const UpdateState state = newer ? UpdateState::Newer : UpdateState::Latest;
        std::wstring newerVersion = newer ? std::wstring{ newestVersion } : std::wstring{};
        if (state == m_state && newerVersion == m_newerVersion && answeredAt == m_answeredAt)
            return;
        m_answeredAt = answeredAt;
        m_newerVersion = std::move(newerVersion);
        setState(state);
    }

    void UpdateCheck::setState(const UpdateState state)
    {
        m_state = state;
        emitEvent<UpdateCheckEvent>(*this);
    }

    // parseVersion, taggedVersions, utcSpelling, parseUtcSpelling

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

        std::wstring utcSpelling(const std::chrono::sys_seconds moment)
        {
            const std::chrono::sys_days day = std::chrono::floor<std::chrono::days>(moment);
            const std::chrono::year_month_day date{ day };
            const std::chrono::hh_mm_ss<std::chrono::seconds> time{ moment - day };
            return std::format(L"{:04}-{:02}-{:02}T{:02}:{:02}:{:02}Z",
                static_cast<int>(date.year()),
                static_cast<unsigned>(date.month()),
                static_cast<unsigned>(date.day()),
                time.hours().count(),
                time.minutes().count(),
                time.seconds().count());
        }

        // THE SPELLING IS READ BY ITS SHAPE: the config is text the user is free to edit, and
        // anything else - a time zone, a fraction of a second, a date that does not exist - is
        // taken for no answer.
        UpdateCheck::AnswerTime parseUtcSpelling(const std::wstring_view spelling)
        {
            constexpr std::wstring_view k_shape = L"0000-00-00T00:00:00Z";
            if (spelling.size() != k_shape.size())
                return std::nullopt;
            for (std::size_t i = 0; i < k_shape.size(); ++i)
            {
                const bool digit = spelling[i] >= L'0' && spelling[i] <= L'9';
                const bool fits = k_shape[i] == L'0' ? digit : spelling[i] == k_shape[i];
                if (!fits)
                    return std::nullopt;
            }
            const auto number = [spelling](const std::size_t start, const std::size_t length) {
                unsigned value = 0;
                for (const wchar_t chr : spelling.substr(start, length))
                    value = value * 10u + static_cast<unsigned>(chr - L'0');
                return value;
            };
            const std::chrono::year_month_day date{
                std::chrono::year{ static_cast<int>(number(0, 4)) },
                std::chrono::month{ number(5, 2) },
                std::chrono::day{ number(8, 2) }
            };
            const unsigned hours = number(11, 2);
            const unsigned minutes = number(14, 2);
            const unsigned seconds = number(17, 2);
            if (!date.ok() || hours > 23u || minutes > 59u || seconds > 59u)
                return std::nullopt;
            return std::chrono::sys_days{ date }
                + std::chrono::hours{ hours }
                + std::chrono::minutes{ minutes }
                + std::chrono::seconds{ seconds };
        }
    }
}
