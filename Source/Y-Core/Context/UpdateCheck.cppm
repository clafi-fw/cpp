module;
#include "../System/EventBindings.h"

export module ClaFi.Core.Context.UpdateCheck;

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
        UpdateCheck(const UpdateSource&, std::wstring_view version);
        UpdateCheck(const UpdateCheck&) = delete;
        UpdateCheck& operator=(const UpdateCheck&) = delete;
    public:
        DECLARE_EVENT(UpdateCheckEvent, OnChange, onChange)
    public:
        // Whether there is anything to ask - a major.minor.patch version and a repository.
        [[nodiscard]] bool available() const;
        [[nodiscard]] UpdateState state() const { return m_state; }
        // The newest published version as its tag spells it - empty unless the state is Newer.
        [[nodiscard]] std::wstring_view newerVersion() const { return m_newerVersion; }
        // The page of the newest release - empty unless the state is Newer.
        [[nodiscard]] std::wstring releaseAddress() const;
        // Asks, unless the question is out already. See Context
        void start();
    private:
        // Where the tags starting with the prefix are listed.
        [[nodiscard]] std::wstring tagsAddress() const;
        // Reads the answer into the state.
        void finish(const WebFetch&);
        void setState(UpdateState);
    private:
        UpdateSource m_source;
        std::wstring_view m_version;
        UpdateState m_state{ UpdateState::Unchecked };
        std::wstring m_newerVersion{};
        std::unique_ptr<WebFetch> m_fetch{};
    };
}
