module;
#include <sys/inotify.h>
#include <unistd.h>
#include <climits>

export module ClaFi.Platform.Linux.DirWatch;

import ClaFi.Platform.Linux.Diagnostic;

import ClaFi.StdLib;

namespace ClaFi::PlatformImplementation::Linux
{
    namespace InotifyMask
    {
        // IN_CLOSE_WRITE rather than IN_MODIFY: a save is interesting once the writer closes
        // the file, not on each partial write. IN_MOVED_TO is not optional - editors that save
        // atomically write a temporary file and rename it over the target, so the rename is the
        // only event a plain IN_MODIFY watch would ever see, and it would see none.
        constexpr std::uint32_t watched =
            IN_CLOSE_WRITE
            | IN_CREATE
            | IN_DELETE
            | IN_MOVED_FROM
            | IN_MOVED_TO
            | IN_DELETE_SELF
            | IN_MOVE_SELF;
    }

    // Sized so a single read drains a full burst. Each record is variable length and the
    // kernel never splits one across reads, so the buffer has to hold the largest possible
    // record outright.
    constexpr std::size_t k_inotifyBufferSize = 16u * (sizeof(inotify_event) + NAME_MAX + 1u);

    // One inotify descriptor serves every watch in the process, and the platform owns it. See
    // Platform
    export class DirWatchManager
    {
    public:
        // Invoked with true when the directory contents changed, and with false when the watch
        // itself went away because the directory was deleted or moved.
        using OnWatchEvent = std::function<void(bool)>;
    public:
        DirWatchManager();
        ~DirWatchManager();
        DirWatchManager(const DirWatchManager&) = delete;
        DirWatchManager& operator=(const DirWatchManager&) = delete;
    public:
        // THE ONE STANDING, or null while the platform holds none. A DirWatch sits below every
        // context, so its platform half reaches the manager through this and nothing else; the
        // manager sets it as it is built and clears it as it goes. The platform stands before
        // anything that owns a DirWatch and after it - see Application - so a watch finding
        // none is a broken order.
        [[nodiscard]] static DirWatchManager* standing() { return s_standing; }
    public:
        [[nodiscard]] int notifyFd() const { return m_notifyFd; }
        void dispatchPending();
        [[nodiscard]] int addWatch(const std::filesystem::path&, const OnWatchEvent&);
        void removeWatch(int watchDescriptor);
    private:
        using WatchMap = std::unordered_map<int, OnWatchEvent>;
    private:
        void notifyAll();
    private:
        inline static DirWatchManager* s_standing{ nullptr };
        int m_notifyFd{ -1 };
        WatchMap m_watches{};
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi::PlatformImplementation::Linux
{

    // DirWatchManager

    DirWatchManager::DirWatchManager()
        :
        m_notifyFd{ ::inotify_init1(IN_NONBLOCK | IN_CLOEXEC) }
    {
        check(m_notifyFd != -1);
        s_standing = this;
    }

    DirWatchManager::~DirWatchManager()
    {
        s_standing = nullptr;
        if (m_notifyFd != -1)
        {
            ::close(m_notifyFd);
        }
    }

    void DirWatchManager::dispatchPending()
    {
        if (m_notifyFd == -1)
        {
            return;
        }
        alignas(inotify_event) char buffer[k_inotifyBufferSize];
        for (;;)
        {
            ssize_t received = ::read(m_notifyFd, buffer, sizeof(buffer));
            if (received <= 0)
            {
                return;
            }
            std::size_t offset = 0;
            while (offset < static_cast<std::size_t>(received))
            {
                const inotify_event* event = reinterpret_cast<const inotify_event*>(buffer + offset);

                if (event->mask & IN_Q_OVERFLOW)
                {
                    // The kernel dropped an unknown number of events. Nothing can be inferred
                    // about which directories were affected, so every watcher rescans.
                    notifyAll();
                }
                else
                {
                    WatchMap::iterator found = m_watches.find(event->wd);
                    if (found != m_watches.end())
                    {
                        // A watch is bound to the inode, not to the path. Once the directory is
                        // deleted or moved the descriptor is dead and the kernel sends
                        // IN_IGNORED, after which nothing further arrives for it.
                        bool stillWatching = (event->mask & (IN_IGNORED | IN_DELETE_SELF | IN_MOVE_SELF)) == 0;
                        OnWatchEvent onEvent = found->second;
                        if (!stillWatching)
                        {
                            m_watches.erase(found);
                        }
                        onEvent(stillWatching);
                    }
                }
                offset += sizeof(inotify_event) + event->len;
            }
        }
    }

    int DirWatchManager::addWatch(const std::filesystem::path& path, const OnWatchEvent& onEvent)
    {
        if (m_notifyFd == -1)
        {
            return -1;
        }
        int watchDescriptor = ::inotify_add_watch(m_notifyFd, path.c_str(), InotifyMask::watched);
        if (!check(watchDescriptor != -1))
        {
            return -1;
        }
        // inotify_add_watch on an already watched inode returns the existing descriptor rather
        // than a second one, so this assignment replaces the previous callback instead of
        // leaking a watch.
        m_watches[watchDescriptor] = onEvent;
        return watchDescriptor;
    }

    void DirWatchManager::removeWatch(int watchDescriptor)
    {
        if (watchDescriptor == -1)
        {
            return;
        }
        if (m_notifyFd != -1)
        {
            ::inotify_rm_watch(m_notifyFd, watchDescriptor);
        }
        m_watches.erase(watchDescriptor);
    }

    void DirWatchManager::notifyAll()
    {
        WatchMap snapshot = m_watches;
        for (const WatchMap::value_type& entry : snapshot)
        {
            entry.second(true);
        }
    }

}
