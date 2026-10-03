module;
#include "Windows.Headers.h"
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

module ClaFi.Core.System.WebFetch;

import ClaFi.Core.System.Timer;

import ClaFi.StdLib;

namespace ClaFi
{
    // WinHTTP in its blocking form, on a thread of the fetch's own. See Platform
    class WebFetch::Impl
    {
    public:
        explicit Impl(WebFetch&);
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        // Closes the request the thread is blocked in, which ends the call, and joins the thread.
        ~Impl();
    private:
        // Closes a WinHTTP handle as it goes out of scope.
        struct HandleCloser
        {
            void operator()(HINTERNET handle) const;
        };
        using InternetHandle = std::unique_ptr<void, HandleCloser>;
    private:
        void threadFunc();
        // The status the server answered with and the body read - 0 where no answer came.
        [[nodiscard]] int fetch(std::string& body);
        // Sends the request and reads the answer, refusing a body longer than a fetch keeps.
        [[nodiscard]] int exchange(HINTERNET request, std::string& body) const;
    private:
        WebFetch& m_owner;
        std::atomic<bool> m_cancelled{ false };
        // The request while it is out, so that the destructor can close it from the UI thread.
        std::mutex m_requestMutex{};
        HINTERNET m_request{ nullptr };
        std::thread m_thread{};
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi
{
    namespace
    {
        // How long each step of a request - resolving, connecting, sending, receiving - may take.
        constexpr int k_stepTimeoutMs{ 10000 };
        // What one read asks WinHTTP for.
        constexpr std::size_t k_readChunkSize{ 4096 };
    }

    // WebFetch::Impl

    WebFetch::Impl::Impl(WebFetch& owner)
        :
        m_owner{ owner }
    {
        m_thread = std::thread{ &Impl::threadFunc, this };
    }

    WebFetch::Impl::~Impl()
    {
        m_cancelled = true;
        {
            std::lock_guard<std::mutex> lock{ m_requestMutex };
            if (m_request != nullptr)
            {
                ::WinHttpCloseHandle(m_request);
                m_request = nullptr;
            }
        }
        if (m_thread.joinable())
            m_thread.join();
    }

    void WebFetch::Impl::HandleCloser::operator()(const HINTERNET handle) const
    {
        ::WinHttpCloseHandle(handle);
    }

    void WebFetch::Impl::threadFunc()
    {
        std::string body{};
        const int status = fetch(body);
        if (m_cancelled)
            return;
        if (status == 0)
            body.clear();
        m_owner.reportDone(status, std::move(body));
    }

    // THE REQUEST IS REGISTERED UNDER THE MUTEX, and closed under it by whichever side gets there
    // first, so the destructor never closes a handle the thread has already closed.
    int WebFetch::Impl::fetch(std::string& body)
    {
        const std::wstring& url = m_owner.url();
        URL_COMPONENTS parts{};
        parts.dwStructSize = sizeof(parts);
        parts.dwHostNameLength = static_cast<DWORD>(-1);
        parts.dwUrlPathLength = static_cast<DWORD>(-1);
        parts.dwExtraInfoLength = static_cast<DWORD>(-1);
        if (!::WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts))
            return 0;
        if (parts.nScheme != INTERNET_SCHEME_HTTPS)
            return 0;
        const std::wstring host{ parts.lpszHostName, parts.dwHostNameLength };
        std::wstring path{ parts.lpszUrlPath, parts.dwUrlPathLength };
        path.append(std::wstring_view{ parts.lpszExtraInfo, parts.dwExtraInfoLength });

        const InternetHandle session{ ::WinHttpOpen(k_userAgent.data(),
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS,
            0) };
        if (!session)
            return 0;
        ::WinHttpSetTimeouts(session.get(), k_stepTimeoutMs, k_stepTimeoutMs, k_stepTimeoutMs,
            k_stepTimeoutMs);
        DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        ::WinHttpSetOption(session.get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy,
            sizeof(redirectPolicy));
        const InternetHandle connection{
            ::WinHttpConnect(session.get(), host.c_str(), parts.nPort, 0) };
        if (!connection)
            return 0;
        const HINTERNET request = ::WinHttpOpenRequest(connection.get(), L"GET", path.c_str(),
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (request == nullptr)
            return 0;
        {
            std::lock_guard<std::mutex> lock{ m_requestMutex };
            if (m_cancelled)
            {
                ::WinHttpCloseHandle(request);
                return 0;
            }
            m_request = request;
        }
        const int status = exchange(request, body);
        {
            std::lock_guard<std::mutex> lock{ m_requestMutex };
            if (m_request != nullptr)
            {
                ::WinHttpCloseHandle(m_request);
                m_request = nullptr;
            }
        }
        return status;
    }

    // A CLOSED REQUEST FAILS THE CALL BLOCKED ON IT, and the flag is read after every call, so
    // the thread makes no further call on a handle the destructor has closed.
    int WebFetch::Impl::exchange(const HINTERNET request, std::string& body) const
    {
        const bool sent = ::WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) != FALSE;
        if (!sent || m_cancelled)
            return 0;
        const bool received = ::WinHttpReceiveResponse(request, nullptr) != FALSE;
        if (!received || m_cancelled)
            return 0;
        DWORD status = 0;
        DWORD statusSize = sizeof(status);
        const bool statusRead = ::WinHttpQueryHeaders(request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
            &status, &statusSize, WINHTTP_NO_HEADER_INDEX) != FALSE;
        if (!statusRead || m_cancelled)
            return 0;
        std::array<char, k_readChunkSize> chunk{};
        while (true)
        {
            DWORD length = 0;
            const bool read = ::WinHttpReadData(request, chunk.data(),
                static_cast<DWORD>(chunk.size()), &length) != FALSE;
            if (!read || m_cancelled)
                return 0;
            if (length == 0)
                return static_cast<int>(status);
            if (body.size() + length > k_maxBodySize)
                return 0;
            body.append(chunk.data(), length);
        }
    }

    // WebFetch - platform members

    WebFetch::WebFetch(const std::wstring_view url)
        :
        m_url{ url }
    {
        m_doneTimer.onTick([this](TimerEvent&) {
            deliver();
        });
        m_impl = std::make_unique<Impl>(*this);
    }

    WebFetch::~WebFetch() = default;
}
