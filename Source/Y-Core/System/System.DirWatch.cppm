export module ClaFi.Core.System.DirWatch;

import ClaFi.Core.System.Events;
import ClaFi.Core.System.Timer;
import ClaFi.Core.System.UiTypes;

import ClaFi.StdLib;

namespace ClaFi
{
    // A single logical save emits several notifications on every backend - editors write a
    // temporary file and rename it over the target, which surfaces as create plus rename plus
    // attribute changes. Listeners are told once per quiet period rather than once per event.
    constexpr MilliSeconds k_dirWatchCoalesceDelay{ 60u };

    export class DirWatch;

    // Carries no description of what changed. See UI-Types
    export class DirWatchChangeEvent : public EventOf<DirWatch>
    {
    public:
        explicit DirWatchChangeEvent(DirWatch& sender);
    };

    // Watches a directory and reports that its contents changed. See UI-Types
    export class DirWatch : public EventComponent
    {
    public:
        explicit DirWatch(const std::filesystem::path&);
        DirWatch(const DirWatch&) = delete;
        DirWatch& operator=(const DirWatch&) = delete;
        ~DirWatch();
    public:
        EventConnection onChange(auto&& callback) {
            return connectEvent<DirWatchChangeEvent>(std::forward<decltype(callback)>(callback));
        }
    public:
        // Re-establishes the watch. Needed after the directory itself is replaced, and useful
        // to drop notifications caused by the caller's own writes.
        void restart();
        // Called by the platform backend. Safe to call from a backend thread.
        void reportChanged();
        [[nodiscard]] bool watching() const;
        [[nodiscard]] const std::filesystem::path& path() const { return m_path; }
    private:
        class Impl;
        using ImplPtr = std::unique_ptr<Impl>;
    private:
        // Built here rather than in a default member initializer: MSVC rejects a this-capturing
        // lambda in that position. Calling this during construction is safe because the lambda
        // only captures the address, and the timer cannot fire until the object is complete.
        // Spelled out rather than deduced: the platform constructors call this from another
        // translation unit, where a deduced return type would need this body to be reachable.
        using CoalesceHandler = OnEvent<std::function<void(TimerEvent&)>>;
        void notifyListeners();
        [[nodiscard]] CoalesceHandler makeCoalesceCallback();
    private:
        // m_coalesceTimer is declared before m_impl so that it is already constructed when the
        // backend starts watching - a watch can report a change as soon as it is established.
        const std::filesystem::path m_path;
        UiTimer m_coalesceTimer;
        ImplPtr m_impl{};
    };
}

//-----------------------------------------------------------------------------

namespace ClaFi
{

    // DirWatchChangeEvent

    DirWatchChangeEvent::DirWatchChangeEvent(DirWatch& sender)
        :
        EventOf<DirWatch>{ sender }
    {
    }

    void DirWatch::reportChanged()
    {
        m_coalesceTimer.start(k_dirWatchCoalesceDelay);
    }

    void DirWatch::notifyListeners()
    {
        emitEvent<DirWatchChangeEvent>(*this);
    }

    // DirWatch - portable members. The constructor, destructor, restart and watching live in
    // the platform implementation unit, where Impl is a complete type.

    DirWatch::CoalesceHandler DirWatch::makeCoalesceCallback()
    {
        return CoalesceHandler{ [this](TimerEvent&) {
            notifyListeners();
        } };
    }

}
