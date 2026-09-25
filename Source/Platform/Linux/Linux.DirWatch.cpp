module ClaFi.Core.System.DirWatch;

import ClaFi.Platform.Linux.DirWatch;

import ClaFi.Core.System.Utils;
import ClaFi.StdLib;

namespace ClaFi
{
    using PlatformImplementation::Linux::DirWatchManager;

    // The inotify descriptor is owned by DirWatchManager and polled by the main loop, so no
    // thread is involved and no marshalling is needed - dispatchPending already runs on the
    // UI thread.
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
        [[nodiscard]] bool watching() const { return m_watchDescriptor != -1; }
    private:
        // The manager the platform holds - a broken order where none stands. See Platform
        [[nodiscard]] static DirWatchManager& standingManager();
    private:
        DirWatch& m_owner;
        int m_watchDescriptor{ -1 };
    };

}

//-----------------------------------------------------------------------------

namespace ClaFi
{

    // DirWatch::Impl

    DirWatch::Impl::Impl(DirWatch& owner)
        :
        m_owner{ owner }
    {
        start();
    }

    DirWatch::Impl::~Impl()
    {
        stop();
    }

    DirWatchManager& DirWatch::Impl::standingManager()
    {
        DirWatchManager* manager = DirWatchManager::standing();
        if (manager == nullptr)
            unreachable("DirWatch: no watch manager stands - a DirWatch outlives the platform");

        return *manager;
    }

    void DirWatch::Impl::start()
    {
        stop();
        m_watchDescriptor = standingManager().addWatch(m_owner.path(), [this](bool stillWatching){
            if (!stillWatching)
            {
                m_watchDescriptor = -1;
            }
            m_owner.reportChanged();
        });
    }

    void DirWatch::Impl::stop()
    {
        standingManager().removeWatch(m_watchDescriptor);
        m_watchDescriptor = -1;
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
