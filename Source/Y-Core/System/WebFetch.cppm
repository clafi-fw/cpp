module;
#include "EventBindings.h"

export module ClaFi.Core.System.WebFetch;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;

import ClaFi.StdLib;

namespace ClaFi
{
    export class WebFetch;

    // A fetch has ended, with an answer or without one. See UI-Types
    export class WebFetchEvent : public EventOf<WebFetch>
    {
    public:
        explicit WebFetchEvent(WebFetch& sender);
    };

    // One HTTPS GET, made on a thread of its own and answered on the UI thread. See UI-Types
    export class WebFetch : public EventComponent
    {
    public:
        // Starts the request at once. An address that is not https:// is answered with nothing.
        explicit WebFetch(std::wstring_view url);
        WebFetch(const WebFetch&) = delete;
        WebFetch& operator=(const WebFetch&) = delete;
        // Cancels a request still running and waits for its thread. See UI-Types
        ~WebFetch();
    public:
        DECLARE_EVENT(WebFetchEvent, OnDone, onDone)
    public:
        [[nodiscard]] const std::wstring& url() const { return m_url; }
        [[nodiscard]] bool done() const { return m_done; } // the answer has reached the UI thread
        [[nodiscard]] int status() const { return m_status; } // the HTTP status, 0 where none came
        [[nodiscard]] const std::string& body() const { return m_body; } // the bytes as they came
    private:
        class Impl;
        using ImplPtr = std::unique_ptr<Impl>;
    private:
        // Called by the platform's thread as it ends, with what arrived. Safe from any thread.
        void reportDone(int status, std::string&& body);
        // Raised by m_doneTimer on the UI thread: takes what the thread left and tells listeners.
        void deliver();
    private:
        // What every request names itself as - GitHub refuses one that names nothing.
        static constexpr std::wstring_view k_userAgent{ L"ClaFi" };
        // The most of a body a fetch keeps; a longer answer is refused whole.
        static constexpr std::size_t k_maxBodySize{ 1024 * 1024 };
        const std::wstring m_url;
        // What the thread leaves for deliver - the one state both threads touch.
        std::mutex m_pendingMutex{};
        int m_pendingStatus{ 0 };
        std::string m_pendingBody{};
        bool m_done{ false };
        int m_status{ 0 };
        std::string m_body{};
        // Declared before m_impl, so it stands for as long as the thread that arms it runs.
        UiTimer m_doneTimer{};
        ImplPtr m_impl{};
    };
}
