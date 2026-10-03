module;
#include <dlfcn.h>

module ClaFi.Core.System.WebFetch;

import ClaFi.Core.System.Timer;
import ClaFi.Core.System.Utils;

import ClaFi.StdLib;

namespace ClaFi
{
    // libcurl, opened when a fetch runs and never closed. See Platform
    class WebFetch::Impl
    {
    public:
        explicit Impl(WebFetch&);
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        // Asks libcurl to abort, which it hears within a second, and joins the thread.
        ~Impl();
    private:
        void threadFunc();
        // The status the server answered with and the body read - 0 where no answer came.
        [[nodiscard]] int fetch(std::string& body) const;
    private:
        WebFetch& m_owner;
        std::atomic<bool> m_cancelled{ false };
        std::thread m_thread{};
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi
{
    namespace
    {
        // libcurl's entries and option numbers - its ABI, so no header is needed. See Platform
        using CurlHandle = void;
        using CurlCode = int;
        using EasyInitFunc = CurlHandle* (*)();
        using EasySetoptFunc = CurlCode (*)(CurlHandle*, int option, ...);
        using EasyPerformFunc = CurlCode (*)(CurlHandle*);
        using EasyGetinfoFunc = CurlCode (*)(CurlHandle*, int info, ...);
        using EasyCleanupFunc = void (*)(CurlHandle*);

        // The entries, as found in a loaded libcurl.
        struct CurlEntries
        {
            EasyInitFunc easyInit;
            EasySetoptFunc easySetopt;
            EasyPerformFunc easyPerform;
            EasyGetinfoFunc easyGetinfo;
            EasyCleanupFunc easyCleanup;
        };

        // What the two callbacks share with the fetch that hands them over.
        struct Download
        {
            std::string& body;
            const std::atomic<bool>& cancelled;
            std::size_t limit;
        };

        namespace CurlOption
        {
            constexpr int writeData{ 10001 };
            constexpr int url{ 10002 };
            constexpr int timeout{ 13 };
            constexpr int userAgent{ 10018 };
            constexpr int writeFunction{ 20011 };
            constexpr int noProgress{ 43 };
            constexpr int progressData{ 10057 };
            constexpr int connectTimeout{ 78 };
            constexpr int noSignal{ 99 };
            constexpr int progressFunction{ 20219 };
        }

        // The entries of the first libcurl the system has, under either name it ships as.
        [[nodiscard]] std::optional<CurlEntries> openCurl();
        // Takes what arrived; answering less than it was handed refuses a body past the limit.
        std::size_t writeBody(char* data, std::size_t size, std::size_t count, void* download);
        // A nonzero answer aborts the transfer. libcurl asks at least once a second.
        int reportProgress(void* download, std::int64_t, std::int64_t, std::int64_t, std::int64_t);

        constexpr int k_responseCodeInfo{ 0x200002 };
        constexpr CurlCode k_curlOk{ 0 };
        constexpr long k_connectTimeoutSeconds{ 10 };
        constexpr long k_transferTimeoutSeconds{ 30 };
        // The OpenSSL build's name, then the GnuTLS build's - the one an Ubuntu desktop carries.
        constexpr std::array<const char*, 2> k_curlNames{ "libcurl.so.4", "libcurl-gnutls.so.4" };
        constexpr std::wstring_view k_secureScheme{ L"https://" };
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
        if (m_thread.joinable())
            m_thread.join();
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

    int WebFetch::Impl::fetch(std::string& body) const
    {
        if (!m_owner.url().starts_with(k_secureScheme))
            return 0;
        const std::optional<CurlEntries> curl = openCurl();
        if (!curl)
            return 0;
        CurlHandle* handle = curl->easyInit();
        if (handle == nullptr)
            return 0;
        const std::string url = toUtf8(m_owner.url());
        const std::string userAgent = toUtf8(k_userAgent);
        Download download{ body, m_cancelled, k_maxBodySize };
        curl->easySetopt(handle, CurlOption::url, url.c_str());
        curl->easySetopt(handle, CurlOption::userAgent, userAgent.c_str());
        // The fetch is not on the main thread, so libcurl must not time a lookup out with SIGALRM.
        curl->easySetopt(handle, CurlOption::noSignal, 1L);
        curl->easySetopt(handle, CurlOption::connectTimeout, k_connectTimeoutSeconds);
        curl->easySetopt(handle, CurlOption::timeout, k_transferTimeoutSeconds);
        curl->easySetopt(handle, CurlOption::writeFunction, &writeBody);
        curl->easySetopt(handle, CurlOption::writeData, &download);
        curl->easySetopt(handle, CurlOption::progressFunction, &reportProgress);
        curl->easySetopt(handle, CurlOption::progressData, &download);
        curl->easySetopt(handle, CurlOption::noProgress, 0L);
        long status = 0;
        if (curl->easyPerform(handle) == k_curlOk)
            curl->easyGetinfo(handle, k_responseCodeInfo, &status);
        curl->easyCleanup(handle);
        return static_cast<int>(status);
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

    // openCurl, writeBody, reportProgress

    namespace
    {
        // NEVER CLOSED: a transfer aborted while a name resolves leaves libcurl's resolver thread
        // detached, running libcurl's code until the lookup returns. See Platform
        std::optional<CurlEntries> openCurl()
        {
            for (const char* name : k_curlNames)
            {
                void* library = ::dlopen(name, RTLD_NOW | RTLD_LOCAL);
                if (library == nullptr)
                    continue;
                const CurlEntries entries{
                    .easyInit = reinterpret_cast<EasyInitFunc>(
                        ::dlsym(library, "curl_easy_init")),
                    .easySetopt = reinterpret_cast<EasySetoptFunc>(
                        ::dlsym(library, "curl_easy_setopt")),
                    .easyPerform = reinterpret_cast<EasyPerformFunc>(
                        ::dlsym(library, "curl_easy_perform")),
                    .easyGetinfo = reinterpret_cast<EasyGetinfoFunc>(
                        ::dlsym(library, "curl_easy_getinfo")),
                    .easyCleanup = reinterpret_cast<EasyCleanupFunc>(
                        ::dlsym(library, "curl_easy_cleanup"))
                };
                const bool complete = entries.easyInit && entries.easySetopt && entries.easyPerform
                    && entries.easyGetinfo && entries.easyCleanup;
                if (complete)
                    return entries;
            }
            return std::nullopt;
        }

        std::size_t writeBody(char* data, const std::size_t size, const std::size_t count,
            void* download)
        {
            Download& target = *static_cast<Download*>(download);
            const std::size_t length = size * count;
            if (target.body.size() + length > target.limit)
                return 0;
            target.body.append(data, length);
            return length;
        }

        int reportProgress(void* download, std::int64_t, std::int64_t, std::int64_t, std::int64_t)
        {
            const Download& source = *static_cast<const Download*>(download);
            return source.cancelled ? 1 : 0;
        }
    }
}
