module;
#include "../System/EventBindings.h"

export module ClaFi.Core.Context.UpdateCheck;

import ClaFi.Core.DomEngine;
import ClaFi.Core.DomEngine_Dt;
import ClaFi.Core.System.Events;
import ClaFi.Core.System.WebFetch;

import ClaFi.StdLib;

namespace ClaFi
{
    // Where an application's releases are published - a GitHub repository and its tags. See Context
    export struct UpdateSource
    {
        std::wstring_view repository{}; // owner/name, as the repository's address spells it
        std::wstring_view tagPrefix{}; // what a release's tag holds ahead of its version
    };

    // What the update check has found out so far.
    export enum class UpdateState
    {
        Unchecked,  // nothing has been asked
        Checking,   // the question is out
        Latest,     // no published release is newer than the running one
        Newer,      // a newer release is published
        Failed      // no answer arrived, or the one that did could not be read
    };

    export class UpdateCheck;

    // The update check's state has moved. See Context
    export class UpdateCheckEvent : public EventOf<UpdateCheck>
    {
    public:
        explicit UpdateCheckEvent(UpdateCheck& sender);
    };

    // Asks where the application's releases are published whether a newer one is. See Context
    export class UpdateCheck : public EventComponent
    {
    public:
        // When an answer arrived, to the second - none where none has.
        using AnswerTime = std::optional<std::chrono::sys_seconds>;
    public:
        UpdateCheck(const UpdateSource&, std::wstring_view version);
        UpdateCheck(const UpdateCheck&) = delete;
        UpdateCheck& operator=(const UpdateCheck&) = delete;
    public:
        DECLARE_EVENT(UpdateCheckEvent, OnChange, onChange)
    public:
        // The section keepIn takes, for an application's config schema. See Context
        [[nodiscard]] static Dom::Dt::Section createConfigSchema();
        // Whether there is anything to ask - a major.minor.patch version and a repository.
        [[nodiscard]] bool available() const;
        [[nodiscard]] UpdateState state() const { return m_state; }
        // The newest published version as its tag spells it - empty unless the state is Newer.
        [[nodiscard]] std::wstring_view newerVersion() const { return m_newerVersion; }
        // The page of the newest release - empty unless the state is Newer.
        [[nodiscard]] std::wstring releaseAddress() const;
        // When the last answer arrived, in this run or an earlier one - none until one has.
        [[nodiscard]] AnswerTime answeredAt() const { return m_answeredAt; }
        // Takes the answer the section holds and keeps every later one there. See Context
        void keepIn(Dom::Section&);
        // Asks, unless the question is out already. See Context
        void start();
    private:
        // Where the tags starting with the prefix are listed.
        [[nodiscard]] std::wstring tagsAddress() const;
        // Reads the answer into the state.
        void finish(const WebFetch&);
        // Writes an answer into the section keepIn took, where it took one.
        void storeAnswer(std::chrono::sys_seconds answeredAt, std::wstring_view newestVersion);
        // Reads the section keepIn took into the state, where it holds an answer.
        void takeRecord();
        // The state an answer makes, raised only where it moves anything. See Context
        void takeAnswer(std::chrono::sys_seconds answeredAt, std::wstring_view newestVersion);
        void setState(UpdateState);
    private:
        UpdateSource m_source;
        std::wstring_view m_version;
        UpdateState m_state{ UpdateState::Unchecked };
        std::wstring m_newerVersion{};
        AnswerTime m_answeredAt{};
        std::unique_ptr<WebFetch> m_fetch{};
        // Where the last answer is kept - the section keepIn took, null until then.
        Dom::Section* m_record{ nullptr };
        ScopedEventConnection m_recordConnection{};
    };
}
