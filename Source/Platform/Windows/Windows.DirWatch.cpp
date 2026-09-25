module;
#include "Windows.Headers.h"

module ClaFi.Core.System.DirWatch;

import ClaFi.StdLib;

namespace ClaFi
{

    // FindFirstChangeNotification reports that the directory changed and nothing more, which
    // is exactly the contract DirWatch publishes. ReadDirectoryChangesW would supply per-entry
    // detail at the cost of an overlapped buffer that silently truncates under load.
    class DirWatch::Impl
    {
    public:
        explicit Impl(DirWatch&);
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        ~Impl();
    public:
        void start();
        void stop();
        [[nodiscard]] bool watching() const { return m_watching; }
    private:
        void threadFunc();
        [[nodiscard]] static std::wstring toLongPath(const std::filesystem::path&);
    private:
        DirWatch& m_owner;
        const std::wstring m_path;
        HANDLE m_stopEventHandle{ ::CreateEventW(nullptr, FALSE, FALSE, nullptr) };
        // Opened by start on the caller's thread: a change made right after restart is on record.
        HANDLE m_changeHandle{ INVALID_HANDLE_VALUE };
        // Cleared by the thread as it ends, so a watch that stopped on its own answers no.
        std::atomic<bool> m_watching{};
        std::thread m_thread{};
    };

}

//-----------------------------------------------------------------------------

namespace ClaFi
{

    // DirWatch::Impl

    DirWatch::Impl::Impl(DirWatch& owner)
        :
        m_owner{ owner },
        m_path{ toLongPath(owner.path()) }
    {
        start();
    }

    DirWatch::Impl::~Impl()
    {
        stop();
        if (m_stopEventHandle)
        {
            ::CloseHandle(m_stopEventHandle);
        }
    }

    void DirWatch::Impl::start()
    {
        stop();
        if (!m_stopEventHandle)
        {
            return;
        }
        m_changeHandle = ::FindFirstChangeNotificationW(
            m_path.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME);
        if (m_changeHandle == INVALID_HANDLE_VALUE)
        {
            return;
        }
        m_watching = true;
        m_thread = std::thread{ &Impl::threadFunc, this };
    }

    void DirWatch::Impl::stop()
    {
        if (m_stopEventHandle)
        {
            ::SetEvent(m_stopEventHandle);
        }
        if (m_thread.joinable())
        {
            m_thread.join();
        }
        if (m_stopEventHandle)
        {
            ::ResetEvent(m_stopEventHandle);
        }
        if (m_changeHandle != INVALID_HANDLE_VALUE)
        {
            ::FindCloseChangeNotification(m_changeHandle);
            m_changeHandle = INVALID_HANDLE_VALUE;
        }
        m_watching = false;
    }

    void DirWatch::Impl::threadFunc()
    {
        HANDLE handles[2]{
            m_changeHandle,
            m_stopEventHandle
        };
        bool stop = false;
        do
        {
            switch (::WaitForMultipleObjects(2, handles, FALSE, INFINITE))
            {
            case WAIT_OBJECT_0 + 0:
                m_owner.reportChanged();

                // Removing the watched directory, including moving it to the recycle bin,
                // raises no notification and leaves FindNextChangeNotification succeeding.
                // Linux reports this directly through IN_DELETE_SELF; on Win32 the existence
                // check is the only signal available.
                if (!std::filesystem::exists(m_path))
                {
                    stop = true;
                }
                if (!::FindNextChangeNotification(handles[0]))
                {
                    stop = true;
                }
                break;

            case WAIT_OBJECT_0 + 1:
                stop = true;
                break;

            default:
                stop = true;
                break;
            }
        }
        while (!stop);
        // The handle stays open for stop to close: it is the thread's to wait on, not to own.
        m_watching = false;
    }

    std::wstring DirWatch::Impl::toLongPath(const std::filesystem::path& path)
    {
        constexpr std::wstring_view prefix = L"\\\\?\\";
        std::wstring source = path.wstring();
        std::wstring result{};
        result.reserve(prefix.size() + source.size());
        result.append(prefix);
        result.append(source);
        for (wchar_t& chr : result)
        {
            if (chr == L'/')
            {
                chr = L'\\';
            }
        }
        return result;
    }

    // DirWatch - platform members

    DirWatch::DirWatch(const std::filesystem::path& path)
        :
        m_path{ path },
        m_coalesceTimer{ makeCoalesceCallback() },
        m_impl{ std::make_unique<Impl>(*this) }
    {
    }

    DirWatch::~DirWatch() = default;

    void DirWatch::restart()
    {
        m_impl->start();
    }

    bool DirWatch::watching() const
    {
        return m_impl->watching();
    }

}
